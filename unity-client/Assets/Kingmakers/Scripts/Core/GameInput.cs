using UnityEngine;
#if ENABLE_INPUT_SYSTEM
using UnityEngine.InputSystem;
#endif

namespace Kingmakers
{
    /// <summary>
    /// One place to read movement: the on-screen joystick and look area (set by the HUD)
    /// plus keyboard and mouse for testing on desktop.
    /// </summary>
    public static class GameInput
    {
        public static Vector2 Joystick;     // set by the HUD, -1..1
        public static Vector2 TouchLook;    // pixels dragged this frame, set by the HUD
        public static bool SprintHeld;      // set by the HUD
        public static bool BrakeHeld;       // set by the HUD
        public static bool TypingInUi;      // true while a text field has focus

        public static Vector2 Move
        {
            get
            {
                var v = Joystick;
                if (!TypingInUi)
                {
                    v.x += Key(KeyName.Right) - Key(KeyName.Left);
                    v.y += Key(KeyName.Up) - Key(KeyName.Down);
                }
                return Vector2.ClampMagnitude(v, 1f);
            }
        }

        public static bool Sprint => SprintHeld || (!TypingInUi && Key(KeyName.Sprint) > 0f);
        public static bool Brake => BrakeHeld || (!TypingInUi && Key(KeyName.Brake) > 0f);

        /// <summary>Camera look delta in pixels for this frame (touch drag or right mouse drag).</summary>
        public static Vector2 ConsumeLook()
        {
            var d = TouchLook;
            TouchLook = Vector2.zero;
#if ENABLE_INPUT_SYSTEM
            var mouse = Mouse.current;
            if (mouse != null && mouse.rightButton.isPressed) d += mouse.delta.ReadValue();
#elif ENABLE_LEGACY_INPUT_MANAGER
            if (Input.GetMouseButton(1)) d += new Vector2(Input.GetAxis("Mouse X"), Input.GetAxis("Mouse Y")) * 10f;
#endif
            return d;
        }

        public enum KeyName { Up, Down, Left, Right, Sprint, Brake, Interact, Vehicle, Chat, Phone }

        public static bool Pressed(KeyName k)
        {
            if (TypingInUi) return false;
#if ENABLE_INPUT_SYSTEM
            var kb = Keyboard.current;
            return kb != null && ToKey(kb, k).wasPressedThisFrame;
#elif ENABLE_LEGACY_INPUT_MANAGER
            return Input.GetKeyDown(ToKeyCode(k));
#else
            return false;
#endif
        }

        static float Key(KeyName k)
        {
#if ENABLE_INPUT_SYSTEM
            var kb = Keyboard.current;
            return kb != null && ToKey(kb, k).isPressed ? 1f : 0f;
#elif ENABLE_LEGACY_INPUT_MANAGER
            return Input.GetKey(ToKeyCode(k)) ? 1f : 0f;
#else
            return 0f;
#endif
        }

#if ENABLE_INPUT_SYSTEM
        static UnityEngine.InputSystem.Controls.KeyControl ToKey(Keyboard kb, KeyName k) => k switch
        {
            KeyName.Up => kb.wKey,
            KeyName.Down => kb.sKey,
            KeyName.Left => kb.aKey,
            KeyName.Right => kb.dKey,
            KeyName.Sprint => kb.leftShiftKey,
            KeyName.Brake => kb.spaceKey,
            KeyName.Interact => kb.eKey,
            KeyName.Vehicle => kb.fKey,
            KeyName.Chat => kb.tKey,
            _ => kb.pKey,
        };
#elif ENABLE_LEGACY_INPUT_MANAGER
        static KeyCode ToKeyCode(KeyName k) => k switch
        {
            KeyName.Up => KeyCode.W,
            KeyName.Down => KeyCode.S,
            KeyName.Left => KeyCode.A,
            KeyName.Right => KeyCode.D,
            KeyName.Sprint => KeyCode.LeftShift,
            KeyName.Brake => KeyCode.Space,
            KeyName.Interact => KeyCode.E,
            KeyName.Vehicle => KeyCode.F,
            KeyName.Chat => KeyCode.T,
            _ => KeyCode.P,
        };
#endif
    }
}
