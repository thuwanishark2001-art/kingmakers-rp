using Kingmakers.Vehicles;
using Unity.Netcode;
using UnityEngine;

namespace Kingmakers.Player
{
    /// <summary>Walking for the local player; rides along in the car while driving.</summary>
    [RequireComponent(typeof(CharacterController), typeof(PlayerState))]
    public class PlayerMotor : NetworkBehaviour
    {
        public float walkSpeed = 3.2f;
        public float runSpeed = 6.5f;
        public float gravity = -20f;
        public float turnSpeed = 720f;

        CharacterController controller;
        PlayerState state;
        Vehicle lastVehicle;
        float verticalSpeed;

        void Awake()
        {
            controller = GetComponent<CharacterController>();
            state = GetComponent<PlayerState>();
        }

        public override void OnNetworkSpawn()
        {
            // Only the owner simulates; everyone else follows the network transform.
            controller.enabled = IsOwner;
        }

        void Update()
        {
            if (!IsSpawned || !IsOwner) return;

            var vehicle = state.CurrentVehicle;
            if (vehicle != null)
            {
                if (controller.enabled) controller.enabled = false;
                lastVehicle = vehicle;
                transform.SetPositionAndRotation(vehicle.transform.position + Vector3.up * 0.5f, vehicle.transform.rotation);
                return;
            }

            if (!controller.enabled)
            {
                // Just got out: stand next to the driver's door.
                if (lastVehicle != null)
                {
                    var t = lastVehicle.transform;
                    transform.SetPositionAndRotation(t.position - t.right * 2.2f + Vector3.up * 0.2f,
                        Quaternion.Euler(0f, t.eulerAngles.y, 0f));
                }
                lastVehicle = null;
                controller.enabled = true;
            }

            var input = GameInput.Move;
            var cam = Camera.main != null ? Camera.main.transform : transform;
            var forward = Vector3.ProjectOnPlane(cam.forward, Vector3.up).normalized;
            var right = Vector3.ProjectOnPlane(cam.right, Vector3.up).normalized;
            var move = forward * input.y + right * input.x;
            float speed = GameInput.Sprint ? runSpeed : walkSpeed;

            if (move.sqrMagnitude > 0.01f)
            {
                var look = Quaternion.LookRotation(move.normalized, Vector3.up);
                transform.rotation = Quaternion.RotateTowards(transform.rotation, look, turnSpeed * Time.deltaTime);
            }

            verticalSpeed = controller.isGrounded ? -1f : verticalSpeed + gravity * Time.deltaTime;
            controller.Move((move * speed + Vector3.up * verticalSpeed) * Time.deltaTime);

            // Fell out of the world (or spawned inside something): back to the hospital.
            if (transform.position.y < -20f && World.CityBuilder.Layout != null)
            {
                controller.enabled = false;
                transform.position = World.CityBuilder.Layout.SpawnPoint.position;
                controller.enabled = true;
            }
        }
    }
}
