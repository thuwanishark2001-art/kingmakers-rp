using UnityEngine;

namespace Kingmakers.Player
{
    /// <summary>Third-person orbit camera that follows the local player or their car.</summary>
    public class CameraRig : MonoBehaviour
    {
        public float lookSensitivity = 0.15f;
        public float walkDistance = 5f;
        public float driveDistance = 9f;
        public float minPitch = -10f, maxPitch = 60f;

        float yaw, pitch = 15f, distance = 5f, lastLookTime;
        const int IgnoreRaycastLayer = 2; // players and cars live on this layer

        void LateUpdate()
        {
            var player = PlayerState.Local;
            var look = GameInput.ConsumeLook();
            if (player == null) return;

            var vehicle = player.CurrentVehicle;
            var target = vehicle != null ? vehicle.transform : player.transform;
            float targetHeight = vehicle != null ? 1.6f : 1.5f;

            if (look.sqrMagnitude > 0f)
            {
                yaw += look.x * lookSensitivity;
                pitch = Mathf.Clamp(pitch - look.y * lookSensitivity, minPitch, maxPitch);
                lastLookTime = Time.time;
            }
            else if (vehicle != null && Time.time - lastLookTime > 1.5f)
            {
                // While driving, drift back behind the car.
                yaw = Mathf.LerpAngle(yaw, target.eulerAngles.y, Time.deltaTime * 2.5f);
                pitch = Mathf.Lerp(pitch, 12f, Time.deltaTime * 2f);
            }

            distance = Mathf.Lerp(distance, vehicle != null ? driveDistance : walkDistance, Time.deltaTime * 4f);
            var pivot = target.position + Vector3.up * targetHeight;
            var rotation = Quaternion.Euler(pitch, yaw, 0f);
            var wanted = pivot - rotation * Vector3.forward * distance;

            // Don't go through walls.
            int mask = ~(1 << IgnoreRaycastLayer);
            var dir = wanted - pivot;
            if (Physics.SphereCast(pivot, 0.3f, dir.normalized, out var hit, dir.magnitude, mask, QueryTriggerInteraction.Ignore))
                wanted = pivot + dir.normalized * Mathf.Max(0.5f, hit.distance);

            transform.SetPositionAndRotation(wanted, Quaternion.LookRotation(pivot - wanted, Vector3.up));
        }
    }
}
