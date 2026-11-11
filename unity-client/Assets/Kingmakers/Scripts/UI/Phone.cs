using Kingmakers.Player;
using Kingmakers.World;
using UnityEngine;
using UnityEngine.UIElements;

namespace Kingmakers.UI
{
    /// <summary>KingPhone: Bank, GPS, Jobs and Settings apps.</summary>
    public class Phone
    {
        static readonly string[] Pages = { "home", "bank", "gps", "jobs", "settings" };

        readonly VisualElement phone;
        readonly Hud hud;
        readonly Label title, balance, cashLine, jobStatus, settingsInfo;
        readonly TextField amount, sendTo;
        readonly ScrollView gpsList;
        string page = "home";

        public Phone(VisualElement phone, Hud hud)
        {
            this.phone = phone;
            this.hud = hud;
            title = phone.Q<Label>("phone-title");
            balance = phone.Q<Label>("bank-balance");
            cashLine = phone.Q<Label>("bank-cash");
            jobStatus = phone.Q<Label>("job-status");
            settingsInfo = phone.Q<Label>("settings-info");
            amount = phone.Q<TextField>("bank-amount");
            sendTo = phone.Q<TextField>("bank-to");
            gpsList = phone.Q<ScrollView>("page-gps");

            foreach (var field in new[] { amount, sendTo })
            {
                field.RegisterCallback<FocusInEvent>(_ => GameInput.TypingInUi = true);
                field.RegisterCallback<FocusOutEvent>(_ => GameInput.TypingInUi = false);
            }

            phone.Q<Button>("phone-back").clicked += () => Show("home");
            phone.Q<Button>("phone-close").clicked += Close;
            phone.Q<Button>("app-bank").clicked += () => Show("bank");
            phone.Q<Button>("app-gps").clicked += () => Show("gps");
            phone.Q<Button>("app-jobs").clicked += () => Show("jobs");
            phone.Q<Button>("app-settings").clicked += () => Show("settings");

            phone.Q<Button>("bank-deposit").clicked += () => Bank(true);
            phone.Q<Button>("bank-withdraw").clicked += () => Bank(false);
            phone.Q<Button>("bank-send").clicked += Send;
            phone.Q<Button>("job-quit").clicked += () => PlayerState.Local?.RequestQuitJob();
            phone.Q<Button>("settings-leave").clicked += () => { Close(); GameBootstrap.Instance.Leave(); };
        }

        public void Toggle()
        {
            if (phone.ClassListContains("hidden")) Open("home");
            else Close();
        }

        public void Open(string pageName)
        {
            phone.RemoveFromClassList("hidden");
            Show(pageName);
        }

        void Close()
        {
            phone.AddToClassList("hidden");
            GameInput.TypingInUi = false;
        }

        void Show(string pageName)
        {
            page = pageName;
            foreach (var p in Pages)
                phone.Q("page-" + p).style.display = p == pageName ? DisplayStyle.Flex : DisplayStyle.None;
            title.text = pageName switch
            {
                "bank" => "Crown Bank",
                "gps" => "GPS",
                "jobs" => "Jobs",
                "settings" => "Settings",
                _ => "KingPhone",
            };
            if (pageName == "gps") BuildGps();
        }

        public void Tick(PlayerState me)
        {
            if (phone.ClassListContains("hidden")) return;
            switch (page)
            {
                case "bank":
                    balance.text = $"${me.Bank.Value:N0}";
                    cashLine.text = $"Cash on hand: ${me.Cash.Value:N0}";
                    break;
                case "jobs":
                    jobStatus.text = hud.JobActive ? "You are on a delivery run." : "You have no job right now.";
                    break;
                case "settings":
                    settingsInfo.text = $"{me.CharacterName.Value}\nKingsport v{GameConfig.Version}" +
                                        (GameConfig.OfflineMode ? "\nOffline mode: progress is not saved" : "");
                    break;
            }
        }

        void BuildGps()
        {
            gpsList.Clear();
            var me = PlayerState.Local;
            var from = me != null ? me.transform.position : Vector3.zero;
            foreach (var poi in CityBuilder.Layout.Pois)
            {
                var target = poi;
                var item = new Button(() =>
                {
                    hud.SetWaypoint(target.Position, target.Name);
                    hud.Toast("Route set to " + target.Name);
                    Close();
                }) { text = $"{poi.Name}   {Vector3.Distance(from, poi.Position):0} m" };
                item.AddToClassList("gps-item");
                gpsList.Add(item);
            }
        }

        void Bank(bool deposit)
        {
            if (!long.TryParse(amount.value.Trim().Replace(",", ""), out var value) || value <= 0)
            {
                hud.Toast("Enter an amount");
                return;
            }
            PlayerState.Local?.RequestBank(deposit, value);
        }

        void Send()
        {
            if (!long.TryParse(amount.value.Trim().Replace(",", ""), out var value) || value <= 0)
            {
                hud.Toast("Enter an amount");
                return;
            }
            if (string.IsNullOrWhiteSpace(sendTo.value))
            {
                hud.Toast("Enter who to send to");
                return;
            }
            PlayerState.Local?.RequestTransfer(sendTo.value.Trim(), value);
        }
    }
}
