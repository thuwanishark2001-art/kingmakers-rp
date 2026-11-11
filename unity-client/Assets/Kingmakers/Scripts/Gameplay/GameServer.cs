using Kingmakers.Player;
using Kingmakers.Vehicles;
using Kingmakers.World;
using Unity.Collections;
using Unity.Netcode;
using UnityEngine;

namespace Kingmakers
{
    /// <summary>Server-only world loop: parked cars, payday and autosave.</summary>
    public class GameServer : MonoBehaviour
    {
        public static GameServer Instance { get; private set; }

        const float PaydayInterval = 15 * 60f;
        const float SaveInterval = 120f;
        const long Salary = 500;

        static readonly Color32[] Paints =
        {
            new Color32(200, 30, 40, 255), new Color32(25, 70, 160, 255), new Color32(235, 235, 235, 255),
            new Color32(25, 25, 28, 255), new Color32(40, 120, 70, 255), new Color32(230, 170, 30, 255),
            new Color32(120, 120, 128, 255), new Color32(90, 40, 110, 255),
        };

        GameObject carPrefab;
        float nextPayday, nextSave;

        public void Init(GameObject carPrefab)
        {
            Instance = this;
            this.carPrefab = carPrefab;
            nextPayday = Time.time + PaydayInterval;
            nextSave = Time.time + SaveInterval;
            foreach (var spot in CityBuilder.Layout.ParkingSpots)
                SpawnVehicle(spot.position, spot.rotation, -1);
            Debug.Log($"[Server] Spawned {CityBuilder.Layout.ParkingSpots.Count} parked cars");
        }

        public Vehicle SpawnVehicle(Vector3 position, Quaternion rotation, int lockedToCharacter)
        {
            var go = Instantiate(carPrefab, position, rotation);
            var v = go.GetComponent<Vehicle>();
            v.LockedToCharacter = lockedToCharacter;
            v.Paint.Value = Paints[Random.Range(0, Paints.Length)];
            v.Plate.Value = new FixedString32Bytes($"KP-{Random.Range(1000, 9999)}");
            go.GetComponent<NetworkObject>().Spawn(true);
            return v;
        }

        void Update()
        {
            if (Time.time >= nextPayday)
            {
                nextPayday = Time.time + PaydayInterval;
                foreach (var p in PlayerState.ServerByClient.Values) p.ServerPayday(Salary);
            }
            if (Time.time >= nextSave)
            {
                nextSave = Time.time + SaveInterval;
                foreach (var p in PlayerState.ServerByClient.Values) p.ServerSave();
            }
        }

        void OnApplicationQuit()
        {
            foreach (var p in PlayerState.ServerByClient.Values) p.ServerSave();
        }
    }
}
