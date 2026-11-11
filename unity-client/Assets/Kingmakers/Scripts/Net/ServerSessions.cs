using System;
using System.Collections.Generic;
using System.Text;
using Unity.Netcode;
using UnityEngine;

namespace Kingmakers.Net
{
    [Serializable]
    public class ConnectPayload
    {
        public string token;
        public string guestName;
        public string version;
    }

    /// <summary>
    /// Server side: checks each connecting client's login token with the backend before
    /// letting it in, and remembers which character each client plays.
    /// </summary>
    public static class ServerSessions
    {
        static readonly Dictionary<ulong, CharacterData> ByClient = new Dictionary<ulong, CharacterData>();
        static int nextGuestId = 1_000_000;

        public static bool TryGet(ulong clientId, out CharacterData character) => ByClient.TryGetValue(clientId, out character);
        public static void Remove(ulong clientId) => ByClient.Remove(clientId);

        static bool IsOnline(int characterId)
        {
            foreach (var c in ByClient.Values) if (c.id == characterId) return true;
            return false;
        }

        public static void Approve(NetworkManager.ConnectionApprovalRequest request, NetworkManager.ConnectionApprovalResponse response)
        {
            response.CreatePlayerObject = false; // spawned in GameBootstrap once connected
            ConnectPayload payload = null;
            try { payload = JsonUtility.FromJson<ConnectPayload>(Encoding.UTF8.GetString(request.Payload)); }
            catch { /* handled below */ }

            if (payload == null || payload.version != GameConfig.Version)
            {
                Deny(response, "Please update the game to version " + GameConfig.Version);
                return;
            }

            if (NetworkManager.Singleton.ConnectedClientsIds.Count >= GameConfig.MaxPlayers)
            {
                Deny(response, "The server is full");
                return;
            }

            if (GameConfig.OfflineMode)
            {
                var name = string.IsNullOrWhiteSpace(payload.guestName) ? "Guest_Player" : payload.guestName.Trim();
                if (name.Length > 24) name = name.Substring(0, 24);
                ByClient[request.ClientNetworkId] = new CharacterData { id = nextGuestId++, name = name, cash = 5000, bank = 20000 };
                response.Approved = true;
                return;
            }

            if (string.IsNullOrEmpty(GameConfig.ServerKey))
            {
                Debug.LogError("[Server] No server key: set GAME_SERVER_KEY or pass -serverKey, or start with -offline.");
                Deny(response, "Server is not configured");
                return;
            }

            response.Pending = true;
            Verify(request.ClientNetworkId, payload.token, response);
        }

        static async void Verify(ulong clientId, string token, NetworkManager.ConnectionApprovalResponse response)
        {
            var result = await BackendClient.VerifyToken(token);
            if (!result.Ok)
            {
                Deny(response, "Login expired, please sign in again");
            }
            else if (IsOnline(result.Data.character.id))
            {
                Deny(response, "This character is already online");
            }
            else
            {
                ByClient[clientId] = result.Data.character;
                response.Approved = true;
            }
            response.Pending = false;
        }

        static void Deny(NetworkManager.ConnectionApprovalResponse response, string reason)
        {
            response.Approved = false;
            response.Reason = reason;
        }
    }
}
