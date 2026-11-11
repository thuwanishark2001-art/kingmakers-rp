using UnityEngine;
using UnityEngine.UIElements;

namespace Kingmakers.UI
{
    /// <summary>Log in or create an account with the backend, then join the game server.</summary>
    public class LoginScreen
    {
        readonly TextField username, password, character;
        readonly Label status;
        readonly Button submit, mode, offline;
        bool registering, busy;

        public LoginScreen(VisualElement screen)
        {
            username = screen.Q<TextField>("username");
            password = screen.Q<TextField>("password");
            character = screen.Q<TextField>("character");
            status = screen.Q<Label>("login-status");
            submit = screen.Q<Button>("btn-submit");
            mode = screen.Q<Button>("btn-mode");
            offline = screen.Q<Button>("btn-offline");

            username.value = PlayerPrefs.GetString("km.username", "");
            submit.clicked += Submit;
            mode.clicked += () => SetRegistering(!registering);
            offline.clicked += PlayOffline;
            SetRegistering(false);
        }

        public void Reset(string reason)
        {
            busy = false;
            SetButtons(true);
            status.text = reason ?? "";
        }

        void SetRegistering(bool value)
        {
            registering = value;
            character.style.display = value ? DisplayStyle.Flex : DisplayStyle.None;
            submit.text = value ? "CREATE ACCOUNT" : "LOG IN";
            mode.text = value ? "I already have an account" : "New here? Create an account";
            status.text = "";
        }

        void SetButtons(bool enabled)
        {
            submit.SetEnabled(enabled);
            mode.SetEnabled(enabled);
            offline.SetEnabled(enabled);
        }

        async void Submit()
        {
            if (busy) return;
            var user = username.value.Trim();
            if (user.Length == 0 || password.value.Length == 0) { status.text = "Enter your username and password"; return; }

            busy = true;
            SetButtons(false);
            status.text = registering ? "Creating your account..." : "Signing in...";

            var auth = registering
                ? await BackendClient.Register(user, password.value, character.value.Trim())
                : await BackendClient.Login(user, password.value);
            if (!auth.Ok) { Reset(auth.Error); return; }
            PlayerPrefs.SetString("km.username", user);

            var server = await BackendClient.GetGameServer();
            if (!server.Ok) { Reset(server.Error); return; }

            status.text = $"Welcome, {auth.Data.character.name}. Connecting...";
            GameBootstrap.Instance.ConnectOnline(auth.Data.token, server.Data.host, (ushort)server.Data.port);
        }

        void PlayOffline()
        {
            if (busy) return;
            busy = true;
            SetButtons(false);
            var name = character.value.Trim();
            GameBootstrap.Instance.PlayOffline(string.IsNullOrEmpty(name) ? "Guest_Player" : name);
        }
    }
}
