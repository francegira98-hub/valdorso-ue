// Valdorso - Il launcher: le cose di base (configurazione, registro, JSON, versioni). Scritto da Claude il 07/11/2026.
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
        public const string Versione = "1.0.0";
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

        public static Configurazione Carica()
        {
            Configurazione c = new Configurazione();
            string file = Path.Combine(Cartelle.Base, "launcher.json");
            if (!File.Exists(file))
            {
                Registro.Scrivi("launcher.json non c'è: uso i valori di serie");
                return c;
            }
            try
            {
                Dictionary<string, object> d = Json.Leggi(File.ReadAllText(file, Encoding.UTF8));
                c.Manifesto = Json.Testo(d, "manifesto", c.Manifesto);
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
                Registro.Scrivi("launcher.json non si legge (" + e.Message + "): uso i valori di serie");
            }
            return c;
        }
    }
}
