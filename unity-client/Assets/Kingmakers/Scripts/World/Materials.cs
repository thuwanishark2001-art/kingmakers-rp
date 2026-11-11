using System.Collections.Generic;
using UnityEngine;

namespace Kingmakers.World
{
    /// <summary>
    /// Colour variants of the two materials the editor setup creates in Resources/Materials
    /// (Lit and Unlit). Loading them from Resources keeps their shaders in the build.
    /// </summary>
    public static class Materials
    {
        static Material litBase, unlitBase;
        static readonly Dictionary<Color32, Material> Lit = new Dictionary<Color32, Material>();
        static readonly Dictionary<Color32, Material> Unlit = new Dictionary<Color32, Material>();

        public static Material GetLit(Color color) => Get(Lit, ref litBase, "Materials/Lit", color);
        public static Material GetUnlit(Color color) => Get(Unlit, ref unlitBase, "Materials/Unlit", color);

        static Material Get(Dictionary<Color32, Material> cache, ref Material baseMat, string path, Color color)
        {
            Color32 key = color;
            if (cache.TryGetValue(key, out var m)) return m;
            if (baseMat == null)
            {
                baseMat = Resources.Load<Material>(path);
                if (baseMat == null)
                {
                    Debug.LogError($"[Kingmakers] Missing {path}. Run Kingmakers > Run Project Setup in the editor.");
                    baseMat = new Material(Shader.Find("Universal Render Pipeline/Lit") ?? Shader.Find("Standard"));
                }
            }
            m = new Material(baseMat) { color = color, enableInstancing = true };
            cache[key] = m;
            return m;
        }

        public static void Paint(GameObject go, Color color, bool unlit = false)
        {
            var r = go.GetComponent<Renderer>();
            if (r != null) r.sharedMaterial = unlit ? GetUnlit(color) : GetLit(color);
        }
    }
}
