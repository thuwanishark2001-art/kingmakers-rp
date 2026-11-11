using System.Collections.Generic;
using Kingmakers.Player;
using Kingmakers.Vehicles;
using Kingmakers.World;
using UnityEngine;
using UnityEngine.UIElements;

namespace Kingmakers.UI
{
    /// <summary>In-game HUD: touch controls, money, speedometer, chat, phone and world labels.</summary>
    public class Hud
    {
        const float JoystickRadius = 120f;
        const int MaxChatLines = 7;
        const float LabelRange = 60f;

        readonly VisualElement screen, joystick, knob, lookArea, labelsLayer, chatLog, toasts, speedo, fuelFill, chatRow, phone;
        readonly Label cash, bank, level, location, objective, speed;
        readonly Button actionButton, vehicleButton, sprintButton, brakeButton;
        readonly TextField chatInput;
        readonly Phone phoneUi;

        int joystickPointer = -1, lookPointer = -1;
        Vector2 lastLookPos;

        Poi nearPoi;
        Vehicle nearVehicle;

        bool jobActive, deliverSent;
        Vector3 jobTarget;
        int jobReward;
        GameObject jobBeam, waypointBeam;
        Vector3? waypoint;
        string waypointName;

        readonly List<Label> labelPool = new List<Label>();

        public Hud(VisualElement screen)
        {
            this.screen = screen;
            joystick = screen.Q("joystick");
            knob = screen.Q("knob");
            lookArea = screen.Q("look-area");
            labelsLayer = screen.Q("labels");
            chatLog = screen.Q("chat-log");
            toasts = screen.Q("toasts");
            speedo = screen.Q("speedo");
            fuelFill = screen.Q("fuel-fill");
            chatRow = screen.Q("chat-input-row");
            phone = screen.Q("phone");
            cash = screen.Q<Label>("cash");
            bank = screen.Q<Label>("bank");
            level = screen.Q<Label>("level");
            location = screen.Q<Label>("location");
            objective = screen.Q<Label>("objective");
            speed = screen.Q<Label>("speed");
            actionButton = screen.Q<Button>("btn-action");
            vehicleButton = screen.Q<Button>("btn-vehicle");
            sprintButton = screen.Q<Button>("btn-sprint");
            brakeButton = screen.Q<Button>("btn-brake");
            chatInput = screen.Q<TextField>("chat-input");

            SetupJoystick();
            SetupLook();
            SetupHold(sprintButton, held => GameInput.SprintHeld = held);
            SetupHold(brakeButton, held => GameInput.BrakeHeld = held);
            actionButton.clicked += DoAction;
            vehicleButton.clicked += DoVehicle;
            screen.Q<Button>("btn-phone").clicked += () => phoneUi.Toggle();
            screen.Q<Button>("btn-chat").clicked += ToggleChat;
            screen.Q<Button>("btn-chat-send").clicked += SendChat;
            chatInput.RegisterCallback<FocusInEvent>(_ => GameInput.TypingInUi = true);
            chatInput.RegisterCallback<FocusOutEvent>(_ => GameInput.TypingInUi = false);
            chatInput.RegisterCallback<KeyDownEvent>(e =>
            {
                if (e.keyCode == KeyCode.Return || e.keyCode == KeyCode.KeypadEnter) SendChat();
            });

            phoneUi = new Phone(phone, this);

            PlayerState.Notified += Toast;
            PlayerState.ChatReceived += (from, text) => AddChat($"{from}: {text}", false);
            PlayerState.JobChanged += OnJobChanged;
        }

        public void OnShown()
        {
            phone.AddToClassList("hidden");
            chatRow.AddToClassList("hidden");
            chatLog.Clear();
            toasts.Clear();
            OnJobChanged(Vector3.zero, 0, false);
            ClearWaypoint();
            AddChat("Welcome to Kingsport! Open your PHONE for the map, bank and jobs.", true);
        }

        // ------------------------------------------------------------ per frame

        public void Tick()
        {
            var me = PlayerState.Local;
            if (me == null) return;

            cash.text = $"${me.Cash.Value:N0}";
            bank.text = $"Bank ${me.Bank.Value:N0}";
            level.text = $"Level {me.Level.Value}";
            var pos = me.transform.position;
            location.text = $"{CityLayout.DistrictName(CityLayout.DistrictAt(pos))}, Kingsport";

            var vehicle = me.CurrentVehicle;
            bool driving = vehicle != null;
            speedo.style.display = driving ? DisplayStyle.Flex : DisplayStyle.None;
            sprintButton.style.display = driving ? DisplayStyle.None : DisplayStyle.Flex;
            brakeButton.style.display = driving ? DisplayStyle.Flex : DisplayStyle.None;
            if (driving)
            {
                speed.text = Mathf.RoundToInt(vehicle.SpeedKmh).ToString();
                fuelFill.style.width = Length.Percent(100f * vehicle.Fuel.Value / Vehicle.TankSize);
            }

            FindNearby(pos, driving);
            UpdateObjective(pos);
            UpdateLabels(me);
            phoneUi.Tick(me);

            if (GameInput.Pressed(GameInput.KeyName.Interact)) DoAction();
            if (GameInput.Pressed(GameInput.KeyName.Vehicle)) DoVehicle();
            if (GameInput.Pressed(GameInput.KeyName.Phone)) phoneUi.Toggle();
            if (GameInput.Pressed(GameInput.KeyName.Chat)) ToggleChat();
        }

        void FindNearby(Vector3 pos, bool driving)
        {
            nearPoi = null;
            float best = float.MaxValue;
            foreach (var poi in CityBuilder.Layout.Pois)
            {
                float d = Vector3.Distance(pos, poi.Position);
                if (d <= poi.Radius && d < best) { best = d; nearPoi = poi; }
            }
            actionButton.style.display = nearPoi != null ? DisplayStyle.Flex : DisplayStyle.None;
            if (nearPoi != null) actionButton.text = ActionText(nearPoi.Type);

            nearVehicle = null;
            if (!driving)
            {
                best = 5f;
                foreach (var v in Vehicle.All)
                {
                    float d = Vector3.Distance(pos, v.transform.position);
                    if (d < best && !v.HasDriver) { best = d; nearVehicle = v; }
                }
            }
            vehicleButton.style.display = driving || nearVehicle != null ? DisplayStyle.Flex : DisplayStyle.None;
            vehicleButton.text = driving ? "EXIT" : "ENTER";
        }

        static string ActionText(PoiType t) => t switch
        {
            PoiType.Atm or PoiType.Bank => "BANK",
            PoiType.DeliveryDepot => "START JOB",
            PoiType.Dealership => "RENT CAR $250",
            PoiType.GasStation => "REFUEL",
            _ => "INFO",
        };

        void UpdateObjective(Vector3 pos)
        {
            if (jobActive)
            {
                float d = Vector3.Distance(pos, jobTarget);
                objective.text = $"Deliver the parcel: {d:0} m  (${jobReward})";
                objective.style.display = DisplayStyle.Flex;
                if (d < 6f && !deliverSent)
                {
                    deliverSent = true;
                    PlayerState.Local.RequestDeliver();
                }
            }
            else if (waypoint.HasValue)
            {
                float d = Vector3.Distance(pos, waypoint.Value);
                objective.text = $"GPS: {waypointName}  {d:0} m";
                objective.style.display = DisplayStyle.Flex;
                if (d < 8f) ClearWaypoint();
            }
            else objective.style.display = DisplayStyle.None;
        }

        void UpdateLabels(PlayerState me)
        {
            var cam = Camera.main;
            int used = 0;
            if (cam != null && screen.panel != null)
            {
                var camPos = cam.transform.position;
                foreach (var p in PlayerState.All)
                {
                    if (p == me || p.InVehicle) continue;
                    used = PlaceLabel(used, cam, p.transform.position + Vector3.up * 2.3f, p.CharacterName.Value.ToString(), false, camPos);
                }
                foreach (var poi in CityBuilder.Layout.Pois)
                    used = PlaceLabel(used, cam, poi.Position + Vector3.up * 2.5f, poi.Name, true, camPos);
            }
            for (int i = used; i < labelPool.Count; i++) labelPool[i].style.display = DisplayStyle.None;
        }

        int PlaceLabel(int index, Camera cam, Vector3 world, string text, bool poi, Vector3 camPos)
        {
            if (Vector3.Distance(camPos, world) > LabelRange) return index;
            var vp = cam.WorldToViewportPoint(world);
            if (vp.z <= 0f || vp.x < 0f || vp.x > 1f || vp.y < 0f || vp.y > 1f) return index;

            if (index == labelPool.Count)
            {
                var l = new Label { pickingMode = PickingMode.Ignore, enableRichText = false };
                l.AddToClassList("world-label");
                labelsLayer.Add(l);
                labelPool.Add(l);
            }
            var label = labelPool[index];
            var panelPos = RuntimePanelUtils.CameraTransformWorldToPanel(screen.panel, world, cam);
            label.text = text;
            label.EnableInClassList("poi", poi);
            label.style.left = panelPos.x;
            label.style.top = panelPos.y;
            label.style.display = DisplayStyle.Flex;
            return index + 1;
        }

        // ------------------------------------------------------------ actions

        void DoAction()
        {
            var me = PlayerState.Local;
            if (me == null || nearPoi == null) return;
            if (nearPoi.Type == PoiType.Atm || nearPoi.Type == PoiType.Bank) phoneUi.Open("bank");
            else me.RequestInteract(nearPoi.Id);
        }

        void DoVehicle()
        {
            var me = PlayerState.Local;
            if (me == null) return;
            if (me.InVehicle) me.RequestExitVehicle();
            else if (nearVehicle != null) me.RequestEnterVehicle(nearVehicle);
        }

        void OnJobChanged(Vector3 target, int reward, bool active)
        {
            jobActive = active;
            jobTarget = target;
            jobReward = reward;
            deliverSent = false;
            SetBeam(ref jobBeam, active ? target : (Vector3?)null, new Color(1f, 0.6f, 0.1f));
        }

        public bool JobActive => jobActive;

        public void SetWaypoint(Vector3 target, string name)
        {
            waypoint = target;
            waypointName = name;
            SetBeam(ref waypointBeam, target, new Color(0.3f, 0.7f, 1f));
        }

        void ClearWaypoint()
        {
            waypoint = null;
            SetBeam(ref waypointBeam, null, Color.white);
        }

        static void SetBeam(ref GameObject beam, Vector3? at, Color color)
        {
            if (!at.HasValue)
            {
                if (beam != null) beam.SetActive(false);
                return;
            }
            if (beam == null)
            {
                beam = GameObject.CreatePrimitive(PrimitiveType.Cylinder);
                beam.name = "Beam";
                Object.Destroy(beam.GetComponent<Collider>());
                beam.transform.localScale = new Vector3(1.2f, 40f, 1.2f);
            }
            Materials.Paint(beam, color, unlit: true);
            beam.transform.position = at.Value + Vector3.up * 40f;
            beam.SetActive(true);
        }

        // ------------------------------------------------------------ chat & toasts

        void ToggleChat()
        {
            bool show = chatRow.ClassListContains("hidden");
            chatRow.EnableInClassList("hidden", !show);
            if (show) chatInput.Focus();
            else GameInput.TypingInUi = false;
        }

        void SendChat()
        {
            PlayerState.Local?.SendChat(chatInput.value);
            chatInput.value = "";
            chatRow.AddToClassList("hidden");
            GameInput.TypingInUi = false;
        }

        void AddChat(string text, bool system)
        {
            var line = new Label(text) { pickingMode = PickingMode.Ignore, enableRichText = false };
            line.AddToClassList("chat-line");
            line.EnableInClassList("system", system);
            chatLog.Add(line);
            while (chatLog.childCount > MaxChatLines) chatLog.RemoveAt(0);
        }

        public void Toast(string text)
        {
            var toast = new Label(text) { pickingMode = PickingMode.Ignore, enableRichText = false };
            toast.AddToClassList("toast");
            toasts.Add(toast);
            while (toasts.childCount > 3) toasts.RemoveAt(0);
            toast.schedule.Execute(() => toast.RemoveFromHierarchy()).StartingIn(4000);
        }

        // ------------------------------------------------------------ touch controls

        void SetupJoystick()
        {
            joystick.RegisterCallback<PointerDownEvent>(e =>
            {
                joystickPointer = e.pointerId;
                joystick.CapturePointer(e.pointerId);
                MoveKnob(e.localPosition);
            });
            joystick.RegisterCallback<PointerMoveEvent>(e =>
            {
                if (e.pointerId == joystickPointer) MoveKnob(e.localPosition);
            });
            EventCallback<PointerUpEvent> release = e =>
            {
                if (e.pointerId != joystickPointer) return;
                joystick.ReleasePointer(e.pointerId);
                joystickPointer = -1;
                GameInput.Joystick = Vector2.zero;
                knob.style.translate = new Translate(0, 0);
            };
            joystick.RegisterCallback(release);
            joystick.RegisterCallback<PointerCaptureOutEvent>(_ =>
            {
                joystickPointer = -1;
                GameInput.Joystick = Vector2.zero;
                knob.style.translate = new Translate(0, 0);
            });
        }

        void MoveKnob(Vector3 local)
        {
            var center = new Vector2(joystick.layout.width / 2f, joystick.layout.height / 2f);
            var offset = Vector2.ClampMagnitude((Vector2)local - center, JoystickRadius);
            knob.style.translate = new Translate(offset.x, offset.y);
            GameInput.Joystick = new Vector2(offset.x, -offset.y) / JoystickRadius; // UI y grows downwards
        }

        void SetupLook()
        {
            lookArea.RegisterCallback<PointerDownEvent>(e =>
            {
                if (lookPointer != -1) return;
                lookPointer = e.pointerId;
                lastLookPos = e.position;
                lookArea.CapturePointer(e.pointerId);
            });
            lookArea.RegisterCallback<PointerMoveEvent>(e =>
            {
                if (e.pointerId != lookPointer) return;
                Vector2 p = e.position;
                var d = p - lastLookPos;
                lastLookPos = p;
                GameInput.TouchLook += new Vector2(d.x, -d.y) * 2f;
            });
            lookArea.RegisterCallback<PointerUpEvent>(e =>
            {
                if (e.pointerId != lookPointer) return;
                lookArea.ReleasePointer(e.pointerId);
                lookPointer = -1;
            });
            lookArea.RegisterCallback<PointerCaptureOutEvent>(_ => lookPointer = -1);
        }

        static void SetupHold(Button button, System.Action<bool> set)
        {
            button.RegisterCallback<PointerDownEvent>(_ => set(true), TrickleDown.TrickleDown);
            button.RegisterCallback<PointerUpEvent>(_ => set(false), TrickleDown.TrickleDown);
            button.RegisterCallback<PointerLeaveEvent>(_ => set(false));
        }
    }
}
