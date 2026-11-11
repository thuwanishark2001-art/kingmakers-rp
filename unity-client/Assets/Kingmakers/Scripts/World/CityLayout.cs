using System.Collections.Generic;
using UnityEngine;

namespace Kingmakers.World
{
    public enum PoiType { Hospital, CityHall, Bank, Atm, Police, DeliveryDepot, Dealership, GasStation }

    public enum District { Downtown, Residential, Harbor, Hills }

    public class Poi
    {
        public int Id;
        public PoiType Type;
        public string Name;
        public Vector3 Position;   // interaction point, on the sidewalk
        public float Radius = 4f;
    }

    public class BuildingSpec
    {
        public Vector3 Center;     // ground centre of the footprint
        public Vector3 Size;
        public Color Color;
        public bool Roof;
    }

    /// <summary>
    /// Kingsport: an original grid city. The layout is pure data, generated from a fixed
    /// seed with our own PRNG, so the server and every client build exactly the same city.
    /// </summary>
    public class CityLayout
    {
        public const int Blocks = 8;          // blocks per side
        public const float Block = 64f;       // block size (sidewalk included)
        public const float Road = 14f;        // road width
        public const float Pitch = Block + Road;
        public const float Sidewalk = 3f;
        public static float HalfExtent => Blocks * Pitch / 2f;

        public readonly List<Poi> Pois = new List<Poi>();
        public readonly List<BuildingSpec> Buildings = new List<BuildingSpec>();
        public readonly List<Vector3> DeliveryPoints = new List<Vector3>();
        public readonly List<Pose> ParkingSpots = new List<Pose>();
        public readonly List<Pose> RentalSpots = new List<Pose>();
        public Pose SpawnPoint;

        static readonly Dictionary<Vector2Int, (PoiType type, string name)> PoiBlocks =
            new Dictionary<Vector2Int, (PoiType, string)>
            {
                { new Vector2Int(3, 3), (PoiType.Hospital, "Kingsport General Hospital") },
                { new Vector2Int(4, 4), (PoiType.CityHall, "City Hall") },
                { new Vector2Int(3, 4), (PoiType.Bank, "Crown Bank") },
                { new Vector2Int(4, 3), (PoiType.Police, "Police Department") },
                { new Vector2Int(2, 0), (PoiType.DeliveryDepot, "Harbor Delivery Depot") },
                { new Vector2Int(5, 1), (PoiType.Dealership, "Bayside Motors") },
                { new Vector2Int(1, 2), (PoiType.GasStation, "West Fuel") },
                { new Vector2Int(6, 5), (PoiType.GasStation, "East Fuel") },
            };

        static readonly Vector2Int[] ExtraAtmBlocks = { new Vector2Int(1, 6), new Vector2Int(6, 1), new Vector2Int(5, 6), new Vector2Int(2, 3) };

        public static Vector3 BlockCenter(int i, int j) =>
            new Vector3((i - (Blocks - 1) / 2f) * Pitch, 0f, (j - (Blocks - 1) / 2f) * Pitch);

        public static District DistrictOf(int i, int j)
        {
            if (j == 0) return District.Harbor;
            if (j >= 6) return District.Hills;
            if (i >= 2 && i <= 5 && j >= 2 && j <= 5) return District.Downtown;
            return District.Residential;
        }

        public static string DistrictName(District d) => d switch
        {
            District.Downtown => "Downtown",
            District.Harbor => "Harbor",
            District.Hills => "Crown Hills",
            _ => "Old Town",
        };

        public static District DistrictAt(Vector3 p)
        {
            int i = Mathf.Clamp(Mathf.FloorToInt(p.x / Pitch + Blocks / 2f), 0, Blocks - 1);
            int j = Mathf.Clamp(Mathf.FloorToInt(p.z / Pitch + Blocks / 2f), 0, Blocks - 1);
            return DistrictOf(i, j);
        }

        public static CityLayout Generate(int seed)
        {
            var city = new CityLayout();
            var rng = new Prng(seed);

            for (int i = 0; i < Blocks; i++)
            for (int j = 0; j < Blocks; j++)
            {
                var center = BlockCenter(i, j);
                var front = center + new Vector3(0f, 0f, -Block / 2f + Sidewalk / 2f); // south sidewalk

                if (PoiBlocks.TryGetValue(new Vector2Int(i, j), out var poi))
                {
                    city.AddPoiBlock(poi.type, poi.name, center, front, rng);
                    continue;
                }

                city.AddLots(DistrictOf(i, j), center, rng);
            }

            int atm = 1;
            foreach (var b in ExtraAtmBlocks)
            {
                var c = BlockCenter(b.x, b.y);
                city.AddPoi(PoiType.Atm, "ATM #" + atm++, c + new Vector3(-Block / 2f + Sidewalk / 2f, 0f, -Block / 2f + Sidewalk / 2f));
            }

            // Kerbside parking along the north-south roads, one spot per block edge.
            for (int k = 1; k < Blocks; k++)
            for (int j = 0; j < Blocks; j++)
            {
                if (rng.Next(100) >= 45) continue;
                float x = (k - Blocks / 2f) * Pitch + Road / 2f - 2.2f;
                float z = BlockCenter(0, j).z + rng.Range(-18f, 18f);
                city.ParkingSpots.Add(new Pose(new Vector3(x, 0.05f, z), Quaternion.Euler(0f, rng.Next(2) == 0 ? 0f : 180f, 0f)));
            }

            return city;
        }

        void AddPoi(PoiType type, string name, Vector3 at)
        {
            Pois.Add(new Poi { Id = Pois.Count, Type = type, Name = name, Position = at });
        }

        void AddPoiBlock(PoiType type, string name, Vector3 center, Vector3 front, Prng rng)
        {
            AddPoi(type, name, front);
            float inner = Block - 2 * Sidewalk;

            switch (type)
            {
                case PoiType.DeliveryDepot:
                    Buildings.Add(new BuildingSpec { Center = center + new Vector3(0f, 0f, 12f), Size = new Vector3(inner - 8f, 10f, 24f), Color = new Color(0.55f, 0.42f, 0.30f) });
                    break;
                case PoiType.Dealership:
                    Buildings.Add(new BuildingSpec { Center = center + new Vector3(0f, 0f, 16f), Size = new Vector3(40f, 7f, 18f), Color = new Color(0.85f, 0.85f, 0.9f), Roof = true });
                    for (int n = 0; n < 4; n++)
                        RentalSpots.Add(new Pose(center + new Vector3(-15f + n * 10f, 0.05f, -10f), Quaternion.identity));
                    break;
                case PoiType.GasStation:
                    Buildings.Add(new BuildingSpec { Center = center + new Vector3(14f, 0f, 14f), Size = new Vector3(14f, 5f, 10f), Color = new Color(0.9f, 0.9f, 0.85f) });
                    Buildings.Add(new BuildingSpec { Center = center + new Vector3(-6f, 6f, -4f), Size = new Vector3(22f, 0.8f, 12f), Color = new Color(0.95f, 0.75f, 0.1f) }); // canopy
                    break;
                case PoiType.Bank:
                    Buildings.Add(new BuildingSpec { Center = center, Size = new Vector3(inner - 6f, 34f, inner - 10f), Color = new Color(0.82f, 0.74f, 0.55f), Roof = true });
                    AddPoi(PoiType.Atm, "Crown Bank ATM", front + new Vector3(8f, 0f, 0f));
                    break;
                case PoiType.Hospital:
                    Buildings.Add(new BuildingSpec { Center = center, Size = new Vector3(inner - 6f, 28f, inner - 12f), Color = new Color(0.92f, 0.92f, 0.95f), Roof = true });
                    SpawnPoint = new Pose(front + new Vector3(0f, 0.1f, -1f), Quaternion.Euler(0f, 180f, 0f));
                    break;
                case PoiType.CityHall:
                    Buildings.Add(new BuildingSpec { Center = center, Size = new Vector3(inner - 14f, 22f, inner - 18f), Color = new Color(0.78f, 0.78f, 0.74f), Roof = true });
                    break;
                case PoiType.Police:
                    Buildings.Add(new BuildingSpec { Center = center, Size = new Vector3(inner - 10f, 18f, inner - 12f), Color = new Color(0.30f, 0.38f, 0.55f), Roof = true });
                    break;
            }
        }

        void AddLots(District district, Vector3 center, Prng rng)
        {
            float inner = Block - 2 * Sidewalk;
            float lot = inner / 2f;
            for (int a = 0; a < 2; a++)
            for (int b = 0; b < 2; b++)
            {
                var lotCenter = center + new Vector3((a - 0.5f) * lot, 0f, (b - 0.5f) * lot);
                float footprint, height;
                Color color;
                switch (district)
                {
                    case District.Downtown:
                        footprint = rng.Range(18f, 26f); height = rng.Range(30f, 90f);
                        color = Color.HSVToRGB(rng.Range(0.52f, 0.62f), rng.Range(0.10f, 0.35f), rng.Range(0.45f, 0.80f));
                        break;
                    case District.Harbor:
                        footprint = rng.Range(20f, 26f); height = rng.Range(8f, 13f);
                        color = Color.HSVToRGB(rng.Range(0.0f, 0.10f), rng.Range(0.25f, 0.50f), rng.Range(0.40f, 0.65f));
                        break;
                    case District.Hills:
                        footprint = rng.Range(12f, 16f); height = rng.Range(6f, 9f);
                        color = Color.HSVToRGB(rng.Range(0.08f, 0.15f), rng.Range(0.10f, 0.25f), rng.Range(0.85f, 0.95f));
                        break;
                    default:
                        footprint = rng.Range(14f, 20f); height = rng.Range(7f, 16f);
                        color = Color.HSVToRGB(rng.Range(0.0f, 0.12f), rng.Range(0.30f, 0.55f), rng.Range(0.55f, 0.80f));
                        break;
                }

                Buildings.Add(new BuildingSpec
                {
                    Center = lotCenter,
                    Size = new Vector3(footprint, height, footprint),
                    Color = color,
                    Roof = district == District.Downtown && rng.Next(3) == 0,
                });

                if (district != District.Downtown)
                {
                    // Door on the sidewalk facing the street this lot borders.
                    float side = b == 0 ? -1f : 1f;
                    DeliveryPoints.Add(new Vector3(lotCenter.x, 0f, center.z + side * (Block / 2f - Sidewalk / 2f)));
                }
            }
        }
    }

    /// <summary>xorshift32: identical results on every platform, unlike System.Random.</summary>
    public class Prng
    {
        uint state;
        public Prng(int seed) { state = (uint)seed == 0 ? 2463534242u : (uint)seed; }

        public uint NextUInt()
        {
            state ^= state << 13;
            state ^= state >> 17;
            state ^= state << 5;
            return state;
        }

        public int Next(int maxExclusive) => (int)(NextUInt() % (uint)maxExclusive);
        public float Value() => (NextUInt() & 0xFFFFFF) / (float)0x1000000;
        public float Range(float min, float max) => min + (max - min) * Value();
    }
}
