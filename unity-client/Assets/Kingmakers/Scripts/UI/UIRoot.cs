using UnityEngine;
using UnityEngine.UIElements;

namespace Kingmakers.UI
{
    /// <summary>
    /// Creates the UI Toolkit document at runtime from Resources/UI (Main.uxml, Main.uss,
    /// RuntimeTheme.tss) and switches between the login screen and the HUD.
    /// </summary>
    public class UIRoot : MonoBehaviour
    {
        UIDocument document;
        VisualElement root, loginScreen, hudScreen;
        LoginScreen login;
        Hud hud;

        public static UIRoot Create()
        {
            var go = new GameObject("UI");
            go.SetActive(false);

            var settings = ScriptableObject.CreateInstance<PanelSettings>();
            settings.themeStyleSheet = Resources.Load<ThemeStyleSheet>("UI/RuntimeTheme");
            settings.scaleMode = PanelScaleMode.ScaleWithScreenSize;
            settings.referenceResolution = new Vector2Int(1920, 1080);
            settings.screenMatchMode = PanelScreenMatchMode.MatchWidthOrHeight;
            settings.match = 0.5f;

            var doc = go.AddComponent<UIDocument>();
            doc.panelSettings = settings;
            doc.visualTreeAsset = Resources.Load<VisualTreeAsset>("UI/Main");

            var ui = go.AddComponent<UIRoot>();
            ui.document = doc;
            go.SetActive(true);
            return ui;
        }

        void OnEnable()
        {
            root = document.rootVisualElement;
            var style = Resources.Load<StyleSheet>("UI/Main");
            if (style != null) root.styleSheets.Add(style);

            loginScreen = root.Q("login");
            hudScreen = root.Q("hud");
            login = new LoginScreen(loginScreen);
            hud = new Hud(hudScreen);

            GameBootstrap.Instance.Connected += ShowHud;
            GameBootstrap.Instance.Disconnected += ShowLogin;
            ShowLogin(null);
        }

        void OnDisable()
        {
            if (GameBootstrap.Instance == null) return;
            GameBootstrap.Instance.Connected -= ShowHud;
            GameBootstrap.Instance.Disconnected -= ShowLogin;
        }

        void ShowHud()
        {
            loginScreen.AddToClassList("hidden");
            hudScreen.RemoveFromClassList("hidden");
            hud.OnShown();
        }

        void ShowLogin(string reason)
        {
            hudScreen.AddToClassList("hidden");
            loginScreen.RemoveFromClassList("hidden");
            login.Reset(reason);
            GameInput.Joystick = Vector2.zero;
        }

        void Update()
        {
            ApplySafeArea();
            if (!hudScreen.ClassListContains("hidden")) hud.Tick();
        }

        Rect lastSafeArea;

        /// <summary>Keep controls clear of notches and rounded corners.</summary>
        void ApplySafeArea()
        {
            var safe = Screen.safeArea;
            if (safe == lastSafeArea || root.panel == null || Screen.width == 0) return;
            lastSafeArea = safe;
            float scaleX = root.layout.width / Screen.width;
            float scaleY = root.layout.height / Screen.height;
            if (float.IsNaN(scaleX) || scaleX <= 0f) { lastSafeArea = default; return; }
            root.style.paddingLeft = safe.xMin * scaleX;
            root.style.paddingRight = (Screen.width - safe.xMax) * scaleX;
            root.style.paddingTop = (Screen.height - safe.yMax) * scaleY;
            root.style.paddingBottom = safe.yMin * scaleY;
        }
    }
}
