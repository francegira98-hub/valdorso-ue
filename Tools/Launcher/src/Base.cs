// Valdorso - Il launcher: le cose di base (configurazione, registro, JSON, versioni). Scritto da Claude il 07/11/2026;
// (08/11) le opzioni si salvano dalla finestra, la cartella cache, le date in italiano.
//
// Il launcher è un programma Windows a parte (non Unreal): si apre prima del gioco, mostra le notizie della valle e lo
// stato del server, scarica e installa gli aggiornamenti, controlla i file e avvia Valdorso con l'indirizzo giusto.
// È scritto in C# 5 per il .NET Framework 4 che c'è già in ogni Windows 10 e 11: si compila con
// "Compila launcher.cmd" senza installare niente (usa il compilatore csc.exe di Windows).

using System;
using System.Collections;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Text;
using System.Web.Script.Serialization;

namespace Valdorso.Launcher
{
    /// <summary>La versione del launcher (si cambia a ogni versione nuova del launcher).</summary>
    public static class Info
    {
        public const string Versione = "1.2.0";
        public const string Nome = "Valdorso";
    }

    /// <summary>Il registro del launcher: launcher.log accanto al programma (o in %LOCALAPPDATA%\Valdorso se non si può).</summary>
    public static class Registro
    {
        private static readonly object Chiave = new object();
        private static string file;

        public static string File
        {
            get
            {
                if (file == null)
                {
                    file = Path.Combine(Cartelle.Base, "launcher.log");
                    try
                    {
                        using (System.IO.File.AppendText(file)) { }
                    }
                    catch (Exception)
                    {
                        string altro = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData), "Valdorso");
                        Directory.CreateDirectory(altro);
                        file = Path.Combine(altro, "launcher.log");
                    }
                }
                return file;
            }
        }

        public static void Scrivi(string testo)
        {
            lock (Chiave)
            {
                try
                {
                    // il registro non cresce per sempre: oltre 1 MB si ricomincia
                    FileInfo info = new FileInfo(File);
                    if (info.Exists && info.Length > 1024 * 1024)
                    {
                        info.Delete();
                    }
                    System.IO.File.AppendAllText(File, DateTime.Now.ToString("yyyy-MM-dd HH:mm:ss", CultureInfo.InvariantCulture) + "  " + testo + Environment.NewLine, Encoding.UTF8);
                }
                catch (Exception)
                {
                    // senza registro si va avanti lo stesso
                }
            }
        }
    }

    /// <summary>Le cartelle: tutto sta accanto al launcher (così si può tenere dove si vuole, anche su una chiavetta).</summary>
    public static class Cartelle
    {
        public static string Base
        {
            get { return AppDomain.CurrentDomain.BaseDirectory; }
        }

        public static string Risorse
        {
            get { return Path.Combine(Base, "risorse"); }
        }

        public static string Scaricati
        {
            get { return Path.Combine(Base, "Scaricati"); }
        }

        private static string cache;

        /// <summary>Le ultime notizie, note e immagini scaricate dal sito (per quando manca la rete).</summary>
        public static string Cache
        {
            get
            {
                if (cache == null)
                {
                    cache = Path.Combine(Base, "cache");
                    try
                    {
                        Directory.CreateDirectory(cache);
                        string prova = Path.Combine(cache, ".prova");
                        System.IO.File.WriteAllText(prova, "");
                        System.IO.File.Delete(prova);
                    }
                    catch (Exception)
                    {
                        // la cartella del launcher non si può scrivere (per esempio dentro Programmi)
                        cache = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData), "Valdorso", "cache");
                        try
                        {
                            Directory.CreateDirectory(cache);
                        }
                        catch (Exception)
                        {
                        }
                    }
                }
                return cache;
            }
        }

        /// <summary>Le opzioni salvate qui quando launcher.json accanto al programma non si può scrivere.</summary>
        public static string OpzioniDiRiserva
        {
            get { return Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData), "Valdorso", "launcher_opzioni.json"); }
        }
    }

    /// <summary>Le misure in italiano: "3,4 GB", "12 min".</summary>
    public static class Misure
    {
        public static string Byte(long n)
        {
            CultureInfo it = CultureInfo.GetCultureInfo("it-IT");
            if (n >= 1L << 30)
            {
                return (n / (double)(1L << 30)).ToString("0.0", it) + " GB";
            }
            if (n >= 1L << 20)
            {
                return (n / (double)(1L << 20)).ToString("0.0", it) + " MB";
            }
            return Math.Max(1, n / 1024).ToString(CultureInfo.InvariantCulture) + " KB";
        }

        public static string Durata(double secondi)
        {
            if (secondi < 90)
            {
                return Math.Max(1, (int)secondi).ToString(CultureInfo.InvariantCulture) + " s";
            }
            if (secondi < 5400)
            {
                return ((int)Math.Round(secondi / 60)).ToString(CultureInfo.InvariantCulture) + " min";
            }
            int ore = (int)(secondi / 3600);
            int minuti = (int)((secondi - ore * 3600) / 60);
            return ore.ToString(CultureInfo.InvariantCulture) + " h " + minuti.ToString("00", CultureInfo.InvariantCulture) + " min";
        }
    }

    /// <summary>Le date: nei file del sito sono "2026-11-08"; nella finestra "8 novembre 2026".</summary>
    public static class Date
    {
        static readonly string[] Mesi = { "gennaio", "febbraio", "marzo", "aprile", "maggio", "giugno", "luglio", "agosto", "settembre", "ottobre", "novembre", "dicembre" };

        public static string Leggibile(string data)
        {
            DateTime d;
            if (Leggi(data, out d))
            {
                return d.Day.ToString(CultureInfo.InvariantCulture) + " " + Mesi[d.Month - 1] + " " + d.Year.ToString(CultureInfo.InvariantCulture);
            }
            return data ?? "";
        }

        /// <summary>Corta: "8 nov".</summary>
        public static string Corta(string data)
        {
            DateTime d;
            if (Leggi(data, out d))
            {
                return d.Day.ToString(CultureInfo.InvariantCulture) + " " + Mesi[d.Month - 1].Substring(0, 3);
            }
            return data ?? "";
        }

        public static bool Leggi(string data, out DateTime d)
        {
            return DateTime.TryParseExact((data ?? "").Trim(), "yyyy-MM-dd", CultureInfo.InvariantCulture, DateTimeStyles.None, out d);
        }
    }

    /// <summary>Un po' di JSON con il lettore del .NET Framework (JavaScriptSerializer): dizionari, elenchi, numeri.</summary>
    public static class Json
    {
        public static Dictionary<string, object> Leggi(string testo)
        {
            JavaScriptSerializer lettore = new JavaScriptSerializer();
            lettore.MaxJsonLength = 64 * 1024 * 1024;
            object valore = lettore.DeserializeObject(testo);
            return valore as Dictionary<string, object>;
        }

        public static string Scrivi(object valore)
        {
            JavaScriptSerializer scrittore = new JavaScriptSerializer();
            scrittore.MaxJsonLength = 64 * 1024 * 1024;
            return scrittore.Serialize(valore);
        }

        public static string Testo(Dictionary<string, object> d, string chiave, string seManca)
        {
            object v;
            if (d != null && d.TryGetValue(chiave, out v) && v != null)
            {
                return Convert.ToString(v, CultureInfo.InvariantCulture);
            }
            return seManca;
        }

        public static double Numero(Dictionary<string, object> d, string chiave, double seManca)
        {
            object v;
            if (d != null && d.TryGetValue(chiave, out v) && v != null)
            {
                try
                {
                    return Convert.ToDouble(v, CultureInfo.InvariantCulture);
                }
                catch (Exception)
                {
                    return seManca;
                }
            }
            return seManca;
        }

        public static bool SiNo(Dictionary<string, object> d, string chiave, bool seManca)
        {
            object v;
            if (d != null && d.TryGetValue(chiave, out v) && v is bool)
            {
                return (bool)v;
            }
            return seManca;
        }

        public static Dictionary<string, object> Oggetto(Dictionary<string, object> d, string chiave)
        {
            object v;
            if (d != null && d.TryGetValue(chiave, out v))
            {
                return v as Dictionary<string, object>;
            }
            return null;
        }

        public static List<Dictionary<string, object>> Elenco(Dictionary<string, object> d, string chiave)
        {
            List<Dictionary<string, object>> fuori = new List<Dictionary<string, object>>();
            object v;
            if (d != null && d.TryGetValue(chiave, out v) && v is IEnumerable && !(v is string))
            {
                foreach (object elemento in (IEnumerable)v)
                {
                    Dictionary<string, object> o = elemento as Dictionary<string, object>;
                    if (o != null)
                    {
                        fuori.Add(o);
                    }
                }
            }
            return fuori;
        }

        /// <summary>Un elenco di scritte (["a", "b"]); le voci che non sono scritte si saltano.</summary>
        public static List<string> Testi(Dictionary<string, object> d, string chiave)
        {
            List<string> fuori = new List<string>();
            object v;
            if (d != null && d.TryGetValue(chiave, out v) && v is IEnumerable && !(v is string))
            {
                foreach (object elemento in (IEnumerable)v)
                {
                    string t = elemento as string;
                    if (!string.IsNullOrEmpty(t))
                    {
                        fuori.Add(t);
                    }
                }
            }
            return fuori;
        }
    }

    /// <summary>Le versioni "0.1.3": si confrontano numero per numero (0.1.10 viene dopo 0.1.9).</summary>
    public static class Versioni
    {
        public static int Confronta(string a, string b)
        {
            string[] pa = (a ?? "").Trim().TrimStart('v', 'V').Split('.');
            string[] pb = (b ?? "").Trim().TrimStart('v', 'V').Split('.');
            int n = Math.Max(pa.Length, pb.Length);
            for (int i = 0; i < n; i++)
            {
                int x = 0;
                int y = 0;
                if (i < pa.Length)
                {
                    int.TryParse(pa[i], NumberStyles.Integer, CultureInfo.InvariantCulture, out x);
                }
                if (i < pb.Length)
                {
                    int.TryParse(pb[i], NumberStyles.Integer, CultureInfo.InvariantCulture, out y);
                }
                if (x != y)
                {
                    return x < y ? -1 : 1;
                }
            }
            return 0;
        }
    }

    /// <summary>La configurazione: launcher.json accanto al programma. Se manca, si usano i valori di serie.</summary>
    public class Configurazione
    {
        public string Manifesto = "https://valdorso.altervista.org/launcher/manifesto.json";
        /// <summary>La cartella del sito con notizie.json, patch.json, roadmap.json, cronache.json e immagini/ (vuoto = quella del manifesto).</summary>
        public string Contenuti = "";
        public string CartellaGioco = "Gioco";
        public string Server = "";
        public string Stato = "";
        public string Sito = "https://valdorso.altervista.org";
        public string Discord = "";
        public bool ChiudiDopoAvvio = true;
        public string ArgomentiInPiu = "";
        /// <summary>Falso: la finestra usa i caratteri di Windows (Georgia) invece di Cinzel ed EB Garamond della cartella risorse.</summary>
        public bool CaratteriPropri = true;

        public string PercorsoGioco
        {
            get
            {
                return Path.IsPathRooted(CartellaGioco) ? CartellaGioco : Path.GetFullPath(Path.Combine(Cartelle.Base, CartellaGioco));
            }
        }

        /// <summary>L'indirizzo della cartella dei contenuti, sempre con la "/" in fondo.</summary>
        public string CartellaContenuti
        {
            get
            {
                string base_ = Contenuti.Trim();
                if (base_.Length == 0)
                {
                    int barra = Manifesto.LastIndexOf('/');
                    base_ = barra > 8 ? Manifesto.Substring(0, barra + 1) : "";
                }
                if (base_.Length > 0 && !base_.EndsWith("/"))
                {
                    base_ += "/";
                }
                return base_;
            }
        }

        static string FileOpzioni
        {
            get { return Path.Combine(Cartelle.Base, "launcher.json"); }
        }

        public static Configurazione Carica()
        {
            Configurazione c = new Configurazione();
            if (!File.Exists(FileOpzioni))
            {
                Registro.Scrivi("launcher.json non c'è: uso i valori di serie");
            }
            else
            {
                c.Leggi(FileOpzioni);
            }
            // se launcher.json non si poteva scrivere, le opzioni scelte nella finestra stanno qui
            if (File.Exists(Cartelle.OpzioniDiRiserva))
            {
                c.Leggi(Cartelle.OpzioniDiRiserva);
            }
            return c;
        }

        void Leggi(string file)
        {
            Configurazione c = this;
            try
            {
                Dictionary<string, object> d = Json.Leggi(File.ReadAllText(file, Encoding.UTF8));
                c.Manifesto = Json.Testo(d, "manifesto", c.Manifesto);
                c.Contenuti = Json.Testo(d, "contenuti", c.Contenuti);
                c.CartellaGioco = Json.Testo(d, "cartella_gioco", c.CartellaGioco);
                c.Server = Json.Testo(d, "server", c.Server);
                c.Stato = Json.Testo(d, "stato", c.Stato);
                c.Sito = Json.Testo(d, "sito", c.Sito);
                c.Discord = Json.Testo(d, "discord", c.Discord);
                c.ChiudiDopoAvvio = Json.SiNo(d, "chiudi_dopo_avvio", c.ChiudiDopoAvvio);
                c.ArgomentiInPiu = Json.Testo(d, "argomenti", c.ArgomentiInPiu);
                c.CaratteriPropri = Json.SiNo(d, "caratteri_propri", c.CaratteriPropri);
            }
            catch (Exception e)
            {
                Registro.Scrivi(Path.GetFileName(file) + " non si legge (" + e.Message + "): uso i valori di serie");
            }
        }

        /// <summary>Salva le opzioni scelte nella finestra in launcher.json (le altre voci del file restano com'erano).
        /// Restituisce falso se non si è potuto scrivere da nessuna parte.</summary>
        public bool Salva()
        {
            Dictionary<string, object> d = null;
            try
            {
                if (File.Exists(FileOpzioni))
                {
                    d = Json.Leggi(File.ReadAllText(FileOpzioni, Encoding.UTF8));
                }
            }
            catch (Exception)
            {
            }
            if (d == null)
            {
                d = new Dictionary<string, object>();
            }
            d["manifesto"] = Manifesto;
            d["contenuti"] = Contenuti;
            d["cartella_gioco"] = CartellaGioco;
            d["server"] = Server;
            d["stato"] = Stato;
            d["sito"] = Sito;
            d["discord"] = Discord;
            d["chiudi_dopo_avvio"] = ChiudiDopoAvvio;
            d["argomenti"] = ArgomentiInPiu;
            d["caratteri_propri"] = CaratteriPropri;
            string testo = InColonna(d);
            try
            {
                File.WriteAllText(FileOpzioni, testo, new UTF8Encoding(false));
                if (File.Exists(Cartelle.OpzioniDiRiserva))
                {
                    File.Delete(Cartelle.OpzioniDiRiserva);
                }
                return true;
            }
            catch (Exception e)
            {
                Registro.Scrivi("launcher.json non si scrive (" + e.Message + "): salvo in " + Cartelle.OpzioniDiRiserva);
            }
            try
            {
                Directory.CreateDirectory(Path.GetDirectoryName(Cartelle.OpzioniDiRiserva));
                File.WriteAllText(Cartelle.OpzioniDiRiserva, testo, new UTF8Encoding(false));
                return true;
            }
            catch (Exception e)
            {
                Registro.Scrivi("opzioni non salvate: " + e.Message);
                return false;
            }
        }

        /// <summary>Un dizionario semplice in JSON, una voce per riga (così launcher.json resta leggibile a mano).</summary>
        static string InColonna(Dictionary<string, object> d)
        {
            StringBuilder s = new StringBuilder();
            s.Append("{\r\n");
            int i = 0;
            foreach (KeyValuePair<string, object> voce in d)
            {
                s.Append("  ").Append(Json.Scrivi(voce.Key)).Append(": ").Append(Json.Scrivi(voce.Value));
                s.Append(++i < d.Count ? ",\r\n" : "\r\n");
            }
            s.Append("}\r\n");
            return s.ToString();
        }
    }
}
