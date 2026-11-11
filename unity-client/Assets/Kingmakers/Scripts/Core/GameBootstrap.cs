using System;
using System.Text;
using Kingmakers.Net;
using Kingmakers.Player;
using Kingmakers.UI;
using Kingmakers.World;
using Unity.Netcode;
using Unity.Netcode.Transports.UTP;
using UnityEngine;
using UnityEngine.EventSystems;
#if ENABLE_INPUT_SYSTEM
using UnityEngine.InputSystem.UI;
#endif

namespace Kingmakers
{
    /// <summary>
    /// The only object in the scene. Builds the city, the network manager and either the
    /// dedicated server or the client (camera, light, UI).
    /// </summary>
    public class GameBootstrap : MonoBehaviour
    {
        public static GameBootstrap Instance { get; private set; }

        /// <summary>Client: fired with a reason when the connection ends or fails.</summary>
        public event Action<string> Disconnected;
        public event Action Connected;

        GameObject playerPrefab, carPrefab;
        NetworkManager network;
        UnityTransport transport;

        void Awake()
        {
            Instance = this;
            playerPrefab = Resources.Load<GameObject>("Prefabs/Player");
            carPrefab = Resources.Load<GameObject>("Prefabs/Car");
            if (playerPrefab == null || carPrefab == null)
                Debug.LogError("[Kingmakers] Prefabs missing. Run Kingmakers > Run Project Setup in the editor.");
        }

        void Start()
        {
            bool server = GameConfig.IsDedicatedServer;
            Application.targetFrameRate = server ? 30 : 60;
            CityBuilder.Build(withVisuals: !server);
            CreateNetworkManager();

            if (server) StartDedicatedServer();
            else
            {
                CreateClientScene();
                UIRoot.Create();
            }
        }

        void CreateNetworkManager()
        {
            var go = new GameObject("NetworkManager");
            transport = go.AddComponent<UnityTransport>();
            network = go.AddComponent<NetworkManager>();
            network.NetworkConfig ??= new NetworkConfig();
            network.NetworkConfig.NetworkTransport = transport;
            network.NetworkConfig.ConnectionApproval = true;
            network.NetworkConfig.EnableSceneManagement = false; // everyone builds the same city locally
            network.NetworkConfig.TickRate = 30;
            network.AddNetworkPrefab(playerPrefab);
            network.AddNetworkPrefab(carPrefab);

            network.ConnectionApprovalCallback = ServerSessions.Approve;
            network.OnClientConnectedCallback += OnClientConnected;
            network.OnClientDisconnectCallback += OnClientDisconnected;
        }

        // ------------------------------------------------------------ server

        void StartDedicatedServer()
        {
            var port = GameConfig.ServerPort;
            transport.SetConnectionData("0.0.0.0", port, "0.0.0.0");
            if (!network.StartServer())
            {
                Debug.LogError("[Server] Failed to start on port " + port);
                Application.Quit(1);
                return;
            }
            Debug.Log($"[Server] Kingsport v{GameConfig.Version} listening on UDP {port}" +
                      (GameConfig.OfflineMode ? " (offline mode: guests allowed, nothing is saved)" : $", backend {GameConfig.BackendUrl}"));
            gameObject.AddComponent<GameServer>().Init(carPrefab);
        }

        void OnClientConnected(ulong clientId)
        {
            if (network.IsServer) SpawnPlayer(clientId);
            if (!network.IsServer && clientId == network.LocalClientId) Connected?.Invoke();
            if (network.IsHost && clientId == network.LocalClientId) Connected?.Invoke();
        }

        void SpawnPlayer(ulong clientId)
        {
            if (!ServerSessions.TryGet(clientId, out var character))
            {
                network.DisconnectClient(clientId, "Session not found");
                return;
            }
            var spawn = CityBuilder.Layout.SpawnPoint;
            var pos = character.hasPosition ? character.position.ToVector3() : spawn.position;
            var rot = character.hasPosition ? Quaternion.Euler(0f, character.rotY, 0f) : spawn.rotation;

            var go = Instantiate(playerPrefab, pos, rot);
            go.GetComponent<PlayerState>().ServerInit(character);
            go.GetComponent<NetworkObject>().SpawnAsPlayerObject(clientId, true);
            Debug.Log($"[Server] {character.name} joined (client {clientId})");
        }

        void OnClientDisconnected(ulong clientId)
        {
            if (network.IsServer && clientId != NetworkManager.ServerClientId)
            {
                ServerSessions.Remove(clientId);
                return;
            }
            // We are the client and lost (or never got) the connection.
            var reason = string.IsNullOrEmpty(network.DisconnectReason) ? "Disconnected from the server" : network.DisconnectReason;
            if (network.IsListening) network.Shutdown();
            Disconnected?.Invoke(reason);
        }

        // ------------------------------------------------------------ client

        void CreateClientScene()
        {
            var cam = new GameObject("Main Camera", typeof(Camera), typeof(AudioListener), typeof(CameraRig));
            cam.tag = "MainCamera";
            var camera = cam.GetComponent<Camera>();
            camera.clearFlags = CameraClearFlags.SolidColor;
            camera.backgroundColor = new Color(0.55f, 0.72f, 0.92f);
            camera.farClipPlane = 900f;
            cam.transform.position = new Vector3(-120f, 90f, -260f);
            cam.transform.LookAt(Vector3.zero);

            var sun = new GameObject("Sun", typeof(Light)).GetComponent<Light>();
            sun.type = LightType.Directional;
            sun.intensity = 1.2f;
            sun.color = new Color(1f, 0.96f, 0.88f);
            sun.shadows = LightShadows.Soft;
            sun.transform.rotation = Quaternion.Euler(50f, -35f, 0f);

            RenderSettings.ambientMode = UnityEngine.Rendering.AmbientMode.Trilight;
            RenderSettings.ambientSkyColor = new Color(0.62f, 0.72f, 0.85f);
            RenderSettings.ambientEquatorColor = new Color(0.55f, 0.55f, 0.52f);
            RenderSettings.ambientGroundColor = new Color(0.30f, 0.30f, 0.27f);
            RenderSettings.fog = true;
            RenderSettings.fogMode = FogMode.Linear;
            RenderSettings.fogColor = camera.backgroundColor;
            RenderSettings.fogStartDistance = 180f;
            RenderSettings.fogEndDistance = 850f;

            if (FindAnyObjectByType<EventSystem>() == null)
            {
                var es = new GameObject("EventSystem", typeof(EventSystem));
#if ENABLE_INPUT_SYSTEM
                es.AddComponent<InputSystemUIInputModule>();
#else
                es.AddComponent<StandaloneInputModule>();
#endif
            }
        }

        /// <summary>Client: join the online server with a login token from the backend.</summary>
        public void ConnectOnline(string token, string host, ushort port)
        {
            GameConfig.OfflineMode = false;
            transport.SetConnectionData(host, port);
            network.NetworkConfig.ConnectionData = Payload(token, null);
            if (!network.StartClient()) Disconnected?.Invoke("Could not start the connection");
        }

        /// <summary>Client: play alone on this device (host mode) to try the game without an account.</summary>
        public void PlayOffline(string guestName)
        {
            GameConfig.OfflineMode = true;
            transport.SetConnectionData("127.0.0.1", GameConfig.DefaultGamePort, "127.0.0.1");
            network.NetworkConfig.ConnectionData = Payload(null, guestName);
            if (network.StartHost()) gameObject.AddComponent<GameServer>().Init(carPrefab);
            else Disconnected?.Invoke("Could not start offline mode");
        }

        public void Leave()
        {
            if (network.IsListening) network.Shutdown();
            var server = GetComponent<GameServer>();
            if (server != null) Destroy(server);
            Disconnected?.Invoke("You left the game");
        }

        static byte[] Payload(string token, string guestName) =>
            Encoding.UTF8.GetBytes(JsonUtility.ToJson(new ConnectPayload { token = token, guestName = guestName, version = GameConfig.Version }));
    }
}
