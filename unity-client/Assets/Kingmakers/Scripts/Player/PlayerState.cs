using System;
using System.Collections.Generic;
using Kingmakers.Net;
using Kingmakers.Vehicles;
using Kingmakers.World;
using Unity.Collections;
using Unity.Netcode;
using UnityEngine;

namespace Kingmakers.Player
{
    /// <summary>
    /// A connected player's character. The server owns every value here; the owning client
    /// asks for changes through RPCs and the server checks them (distance, money, rate).
    /// </summary>
    public class PlayerState : NetworkBehaviour
    {
        public const ulong NoVehicle = ulong.MaxValue;
        const float InteractSlack = 3f;      // extra metres allowed for network lag
        const long RentalPrice = 250;
        const float FuelPrice = 3f;          // per litre

        public static PlayerState Local { get; private set; }
        public static readonly Dictionary<ulong, PlayerState> ServerByClient = new Dictionary<ulong, PlayerState>();
        public static readonly List<PlayerState> All = new List<PlayerState>();

        // Client-side events for the UI.
        public static event Action<string> Notified;
        public static event Action<string, string> ChatReceived;
        public static event Action<Vector3, int, bool> JobChanged;

        public readonly NetworkVariable<FixedString64Bytes> CharacterName = new NetworkVariable<FixedString64Bytes>();
        public readonly NetworkVariable<long> Cash = new NetworkVariable<long>(0, NetworkVariableReadPermission.Owner);
        public readonly NetworkVariable<long> Bank = new NetworkVariable<long>(0, NetworkVariableReadPermission.Owner);
        public readonly NetworkVariable<int> Level = new NetworkVariable<int>(1);
        public readonly NetworkVariable<ulong> VehicleId = new NetworkVariable<ulong>(NoVehicle);

        // ---- server-only state ----
        CharacterData character;
        float lastSaveTime, lastChatTime;
        bool onJob;
        Vector3 jobTarget;
        int jobReward;
        Vehicle rental;
        Renderer[] bodyRenderers;

        public CharacterData Character => character;
        public bool InVehicle => VehicleId.Value != NoVehicle;

        public void ServerInit(CharacterData c)
        {
            character = c;
            CharacterName.Value = new FixedString64Bytes(c.name);
            Cash.Value = c.cash;
            Bank.Value = c.bank;
            Level.Value = Mathf.Max(1, c.level);
            lastSaveTime = Time.time;
        }

        void Awake() => bodyRenderers = GetComponentsInChildren<Renderer>();

        public override void OnNetworkSpawn()
        {
            All.Add(this);
            if (IsOwner) Local = this;
            if (IsServer) ServerByClient[OwnerClientId] = this;
            VehicleId.OnValueChanged += (_, __) => UpdateVisibility();
            UpdateVisibility();
        }

        public override void OnNetworkDespawn()
        {
            if (IsServer)
            {
                ServerLeaveVehicle();
                if (rental != null && rental.NetworkObject.IsSpawned) rental.NetworkObject.Despawn();
                ServerSave();
                ServerByClient.Remove(OwnerClientId);
                ServerSessions.Remove(OwnerClientId);
            }
            All.Remove(this);
            if (Local == this) Local = null;
        }

        void UpdateVisibility()
        {
            foreach (var r in bodyRenderers) r.enabled = !InVehicle;
        }

        public Vehicle CurrentVehicle =>
            InVehicle && NetworkManager.SpawnManager.SpawnedObjects.TryGetValue(VehicleId.Value, out var no)
                ? no.GetComponent<Vehicle>() : null;

        // ================= client API (called by UI / input) =================

        public void RequestBank(bool deposit, long amount) => BankRpc(deposit, amount);
        public void RequestTransfer(string toName, long amount) => TransferRpc(toName ?? "", amount);
        public void RequestInteract(int poiId) => InteractRpc(poiId);
        public void RequestEnterVehicle(Vehicle v) => VehicleRpc(v.NetworkObjectId);
        public void RequestExitVehicle() => VehicleRpc(NoVehicle);
        public void RequestQuitJob() => QuitJobRpc();
        public void RequestDeliver() => DeliverRpc();
        public void SendChat(string text) { if (!string.IsNullOrWhiteSpace(text)) ChatRpc(text); }

        // ================= server RPC handlers =================

        [Rpc(SendTo.Server, RequireOwnership = true)]
        void BankRpc(bool deposit, long amount)
        {
            if (amount <= 0) return;
            if (!NearPoi(PoiType.Atm) && !NearPoi(PoiType.Bank)) { Notify("Go to an ATM or the bank"); return; }
            if (deposit)
            {
                if (Cash.Value < amount) { Notify("You don't have that much cash"); return; }
                Cash.Value -= amount; Bank.Value += amount;
                Notify($"Deposited ${amount:N0}");
            }
            else
            {
                if (Bank.Value < amount) { Notify("Not enough money in the bank"); return; }
                Bank.Value -= amount; Cash.Value += amount;
                Notify($"Withdrew ${amount:N0}");
            }
        }

        [Rpc(SendTo.Server, RequireOwnership = true)]
        void TransferRpc(string toName, long amount)
        {
            if (amount <= 0 || toName.Length > 40) return;
            PlayerState target = null;
            foreach (var p in ServerByClient.Values)
                if (p != this && string.Equals(p.CharacterName.Value.ToString(), toName.Trim(), StringComparison.OrdinalIgnoreCase))
                    target = p;
            if (target == null) { Notify("No player with that name is online"); return; }
            if (Bank.Value < amount) { Notify("Not enough money in the bank"); return; }
            Bank.Value -= amount;
            target.Bank.Value += amount;
            Notify($"Sent ${amount:N0} to {target.CharacterName.Value}");
            target.Notify($"{CharacterName.Value} sent you ${amount:N0}");
        }

        [Rpc(SendTo.Server, RequireOwnership = true)]
        void InteractRpc(int poiId)
        {
            var city = CityBuilder.Layout;
            if (city == null || poiId < 0 || poiId >= city.Pois.Count) return;
            var poi = city.Pois[poiId];
            if (Vector3.Distance(transform.position, poi.Position) > poi.Radius + InteractSlack) return;

            switch (poi.Type)
            {
                case PoiType.DeliveryDepot:
                    if (onJob) Notify("You are already on a delivery run");
                    else { onJob = true; NextDelivery(); Notify("Delivery job started. Drive to the marker."); }
                    break;
                case PoiType.Dealership:
                    RentCar();
                    break;
                case PoiType.GasStation:
                    Refuel();
                    break;
                case PoiType.Hospital:
                    Notify("Kingsport General: you are healthy. New arrivals spawn here.");
                    break;
                case PoiType.CityHall:
                    Notify("City Hall: find work at the Harbor Delivery Depot.");
                    break;
                case PoiType.Police:
                    Notify("Police Department: officer careers open in a later update.");
                    break;
                default:
                    Notify("Use the Bank app on your phone here");
                    break;
            }
        }

        [Rpc(SendTo.Server, RequireOwnership = true)]
        void VehicleRpc(ulong vehicleId)
        {
            if (vehicleId == NoVehicle) { ServerLeaveVehicle(); return; }
            if (InVehicle) return;
            if (!NetworkManager.SpawnManager.SpawnedObjects.TryGetValue(vehicleId, out var no)) return;
            var v = no.GetComponent<Vehicle>();
            if (v == null || Vector3.Distance(transform.position, v.transform.position) > 6f) return;
            if (v.HasDriver) { Notify("Someone is driving this car"); return; }
            if (v.LockedToCharacter >= 0 && v.LockedToCharacter != character.id) { Notify("This car is rented by someone else"); return; }
            v.ServerSetDriver(OwnerClientId);
            VehicleId.Value = vehicleId;
        }

        public void ServerLeaveVehicle()
        {
            var v = CurrentVehicle;
            if (v != null) v.ServerClearDriver();
            VehicleId.Value = NoVehicle;
        }

        [Rpc(SendTo.Server, RequireOwnership = true)]
        void QuitJobRpc()
        {
            if (!onJob) return;
            onJob = false;
            JobRpc(Vector3.zero, 0, false);
            Notify("You quit the delivery job");
        }

        [Rpc(SendTo.Server, RequireOwnership = true)]
        void DeliverRpc()
        {
            if (!onJob || Vector3.Distance(transform.position, jobTarget) > 8f) return;
            Cash.Value += jobReward;
            character.exp += 1;
            Level.Value = 1 + character.exp / 8;
            Notify($"Delivered! +${jobReward:N0}");
            NextDelivery();
        }

        [Rpc(SendTo.Server, RequireOwnership = true)]
        void ChatRpc(string text)
        {
            if (Time.time - lastChatTime < 1f) return;
            lastChatTime = Time.time;
            text = text.Replace("<", "").Replace(">", "").Trim();
            if (text.Length == 0) return;
            if (text.Length > 120) text = text.Substring(0, 120);
            ChatBroadcastRpc(CharacterName.Value.ToString(), text);
        }

        // ================= server helpers =================

        bool NearPoi(PoiType type)
        {
            foreach (var poi in CityBuilder.Layout.Pois)
                if (poi.Type == type && Vector3.Distance(transform.position, poi.Position) <= poi.Radius + InteractSlack)
                    return true;
            return false;
        }

        void NextDelivery()
        {
            var points = CityBuilder.Layout.DeliveryPoints;
            Vector3 pick = points[UnityEngine.Random.Range(0, points.Count)];
            for (int tries = 0; tries < 10 && Vector3.Distance(pick, transform.position) < 150f; tries++)
                pick = points[UnityEngine.Random.Range(0, points.Count)];
            jobTarget = pick;
            jobReward = 120 + Mathf.RoundToInt(Vector3.Distance(transform.position, pick) * 0.6f);
            JobRpc(jobTarget, jobReward, true);
        }

        void RentCar()
        {
            if (Cash.Value < RentalPrice) { Notify($"Renting a car costs ${RentalPrice}"); return; }
            var spots = CityBuilder.Layout.RentalSpots;
            var spot = spots[UnityEngine.Random.Range(0, spots.Count)];
            if (rental != null && rental.NetworkObject.IsSpawned)
            {
                if (rental.HasDriver) { Notify("Get out of your current rental first"); return; }
                rental.NetworkObject.Despawn();
            }
            Cash.Value -= RentalPrice;
            rental = GameServer.Instance.SpawnVehicle(spot.position, spot.rotation, character.id);
            Notify($"Rental ready in the lot. -${RentalPrice}");
        }

        void Refuel()
        {
            var v = CurrentVehicle;
            if (v == null) { Notify("Drive a car to the pump to refuel"); return; }
            float litres = Vehicle.TankSize - v.Fuel.Value;
            if (litres < 1f) { Notify("The tank is already full"); return; }
            long cost = (long)Mathf.Ceil(litres * FuelPrice);
            if (Cash.Value < cost) { Notify($"A full tank costs ${cost}"); return; }
            Cash.Value -= cost;
            v.Fuel.Value = Vehicle.TankSize;
            Notify($"Refueled {litres:0} L for ${cost}");
        }

        public void ServerPayday(long salary)
        {
            long interest = Math.Min(Bank.Value / 1000, 2000);
            Bank.Value += salary + interest;
            Notify($"Payday: salary ${salary:N0} + interest ${interest:N0} paid into your bank");
        }

        public void ServerSave()
        {
            if (character == null) return;
            var v = CurrentVehicle;
            var t = v != null ? v.transform : transform;
            character.cash = Cash.Value;
            character.bank = Bank.Value;
            character.level = Level.Value;
            character.hasPosition = true;
            character.position = new Vec3Data { x = t.position.x, y = t.position.y + 0.5f, z = t.position.z };
            character.rotY = t.eulerAngles.y;
            character.playMinutes += Mathf.RoundToInt((Time.time - lastSaveTime) / 60f);
            lastSaveTime = Time.time;
            if (!GameConfig.OfflineMode) SaveAsync(character);
        }

        static async void SaveAsync(CharacterData c)
        {
            var result = await BackendClient.SaveCharacter(c);
            if (!result.Ok) Debug.LogWarning($"[Server] Saving {c.name} failed: {result.Error}");
        }

        public void Notify(string text) => NotifyRpc(text);

        // ================= server -> client =================

        [Rpc(SendTo.Owner)]
        void NotifyRpc(string text) => Notified?.Invoke(text);

        [Rpc(SendTo.Owner)]
        void JobRpc(Vector3 target, int reward, bool active) => JobChanged?.Invoke(target, reward, active);

        [Rpc(SendTo.ClientsAndHost)]
        void ChatBroadcastRpc(string from, string text) => ChatReceived?.Invoke(from, text);
    }
}
