using UnityEngine;

namespace Kingmakers.World
{
    /// <summary>Turns a <see cref="CityLayout"/> into GameObjects. Runs on server and clients.</summary>
    public class CityBuilder : MonoBehaviour
    {
        public static CityLayout Layout { get; private set; }

        static readonly Color Grass = new Color(0.33f, 0.50f, 0.27f);
        static readonly Color Asphalt = new Color(0.17f, 0.17f, 0.19f);
        static readonly Color Pavement = new Color(0.62f, 0.61f, 0.58f);
        static readonly Color Lane = new Color(0.95f, 0.85f, 0.35f);

        public static CityLayout Build(bool withVisuals)
        {
            Layout = CityLayout.Generate(GameConfig.CitySeed);
            var root = new GameObject("Kingsport");
            root.AddComponent<CityBuilder>();

            float size = CityLayout.Blocks * CityLayout.Pitch + 400f;
            Box(root, "Ground", new Vector3(0f, -0.5f, 0f), new Vector3(size, 1f, size), Grass, collider: true);

            float roadLength = CityLayout.Blocks * CityLayout.Pitch + CityLayout.Road;
            for (int k = 0; k <= CityLayout.Blocks; k++)
            {
                float c = (k - CityLayout.Blocks / 2f) * CityLayout.Pitch;
                Box(root, "Road NS", new Vector3(c, 0.01f, 0f), new Vector3(CityLayout.Road, 0.02f, roadLength), Asphalt);
                Box(root, "Road EW", new Vector3(0f, 0.012f, c), new Vector3(roadLength, 0.02f, CityLayout.Road), Asphalt);
                if (withVisuals)
                {
                    Box(root, "Lane NS", new Vector3(c, 0.025f, 0f), new Vector3(0.25f, 0.01f, roadLength), Lane);
                    Box(root, "Lane EW", new Vector3(0f, 0.026f, c), new Vector3(roadLength, 0.01f, 0.25f), Lane);
                }
            }

            for (int i = 0; i < CityLayout.Blocks; i++)
            for (int j = 0; j < CityLayout.Blocks; j++)
                Box(root, "Block", CityLayout.BlockCenter(i, j) + new Vector3(0f, 0.04f, 0f),
                    new Vector3(CityLayout.Block, 0.08f, CityLayout.Block), Pavement);

            foreach (var b in Layout.Buildings)
            {
                var pos = b.Center + new Vector3(0f, b.Size.y / 2f, 0f);
                Box(root, "Building", pos, b.Size, b.Color, collider: true);
                if (b.Roof && withVisuals)
                    Box(root, "Roof", pos + new Vector3(0f, b.Size.y / 2f + 1.5f, 0f),
                        new Vector3(b.Size.x * 0.4f, 3f, b.Size.z * 0.4f), b.Color * 0.8f, collider: true);
            }

            if (withVisuals)
            {
                foreach (var poi in Layout.Pois)
                {
                    var marker = GameObject.CreatePrimitive(PrimitiveType.Cylinder);
                    marker.name = "Marker " + poi.Name;
                    marker.transform.SetParent(root.transform, false);
                    marker.transform.position = poi.Position + new Vector3(0f, 0.1f, 0f);
                    marker.transform.localScale = new Vector3(1.6f, 0.05f, 1.6f);
                    Destroy(marker.GetComponent<Collider>());
                    Materials.Paint(marker, PoiColor(poi.Type), unlit: true);
                }
                StaticBatchingUtility.Combine(root);
            }

            return Layout;
        }

        public static Color PoiColor(PoiType t) => t switch
        {
            PoiType.Atm or PoiType.Bank => new Color(0.2f, 0.9f, 0.4f),
            PoiType.DeliveryDepot => new Color(1f, 0.6f, 0.1f),
            PoiType.Dealership => new Color(0.3f, 0.6f, 1f),
            PoiType.GasStation => new Color(1f, 0.85f, 0.1f),
            PoiType.Hospital => new Color(1f, 0.3f, 0.3f),
            PoiType.Police => new Color(0.2f, 0.4f, 1f),
            _ => Color.white,
        };

        static void Box(GameObject root, string name, Vector3 pos, Vector3 size, Color color, bool collider = false)
        {
            var go = GameObject.CreatePrimitive(PrimitiveType.Cube);
            go.name = name;
            go.isStatic = true;
            go.transform.SetParent(root.transform, false);
            go.transform.position = pos;
            go.transform.localScale = size;
            if (!collider) Destroy(go.GetComponent<Collider>());
            Materials.Paint(go, color);
        }
    }
}
