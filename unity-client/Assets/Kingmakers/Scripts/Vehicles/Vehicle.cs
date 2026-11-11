using System.Collections.Generic;
using Kingmakers.World;
using Unity.Collections;
using Unity.Netcode;
using UnityEngine;

namespace Kingmakers.Vehicles
{
    /// <summary>
    /// Arcade car. The driver's client owns the object while driving (so steering feels
    /// instant) and the server takes it back when the seat is empty. Fuel is counted on
    /// the server from the distance the car actually moved.
    /// </summary>
    [RequireComponent(typeof(Rigidbody))]
    public class Vehicle : NetworkBehaviour
    {
        public const ulong NoDriver = ulong.MaxValue;
        public const float TankSize = 60f;
        const float LitresPerMetre = 0.012f;

        public static readonly List<Vehicle> All = new List<Vehicle>();

        public readonly NetworkVariable<ulong> Driver = new NetworkVariable<ulong>(NoDriver);
        public readonly NetworkVariable<float> Fuel = new NetworkVariable<float>(TankSize);
        public readonly NetworkVariable<Color32> Paint = new NetworkVariable<Color32>(new Color32(200, 30, 40, 255));
        public readonly NetworkVariable<FixedString32Bytes> Plate = new NetworkVariable<FixedString32Bytes>();

        [Header("Handling")]
        public float acceleration = 14f;
        public float reverseAcceleration = 7f;
        public float maxSpeed = 38f;       // m/s, ~137 km/h
        public float turnRate = 95f;       // degrees per second at speed
        public float grip = 6f;
        public float brakeStrength = 22f;

        /// <summary>Server only: character id allowed to drive (rentals), -1 = anyone.</summary>
        public int LockedToCharacter = -1;

        Rigidbody rb;
        Vector3 lastServerPos;

        public bool HasDriver => Driver.Value != NoDriver;
        public bool IsLocalDriver => HasDriver && NetworkManager != null && Driver.Value == NetworkManager.LocalClientId;
        public float SpeedKmh => rb != null ? rb.linearVelocity.magnitude * 3.6f : 0f;

        void Awake()
        {
            rb = GetComponent<Rigidbody>();
            rb.constraints = RigidbodyConstraints.FreezeRotationX | RigidbodyConstraints.FreezeRotationZ;
            rb.centerOfMass = new Vector3(0f, 0.4f, 0f);
            var slick = new PhysicsMaterial("Car") { dynamicFriction = 0f, staticFriction = 0f, frictionCombine = PhysicsMaterialCombine.Minimum };
            foreach (var c in GetComponentsInChildren<Collider>()) c.sharedMaterial = slick;
        }

        public override void OnNetworkSpawn()
        {
            All.Add(this);
            Paint.OnValueChanged += (_, c) => Repaint(c);
            Repaint(Paint.Value);
            lastServerPos = transform.position;
            UpdatePhysicsMode();
        }

        public override void OnNetworkDespawn() => All.Remove(this);
        public override void OnGainedOwnership() => UpdatePhysicsMode();
        public override void OnLostOwnership() => UpdatePhysicsMode();

        void UpdatePhysicsMode()
        {
            rb.isKinematic = !IsOwner;
            rb.interpolation = IsOwner && !IsServer ? RigidbodyInterpolation.Interpolate : RigidbodyInterpolation.None;
        }

        void Repaint(Color32 c)
        {
            var body = transform.Find("Body");
            if (body != null) Materials.Paint(body.gameObject, c);
        }

        public void ServerSetDriver(ulong clientId)
        {
            Driver.Value = clientId;
            if (clientId != NetworkManager.ServerClientId) NetworkObject.ChangeOwnership(clientId);
        }

        public void ServerClearDriver()
        {
            Driver.Value = NoDriver;
            if (NetworkObject.IsSpawned && NetworkObject.OwnerClientId != NetworkManager.ServerClientId)
                NetworkObject.RemoveOwnership();
        }

        void FixedUpdate()
        {
            if (!IsSpawned) return;

            if (IsServer)
            {
                var pos = transform.position;
                if (HasDriver) Fuel.Value = Mathf.Max(0f, Fuel.Value - Vector3.Distance(pos, lastServerPos) * LitresPerMetre);
                lastServerPos = pos;
            }

            if (!IsOwner) return;

            if (!IsLocalDriver)
            {
                rb.linearDamping = 2.5f;   // parked: roll to a stop
                return;
            }

            rb.linearDamping = 0.15f;
            var input = GameInput.Move;
            float throttle = Fuel.Value > 0f ? input.y : 0f;
            float steer = input.x;
            var fwd = transform.forward;
            var right = transform.right;
            float speed = Vector3.Dot(rb.linearVelocity, fwd);

            if (throttle > 0f && speed < maxSpeed) rb.AddForce(fwd * (throttle * acceleration), ForceMode.Acceleration);
            else if (throttle < 0f)
            {
                if (speed > 1f) rb.AddForce(-fwd * brakeStrength, ForceMode.Acceleration);
                else if (speed > -maxSpeed * 0.3f) rb.AddForce(fwd * (throttle * reverseAcceleration), ForceMode.Acceleration);
            }
            if (GameInput.Brake && Mathf.Abs(speed) > 0.5f)
                rb.AddForce(-fwd * (Mathf.Sign(speed) * brakeStrength), ForceMode.Acceleration);

            // Steering scales with speed and flips when reversing.
            float turn = steer * turnRate * Mathf.Clamp(speed / 8f, -1f, 1f) * Time.fixedDeltaTime;
            rb.MoveRotation(rb.rotation * Quaternion.Euler(0f, turn, 0f));

            // Tyre grip: cancel sideways sliding.
            float lateral = Vector3.Dot(rb.linearVelocity, right);
            rb.AddForce(-right * (lateral * grip), ForceMode.Acceleration);
        }
    }
}
