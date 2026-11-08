// Valdorso - Il launcher: la forma delle rune. FILE GENERATO da Tools/Generatori/alfabeto_rune.py: non toccarlo
// a mano, cambia le forme lì e rilancia: python Tools/Generatori/alfabeto_rune.py Tools/Launcher/src/Rune.cs
// Coordinate da -1 a 1, y in su (come nel gioco). Gli elementi stanno sulla linea di base, i verbi no.

namespace Valdorso.Launcher
{
    public static class Rune
    {
        public static readonly string[] Nomi = { "Aeth", "Nir", "Dorn", "Sil", "Lume", "Var", "Ka", "Ren", "Vel", "Tor", "Ur" };

        public static readonly string[] Significati = { "il fuoco", "l'acqua", "la terra", "il vento", "la luce", "andare, scagliare", "proteggere", "guarire", "vedere, guidare", "fermare", "orso, custode, la porta" };

        public static readonly float[][][] Linee =
        {
            // Aeth (il fuoco)
            new float[][]
            {
                new float[] { 0.000f, 1.000f, 0.000f, -0.850f },
                new float[] { -0.400f, -0.850f, 0.000f, 0.500f, 0.400f, -0.850f },
                new float[] { -0.820f, -0.850f, 0.820f, -0.850f },
            },
            // Nir (l'acqua)
            new float[][]
            {
                new float[] { -0.583f, 0.476f, -0.514f, 0.385f, -0.415f, 0.308f, -0.291f, 0.249f, -0.150f, 0.212f, -0.000f, 0.200f, 0.150f, 0.212f, 0.291f, 0.249f, 0.415f, 0.308f, 0.514f, 0.385f, 0.583f, 0.476f },
                new float[] { -0.583f, -0.024f, -0.514f, -0.115f, -0.415f, -0.192f, -0.291f, -0.251f, -0.150f, -0.288f, -0.000f, -0.300f, 0.150f, -0.288f, 0.291f, -0.251f, 0.415f, -0.192f, 0.514f, -0.115f, 0.583f, -0.024f },
                new float[] { -0.820f, -0.850f, 0.820f, -0.850f },
            },
            // Dorn (la terra)
            new float[][]
            {
                new float[] { -0.580f, -0.850f, 0.000f, 0.850f, 0.580f, -0.850f },
                new float[] { -0.950f, -0.850f, 0.950f, -0.850f },
            },
            // Sil (il vento)
            new float[][]
            {
                new float[] { -0.520f, 0.550f, 0.550f, 0.550f, 0.635f, 0.573f, 0.697f, 0.635f, 0.720f, 0.720f, 0.697f, 0.805f, 0.635f, 0.867f, 0.550f, 0.890f, 0.465f, 0.867f, 0.403f, 0.805f },
                new float[] { -0.820f, 0.150f, 0.820f, 0.150f },
                new float[] { -0.550f, -0.300f, 0.550f, -0.300f },
                new float[] { -0.820f, -0.850f, 0.820f, -0.850f },
            },
            // Lume (la luce)
            new float[][]
            {
                new float[] { 0.300f, 0.100f, 0.277f, 0.215f, 0.212f, 0.312f, 0.115f, 0.377f, 0.000f, 0.400f, -0.115f, 0.377f, -0.212f, 0.312f, -0.277f, 0.215f, -0.300f, 0.100f, -0.277f, -0.015f, -0.212f, -0.112f, -0.115f, -0.177f, -0.000f, -0.200f, 0.115f, -0.177f, 0.212f, -0.112f, 0.277f, -0.015f, 0.300f, 0.100f },
                new float[] { 0.000f, 0.450f, 0.000f, 0.800f },
                new float[] { 0.000f, -0.250f, 0.000f, -0.550f },
                new float[] { -0.450f, 0.100f, -0.780f, 0.100f },
                new float[] { 0.450f, 0.100f, 0.780f, 0.100f },
                new float[] { -0.820f, -0.850f, 0.820f, -0.850f },
            },
            // Var (andare, scagliare)
            new float[][]
            {
                new float[] { -0.850f, 0.000f, 0.850f, 0.000f },
                new float[] { 0.400f, 0.450f, 0.850f, 0.000f, 0.400f, -0.450f },
            },
            // Ka (proteggere)
            new float[][]
            {
                new float[] { -0.709f, -0.170f, -0.647f, 0.029f, -0.535f, 0.202f, -0.382f, 0.336f, -0.198f, 0.421f, 0.000f, 0.450f, 0.198f, 0.421f, 0.382f, 0.336f, 0.535f, 0.202f, 0.647f, 0.029f, 0.709f, -0.170f },
                new float[] { 0.090f, -0.500f, 0.064f, -0.436f, 0.000f, -0.410f, -0.064f, -0.436f, -0.090f, -0.500f, -0.064f, -0.564f, -0.000f, -0.590f, 0.064f, -0.564f, 0.090f, -0.500f },
            },
            // Ren (guarire)
            new float[][]
            {
                new float[] { -0.700f, 0.600f, -0.537f, 0.125f, -0.438f, -0.072f, -0.310f, -0.223f, -0.160f, -0.318f, -0.000f, -0.350f, 0.160f, -0.318f, 0.310f, -0.223f, 0.438f, -0.072f, 0.537f, 0.125f, 0.700f, 0.600f },
                new float[] { 0.090f, 0.000f, 0.064f, 0.064f, 0.000f, 0.090f, -0.064f, 0.064f, -0.090f, 0.000f, -0.064f, -0.064f, -0.000f, -0.090f, 0.064f, -0.064f, 0.090f, -0.000f },
            },
            // Vel (vedere, guidare)
            new float[][]
            {
                new float[] { -0.736f, -0.000f, -0.632f, 0.152f, -0.500f, 0.278f, -0.346f, 0.372f, -0.177f, 0.430f, 0.000f, 0.450f, 0.177f, 0.430f, 0.346f, 0.372f, 0.500f, 0.278f, 0.632f, 0.152f, 0.736f, -0.000f },
                new float[] { -0.736f, -0.000f, -0.632f, -0.152f, -0.500f, -0.278f, -0.346f, -0.372f, -0.177f, -0.430f, -0.000f, -0.450f, 0.177f, -0.430f, 0.346f, -0.372f, 0.500f, -0.278f, 0.632f, -0.152f, 0.736f, -0.000f },
                new float[] { 0.130f, 0.000f, 0.092f, 0.092f, 0.000f, 0.130f, -0.092f, 0.092f, -0.130f, 0.000f, -0.092f, -0.092f, -0.000f, -0.130f, 0.092f, -0.092f, 0.130f, -0.000f },
            },
            // Tor (fermare)
            new float[][]
            {
                new float[] { -0.850f, 0.550f, 0.850f, 0.550f },
                new float[] { -0.450f, 0.550f, -0.450f, -0.600f },
                new float[] { 0.450f, 0.550f, 0.450f, -0.600f },
            },
            // Ur (orso, custode, la porta)
            new float[][]
            {
                new float[] { -0.800f, 0.050f, -0.761f, 0.282f, -0.647f, 0.491f, -0.470f, 0.657f, -0.247f, 0.763f, 0.000f, 0.800f, 0.247f, 0.763f, 0.470f, 0.657f, 0.647f, 0.491f, 0.761f, 0.282f, 0.800f, 0.050f },
                new float[] { -0.420f, 0.250f, -0.420f, -0.950f },
                new float[] { 0.000f, 0.320f, 0.000f, -0.950f },
                new float[] { 0.420f, 0.250f, 0.420f, -0.950f },
            },
        };

        /// <summary>Il numero della runa dal nome (maiuscole e minuscole uguali), -1 se non c'è.</summary>
        public static int Indice(string nome)
        {
            for (int i = 0; i < Nomi.Length; i++)
            {
                if (string.Equals(Nomi[i], (nome ?? "").Trim(), System.StringComparison.OrdinalIgnoreCase))
                {
                    return i;
                }
            }
            return -1;
        }
    }
}
