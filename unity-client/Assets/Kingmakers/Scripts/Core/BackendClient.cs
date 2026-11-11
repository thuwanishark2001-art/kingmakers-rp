using System;
using System.Text;
using System.Threading.Tasks;
using UnityEngine;
using UnityEngine.Networking;

namespace Kingmakers
{
    [Serializable]
    public class Vec3Data
    {
        public float x, y, z;
        public Vector3 ToVector3() => new Vector3(x, y, z);
    }

    [Serializable]
    public class CharacterData
    {
        public int id;
        public string name;
        public long cash;
        public long bank;
        public int level = 1;
        public int exp;
        public int playMinutes;
        public bool hasPosition;
        public Vec3Data position = new Vec3Data();
        public float rotY;
    }

    [Serializable] public class AuthResponse { public string token; public CharacterData character; }
    [Serializable] public class VerifyResponse { public CharacterData character; }
    [Serializable] public class GameServerInfo { public string host; public int port; public string version; }
    [Serializable] public class SaveAck { public bool ok; }
    [Serializable] class ErrorBody { public string error; }

    [Serializable] class RegisterBody { public string username; public string password; public string characterName; }
    [Serializable] class LoginBody { public string username; public string password; }
    [Serializable] class TokenBody { public string token; }

    [Serializable]
    class SaveBody
    {
        public long cash;
        public long bank;
        public int level;
        public int exp;
        public int playMinutes;
        public Vec3Data position;
        public float rotY;
    }

    public class ApiResult<T>
    {
        public bool Ok;
        public T Data;
        public string Error;
    }

    /// <summary>Calls the Node.js backend (/api/v2). Used by the client and the game server.</summary>
    public static class BackendClient
    {
        public static Task<ApiResult<AuthResponse>> Login(string username, string password) =>
            Send<AuthResponse>("POST", "/api/v2/auth/login", JsonUtility.ToJson(new LoginBody { username = username, password = password }));

        public static Task<ApiResult<AuthResponse>> Register(string username, string password, string characterName) =>
            Send<AuthResponse>("POST", "/api/v2/auth/register",
                JsonUtility.ToJson(new RegisterBody { username = username, password = password, characterName = characterName }));

        public static Task<ApiResult<GameServerInfo>> GetGameServer() =>
            Send<GameServerInfo>("GET", "/api/v2/game/config", null);

        // ---- game server only ----

        public static Task<ApiResult<VerifyResponse>> VerifyToken(string token) =>
            Send<VerifyResponse>("POST", "/api/v2/server/verify", JsonUtility.ToJson(new TokenBody { token = token }), GameConfig.ServerKey);

        public static Task<ApiResult<SaveAck>> SaveCharacter(CharacterData c)
        {
            var body = new SaveBody
            {
                cash = c.cash, bank = c.bank, level = c.level, exp = c.exp, playMinutes = c.playMinutes,
                position = c.position, rotY = c.rotY,
            };
            return Send<SaveAck>("PUT", "/api/v2/server/characters/" + c.id, JsonUtility.ToJson(body), GameConfig.ServerKey);
        }

        static async Task<ApiResult<T>> Send<T>(string method, string path, string json, string serverKey = null)
        {
            using var request = new UnityWebRequest(GameConfig.BackendUrl + path, method);
            request.downloadHandler = new DownloadHandlerBuffer();
            if (json != null)
            {
                request.uploadHandler = new UploadHandlerRaw(Encoding.UTF8.GetBytes(json));
                request.SetRequestHeader("Content-Type", "application/json");
            }
            if (serverKey != null) request.SetRequestHeader("x-server-key", serverKey);
            request.timeout = 10;

            var done = new TaskCompletionSource<bool>();
            request.SendWebRequest().completed += _ => done.TrySetResult(true);
            await done.Task;

            var text = request.downloadHandler.text;
            if (request.result == UnityWebRequest.Result.Success)
            {
                try { return new ApiResult<T> { Ok = true, Data = JsonUtility.FromJson<T>(text) }; }
                catch (Exception e) { return new ApiResult<T> { Error = "Bad response: " + e.Message }; }
            }

            string error = null;
            if (!string.IsNullOrEmpty(text))
            {
                try { error = JsonUtility.FromJson<ErrorBody>(text)?.error; } catch { /* not JSON */ }
            }
            if (string.IsNullOrEmpty(error))
                error = request.result == UnityWebRequest.Result.ConnectionError
                    ? "Cannot reach the server. Check your internet connection."
                    : $"Server error ({request.responseCode})";
            return new ApiResult<T> { Error = error };
        }
    }
}
