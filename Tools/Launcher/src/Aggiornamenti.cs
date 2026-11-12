// Valdorso - Il launcher: il manifesto, lo scaricamento con ripresa, il controllo SHA-256, l'installazione e lo stato
// del mondo. Scritto da Claude il 07/11/2026; (08/11) il gioco in più parti (le Release di GitHub tengono al massimo
// 2 GB per file), i rinvii seguiti a mano tenendo la ripresa, il controllo dello spazio sul disco, la pausa.
//
// Il manifesto (manifesto.json, sul sito) lo prepara Tools/Launcher/prepara_aggiornamento.py dal gioco pacchettizzato:
//   versione, nome, data, pacchetto (parti: url, sha256, byte; byte = tutto; installato = byte da scompattato),
//   eseguibile, argomenti, server (indirizzo, stato), launcher_versione, launcher_url, notizie, file (p, s, b).
// Va bene anche il manifesto di prima, con lo zip intero: pacchetto (url, sha256, byte) diventa una parte sola.
// Le parti sono pezzi uguali dello stesso zip (Valdorso_0.1.3.zip.001, .002...): non si riuniscono su disco (sarebbe
// il doppio dello spazio), si leggono una dopo l'altra come un file solo (FlussoUnito).
// (12/11) Le PATCH: se il manifesto ha una "patch" che parte dalla versione installata ("da"), il launcher scarica
// solo quella (i file cambiati, in parti come il pacchetto) e la mette sopra il gioco: i file nuovi al posto dei
// vecchi (che restano da parte finché non e' finito tutto), quelli da togliere ("togli") tolti. Se qualcosa va storto,
// rimette tutto com'era e scarica il gioco intero. I launcher di prima ignorano la patch e scaricano tutto.
// L'installazione non lascia mai il gioco a metà: si scompatta in "Gioco_nuovo", poi si scambiano le cartelle; le
// impostazioni del giocatore (la cartella Saved del gioco) passano nella versione nuova.

using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.IO.Compression;
using System.Net;
using System.Security.Cryptography;
using System.Text;
using System.Threading;

namespace Valdorso.Launcher
{
    public class FileDelGioco
    {
        public string Percorso = "";
        public string Sha256 = "";
        public long Byte;
    }

    /// <summary>Un pezzo dello zip del gioco: dove sta, la sua impronta, quanto è grande.</summary>
    public class ParteDelPacchetto
    {
        public string Url = "";
        public string Sha256 = "";
        public long Byte;
    }

    /// <summary>(12/11) Una patch: dalla versione "Da" a quella del manifesto, solo i file cambiati.</summary>
    public class PatchDelGioco
    {
        public string Da = "";
        public List<ParteDelPacchetto> Parti = new List<ParteDelPacchetto>();
        public long Byte;
        /// <summary>Quanto occupano i file della patch scompattati.</summary>
        public long Installato;
        public List<string> Togli = new List<string>();
    }

    public class Manifesto
    {
        public string Versione = "";
        public string NomeVersione = "";
        public string Data = "";
        public List<ParteDelPacchetto> Parti = new List<ParteDelPacchetto>();
        /// <summary>Quanto pesa tutto lo zip (la somma delle parti).</summary>
        public long BytePacchetto;
        /// <summary>Quanto pesa il gioco scompattato (0 se il manifesto non lo dice).</summary>
        public long ByteInstallato;
        public string Eseguibile = "Valdorso.exe";
        public string Argomenti = "";
        public string Server = "";
        public string Stato = "";
        /// <summary>(12/11, 1.3.1) La chiave PUBBLICA del server (PEM): il launcher la mette nel gioco prima di avviarlo.</summary>
        public string ChiaveServer = "";
        public string VersioneLauncher = "";
        public string UrlLauncher = "";
        public bool Aperto = true;
        public List<Notizia> Notizie = new List<Notizia>();
        public List<FileDelGioco> File = new List<FileDelGioco>();
        public List<PatchDelGioco> Patch = new List<PatchDelGioco>();

        public static Manifesto DaJson(string testo)
        {
            Dictionary<string, object> d = Json.Leggi(testo);
            if (d == null || Json.Testo(d, "gioco", "") != "Valdorso")
            {
                throw new InvalidDataException("non è un manifesto di Valdorso");
            }
            Manifesto m = new Manifesto();
            m.Versione = Json.Testo(d, "versione", "");
            m.NomeVersione = Json.Testo(d, "nome", "");
            m.Data = Json.Testo(d, "data", "");
            m.Eseguibile = Json.Testo(d, "eseguibile", m.Eseguibile);
            m.Argomenti = Json.Testo(d, "argomenti", "");
            m.Aperto = Json.SiNo(d, "aperto", true);

            Dictionary<string, object> p = Json.Oggetto(d, "pacchetto");
            foreach (Dictionary<string, object> x in Json.Elenco(p, "parti"))
            {
                ParteDelPacchetto parte = new ParteDelPacchetto();
                parte.Url = Json.Testo(x, "url", "").Trim();
                parte.Sha256 = Json.Testo(x, "sha256", "").Trim().ToLowerInvariant();
                parte.Byte = (long)Json.Numero(x, "byte", 0);
                if (parte.Url.Length > 0)
                {
                    m.Parti.Add(parte);
                }
            }
            if (m.Parti.Count == 0 && Json.Testo(p, "url", "").Trim().Length > 0)
            {
                // il manifesto di prima: lo zip intero è una parte sola
                ParteDelPacchetto parte = new ParteDelPacchetto();
                parte.Url = Json.Testo(p, "url", "").Trim();
                parte.Sha256 = Json.Testo(p, "sha256", "").Trim().ToLowerInvariant();
                parte.Byte = (long)Json.Numero(p, "byte", 0);
                m.Parti.Add(parte);
            }
            foreach (ParteDelPacchetto parte in m.Parti)
            {
                m.BytePacchetto += parte.Byte;
            }
            if (m.BytePacchetto == 0)
            {
                m.BytePacchetto = (long)Json.Numero(p, "byte", 0);
            }
            m.ByteInstallato = (long)Json.Numero(p, "installato", 0);

            Dictionary<string, object> s = Json.Oggetto(d, "server");
            m.Server = Json.Testo(s, "indirizzo", "").Trim();
            m.Stato = Json.Testo(s, "stato", "").Trim();
            m.ChiaveServer = Json.Testo(s, "chiave", "").Trim();
            Dictionary<string, object> l = Json.Oggetto(d, "launcher");
            m.VersioneLauncher = Json.Testo(d, "launcher_versione", Json.Testo(l, "versione", ""));
            m.UrlLauncher = Json.Testo(d, "launcher_url", Json.Testo(l, "url", ""));
            foreach (Dictionary<string, object> n in Json.Elenco(d, "notizie"))
            {
                Notizia x = Notizia.DaJson(n);
                if (x.Titolo.Length > 0 || x.Testo.Length > 0)
                {
                    m.Notizie.Add(x);
                }
            }
            long somma = 0;
            foreach (Dictionary<string, object> f in Json.Elenco(d, "file"))
            {
                FileDelGioco x = new FileDelGioco();
                x.Percorso = Json.Testo(f, "p", "");
                x.Sha256 = Json.Testo(f, "s", "").ToLowerInvariant();
                x.Byte = (long)Json.Numero(f, "b", 0);
                if (x.Percorso.Length > 0)
                {
                    m.File.Add(x);
                    somma += x.Byte;
                }
            }
            if (m.ByteInstallato == 0)
            {
                m.ByteInstallato = somma;
            }
            // (12/11) le patch dalle versioni di prima
            foreach (Dictionary<string, object> x in Json.Elenco(d, "patch"))
            {
                PatchDelGioco pa = new PatchDelGioco();
                pa.Da = Json.Testo(x, "da", "").Trim();
                foreach (Dictionary<string, object> y in Json.Elenco(x, "parti"))
                {
                    ParteDelPacchetto parte = new ParteDelPacchetto();
                    parte.Url = Json.Testo(y, "url", "").Trim();
                    parte.Sha256 = Json.Testo(y, "sha256", "").Trim().ToLowerInvariant();
                    parte.Byte = (long)Json.Numero(y, "byte", 0);
                    if (parte.Url.Length > 0)
                    {
                        pa.Parti.Add(parte);
                        pa.Byte += parte.Byte;
                    }
                }
                pa.Togli.AddRange(Json.Testi(x, "togli"));
                pa.Installato = (long)Json.Numero(x, "installato", 0);
                if (pa.Da.Length > 0 && pa.Parti.Count > 0)
                {
                    m.Patch.Add(pa);
                }
            }
            return m;
        }
    }

    /// <summary>Lo stato del mondo che il server dà su http://server:7780/stato (ValdorsoStatoMondo.cpp, lo stesso del
    /// menu del gioco): gioco, versione, nome_versione, ora, tempo, stagione, coloni, posti, notizia, aggiornato, aperto.</summary>
    public class StatoMondo
    {
        public string Versione = "";
        public string NomeVersione = "";
        public double Ora = -1;
        public string Tempo = "";
        public string Stagione = "";
        public int Coloni;
        public int Posti;
        public string Notizia = "";
        public string Aggiornato = "";
        public bool Aperto = true;

        public static StatoMondo DaJson(string testo)
        {
            Dictionary<string, object> d = Json.Leggi(testo);
            if (d == null || Json.Testo(d, "gioco", "") != "Valdorso")
            {
                return null;
            }
            StatoMondo s = new StatoMondo();
            s.Versione = Json.Testo(d, "versione", "");
            s.NomeVersione = Json.Testo(d, "nome_versione", "");
            s.Ora = Json.Numero(d, "ora", -1);
            s.Tempo = Json.Testo(d, "tempo", "");
            s.Stagione = Json.Testo(d, "stagione", "");
            s.Coloni = Math.Max(0, (int)Json.Numero(d, "coloni", 0));
            s.Posti = Math.Max(0, (int)Json.Numero(d, "posti", 0));
            s.Notizia = Json.Testo(d, "notizia", "");
            if (s.Notizia.Length > 200)
            {
                s.Notizia = s.Notizia.Substring(0, 200);
            }
            s.Aggiornato = Json.Testo(d, "aggiornato", "");
            s.Aperto = Json.SiNo(d, "aperto", true);
            return s;
        }

        /// <summary>"18:40" come nel gioco.</summary>
        public static string OraLeggibile(double ora)
        {
            if (ora < 0)
            {
                return "";
            }
            int minuti = (int)Math.Floor((ora % 24.0) * 60.0 + 0.001);
            minuti = Math.Max(0, Math.Min(24 * 60 - 1, minuti));
            return string.Format(CultureInfo.InvariantCulture, "{0:00}:{1:00}", minuti / 60, minuti % 60);
        }

        /// <summary>Il momento del giorno, come lo direbbe un contadino della valle.</summary>
        public static string Momento(double ora)
        {
            if (ora < 0)
            {
                return "";
            }
            double h = ora % 24.0;
            if (h < 5) return "notte fonda";
            if (h < 7) return "l'alba";
            if (h < 11) return "il mattino";
            if (h < 14) return "mezzogiorno";
            if (h < 17.5) return "il pomeriggio";
            if (h < 20) return "il tramonto";
            if (h < 22) return "la sera";
            return "la notte";
        }

        /// <summary>La riga come nel menu del gioco: "Nella valle sono le 18:40 · sereno · autunno · 3 coloni".</summary>
        public string Riga()
        {
            if (!Aperto)
            {
                return "La valle è chiusa per lavori: lo staff la riapre presto.";
            }
            List<string> pezzi = new List<string>();
            string ora = OraLeggibile(Ora);
            if (ora.Length > 0)
            {
                pezzi.Add("Nella valle sono le " + ora);
            }
            if (Tempo.Length > 0)
            {
                pezzi.Add(Tempo.ToLowerInvariant());
            }
            if (Stagione.Length > 0)
            {
                pezzi.Add(Stagione);
            }
            pezzi.Add(Coloni == 1 ? "un colono" : (Coloni > 1 ? Coloni + " coloni" : "nessun colono in giro"));
            return string.Join(" · ", pezzi.ToArray());
        }
    }

    /// <summary>Le richieste in rete (HTTPS con TLS 1.2, tempi brevi per le pagine piccole).</summary>
    public static class Rete
    {
        static Rete()
        {
            try
            {
                ServicePointManager.SecurityProtocol |= (SecurityProtocolType)3072; // TLS 1.2
            }
            catch (Exception)
            {
            }
            try
            {
                ServicePointManager.DefaultConnectionLimit = Math.Max(ServicePointManager.DefaultConnectionLimit, 8);
            }
            catch (Exception)
            {
            }
        }

        public static string Agente
        {
            get { return "ValdorsoLauncher/" + Info.Versione; }
        }

        public static string Pagina(string url, int millisecondi)
        {
            return Encoding.UTF8.GetString(Byte(url, millisecondi, 4 * 1024 * 1024));
        }

        /// <summary>Un file piccolo (pagine, immagini): al massimo "limite" byte.</summary>
        public static byte[] Byte(string url, int millisecondi, int limite)
        {
            HttpWebRequest r = (HttpWebRequest)WebRequest.Create(url);
            r.Timeout = millisecondi;
            r.ReadWriteTimeout = millisecondi;
            r.UserAgent = Agente;
            r.Headers[HttpRequestHeader.CacheControl] = "no-cache";
            r.AutomaticDecompression = DecompressionMethods.GZip | DecompressionMethods.Deflate;
            using (HttpWebResponse risposta = (HttpWebResponse)r.GetResponse())
            using (Stream dentro = risposta.GetResponseStream())
            using (MemoryStream fuori = new MemoryStream())
            {
                byte[] pezzo = new byte[1 << 16];
                int letti;
                while ((letti = dentro.Read(pezzo, 0, pezzo.Length)) > 0)
                {
                    fuori.Write(pezzo, 0, letti);
                    if (fuori.Length > limite)
                    {
                        throw new InvalidDataException("il file è troppo grande: " + url);
                    }
                }
                byte[] tutto = fuori.ToArray();
                // via il BOM, se c'è (JavaScriptSerializer non lo vuole)
                if (tutto.Length >= 3 && tutto[0] == 0xEF && tutto[1] == 0xBB && tutto[2] == 0xBF)
                {
                    byte[] senza = new byte[tutto.Length - 3];
                    Array.Copy(tutto, 3, senza, 0, senza.Length);
                    return senza;
                }
                return tutto;
            }
        }
    }

    public enum Fase
    {
        Scarico,
        Controllo,
        Installo,
        Verifica
    }

    public delegate void Avanzamento(Fase fase, string testo, long fatti, long totale, double bytePerSecondo);

    /// <summary>Quando sul disco non c'è posto per l'aggiornamento.</summary>
    public class SpazioInsufficiente : Exception
    {
        public SpazioInsufficiente(string messaggio) : base(messaggio)
        {
        }
    }

    /// <summary>Scarica, controlla e installa. Tutto qui gira fuori dalla finestra (in un altro thread).</summary>
    public class Aggiornatore
    {
        private readonly Configurazione config;
        public volatile bool Annulla;
        private volatile HttpWebRequest richiesta;
        private double velocita;
        /// <summary>Vero mentre si scambiano le cartelle (un attimo): il launcher non si chiude a metà.</summary>
        public volatile bool Scambio;

        public Aggiornatore(Configurazione config)
        {
            this.config = config;
        }

        /// <summary>Ferma il lavoro in corso (subito: chiude anche lo scaricamento a metà; il pezzo scaricato resta).</summary>
        public void Ferma()
        {
            Annulla = true;
            HttpWebRequest r = richiesta;
            if (r != null)
            {
                try
                {
                    r.Abort();
                }
                catch (Exception)
                {
                }
            }
        }

        public string FileVersione
        {
            get { return Path.Combine(config.PercorsoGioco, "valdorso_versione.txt"); }
        }

        /// <summary>Se l'ultima installazione si è fermata a metà dello scambio (il PC spento in quel secondo), rimette
        /// a posto la versione vecchia.</summary>
        public void Ripristina()
        {
            try
            {
                string gioco = config.PercorsoGioco.TrimEnd('\\', '/');
                string vecchio = gioco + "_vecchio";
                if (!Directory.Exists(gioco) && Directory.Exists(vecchio))
                {
                    Directory.Move(vecchio, gioco);
                    Registro.Scrivi("rimessa a posto la versione di prima (lo scambio si era fermato a metà)");
                }
            }
            catch (Exception e)
            {
                Registro.Scrivi("ripristino non riuscito: " + e.Message);
            }
        }

        /// <summary>La cartella scelta dal giocatore, resa sicura: se non è vuota e non è già una cartella di Valdorso,
        /// si usa una cartella "Valdorso" dentro (un aggiornamento sostituisce tutta la cartella del gioco!).</summary>
        public static string CartellaSicura(string scelta)
        {
            try
            {
                string piena = Path.GetFullPath(scelta);
                if (!Directory.Exists(piena) || File.Exists(Path.Combine(piena, "valdorso_versione.txt")) || Directory.GetFileSystemEntries(piena).Length == 0)
                {
                    return piena;
                }
                return Path.Combine(piena, "Valdorso");
            }
            catch (Exception)
            {
                return Path.Combine(scelta, "Valdorso");
            }
        }

        /// <summary>Vero se la cartella del gioco non c'è, è vuota o è già di Valdorso: solo così si può sostituire.</summary>
        public bool CartellaDelGiocoBuona()
        {
            string gioco = config.PercorsoGioco;
            return !Directory.Exists(gioco) || File.Exists(FileVersione) || Directory.GetFileSystemEntries(gioco).Length == 0;
        }

        /// <summary>La versione installata ("" se il gioco non c'è).</summary>
        public string VersioneInstallata()
        {
            try
            {
                if (File.Exists(FileVersione))
                {
                    return File.ReadAllText(FileVersione).Trim();
                }
            }
            catch (Exception)
            {
            }
            return "";
        }

        public string Eseguibile(Manifesto m)
        {
            string nome = m != null && m.Eseguibile.Length > 0 ? m.Eseguibile : "Valdorso.exe";
            return Path.Combine(config.PercorsoGioco, nome.Replace('/', Path.DirectorySeparatorChar));
        }

        string NomeParte(Manifesto m, int indice)
        {
            return Path.Combine(Cartelle.Scaricati, "Valdorso_" + Pulito(m.Versione) + ".zip." + (indice + 1).ToString("000", CultureInfo.InvariantCulture));
        }

        /// <summary>(12/11) Il nome di una parte della patch (comincia come quelle del pacchetto: PulisciScaricati la tiene).</summary>
        string NomeParchePatch(Manifesto m, PatchDelGioco pa, int indice)
        {
            return Path.Combine(Cartelle.Scaricati, "Valdorso_" + Pulito(m.Versione) + ".zip.patch_" + Pulito(pa.Da) + "." +
                (indice + 1).ToString("000", CultureInfo.InvariantCulture));
        }

        /// <summary>(12/11) La patch che va bene per il gioco installato (null se non c'e': si scarica tutto).</summary>
        public PatchDelGioco PatchPer(Manifesto m)
        {
            string installata = VersioneInstallata();
            if (m == null || installata.Length == 0 || installata == m.Versione || !Directory.Exists(config.PercorsoGioco)
                || !File.Exists(Eseguibile(m)))
            {
                return null;
            }
            foreach (PatchDelGioco pa in m.Patch)
            {
                if (pa.Da == installata)
                {
                    return pa;
                }
            }
            return null;
        }

        static bool ParteFinita(ParteDelPacchetto p, string file)
        {
            try
            {
                string ok = file + ".ok";
                return File.Exists(file) && File.Exists(ok) && (p.Byte <= 0 || new FileInfo(file).Length == p.Byte)
                    && File.ReadAllText(ok).Trim() == (p.Sha256.Length > 0 ? p.Sha256 : "senza impronta");
            }
            catch (Exception)
            {
                return false;
            }
        }

        /// <summary>Quanti byte mancano da scaricare (le parti già finite non contano, quelle a metà solo per quel che manca).</summary>
        public long DaScaricare(Manifesto m)
        {
            long fuori = 0;
            PatchDelGioco pa = PatchPer(m);
            List<ParteDelPacchetto> parti = pa != null ? pa.Parti : m.Parti;
            for (int i = 0; i < parti.Count; i++)
            {
                ParteDelPacchetto p = parti[i];
                string file = pa != null ? NomeParchePatch(m, pa, i) : NomeParte(m, i);
                if (ParteFinita(p, file))
                {
                    continue;
                }
                long gia = 0;
                try
                {
                    string scarico = file + ".scarico";
                    if (File.Exists(scarico))
                    {
                        gia = new FileInfo(scarico).Length;
                    }
                }
                catch (Exception)
                {
                }
                fuori += Math.Max(0, p.Byte - gia);
            }
            return fuori;
        }

        /// <summary>Controlla che sui dischi ci sia posto. "" se va bene, altrimenti cosa manca, da dire al giocatore.</summary>
        public string ControllaSpazio(Manifesto m)
        {
            const long margine = 300L * 1024 * 1024;
            long scarico = DaScaricare(m);
            PatchDelGioco pa = PatchPer(m);
            long installo = pa != null ? (pa.Installato > 0 ? pa.Installato + pa.Byte : pa.Byte * 3) : (m.ByteInstallato > 0 ? m.ByteInstallato : (long)(m.BytePacchetto * 1.2));
            string discoScarico = Disco(Cartelle.Scaricati);
            string discoGioco = Disco(config.PercorsoGioco);
            Dictionary<string, long> servono = new Dictionary<string, long>(StringComparer.OrdinalIgnoreCase);
            if (discoScarico.Length > 0)
            {
                servono[discoScarico] = scarico + margine;
            }
            if (discoGioco.Length > 0)
            {
                long gia;
                servono.TryGetValue(discoGioco, out gia);
                servono[discoGioco] = gia + installo + (gia > 0 ? 0 : margine);
            }
            foreach (KeyValuePair<string, long> voce in servono)
            {
                try
                {
                    DriveInfo disco = new DriveInfo(voce.Key);
                    long liberi = disco.AvailableFreeSpace;
                    if (liberi < voce.Value)
                    {
                        return "Sul disco " + voce.Key.TrimEnd('\\', '/') + " non c'è abbastanza posto: servono " + Misure.Byte(voce.Value) +
                               ", ce ne sono " + Misure.Byte(liberi) + ". Libera " + Misure.Byte(voce.Value - liberi) + " o scegli un'altra cartella nelle Opzioni.";
                    }
                }
                catch (Exception e)
                {
                    // dischi di rete e simili: non si sa, si prova lo stesso
                    Registro.Scrivi("spazio sul disco " + voce.Key + " non controllato: " + e.Message);
                }
            }
            return "";
        }

        static string Disco(string cartella)
        {
            try
            {
                string radice = Path.GetPathRoot(Path.GetFullPath(cartella));
                return radice != null && !radice.StartsWith("\\\\") ? radice : "";
            }
            catch (Exception)
            {
                return "";
            }
        }

        /// <summary>Scarica le parti (riprende da dove si era fermato), le controlla e installa il gioco.</summary>
        public void Installa(Manifesto m, Avanzamento avanza)
        {
            // (12/11) prima la patch, se c'e' quella giusta; se non riesce, il gioco intero
            PatchDelGioco pa = PatchPer(m);
            if (pa != null)
            {
                try
                {
                    Directory.CreateDirectory(Cartelle.Scaricati);
                    PulisciScaricati(m);
                    string spazioPatch = ControllaSpazio(m);
                    if (spazioPatch.Length > 0)
                    {
                        throw new SpazioInsufficiente(spazioPatch);
                    }
                    InstallaPatch(m, pa, avanza);
                    return;
                }
                catch (OperationCanceledException)
                {
                    throw;
                }
                catch (SpazioInsufficiente)
                {
                    throw;
                }
                catch (PatchNonVa e)
                {
                    // il gioco installato non e' quello che la patch si aspetta: si scarica tutto (gli errori di rete
                    // e i file aperti invece arrivano al giocatore com'erano, e la prossima volta si riprova la patch)
                    Registro.Scrivi("la patch da " + pa.Da + " non va (" + e.Message + "): scarico il gioco intero");
                    PulisciPatch(m, pa);
                    m.Patch.Remove(pa);
                }
            }
            if (m.Parti.Count == 0)
            {
                throw new InvalidDataException("il manifesto non dice da dove scaricare il gioco");
            }
            Directory.CreateDirectory(Cartelle.Scaricati);
            PulisciScaricati(m);
            PulisciPatch(m, null);
            string spazio = ControllaSpazio(m);
            if (spazio.Length > 0)
            {
                throw new SpazioInsufficiente(spazio);
            }
            if (!CartellaDelGiocoBuona())
            {
                throw new InvalidDataException("la cartella del gioco (" + config.PercorsoGioco + ") contiene altre cose: scegline una vuota nelle Opzioni");
            }

            // 1. le parti
            long totale = 0;
            foreach (ParteDelPacchetto p in m.Parti)
            {
                totale += Math.Max(0, p.Byte);
            }
            List<string> file = new List<string>();
            long primaDiQuesta = 0;
            velocita = 0;
            for (int i = 0; i < m.Parti.Count; i++)
            {
                if (Annulla)
                {
                    throw new OperationCanceledException();
                }
                string nome = NomeParte(m, i);
                ScaricaParte(m.Parti[i], nome, i, m.Parti.Count, primaDiQuesta, totale, avanza);
                file.Add(nome);
                primaDiQuesta += new FileInfo(nome).Length;
            }
            Registro.Scrivi("pacchetto pronto: " + m.Parti.Count + " parti, " + Misure.Byte(primaDiQuesta));

            // 2. si scompatta accanto, poi si scambiano le cartelle
            string gioco = config.PercorsoGioco;
            string nuovo = gioco.TrimEnd('\\', '/') + "_nuovo";
            string vecchio = gioco.TrimEnd('\\', '/') + "_vecchio";
            if (Directory.Exists(nuovo))
            {
                Directory.Delete(nuovo, true);
            }
            avanza(Fase.Installo, "Installo", 0, 1, 0);
            Scompatta(file, nuovo, avanza);

            // se lo zip ha una sola cartella dentro (per esempio "Windows"), quella è il gioco
            string radice = nuovo;
            string[] cartelle = Directory.GetDirectories(nuovo);
            if (cartelle.Length == 1 && Directory.GetFiles(nuovo).Length == 0)
            {
                radice = cartelle[0];
            }
            File.WriteAllText(Path.Combine(radice, "valdorso_versione.txt"), m.Versione);

            // la cartella del gioco si sostituisce tutta: deve essere davvero di Valdorso (o vuota)
            if (!CartellaDelGiocoBuona())
            {
                throw new InvalidDataException("la cartella del gioco (" + gioco + ") contiene altre cose: scegline una vuota nelle Opzioni");
            }
            if (Directory.Exists(vecchio))
            {
                Directory.Delete(vecchio, true);
            }
            Scambio = true;
            try
            {
                if (Directory.Exists(gioco))
                {
                    Directory.Move(gioco, vecchio);
                }
                Directory.CreateDirectory(Path.GetDirectoryName(Path.GetFullPath(gioco).TrimEnd('\\', '/')));
                Directory.Move(radice, gioco);
            }
            catch (Exception)
            {
                // per esempio un file del gioco ancora aperto: si rimette tutto com'era
                if (!Directory.Exists(gioco) && Directory.Exists(vecchio))
                {
                    Directory.Move(vecchio, gioco);
                }
                throw;
            }
            finally
            {
                Scambio = false;
            }
            if (radice != nuovo && Directory.Exists(nuovo))
            {
                Directory.Delete(nuovo, true);
            }
            // le impostazioni del giocatore (Saved del gioco: GameUserSettings.ini, i biglietti...) passano al nuovo
            if (Directory.Exists(vecchio))
            {
                try
                {
                    SpostaSaved(vecchio, gioco);
                    Directory.Delete(vecchio, true);
                }
                catch (Exception e)
                {
                    Registro.Scrivi("pulizia della versione vecchia non finita: " + e.Message);
                }
            }
            // le parti non servono più (occupano spazio)
            foreach (string f in file)
            {
                Cancella(f);
                Cancella(f + ".ok");
            }
            Registro.Scrivi("installata la versione " + m.Versione);
        }

        /// <summary>(12/11) La patch non si puo' mettere (il gioco installato non e' quello giusto, uno zip rovinato...):
        /// si scarica il gioco intero. Gli errori di rete invece non cambiano strada: si riprova la patch.</summary>
        class PatchNonVa : Exception
        {
            public PatchNonVa(string messaggio, Exception dentro) : base(messaggio, dentro)
            {
            }
        }

        /// <summary>(12/11) Vero se l'errore e' un file aperto da un altro programma (il gioco acceso).</summary>
        static bool FileInUso(Exception e)
        {
            IOException io = e as IOException;
            int codice = io != null ? (io.HResult & 0xFFFF) : 0;
            return codice == 32 || codice == 33;
        }

        /// <summary>(12/11) Via le cartelle e i pezzi di una patch (dopo, o se si passa al gioco intero).</summary>
        void PulisciPatch(Manifesto m, PatchDelGioco pa)
        {
            string gioco = Path.GetFullPath(config.PercorsoGioco).TrimEnd('\\', '/');
            foreach (string c in new string[] { gioco + "_patch", gioco + "_patch_vecchio" })
            {
                try
                {
                    if (Directory.Exists(c))
                    {
                        Directory.Delete(c, true);
                    }
                }
                catch (Exception e)
                {
                    Registro.Scrivi("pulizia della patch non finita: " + e.Message);
                }
            }
            if (pa != null)
            {
                for (int i = 0; i < pa.Parti.Count; i++)
                {
                    Cancella(NomeParchePatch(m, pa, i));
                    Cancella(NomeParchePatch(m, pa, i) + ".ok");
                }
            }
        }

        /// <summary>(12/11) La patch: scarica le sue parti, le scompatta accanto e mette i file sopra il gioco, tenendo da
        /// parte i vecchi finche' non e' finito tutto. Poi controlla che ogni file del gioco abbia la misura giusta: se
        /// qualcosa non torna, rimette tutto com'era e lancia PatchNonVa (allora si scarica il gioco intero).</summary>
        void InstallaPatch(Manifesto m, PatchDelGioco pa, Avanzamento avanza)
        {
            long totale = 0;
            foreach (ParteDelPacchetto p in pa.Parti)
            {
                totale += Math.Max(0, p.Byte);
            }
            List<string> file = new List<string>();
            long primaDiQuesta = 0;
            velocita = 0;
            // lo scarico: se la rete cade l'errore arriva al giocatore com'e' (riprovando, si riprende la patch)
            for (int i = 0; i < pa.Parti.Count; i++)
            {
                if (Annulla)
                {
                    throw new OperationCanceledException();
                }
                string nome = NomeParchePatch(m, pa, i);
                ScaricaParte(pa.Parti[i], nome, i, pa.Parti.Count, primaDiQuesta, totale, avanza);
                file.Add(nome);
                primaDiQuesta += new FileInfo(nome).Length;
            }
            Registro.Scrivi("patch da " + pa.Da + " a " + m.Versione + " pronta: " + Misure.Byte(primaDiQuesta));

            string gioco = Path.GetFullPath(config.PercorsoGioco).TrimEnd('\\', '/');
            string sopra = gioco + "_patch";
            string daParte = gioco + "_patch_vecchio";
            PulisciPatch(m, null);
            avanza(Fase.Installo, "Installo le novita'", 0, 1, 0);
            try
            {
                Scompatta(file, sopra, avanza);
            }
            catch (OperationCanceledException)
            {
                PulisciPatch(m, null);
                throw;
            }
            catch (Exception e)
            {
                PulisciPatch(m, null);
                throw new PatchNonVa("la patch non si scompatta: " + e.Message, e);
            }

            List<string> aggiunti = new List<string>();
            List<string> spostati = new List<string>();
            HashSet<string> nuovi = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
            string baseSopra = sopra + Path.DirectorySeparatorChar;
            string installata = VersioneInstallata();
            Scambio = true;
            try
            {
                // mentre si lavora il gioco non ha una versione: se il PC si spegne a meta', la prossima volta si
                // scarica il gioco intero (invece di partire con meta' dei file vecchi e meta' nuovi)
                Cancella(FileVersione);
                foreach (string nuovo in Directory.GetFiles(sopra, "*", SearchOption.AllDirectories))
                {
                    string relativo = nuovo.Substring(baseSopra.Length);
                    nuovi.Add(relativo.Replace('\\', '/'));
                    string dove = Path.Combine(gioco, relativo);
                    if (File.Exists(dove))
                    {
                        string vecchio = Path.Combine(daParte, relativo);
                        Directory.CreateDirectory(Path.GetDirectoryName(vecchio));
                        File.Move(dove, vecchio);
                        spostati.Add(relativo);
                    }
                    else
                    {
                        aggiunti.Add(relativo);
                    }
                    Directory.CreateDirectory(Path.GetDirectoryName(dove));
                    File.Move(nuovo, dove);
                }
                foreach (string togli in pa.Togli)
                {
                    // un file che ha solo cambiato maiuscole e' gia' arrivato nuovo: non si toglie
                    if (nuovi.Contains(togli.Replace('\\', '/')))
                    {
                        continue;
                    }
                    string relativo = togli.Replace('/', Path.DirectorySeparatorChar);
                    string dove = Path.GetFullPath(Path.Combine(gioco, relativo));
                    if (!dove.StartsWith(gioco + Path.DirectorySeparatorChar, StringComparison.OrdinalIgnoreCase) || !File.Exists(dove))
                    {
                        continue;
                    }
                    string vecchio = Path.Combine(daParte, relativo);
                    Directory.CreateDirectory(Path.GetDirectoryName(vecchio));
                    File.Move(dove, vecchio);
                    spostati.Add(relativo);
                }
                // il controllo: ogni file del gioco nuovo c'e', con la misura giusta
                foreach (FileDelGioco f in m.File)
                {
                    string dove = Path.Combine(gioco, f.Percorso.Replace('/', Path.DirectorySeparatorChar));
                    FileInfo info = new FileInfo(dove);
                    if (!info.Exists || (f.Byte > 0 && info.Length != f.Byte))
                    {
                        throw new PatchNonVa("dopo la patch " + f.Percorso + (info.Exists ? " ha la misura sbagliata" : " manca") +
                            ": il gioco installato non era la v" + pa.Da + " giusta", null);
                    }
                }
                File.WriteAllText(FileVersione, m.Versione);
            }
            catch (Exception e)
            {
                // si rimette tutto com'era: via i file nuovi, tornano i vecchi
                foreach (string relativo in aggiunti)
                {
                    Cancella(Path.Combine(gioco, relativo));
                }
                foreach (string relativo in spostati)
                {
                    try
                    {
                        string dove = Path.Combine(gioco, relativo);
                        Cancella(dove);
                        File.Move(Path.Combine(daParte, relativo), dove);
                    }
                    catch (Exception e2)
                    {
                        Registro.Scrivi("patch: non riesco a rimettere " + relativo + ": " + e2.Message);
                    }
                }
                try
                {
                    if (installata.Length > 0)
                    {
                        File.WriteAllText(FileVersione, installata);
                    }
                }
                catch (Exception e3)
                {
                    Registro.Scrivi("patch: non riesco a rimettere la versione: " + e3.Message);
                }
                Scambio = false;
                PulisciPatch(m, null);
                if (FileInUso(e))
                {
                    throw new IOException("un file del gioco e' aperto (il gioco e' acceso?): chiudi Valdorso e riprova", e);
                }
                if (e is PatchNonVa)
                {
                    throw;
                }
                throw new PatchNonVa("la patch non si mette: " + e.Message, e);
            }
            finally
            {
                Scambio = false;
            }
            PulisciPatch(m, pa);
            Registro.Scrivi("installata la patch: dalla " + pa.Da + " alla " + m.Versione + " (" + aggiunti.Count + " file nuovi, " +
                spostati.Count + " cambiati o tolti)");
        }

        /// <summary>Via dalla cartella Scaricati i pezzi delle versioni vecchie (lo zip intero del launcher 1.0 compreso).</summary>
        void PulisciScaricati(Manifesto m)
        {
            string mio = "Valdorso_" + Pulito(m.Versione) + ".zip.";
            try
            {
                foreach (string f in Directory.GetFiles(Cartelle.Scaricati, "Valdorso_*"))
                {
                    if (!Path.GetFileName(f).StartsWith(mio, StringComparison.OrdinalIgnoreCase))
                    {
                        Cancella(f);
                        Registro.Scrivi("tolto il pezzo vecchio " + Path.GetFileName(f));
                    }
                }
            }
            catch (Exception e)
            {
                Registro.Scrivi("pulizia di Scaricati: " + e.Message);
            }
        }

        static void Cancella(string f)
        {
            try
            {
                if (File.Exists(f))
                {
                    File.Delete(f);
                }
            }
            catch (Exception)
            {
            }
        }

        void ScaricaParte(ParteDelPacchetto p, string finale, int indice, int quante, long primaDiQuesta, long totale, Avanzamento avanza)
        {
            string chi = quante > 1 ? "la parte " + (indice + 1) + " di " + quante : "il pacchetto";
            string impronta = p.Sha256.Length > 0 ? p.Sha256 : "senza impronta";
            if (ParteFinita(p, finale))
            {
                return;
            }
            // un file intero senza il segno ".ok" (scaricato a mano, o il controllo interrotto): si controlla
            if (File.Exists(finale))
            {
                if (p.Sha256.Length > 0 && (p.Byte <= 0 || new FileInfo(finale).Length == p.Byte) &&
                    Sha256File(finale, Fase.Controllo, "Controllo " + chi, avanza, this) == p.Sha256)
                {
                    File.WriteAllText(finale + ".ok", impronta);
                    return;
                }
                Cancella(finale);
            }
            string scarico = finale + ".scarico";
            for (int giro = 1; ; giro++)
            {
                Scarica(p.Url, scarico, p.Byte, primaDiQuesta, totale, "Scarico " + chi, avanza);
                if (Annulla)
                {
                    throw new OperationCanceledException();
                }
                string letta = Sha256File(scarico, Fase.Controllo, "Controllo " + chi, avanza, this);
                if (p.Sha256.Length == 0 || letta == p.Sha256)
                {
                    break;
                }
                Registro.Scrivi(chi + ": impronta diversa (" + letta + " invece di " + p.Sha256 + "), giro " + giro);
                Cancella(scarico);
                if (giro >= 2)
                {
                    throw new InvalidDataException(chi + " arriva rovinata anche scaricandola di nuovo: riprova più tardi (o scrivi a Fra)");
                }
            }
            Cancella(finale);
            File.Move(scarico, finale);
            File.WriteAllText(finale + ".ok", impronta);
        }

        /// <summary>Apre l'indirizzo seguendo i rinvii a mano (GitHub manda le Release su objects.githubusercontent.com):
        /// così la richiesta "da questo byte in poi" arriva fino all'ultimo indirizzo.</summary>
        HttpWebResponse Apri(string url, long da)
        {
            Uri dove = new Uri(url);
            for (int salti = 0; salti < 10; salti++)
            {
                HttpWebRequest r = (HttpWebRequest)WebRequest.Create(dove);
                r.AllowAutoRedirect = false;
                r.UserAgent = Rete.Agente;
                r.Timeout = 30000;
                r.ReadWriteTimeout = 60000;
                r.Accept = "*/*";
                if (da > 0)
                {
                    r.AddRange(da);
                }
                richiesta = r;
                if (Annulla)
                {
                    throw new OperationCanceledException();
                }
                HttpWebResponse risposta;
                try
                {
                    risposta = (HttpWebResponse)r.GetResponse();
                }
                catch (WebException e)
                {
                    risposta = e.Response as HttpWebResponse;
                    if (risposta == null || !Rinvio((int)risposta.StatusCode))
                    {
                        throw;
                    }
                }
                int codice = (int)risposta.StatusCode;
                if (Rinvio(codice))
                {
                    string altrove = risposta.Headers[HttpResponseHeader.Location];
                    risposta.Close();
                    if (string.IsNullOrEmpty(altrove))
                    {
                        throw new WebException("rinvio senza indirizzo (" + codice + ")");
                    }
                    dove = new Uri(dove, altrove);
                    continue;
                }
                return risposta;
            }
            throw new WebException("troppi rinvii per " + url);
        }

        static bool Rinvio(int codice)
        {
            return codice == 301 || codice == 302 || codice == 303 || codice == 307 || codice == 308;
        }

        /// <summary>L'inizio scritto in "Content-Range: bytes 1000-1999/2000" (-1 se non c'è).</summary>
        static long InizioIntervallo(HttpWebResponse r)
        {
            string intervallo = r.Headers["Content-Range"] ?? "";
            int spazio = intervallo.IndexOf(' ');
            int trattino = intervallo.IndexOf('-');
            long inizio;
            if (spazio >= 0 && trattino > spazio && long.TryParse(intervallo.Substring(spazio + 1, trattino - spazio - 1), NumberStyles.Integer, CultureInfo.InvariantCulture, out inizio))
            {
                return inizio;
            }
            return -1;
        }

        void Scarica(string url, string file, long attesi, long primaDiQuesta, long totale, string testo, Avanzamento avanza)
        {
            int tentativo = 0;
            while (true)
            {
                long gia = File.Exists(file) ? new FileInfo(file).Length : 0;
                if (attesi > 0 && gia > attesi)
                {
                    Cancella(file);
                    gia = 0;
                }
                if (attesi > 0 && gia == attesi)
                {
                    return;
                }
                long all_inizio = gia;
                try
                {
                    using (HttpWebResponse risposta = Apri(url, gia))
                    {
                        bool riprende = gia > 0 && risposta.StatusCode == HttpStatusCode.PartialContent && InizioIntervallo(risposta) == gia;
                        if (gia > 0 && !riprende)
                        {
                            Registro.Scrivi("il server non riprende da metà (" + (int)risposta.StatusCode + "): ricomincio questo pezzo");
                            gia = 0;
                        }
                        using (Stream dentro = risposta.GetResponseStream())
                        using (FileStream fuori = new FileStream(file, riprende ? FileMode.Append : FileMode.Create, FileAccess.Write, FileShare.None, 1 << 20))
                        {
                            byte[] pezzo = new byte[1 << 18];
                            DateTime finestra = DateTime.UtcNow;
                            long inFinestra = 0;
                            DateTime ultimoAvviso = DateTime.MinValue;
                            int letti;
                            while ((letti = dentro.Read(pezzo, 0, pezzo.Length)) > 0)
                            {
                                if (Annulla)
                                {
                                    throw new OperationCanceledException();
                                }
                                fuori.Write(pezzo, 0, letti);
                                gia += letti;
                                inFinestra += letti;
                                double secondi = (DateTime.UtcNow - finestra).TotalSeconds;
                                if (secondi >= 1.0)
                                {
                                    // la velocità si addolcisce: niente cifre che saltano
                                    double adesso = inFinestra / secondi;
                                    velocita = velocita <= 0 ? adesso : velocita * 0.75 + adesso * 0.25;
                                    finestra = DateTime.UtcNow;
                                    inFinestra = 0;
                                }
                                if ((DateTime.UtcNow - ultimoAvviso).TotalMilliseconds > 200)
                                {
                                    long tutto = totale > 0 ? totale : (risposta.ContentLength > 0 ? risposta.ContentLength + (riprende ? all_inizio : 0) : 0);
                                    avanza(Fase.Scarico, testo, primaDiQuesta + gia, tutto, velocita);
                                    ultimoAvviso = DateTime.UtcNow;
                                }
                            }
                        }
                    }
                    if (attesi > 0 && new FileInfo(file).Length < attesi)
                    {
                        throw new IOException("la connessione si è chiusa prima della fine");
                    }
                    return;
                }
                catch (OperationCanceledException)
                {
                    throw;
                }
                catch (Exception e)
                {
                    if (Annulla)
                    {
                        throw new OperationCanceledException();
                    }
                    if (!(e is WebException || e is IOException))
                    {
                        throw;
                    }
                    WebException w = e as WebException;
                    HttpWebResponse r = w != null ? w.Response as HttpWebResponse : null;
                    int codice = r != null ? (int)r.StatusCode : 0;
                    if (codice == 416 && gia > 0)
                    {
                        // il pezzo scaricato è già tutto il file (o è sbagliato): lo dice il controllo dopo
                        if (attesi <= 0 || gia >= attesi)
                        {
                            return;
                        }
                        Cancella(file);
                        continue;
                    }
                    if (codice == 404 || codice == 403 || codice == 410)
                    {
                        throw new WebException("il file non c'è sul sito (" + codice + "): forse l'aggiornamento non è ancora caricato tutto. " + url);
                    }
                    long ora = File.Exists(file) ? new FileInfo(file).Length : 0;
                    // se in questo giro si è andati avanti, i tentativi ripartono: per scaricare GB basta che la rete torni
                    tentativo = ora > all_inizio ? 1 : tentativo + 1;
                    Registro.Scrivi("scaricamento interrotto (tentativo " + tentativo + "): " + e.Message);
                    if (tentativo >= 6)
                    {
                        throw;
                    }
                    avanza(Fase.Scarico, "La rete si è interrotta: riprovo tra poco", primaDiQuesta + ora, totale, 0);
                    for (int i = 0; i < Math.Min(30, 3 * tentativo) * 10; i++)
                    {
                        if (Annulla)
                        {
                            throw new OperationCanceledException();
                        }
                        Thread.Sleep(100);
                    }
                }
            }
        }

        /// <summary>Controlla ogni file del gioco con l'elenco del manifesto. Restituisce i file mancanti o diversi.</summary>
        public List<string> Verifica(Manifesto m, Avanzamento avanza)
        {
            List<string> sbagliati = new List<string>();
            long totale = 0;
            foreach (FileDelGioco f in m.File)
            {
                totale += f.Byte;
            }
            long fatti = 0;
            int contati = 0;
            foreach (FileDelGioco f in m.File)
            {
                if (Annulla)
                {
                    throw new OperationCanceledException();
                }
                string percorso = Path.Combine(config.PercorsoGioco, f.Percorso.Replace('/', Path.DirectorySeparatorChar));
                if (!File.Exists(percorso) || new FileInfo(percorso).Length != f.Byte || Sha256File(percorso, Fase.Verifica, null, null, this) != f.Sha256)
                {
                    sbagliati.Add(f.Percorso);
                }
                fatti += f.Byte;
                contati++;
                avanza(Fase.Verifica, "Controllo i file (" + contati + " di " + m.File.Count + ")", fatti, Math.Max(1, totale), 0);
            }
            return sbagliati;
        }

        private void Scompatta(List<string> parti, string dove, Avanzamento avanza)
        {
            Directory.CreateDirectory(dove);
            string base_ = Path.GetFullPath(dove).TrimEnd(Path.DirectorySeparatorChar) + Path.DirectorySeparatorChar;
            using (FlussoUnito flusso = new FlussoUnito(parti))
            using (ZipArchive archivio = new ZipArchive(flusso, ZipArchiveMode.Read, false))
            {
                long totale = 0;
                foreach (ZipArchiveEntry e in archivio.Entries)
                {
                    totale += e.Length;
                }
                long fatti = 0;
                DateTime ultimoAvviso = DateTime.MinValue;
                byte[] pezzo = new byte[1 << 20];
                foreach (ZipArchiveEntry e in archivio.Entries)
                {
                    if (Annulla)
                    {
                        throw new OperationCanceledException();
                    }
                    string destinazione = Path.GetFullPath(Path.Combine(dove, e.FullName));
                    // niente file fuori dalla cartella (uno zip fatto male non deve scrivere altrove)
                    if (!destinazione.StartsWith(base_, StringComparison.OrdinalIgnoreCase))
                    {
                        throw new InvalidDataException("il pacchetto contiene un percorso non valido: " + e.FullName);
                    }
                    if (e.FullName.EndsWith("/") || e.FullName.EndsWith("\\"))
                    {
                        Directory.CreateDirectory(destinazione);
                        continue;
                    }
                    Directory.CreateDirectory(Path.GetDirectoryName(destinazione));
                    // a pezzi, così anche un file grande (i .pak) fa muovere la barra
                    using (Stream dentro = e.Open())
                    using (FileStream fuori = new FileStream(destinazione, FileMode.Create, FileAccess.Write, FileShare.None, 1 << 20))
                    {
                        int letti;
                        while ((letti = dentro.Read(pezzo, 0, pezzo.Length)) > 0)
                        {
                            if (Annulla)
                            {
                                throw new OperationCanceledException();
                            }
                            fuori.Write(pezzo, 0, letti);
                            fatti += letti;
                            if ((DateTime.UtcNow - ultimoAvviso).TotalMilliseconds > 200)
                            {
                                avanza(Fase.Installo, "Installo", fatti, Math.Max(1, totale), 0);
                                ultimoAvviso = DateTime.UtcNow;
                            }
                        }
                    }
                    try
                    {
                        File.SetLastWriteTime(destinazione, e.LastWriteTime.DateTime);
                    }
                    catch (Exception)
                    {
                    }
                }
                avanza(Fase.Installo, "Installo", totale, Math.Max(1, totale), 0);
            }
        }

        private static void SpostaSaved(string vecchio, string nuovo)
        {
            foreach (string progetto in Directory.GetDirectories(vecchio))
            {
                string saved = Path.Combine(progetto, "Saved");
                if (!Directory.Exists(saved))
                {
                    continue;
                }
                string destinazione = Path.Combine(nuovo, Path.GetFileName(progetto), "Saved");
                if (Directory.Exists(destinazione))
                {
                    continue;
                }
                Directory.CreateDirectory(Path.GetDirectoryName(destinazione));
                Directory.Move(saved, destinazione);
                Registro.Scrivi("impostazioni del giocatore portate nella versione nuova: " + destinazione);
            }
        }

        /// <summary>L'impronta SHA-256 di un file. Se "chi" viene fermato, si ferma anche il conto.</summary>
        public static string Sha256File(string percorso, Fase fase, string testo, Avanzamento avanza, Aggiornatore chi)
        {
            using (SHA256 sha = SHA256.Create())
            using (FileStream f = new FileStream(percorso, FileMode.Open, FileAccess.Read, FileShare.Read, 1 << 20))
            {
                byte[] pezzo = new byte[1 << 20];
                long fatti = 0;
                long totale = Math.Max(1, f.Length);
                int letti;
                DateTime ultimoAvviso = DateTime.MinValue;
                while ((letti = f.Read(pezzo, 0, pezzo.Length)) > 0)
                {
                    if (chi != null && chi.Annulla)
                    {
                        throw new OperationCanceledException();
                    }
                    sha.TransformBlock(pezzo, 0, letti, null, 0);
                    fatti += letti;
                    if (avanza != null && testo != null && (DateTime.UtcNow - ultimoAvviso).TotalMilliseconds > 200)
                    {
                        avanza(fase, testo, fatti, totale, 0);
                        ultimoAvviso = DateTime.UtcNow;
                    }
                }
                sha.TransformFinalBlock(pezzo, 0, 0);
                StringBuilder s = new StringBuilder();
                foreach (byte b in sha.Hash)
                {
                    s.Append(b.ToString("x2", CultureInfo.InvariantCulture));
                }
                return s.ToString();
            }
        }

        private static string Pulito(string versione)
        {
            StringBuilder s = new StringBuilder();
            foreach (char c in versione ?? "")
            {
                if (char.IsLetterOrDigit(c) || c == '.' || c == '-')
                {
                    s.Append(c);
                }
            }
            return s.Length > 0 ? s.ToString() : "nuova";
        }
    }

    /// <summary>Più file letti come uno solo, uno dopo l'altro (le parti dello zip), solo in lettura, con lo spostamento:
    /// è quello che serve a ZipArchive, che legge prima l'indice in fondo e poi salta ai file.</summary>
    public class FlussoUnito : Stream
    {
        readonly List<FileStream> file = new List<FileStream>();
        readonly List<long> inizi = new List<long>();
        readonly long lunghezza;
        long posizione;

        public FlussoUnito(List<string> percorsi)
        {
            long somma = 0;
            try
            {
                foreach (string p in percorsi)
                {
                    FileStream f = new FileStream(p, FileMode.Open, FileAccess.Read, FileShare.Read, 1 << 16);
                    file.Add(f);
                    inizi.Add(somma);
                    somma += f.Length;
                }
            }
            catch
            {
                Dispose(true);
                throw;
            }
            lunghezza = somma;
        }

        public override bool CanRead { get { return true; } }
        public override bool CanSeek { get { return true; } }
        public override bool CanWrite { get { return false; } }
        public override long Length { get { return lunghezza; } }

        public override long Position
        {
            get { return posizione; }
            set { Seek(value, SeekOrigin.Begin); }
        }

        public override int Read(byte[] buffer, int offset, int count)
        {
            int letti = 0;
            while (count > 0 && posizione < lunghezza)
            {
                int k = file.Count - 1;
                while (k > 0 && inizi[k] > posizione)
                {
                    k--;
                }
                FileStream f = file[k];
                long dentro = posizione - inizi[k];
                long restano = f.Length - dentro;
                if (restano <= 0)
                {
                    // un file vuoto in mezzo: si passa oltre
                    if (k == file.Count - 1)
                    {
                        break;
                    }
                    posizione = inizi[k + 1];
                    continue;
                }
                if (f.Position != dentro)
                {
                    f.Position = dentro;
                }
                int n = f.Read(buffer, offset, (int)Math.Min(count, restano));
                if (n <= 0)
                {
                    break;
                }
                letti += n;
                offset += n;
                count -= n;
                posizione += n;
            }
            return letti;
        }

        public override long Seek(long offset, SeekOrigin origin)
        {
            long nuova = origin == SeekOrigin.Begin ? offset : (origin == SeekOrigin.Current ? posizione + offset : lunghezza + offset);
            if (nuova < 0)
            {
                throw new IOException("posizione prima dell'inizio");
            }
            posizione = nuova;
            return posizione;
        }

        public override void Flush()
        {
        }

        public override void SetLength(long value)
        {
            throw new NotSupportedException();
        }

        public override void Write(byte[] buffer, int offset, int count)
        {
            throw new NotSupportedException();
        }

        protected override void Dispose(bool disposing)
        {
            if (disposing)
            {
                foreach (FileStream f in file)
                {
                    f.Dispose();
                }
                file.Clear();
            }
            base.Dispose(disposing);
        }
    }
}
