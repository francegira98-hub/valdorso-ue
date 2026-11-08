// Valdorso - Il launcher: le pagine che si leggono (notizie, note delle patch, roadmap, cronache). Scritto da Claude
// l'08/11/2026.
//
// Stanno in quattro file JSON sul sito, nella stessa cartella del manifesto (https://valdorso.altervista.org/launcher/):
//   notizie.json   le notizie della valle (data, titolo, testo, link, immagine, etichetta) e le voci dalla locanda
//   patch.json     le note di ogni versione (versione, nome, data, sommario, gruppi: Novità, Sistemazioni...)
//   roadmap.json   le tappe (versione, nome, stato: fatta, in_corso, prossima, futura; voci)
//   cronache.json  i racconti della valle, le parole delle rune e i volti della valle
// Ogni file ha "gioco": "Valdorso" e "aggiornato" (la data: "2026-11-08", anche con l'ora "2026-11-08 21:30").
// Una copia di tutti e quattro è dentro il launcher (Tools/Launcher/contenuti, messi nell'exe da "Compila launcher.cmd")
// e in risorse\contenuti; l'ultima scaricata resta in cache\. Si usa sempre la copia con "aggiornato" più recente:
// così il launcher è pieno anche senza rete, e il sito vince appena c'è.

using System;
using System.Collections.Generic;
using System.IO;
using System.Reflection;
using System.Text;

namespace Valdorso.Launcher
{
    public class Notizia
    {
        public string Data = "";
        public string Titolo = "";
        public string Testo = "";
        public string Link = "";
        public string Immagine = "";
        public string Etichetta = "";

        public static Notizia DaJson(Dictionary<string, object> n)
        {
            Notizia x = new Notizia();
            x.Data = Json.Testo(n, "data", "");
            x.Titolo = Json.Testo(n, "titolo", "");
            x.Testo = Json.Testo(n, "testo", "");
            x.Link = Json.Testo(n, "link", "");
            x.Immagine = Json.Testo(n, "immagine", "");
            x.Etichetta = Json.Testo(n, "etichetta", "");
            return x;
        }
    }

    public class GruppoPatch
    {
        public string Titolo = "";
        public List<string> Voci = new List<string>();
    }

    public class VersionePatch
    {
        public string Versione = "";
        public string Nome = "";
        public string Data = "";
        public string Sommario = "";
        public List<GruppoPatch> Gruppi = new List<GruppoPatch>();
    }

    public class Tappa
    {
        public string Versione = "";
        public string Nome = "";
        /// <summary>"fatta", "in_corso", "prossima" o "futura".</summary>
        public string Stato = "futura";
        public string Data = "";
        public string Sommario = "";
        public List<string> Voci = new List<string>();
    }

    public class Capitolo
    {
        public string Titolo = "";
        public string Testo = "";
    }

    public class Volto
    {
        public string Nome = "";
        public string Ruolo = "";
        public string Testo = "";
    }

    public class ParolaRuna
    {
        public string Rune = "";
        public string Nome = "";
    }

    public class Contenuti
    {
        public List<Notizia> Notizie = new List<Notizia>();
        public List<string> Voci = new List<string>();
        public List<VersionePatch> Patch = new List<VersionePatch>();
        public string RoadmapIntro = "";
        public List<Tappa> Tappe = new List<Tappa>();
        public string RoadmapDopo = "";
        public List<Capitolo> Capitoli = new List<Capitolo>();
        public string RuneIntro = "";
        public List<ParolaRuna> Parole = new List<ParolaRuna>();
        public string VoltiIntro = "";
        public List<Volto> Volti = new List<Volto>();
        /// <summary>Da dove viene ogni file ("nel launcher", "dal sito, 2026-11-08"...), per le Opzioni.</summary>
        public Dictionary<string, string> Origini = new Dictionary<string, string>();

        public void Leggi(string nome, Dictionary<string, object> d)
        {
            if (nome == "notizie.json")
            {
                foreach (Dictionary<string, object> n in Json.Elenco(d, "notizie"))
                {
                    Notizia x = Notizia.DaJson(n);
                    if (x.Titolo.Length > 0 || x.Testo.Length > 0)
                    {
                        Notizie.Add(x);
                    }
                }
                Voci = Json.Testi(d, "voci");
            }
            else if (nome == "patch.json")
            {
                foreach (Dictionary<string, object> v in Json.Elenco(d, "versioni"))
                {
                    VersionePatch x = new VersionePatch();
                    x.Versione = Json.Testo(v, "versione", "");
                    x.Nome = Json.Testo(v, "nome", "");
                    x.Data = Json.Testo(v, "data", "");
                    x.Sommario = Json.Testo(v, "sommario", "");
                    foreach (Dictionary<string, object> g in Json.Elenco(v, "gruppi"))
                    {
                        GruppoPatch y = new GruppoPatch();
                        y.Titolo = Json.Testo(g, "titolo", "");
                        y.Voci = Json.Testi(g, "voci");
                        x.Gruppi.Add(y);
                    }
                    if (x.Versione.Length > 0)
                    {
                        Patch.Add(x);
                    }
                }
            }
            else if (nome == "roadmap.json")
            {
                RoadmapIntro = Json.Testo(d, "intro", "");
                RoadmapDopo = Json.Testo(d, "dopo", "");
                foreach (Dictionary<string, object> t in Json.Elenco(d, "tappe"))
                {
                    Tappa x = new Tappa();
                    x.Versione = Json.Testo(t, "versione", "");
                    x.Nome = Json.Testo(t, "nome", "");
                    x.Stato = Json.Testo(t, "stato", "futura").Trim().ToLowerInvariant().Replace(' ', '_');
                    x.Data = Json.Testo(t, "data", "");
                    x.Sommario = Json.Testo(t, "sommario", "");
                    x.Voci = Json.Testi(t, "voci");
                    if (x.Nome.Length > 0 || x.Versione.Length > 0)
                    {
                        Tappe.Add(x);
                    }
                }
            }
            else if (nome == "cronache.json")
            {
                foreach (Dictionary<string, object> c in Json.Elenco(d, "capitoli"))
                {
                    Capitolo x = new Capitolo();
                    x.Titolo = Json.Testo(c, "titolo", "");
                    x.Testo = Json.Testo(c, "testo", "");
                    Capitoli.Add(x);
                }
                Dictionary<string, object> r = Json.Oggetto(d, "rune");
                RuneIntro = Json.Testo(r, "intro", "");
                foreach (Dictionary<string, object> p in Json.Elenco(r, "parole"))
                {
                    ParolaRuna x = new ParolaRuna();
                    x.Rune = Json.Testo(p, "rune", "");
                    x.Nome = Json.Testo(p, "nome", "");
                    if (x.Rune.Length > 0)
                    {
                        Parole.Add(x);
                    }
                }
                VoltiIntro = Json.Testo(d, "volti_intro", "");
                foreach (Dictionary<string, object> v in Json.Elenco(d, "volti"))
                {
                    Volto x = new Volto();
                    x.Nome = Json.Testo(v, "nome", "");
                    x.Ruolo = Json.Testo(v, "ruolo", "");
                    x.Testo = Json.Testo(v, "testo", "");
                    if (x.Nome.Length > 0)
                    {
                        Volti.Add(x);
                    }
                }
            }
        }
    }

    /// <summary>Dove si trovano i contenuti: dentro il launcher, in risorse\contenuti, in cache\ e sul sito.</summary>
    public static class Archivio
    {
        public static readonly string[] Nomi = { "notizie.json", "patch.json", "roadmap.json", "cronache.json" };

        static string CartellaRisorse
        {
            get { return Path.Combine(Cartelle.Risorse, "contenuti"); }
        }

        /// <summary>Legge le copie che ci sono già sul PC (senza rete): per ogni file la più recente.</summary>
        public static Contenuti CaricaLocali()
        {
            Contenuti c = new Contenuti();
            foreach (string nome in Nomi)
            {
                string origine;
                Dictionary<string, object> d = Migliore(nome, out origine);
                if (d != null)
                {
                    try
                    {
                        c.Leggi(nome, d);
                        c.Origini[nome] = origine;
                    }
                    catch (Exception e)
                    {
                        Registro.Scrivi(nome + " non si legge: " + e.Message);
                    }
                }
            }
            return c;
        }

        static Dictionary<string, object> Migliore(string nome, out string origine)
        {
            Dictionary<string, object> scelto = null;
            string data = "";
            origine = "";
            // l'ordine conta a parità di data: vince l'ultimo (la cache, cioè il sito)
            string[] testi = { DaRisorsaInterna(nome), DaFile(Path.Combine(CartellaRisorse, nome)), DaFile(Path.Combine(Cartelle.Cache, nome)) };
            string[] dove = { "nel launcher", "in risorse", "dal sito" };
            for (int i = 0; i < testi.Length; i++)
            {
                if (testi[i] == null)
                {
                    continue;
                }
                Dictionary<string, object> d = Valido(testi[i]);
                if (d == null)
                {
                    Registro.Scrivi(nome + " (" + dove[i] + ") non è valido: lo salto");
                    continue;
                }
                string questa = Json.Testo(d, "aggiornato", "");
                if (scelto == null || string.CompareOrdinal(questa, data) >= 0)
                {
                    scelto = d;
                    data = questa;
                    origine = dove[i] + (questa.Length > 0 ? ", " + questa : "");
                }
            }
            return scelto;
        }

        static Dictionary<string, object> Valido(string testo)
        {
            try
            {
                Dictionary<string, object> d = Json.Leggi(testo.TrimStart('﻿'));
                return d != null && Json.Testo(d, "gioco", "") == "Valdorso" ? d : null;
            }
            catch (Exception)
            {
                return null;
            }
        }

        static string DaFile(string percorso)
        {
            try
            {
                return File.Exists(percorso) ? File.ReadAllText(percorso, Encoding.UTF8) : null;
            }
            catch (Exception)
            {
                return null;
            }
        }

        static string DaRisorsaInterna(string nome)
        {
            try
            {
                Assembly a = Assembly.GetExecutingAssembly();
                using (Stream s = a.GetManifestResourceStream(nome))
                {
                    if (s == null)
                    {
                        return null;
                    }
                    using (StreamReader r = new StreamReader(s, Encoding.UTF8))
                    {
                        return r.ReadToEnd();
                    }
                }
            }
            catch (Exception)
            {
                return null;
            }
        }

        /// <summary>Scarica i quattro file dal sito e li tiene in cache. Restituisce quanti ne sono arrivati.</summary>
        public static int ScaricaDalSito(string cartella)
        {
            if (string.IsNullOrEmpty(cartella))
            {
                return 0;
            }
            int arrivati = 0;
            foreach (string nome in Nomi)
            {
                try
                {
                    string testo = Rete.Pagina(cartella + nome, 10000);
                    if (Valido(testo) == null)
                    {
                        Registro.Scrivi(nome + " dal sito non è valido: tengo la copia che ho");
                        continue;
                    }
                    ScriviSicuro(Path.Combine(Cartelle.Cache, nome), Encoding.UTF8.GetBytes(testo));
                    arrivati++;
                }
                catch (Exception e)
                {
                    Registro.Scrivi(nome + " non scaricato: " + e.Message);
                }
            }
            return arrivati;
        }

        /// <summary>Scrive prima accanto e poi sostituisce: un file a metà non resta mai.</summary>
        public static void ScriviSicuro(string percorso, byte[] dati)
        {
            Directory.CreateDirectory(Path.GetDirectoryName(percorso));
            string accanto = percorso + ".nuovo";
            File.WriteAllBytes(accanto, dati);
            if (File.Exists(percorso))
            {
                File.Delete(percorso);
            }
            File.Move(accanto, percorso);
        }

        /// <summary>Un nome d'immagine accettabile: solo lettere, cifre, "-", "_" e "." (niente cartelle).</summary>
        public static bool NomeBuono(string nome)
        {
            if (string.IsNullOrEmpty(nome) || nome.Length > 80 || nome.StartsWith("."))
            {
                return false;
            }
            foreach (char c in nome)
            {
                if (!(char.IsLetterOrDigit(c) || c == '-' || c == '_' || c == '.'))
                {
                    return false;
                }
            }
            string e = Path.GetExtension(nome).ToLowerInvariant();
            return e == ".jpg" || e == ".jpeg" || e == ".png";
        }

        /// <summary>Dove sta l'immagine sul PC (cache o risorse\immagini), null se non c'è.</summary>
        public static string Immagine(string nome)
        {
            if (!NomeBuono(nome))
            {
                return null;
            }
            string[] posti = { Path.Combine(Cartelle.Cache, "immagini", nome), Path.Combine(Cartelle.Risorse, "immagini", nome) };
            foreach (string p in posti)
            {
                if (File.Exists(p))
                {
                    return p;
                }
            }
            return null;
        }

        /// <summary>Scarica un'immagine dal sito (cartella immagini/) nella cache. Al massimo 3 MB.</summary>
        public static bool ScaricaImmagine(string cartella, string nome)
        {
            if (!NomeBuono(nome) || string.IsNullOrEmpty(cartella))
            {
                return false;
            }
            try
            {
                byte[] dati = Rete.Byte(cartella + "immagini/" + nome, 10000, 3 * 1024 * 1024);
                bool jpg = dati.Length > 3 && dati[0] == 0xFF && dati[1] == 0xD8;
                bool png = dati.Length > 8 && dati[0] == 0x89 && dati[1] == 0x50 && dati[2] == 0x4E && dati[3] == 0x47;
                if (!jpg && !png)
                {
                    Registro.Scrivi("immagine " + nome + ": il sito non ha dato un'immagine");
                    return false;
                }
                ScriviSicuro(Path.Combine(Cartelle.Cache, "immagini", nome), dati);
                return true;
            }
            catch (Exception e)
            {
                Registro.Scrivi("immagine " + nome + " non scaricata: " + e.Message);
                return false;
            }
        }
    }
}
