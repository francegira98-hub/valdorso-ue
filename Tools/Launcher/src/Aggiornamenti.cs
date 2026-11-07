// Valdorso - Il launcher: il manifesto, lo scaricamento con ripresa, il controllo SHA-256, l'installazione e lo stato
// del mondo. Scritto da Claude il 07/11/2026.
//
// Il manifesto (manifesto.json, sul sito) lo prepara Tools/Launcher/prepara_aggiornamento.py dal gioco pacchettizzato:
//   versione, nome, data, pacchetto (url, sha256, byte), eseguibile, argomenti, server (indirizzo, stato),
//   notizie (data, titolo, testo, link), launcher (versione, url), file (p = percorso, s = sha256, b = byte).
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
    public class Notizia
    {
        public string Data = "";
        public string Titolo = "";
        public string Testo = "";
        public string Link = "";
    }

    public class FileDelGioco
    {
        public string Percorso = "";
        public string Sha256 = "";
        public long Byte;
    }

    public class Manifesto
    {
        public string Versione = "";
        public string NomeVersione = "";
        public string Data = "";
        public string UrlPacchetto = "";
        public string Sha256Pacchetto = "";
        public long BytePacchetto;
        public string Eseguibile = "Valdorso.exe";
        public string Argomenti = "";
        public string Server = "";
        public string Stato = "";
        public string VersioneLauncher = "";
        public string UrlLauncher = "";
        public bool Aperto = true;
        public List<Notizia> Notizie = new List<Notizia>();
        public List<FileDelGioco> File = new List<FileDelGioco>();

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
            m.UrlPacchetto = Json.Testo(p, "url", "");
            m.Sha256Pacchetto = Json.Testo(p, "sha256", "").ToLowerInvariant();
            m.BytePacchetto = (long)Json.Numero(p, "byte", 0);
            Dictionary<string, object> s = Json.Oggetto(d, "server");
            m.Server = Json.Testo(s, "indirizzo", "");
            m.Stato = Json.Testo(s, "stato", "");
            Dictionary<string, object> l = Json.Oggetto(d, "launcher");
            m.VersioneLauncher = Json.Testo(l, "versione", "");
            m.UrlLauncher = Json.Testo(l, "url", "");
            foreach (Dictionary<string, object> n in Json.Elenco(d, "notizie"))
            {
                Notizia x = new Notizia();
                x.Data = Json.Testo(n, "data", "");
                x.Titolo = Json.Testo(n, "titolo", "");
                x.Testo = Json.Testo(n, "testo", "");
                x.Link = Json.Testo(n, "link", "");
                m.Notizie.Add(x);
            }
            foreach (Dictionary<string, object> f in Json.Elenco(d, "file"))
            {
                FileDelGioco x = new FileDelGioco();
                x.Percorso = Json.Testo(f, "p", "");
                x.Sha256 = Json.Testo(f, "s", "").ToLowerInvariant();
                x.Byte = (long)Json.Numero(f, "b", 0);
                if (x.Percorso.Length > 0)
                {
                    m.File.Add(x);
                }
            }
            return m;
        }
    }

    /// <summary>Lo stato del mondo che il server dà su http://server:7780/stato (lo stesso del menu del gioco).</summary>
    public class StatoMondo
    {
        public string Versione = "";
        public double Ora = -1;
        public string Tempo = "";
        public string Stagione = "";
        public int Coloni;
        public int Posti;
        public string Notizia = "";
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
            s.Ora = Json.Numero(d, "ora", -1);
            s.Tempo = Json.Testo(d, "tempo", "");
            s.Stagione = Json.Testo(d, "stagione", "");
            s.Coloni = (int)Json.Numero(d, "coloni", 0);
            s.Posti = (int)Json.Numero(d, "posti", 0);
            s.Notizia = Json.Testo(d, "notizia", "");
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
        }

        public static string Pagina(string url, int millisecondi)
        {
            HttpWebRequest r = (HttpWebRequest)WebRequest.Create(url);
            r.Timeout = millisecondi;
            r.ReadWriteTimeout = millisecondi;
            r.UserAgent = "ValdorsoLauncher/" + Info.Versione;
            r.Headers[HttpRequestHeader.CacheControl] = "no-cache";
            r.AutomaticDecompression = DecompressionMethods.GZip | DecompressionMethods.Deflate;
            using (HttpWebResponse risposta = (HttpWebResponse)r.GetResponse())
            using (StreamReader lettore = new StreamReader(risposta.GetResponseStream(), Encoding.UTF8))
            {
                return lettore.ReadToEnd();
            }
        }
    }

    public delegate void Avanzamento(string fase, long fatti, long totale, double bytePerSecondo);

    /// <summary>Scarica, controlla e installa. Tutto qui gira fuori dalla finestra (in un altro thread).</summary>
    public class Aggiornatore
    {
        private readonly Configurazione config;
        public volatile bool Annulla;

        public Aggiornatore(Configurazione config)
        {
            this.config = config;
        }

        public string FileVersione
        {
            get { return Path.Combine(config.PercorsoGioco, "valdorso_versione.txt"); }
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

        /// <summary>Scarica il pacchetto (riprende da dove si era fermato), lo controlla e lo installa.</summary>
        public void Installa(Manifesto m, Avanzamento avanza)
        {
            if (string.IsNullOrEmpty(m.UrlPacchetto))
            {
                throw new InvalidDataException("il manifesto non dice da dove scaricare il gioco");
            }
            Directory.CreateDirectory(Cartelle.Scaricati);
            string zip = Path.Combine(Cartelle.Scaricati, "Valdorso_" + Pulito(m.Versione) + ".zip");
            string parte = zip + ".part";

            if (!(File.Exists(zip) && Sha256File(zip, null, avanza) == m.Sha256Pacchetto))
            {
                Scarica(m.UrlPacchetto, parte, m.BytePacchetto, avanza);
                avanza("Controllo il pacchetto", 0, 1, 0);
                string impronta = Sha256File(parte, "Controllo il pacchetto", avanza);
                if (m.Sha256Pacchetto.Length > 0 && impronta != m.Sha256Pacchetto)
                {
                    File.Delete(parte);
                    throw new InvalidDataException("il pacchetto scaricato non è giusto (impronta diversa): riprova");
                }
                if (File.Exists(zip))
                {
                    File.Delete(zip);
                }
                File.Move(parte, zip);
            }
            Registro.Scrivi("pacchetto pronto: " + zip);

            // si scompatta accanto, poi si scambiano le cartelle
            string gioco = config.PercorsoGioco;
            string nuovo = gioco.TrimEnd('\\', '/') + "_nuovo";
            string vecchio = gioco.TrimEnd('\\', '/') + "_vecchio";
            if (Directory.Exists(nuovo))
            {
                Directory.Delete(nuovo, true);
            }
            avanza("Installo", 0, 1, 0);
            Scompatta(zip, nuovo, avanza);

            // se lo zip ha una sola cartella dentro (per esempio "Windows"), quella è il gioco
            string radice = nuovo;
            string[] cartelle = Directory.GetDirectories(nuovo);
            if (cartelle.Length == 1 && Directory.GetFiles(nuovo).Length == 0)
            {
                radice = cartelle[0];
            }
            File.WriteAllText(Path.Combine(radice, "valdorso_versione.txt"), m.Versione);

            if (Directory.Exists(vecchio))
            {
                Directory.Delete(vecchio, true);
            }
            if (Directory.Exists(gioco))
            {
                Directory.Move(gioco, vecchio);
            }
            Directory.Move(radice, gioco);
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
            // lo zip non serve più (occupa spazio)
            try
            {
                File.Delete(zip);
            }
            catch (Exception)
            {
            }
            Registro.Scrivi("installata la versione " + m.Versione);
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
            foreach (FileDelGioco f in m.File)
            {
                if (Annulla)
                {
                    break;
                }
                string percorso = Path.Combine(config.PercorsoGioco, f.Percorso.Replace('/', Path.DirectorySeparatorChar));
                if (!File.Exists(percorso) || new FileInfo(percorso).Length != f.Byte || Sha256File(percorso, null, null) != f.Sha256)
                {
                    sbagliati.Add(f.Percorso);
                }
                fatti += f.Byte;
                avanza("Controllo i file", fatti, Math.Max(1, totale), 0);
            }
            return sbagliati;
        }

        private void Scarica(string url, string parte, long attesi, Avanzamento avanza)
        {
            long gia = File.Exists(parte) ? new FileInfo(parte).Length : 0;
            for (int tentativo = 1; ; tentativo++)
            {
                try
                {
                    HttpWebRequest r = (HttpWebRequest)WebRequest.Create(url);
                    r.UserAgent = "ValdorsoLauncher/" + Info.Versione;
                    r.Timeout = 30000;
                    r.ReadWriteTimeout = 30000;
                    if (gia > 0)
                    {
                        r.AddRange(gia);
                    }
                    using (HttpWebResponse risposta = (HttpWebResponse)r.GetResponse())
                    {
                        bool riprende = gia > 0 && risposta.StatusCode == HttpStatusCode.PartialContent;
                        if (!riprende)
                        {
                            gia = 0;
                        }
                        long totale = attesi > 0 ? attesi : (risposta.ContentLength > 0 ? risposta.ContentLength + gia : 0);
                        using (Stream dentro = risposta.GetResponseStream())
                        using (FileStream fuori = new FileStream(parte, riprende ? FileMode.Append : FileMode.Create, FileAccess.Write, FileShare.None, 1 << 20))
                        {
                            byte[] pezzo = new byte[1 << 18];
                            DateTime inizio = DateTime.UtcNow;
                            long inQuestoGiro = 0;
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
                                inQuestoGiro += letti;
                                if ((DateTime.UtcNow - ultimoAvviso).TotalMilliseconds > 150)
                                {
                                    double secondi = Math.Max(0.001, (DateTime.UtcNow - inizio).TotalSeconds);
                                    avanza("Scarico l'aggiornamento", gia, totale, inQuestoGiro / secondi);
                                    ultimoAvviso = DateTime.UtcNow;
                                }
                            }
                        }
                    }
                    return;
                }
                catch (OperationCanceledException)
                {
                    throw;
                }
                catch (WebException e)
                {
                    HttpWebResponse r = e.Response as HttpWebResponse;
                    // 416: il pezzo già scaricato è tutto il file (o è sbagliato): si ricomincia da capo una volta
                    if (r != null && (int)r.StatusCode == 416 && gia > 0)
                    {
                        File.Delete(parte);
                        gia = 0;
                        continue;
                    }
                    Registro.Scrivi("scaricamento interrotto (tentativo " + tentativo + "): " + e.Message);
                    if (tentativo >= 5)
                    {
                        throw;
                    }
                    Thread.Sleep(2000 * tentativo);
                    gia = File.Exists(parte) ? new FileInfo(parte).Length : 0;
                }
                catch (IOException e)
                {
                    Registro.Scrivi("scaricamento interrotto (tentativo " + tentativo + "): " + e.Message);
                    if (tentativo >= 5)
                    {
                        throw;
                    }
                    Thread.Sleep(2000 * tentativo);
                    gia = File.Exists(parte) ? new FileInfo(parte).Length : 0;
                }
            }
        }

        private void Scompatta(string zip, string dove, Avanzamento avanza)
        {
            Directory.CreateDirectory(dove);
            string base_ = Path.GetFullPath(dove) + Path.DirectorySeparatorChar;
            using (ZipArchive archivio = ZipFile.OpenRead(zip))
            {
                long totale = 0;
                foreach (ZipArchiveEntry e in archivio.Entries)
                {
                    totale += e.Length;
                }
                long fatti = 0;
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
                    e.ExtractToFile(destinazione, true);
                    fatti += e.Length;
                    avanza("Installo", fatti, Math.Max(1, totale), 0);
                }
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

        public static string Sha256File(string percorso, string fase, Avanzamento avanza)
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
                    sha.TransformBlock(pezzo, 0, letti, null, 0);
                    fatti += letti;
                    if (avanza != null && fase != null && (DateTime.UtcNow - ultimoAvviso).TotalMilliseconds > 150)
                    {
                        avanza(fase, fatti, totale, 0);
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
}
