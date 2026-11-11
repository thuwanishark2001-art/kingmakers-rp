using System;
using System.IO;
using Kingmakers.Net;
using Kingmakers.Player;
using Kingmakers.Vehicles;
using Unity.Netcode;
using UnityEditor;
using UnityEditor.Build;
using UnityEditor.SceneManagement;
using UnityEngine;
using UnityEngine.Rendering;
using UnityEngine.Rendering.Universal;
using Object = UnityEngine.Object;

namespace Kingmakers.EditorTools
{
    /// <summary>
    /// Creates everything that has to be an asset: render pipeline, materials, network
    /// prefabs, the scene and player settings. Runs by itself the first time the project
    /// opens (when the scene is missing), from Kingmakers > Run Project Setup, and before
    /// every CI build. Safe to run again: existing assets are kept.
    /// </summary>
    public static class ProjectSetup
    {
        const string Root = "Assets/Kingmakers";
        const string SettingsDir = Root + "/Settings";
        const string MaterialsDir = Root + "/Resources/Materials";
        const string PrefabsDir = Root + "/Resources/Prefabs";
        public const string ScenePath = Root + "/Scenes/Main.unity";
        const int IgnoreRaycastLayer = 2;

        [InitializeOnLoadMethod]
        static void AutoRun()
        {
            if (Application.isBatchMode) return;
            EditorApplication.delayCall += () =>
            {
                if (!File.Exists(ScenePath)) Run();
            };
        }

        [MenuItem("Kingmakers/Run Project Setup")]
        public static void Run()
        {
            foreach (var dir in new[] { SettingsDir, MaterialsDir, PrefabsDir, Root + "/Scenes" })
                Directory.CreateDirectory(dir);
            AssetDatabase.Refresh();

            EnsureRenderPipeline();
            var lit = EnsureMaterial("Lit", false);
            EnsureMaterial("Unlit", true);
            EnsurePlayerPrefab(lit);
            EnsureCarPrefab(lit);
            EnsureScene();
            ConfigurePlayer();

            AssetDatabase.SaveAssets();
            Debug.Log("[Kingmakers] Project setup complete. Open " + ScenePath + " and press Play.");
        }

        static void EnsureRenderPipeline()
        {
            try
            {
                var path = SettingsDir + "/URP.asset";
                var urp = AssetDatabase.LoadAssetAtPath<UniversalRenderPipelineAsset>(path);
                if (urp == null)
                {
                    var renderer = ScriptableObject.CreateInstance<UniversalRendererData>();
                    AssetDatabase.CreateAsset(renderer, SettingsDir + "/URP-Renderer.asset");
                    urp = UniversalRenderPipelineAsset.Create(renderer);
                    urp.shadowDistance = 90f;
                    AssetDatabase.CreateAsset(urp, path);
                }
                GraphicsSettings.defaultRenderPipeline = urp;
                int current = QualitySettings.GetQualityLevel();
                for (int i = 0; i < QualitySettings.names.Length; i++)
                {
                    QualitySettings.SetQualityLevel(i, false);
                    QualitySettings.renderPipeline = urp;
                }
                QualitySettings.SetQualityLevel(current, false);
            }
            catch (Exception e)
            {
                // The game still renders with the built-in pipeline; materials fall back below.
                Debug.LogWarning("[Kingmakers] URP setup failed, using the built-in pipeline: " + e.Message);
            }
        }

        static Material EnsureMaterial(string name, bool unlit)
        {
            var path = $"{MaterialsDir}/{name}.mat";
            var mat = AssetDatabase.LoadAssetAtPath<Material>(path);
            if (mat != null) return mat;

            bool urp = GraphicsSettings.defaultRenderPipeline != null;
            Shader shader = unlit
                ? Shader.Find(urp ? "Universal Render Pipeline/Unlit" : "Unlit/Color")
                : (urp ? GraphicsSettings.defaultRenderPipeline.defaultShader : Shader.Find("Standard"));
            if (shader == null) throw new Exception($"Shader for {name} not found");

            mat = new Material(shader) { color = Color.white, enableInstancing = true };
            AssetDatabase.CreateAsset(mat, path);
            return mat;
        }

        static GameObject Part(PrimitiveType type, string name, Transform parent, Vector3 pos, Vector3 scale, Material mat)
        {
            var go = GameObject.CreatePrimitive(type);
            go.name = name;
            go.transform.SetParent(parent, false);
            go.transform.localPosition = pos;
            go.transform.localScale = scale;
            Object.DestroyImmediate(go.GetComponent<Collider>());
            go.GetComponent<Renderer>().sharedMaterial = mat;
            return go;
        }

        static void EnsurePlayerPrefab(Material mat)
        {
            var path = PrefabsDir + "/Player.prefab";
            if (File.Exists(path)) return;

            var root = new GameObject("Player");
            Part(PrimitiveType.Capsule, "Body", root.transform, new Vector3(0f, 1f, 0f), Vector3.one, mat);
            Part(PrimitiveType.Cube, "Visor", root.transform, new Vector3(0f, 1.55f, 0.35f), new Vector3(0.6f, 0.2f, 0.2f), mat);

            var cc = root.AddComponent<CharacterController>();
            cc.center = new Vector3(0f, 1f, 0f);
            cc.height = 2f;
            cc.radius = 0.4f;
            cc.stepOffset = 0.4f;

            root.AddComponent<NetworkObject>();
            var nt = root.AddComponent<OwnerNetworkTransform>();
            nt.SyncScaleX = nt.SyncScaleY = nt.SyncScaleZ = false;
            nt.SyncRotAngleX = nt.SyncRotAngleZ = false;
            root.AddComponent<PlayerState>();
            root.AddComponent<PlayerMotor>();
            SetLayer(root, IgnoreRaycastLayer);

            PrefabUtility.SaveAsPrefabAsset(root, path);
            Object.DestroyImmediate(root);
        }

        static void EnsureCarPrefab(Material mat)
        {
            var path = PrefabsDir + "/Car.prefab";
            if (File.Exists(path)) return;

            var root = new GameObject("Car");
            Part(PrimitiveType.Cube, "Body", root.transform, new Vector3(0f, 0.75f, 0f), new Vector3(2f, 0.8f, 4.4f), mat);
            Part(PrimitiveType.Cube, "Cabin", root.transform, new Vector3(0f, 1.4f, -0.3f), new Vector3(1.8f, 0.6f, 2.2f), mat);
            foreach (var x in new[] { -0.95f, 0.95f })
            foreach (var z in new[] { -1.4f, 1.4f })
            {
                var wheel = Part(PrimitiveType.Cylinder, "Wheel", root.transform, new Vector3(x, 0.4f, z), new Vector3(0.8f, 0.15f, 0.8f), mat);
                wheel.transform.localRotation = Quaternion.Euler(0f, 0f, 90f);
            }

            var box = root.AddComponent<BoxCollider>();
            box.center = new Vector3(0f, 0.85f, 0f);
            box.size = new Vector3(2f, 1.3f, 4.4f);

            var rb = root.AddComponent<Rigidbody>();
            rb.mass = 1200f;
            rb.angularDamping = 3f;
            rb.collisionDetectionMode = CollisionDetectionMode.ContinuousDynamic;

            var no = root.AddComponent<NetworkObject>();
            no.DontDestroyWithOwner = true; // the car stays when its driver disconnects
            var nt = root.AddComponent<OwnerNetworkTransform>();
            nt.SyncScaleX = nt.SyncScaleY = nt.SyncScaleZ = false;
            nt.SyncRotAngleX = nt.SyncRotAngleZ = false;
            root.AddComponent<Vehicle>();
            SetLayer(root, IgnoreRaycastLayer);

            PrefabUtility.SaveAsPrefabAsset(root, path);
            Object.DestroyImmediate(root);
        }

        static void SetLayer(GameObject go, int layer)
        {
            foreach (var t in go.GetComponentsInChildren<Transform>(true)) t.gameObject.layer = layer;
        }

        static void EnsureScene()
        {
            if (!File.Exists(ScenePath))
            {
                if (!Application.isBatchMode && !EditorSceneManager.SaveCurrentModifiedScenesIfUserWantsTo()) return;
                var scene = EditorSceneManager.NewScene(NewSceneSetup.EmptyScene, NewSceneMode.Single);
                new GameObject("Bootstrap").AddComponent<GameBootstrap>();
                EditorSceneManager.SaveScene(scene, ScenePath);
            }
            EditorBuildSettings.scenes = new[] { new EditorBuildSettingsScene(ScenePath, true) };
        }

        static void ConfigurePlayer()
        {
            PlayerSettings.companyName = "Kingmakers";
            PlayerSettings.productName = "Kingmakers RP";
            PlayerSettings.bundleVersion = GameConfig.Version;
            PlayerSettings.runInBackground = true;
            PlayerSettings.insecureHttpOption = InsecureHttpOption.AlwaysAllowed; // backend is plain HTTP for now

            PlayerSettings.defaultInterfaceOrientation = UIOrientation.AutoRotation;
            PlayerSettings.allowedAutorotateToPortrait = false;
            PlayerSettings.allowedAutorotateToPortraitUpsideDown = false;
            PlayerSettings.allowedAutorotateToLandscapeLeft = true;
            PlayerSettings.allowedAutorotateToLandscapeRight = true;

            var android = NamedBuildTarget.Android;
            PlayerSettings.SetApplicationIdentifier(android, "com.kingmakers.rp");
            PlayerSettings.SetScriptingBackend(android, ScriptingImplementation.IL2CPP);
            PlayerSettings.Android.targetArchitectures = AndroidArchitecture.ARM64;
            PlayerSettings.Android.minSdkVersion = AndroidSdkVersions.AndroidApiLevel26;
        }
    }
}
