// Valdorso - Il launcher: la finestra "Oro e brace". Scritto da Claude il 07/11/2026.
//
// Una finestra senza bordi di Windows (si trascina dalla parte alta), con lo sfondo della valle al tramonto, il titolo,
// le notizie dalla valle, lo stato del mondo dal server e in basso la barra con l'avanzamento e il pulsante grande:
// INSTALLA, AGGIORNA o GIOCA. Tutto è disegnato qui (niente controlli di Windows), con i colori del gioco.

using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.Drawing.Text;
using System.Globalization;
using System.IO;
using System.Runtime.InteropServices;
using System.Threading;
using System.Windows.Forms;

namespace Valdorso.Launcher
{
    public enum StatoLauncher
    {
        Avvio,
        Controllo,
        DaInstallare,
        DaAggiornare,
        Pronto,
        Lavoro,
        Errore,
        SenzaRete
    }

    /// <summary>Un pulsante disegnato: un rettangolo, una scritta, un'azione.</summary>
    public class Pulsante
    {
        public RectangleF Area;
        public string Testo = "";
        public bool Grande;
        public bool Attivo = true;
        public bool Visibile = true;
        public bool Sopra;
        public string Suggerimento = "";
        public Action Azione;
    }

    public class Finestra : Form
    {
        // i colori del tema "Oro e brace" (ValdorsoTemaUI.h)
        static readonly Color Fondo = Color.FromArgb(14, 11, 9);
        static readonly Color Pannelli = Color.FromArgb(21, 17, 13);
        static readonly Color Oro = Color.FromArgb(212, 175, 55);
        static readonly Color OroChiaro = Color.FromArgb(243, 220, 140);
        static readonly Color Pergamena = Color.FromArgb(232, 217, 181);
        static readonly Color Secondario = Color.FromArgb(201, 185, 154);
        static readonly Color Brace = Color.FromArgb(217, 119, 43);
        static readonly Color Rosso = Color.FromArgb(190, 70, 55);

        [DllImport("user32.dll")]
        static extern bool ReleaseCapture();

        [DllImport("user32.dll")]
        static extern IntPtr SendMessage(IntPtr hWnd, int msg, int wParam, int lParam);

        readonly Configurazione config;
        readonly Aggiornatore aggiornatore;
        readonly PrivateFontCollection caratteri = new PrivateFontCollection();
        readonly float S;
        Image sfondo;
        FontFamily famigliaTitolo;
        FontFamily famigliaTesto;
        FontFamily famigliaCorsivo;
        readonly List<Pulsante> pulsanti = new List<Pulsante>();
        Pulsante principale;
        Pulsante verifica;
        Pulsante discord;
        readonly System.Windows.Forms.Timer battito = new System.Windows.Forms.Timer();
        readonly System.Windows.Forms.Timer orologioStato = new System.Windows.Forms.Timer();
        readonly DateTime inizio = DateTime.UtcNow;

        StatoLauncher stato = StatoLauncher.Avvio;
        Manifesto manifesto;
        string versioneInstallata = "";
        string rigaStato = "Cerco la valle...";
        string rigaMondo = "";
        string notiziaServer = "";
        string avvisoLauncher = "";
        double avanzamento = -1;
        bool erroreRiga;
        int statoInViaggio;

        public Finestra()
        {
            config = Configurazione.Carica();
            aggiornatore = new Aggiornatore(config);

            using (Graphics g = CreateGraphics())
            {
                S = Math.Max(1f, g.DpiX / 96f);
            }
            Text = "Valdorso";
            FormBorderStyle = FormBorderStyle.None;
            StartPosition = FormStartPosition.CenterScreen;
            ClientSize = new Size((int)(1100 * S), (int)(640 * S));
            BackColor = Fondo;
            DoubleBuffered = true;
            SetStyle(ControlStyles.AllPaintingInWmPaint | ControlStyles.UserPaint | ControlStyles.OptimizedDoubleBuffer | ControlStyles.ResizeRedraw, true);
            KeyPreview = true;

            CaricaRisorse();
            CreaPulsanti();

            battito.Interval = 50;
            battito.Tick += delegate { Invalidate(new Rectangle(0, (int)(ClientSize.Height - 120 * S), ClientSize.Width, (int)(120 * S))); };
            battito.Start();
            orologioStato.Interval = 30000;
            orologioStato.Tick += delegate { ChiediStato(); };
            orologioStato.Start();

            MouseMove += SuMouseMove;
            MouseDown += SuMouseDown;
            MouseLeave += delegate { foreach (Pulsante p in pulsanti) { p.Sopra = false; } Cursor = Cursors.Default; Invalidate(); };
            KeyDown += SuTasto;
            Shown += delegate { Controlla(); };
            FormClosing += delegate { aggiornatore.Annulla = true; };
        }

        // ------------------------------------------------------------------------------------ risorse

        void CaricaRisorse()
        {
            try
            {
                string icona = Path.Combine(Cartelle.Risorse, "icona.ico");
                if (File.Exists(icona))
                {
                    Icon = new Icon(icona);
                }
                else
                {
                    Icon = Icon.ExtractAssociatedIcon(Application.ExecutablePath);
                }
            }
            catch (Exception)
            {
            }
            try
            {
                string img = Path.Combine(Cartelle.Risorse, "sfondo.jpg");
                if (!File.Exists(img))
                {
                    img = Path.Combine(Cartelle.Risorse, "sfondo.png");
                }
                if (File.Exists(img))
                {
                    // si copia in memoria: così il file non resta bloccato
                    using (Image letta = Image.FromFile(img))
                    {
                        sfondo = new Bitmap(letta);
                    }
                }
            }
            catch (Exception e)
            {
                Registro.Scrivi("sfondo non caricato: " + e.Message);
            }
            famigliaTitolo = Famiglia("Cinzel.ttf", "Cinzel", "Georgia");
            famigliaTesto = Famiglia("EBGaramond.ttf", "EB Garamond", "Georgia");
            famigliaCorsivo = Famiglia("EBGaramond-Italic.ttf", "EB Garamond", "Georgia");
        }

        FontFamily Famiglia(string file, string nome, string riserva)
        {
            try
            {
                string percorso = Path.Combine(Cartelle.Risorse, file);
                if (config.CaratteriPropri && File.Exists(percorso))
                {
                    int prima = caratteri.Families.Length;
                    caratteri.AddFontFile(percorso);
                    FontFamily[] tutte = caratteri.Families;
                    foreach (FontFamily f in tutte)
                    {
                        if (f.Name.StartsWith(nome, StringComparison.OrdinalIgnoreCase))
                        {
                            return f;
                        }
                    }
                    if (tutte.Length > prima)
                    {
                        return tutte[tutte.Length - 1];
                    }
                }
            }
            catch (Exception e)
            {
                Registro.Scrivi("carattere " + file + " non caricato: " + e.Message);
            }
            return new FontFamily(riserva);
        }

        Font Carattere(FontFamily f, float punti, FontStyle stile)
        {
            if (!f.IsStyleAvailable(stile))
            {
                stile = FontStyle.Regular;
                if (!f.IsStyleAvailable(stile))
                {
                    stile = f.IsStyleAvailable(FontStyle.Italic) ? FontStyle.Italic : FontStyle.Bold;
                }
            }
            return new Font(f, punti * S, stile, GraphicsUnit.Pixel);
        }

        // ------------------------------------------------------------------------------------ pulsanti

        void CreaPulsanti()
        {
            float W = ClientSize.Width;
            float H = ClientSize.Height;

            principale = new Pulsante();
            principale.Grande = true;
            principale.Area = new RectangleF(W - 300 * S, H - 96 * S, 250 * S, 62 * S);
            principale.Testo = "ATTENDI";
            principale.Attivo = false;
            principale.Azione = Principale;
            pulsanti.Add(principale);

            // in alto a destra: chiudi e riduci
            Pulsante chiudi = Piccolo(W - 44 * S, 10 * S, 34 * S, "✕", delegate { Close(); });
            chiudi.Suggerimento = "Chiudi";
            Pulsante riduci = Piccolo(W - 82 * S, 10 * S, 34 * S, "–", delegate { WindowState = FormWindowState.Minimized; });
            riduci.Suggerimento = "Riduci a icona";

            // in basso a sinistra: i collegamenti
            float x = 52 * S;
            float y = H - 34 * S;
            Pulsante sito = Collegamento(ref x, y, "Il sito", delegate { Apri(config.Sito); });
            sito.Visibile = config.Sito.Length > 0;
            discord = Collegamento(ref x, y, "Discord", delegate { Apri(config.Discord); });
            discord.Visibile = config.Discord.Length > 0;
            Collegamento(ref x, y, "Cartella del gioco", delegate
            {
                Directory.CreateDirectory(config.PercorsoGioco);
                Apri(config.PercorsoGioco);
            });
            verifica = Collegamento(ref x, y, "Controlla i file", Verifica);
            verifica.Visibile = false;
            Collegamento(ref x, y, "Registro", delegate { Apri(Registro.File); });
        }

        Pulsante Piccolo(float x, float y, float lato, string testo, Action azione)
        {
            Pulsante p = new Pulsante();
            p.Area = new RectangleF(x, y, lato, lato);
            p.Testo = testo;
            p.Azione = azione;
            pulsanti.Add(p);
            return p;
        }

        Pulsante Collegamento(ref float x, float y, string testo, Action azione)
        {
            Pulsante p = new Pulsante();
            using (Graphics g = CreateGraphics())
            using (Font f = Carattere(famigliaTesto, 15, FontStyle.Regular))
            {
                SizeF misura = g.MeasureString(testo, f);
                p.Area = new RectangleF(x, y - misura.Height / 2, misura.Width + 6 * S, misura.Height);
                x += misura.Width + 26 * S;
            }
            p.Testo = testo;
            p.Azione = azione;
            pulsanti.Add(p);
            return p;
        }

        void Apri(string cosa)
        {
            if (string.IsNullOrEmpty(cosa))
            {
                return;
            }
            try
            {
                Process.Start(cosa);
            }
            catch (Exception e)
            {
                Registro.Scrivi("non riesco ad aprire " + cosa + ": " + e.Message);
            }
        }

        Pulsante Sotto(Point punto)
        {
            foreach (Pulsante p in pulsanti)
            {
                if (p.Visibile && p.Area.Contains(punto))
                {
                    return p;
                }
            }
            return null;
        }

        void SuMouseMove(object sender, MouseEventArgs e)
        {
            Pulsante sotto = Sotto(e.Location);
            bool cambiato = false;
            foreach (Pulsante p in pulsanti)
            {
                bool ora = p == sotto;
                if (p.Sopra != ora)
                {
                    p.Sopra = ora;
                    cambiato = true;
                }
            }
            Cursor = sotto != null && sotto.Attivo ? Cursors.Hand : Cursors.Default;
            if (cambiato)
            {
                Invalidate();
            }
        }

        void SuMouseDown(object sender, MouseEventArgs e)
        {
            if (e.Button != MouseButtons.Left)
            {
                return;
            }
            Pulsante sotto = Sotto(e.Location);
            if (sotto != null)
            {
                if (sotto.Attivo && sotto.Azione != null)
                {
                    sotto.Azione();
                }
                return;
            }
            // la parte alta della finestra la trascina
            if (e.Y < 90 * S)
            {
                ReleaseCapture();
                SendMessage(Handle, 0xA1, 0x2, 0);
            }
        }

        void SuTasto(object sender, KeyEventArgs e)
        {
            if (e.KeyCode == Keys.Enter && principale.Attivo)
            {
                Principale();
            }
            else if (e.KeyCode == Keys.Escape && stato != StatoLauncher.Lavoro)
            {
                Close();
            }
        }

        // ------------------------------------------------------------------------------------ cosa fa

        /// <summary>All'apertura (e con RIPROVA): legge la versione installata e il manifesto, poi decide cosa si può fare.</summary>
        void Controlla()
        {
            Imposta(StatoLauncher.Controllo, "Cerco la valle...", -1, false);
            versioneInstallata = aggiornatore.VersioneInstallata();
            ThreadPool.QueueUserWorkItem(delegate
            {
                Manifesto letto = null;
                string errore = "";
                try
                {
                    letto = Manifesto.DaJson(Rete.Pagina(config.Manifesto, 12000));
                    Registro.Scrivi("manifesto letto: versione " + letto.Versione + ", installata " + (versioneInstallata.Length > 0 ? versioneInstallata : "nessuna"));
                }
                catch (Exception e)
                {
                    errore = e.Message;
                    Registro.Scrivi("manifesto non letto (" + config.Manifesto + "): " + e.Message);
                }
                Sulla(delegate { DopoIlManifesto(letto, errore); });
            });
        }

        void DopoIlManifesto(Manifesto letto, string errore)
        {
            manifesto = letto;
            bool installato = versioneInstallata.Length > 0 && File.Exists(aggiornatore.Eseguibile(manifesto));
            if (manifesto == null)
            {
                if (installato)
                {
                    Imposta(StatoLauncher.SenzaRete, "Non raggiungo il sito degli aggiornamenti: puoi giocare con la versione che hai (v" + versioneInstallata + ").", -1, true);
                }
                else
                {
                    Imposta(StatoLauncher.Errore, "Non raggiungo il sito degli aggiornamenti (" + errore + "). Controlla la rete e riprova.", -1, true);
                }
            }
            else if (!installato)
            {
                string misura = manifesto.BytePacchetto > 0 ? " (" + Byte(manifesto.BytePacchetto) + ")" : "";
                Imposta(StatoLauncher.DaInstallare, "Valdorso v" + manifesto.Versione + Nome(manifesto) + " è pronto da installare" + misura + ".", -1, false);
            }
            else if (Versioni.Confronta(versioneInstallata, manifesto.Versione) < 0)
            {
                Imposta(StatoLauncher.DaAggiornare, "C'è la versione nuova: v" + manifesto.Versione + Nome(manifesto) + " (hai la v" + versioneInstallata + ").", -1, false);
            }
            else
            {
                Imposta(StatoLauncher.Pronto, "Pronto a partire · v" + versioneInstallata + Nome(manifesto), -1, false);
            }
            if (manifesto != null && manifesto.VersioneLauncher.Length > 0 && Versioni.Confronta(Info.Versione, manifesto.VersioneLauncher) < 0)
            {
                avvisoLauncher = "C'è un launcher nuovo (" + manifesto.VersioneLauncher + "): scaricalo dal sito.";
            }
            verifica.Visibile = installato && manifesto != null && manifesto.File.Count > 0;
            ChiediStato();
            Invalidate();
        }

        static string Nome(Manifesto m)
        {
            return m != null && m.NomeVersione.Length > 0 ? " «" + m.NomeVersione + "»" : "";
        }

        void Principale()
        {
            switch (stato)
            {
                case StatoLauncher.DaInstallare:
                case StatoLauncher.DaAggiornare:
                    Installa();
                    break;
                case StatoLauncher.Pronto:
                case StatoLauncher.SenzaRete:
                    Gioca();
                    break;
                case StatoLauncher.Lavoro:
                    aggiornatore.Annulla = true;
                    principale.Attivo = false;
                    rigaStato = "Mi fermo... (il pezzo già scaricato resta: si riprende da qui)";
                    Invalidate();
                    break;
                case StatoLauncher.Errore:
                    Controlla();
                    break;
            }
        }

        void Installa()
        {
            Manifesto m = manifesto;
            aggiornatore.Annulla = false;
            Imposta(StatoLauncher.Lavoro, "Comincio...", 0, false);
            ThreadPool.QueueUserWorkItem(delegate
            {
                try
                {
                    aggiornatore.Installa(m, Avanza);
                    Sulla(delegate
                    {
                        versioneInstallata = aggiornatore.VersioneInstallata();
                        verifica.Visibile = m.File.Count > 0;
                        Imposta(StatoLauncher.Pronto, "Installato! Pronto a partire · v" + versioneInstallata + Nome(m), -1, false);
                    });
                }
                catch (OperationCanceledException)
                {
                    Sulla(delegate { DopoIlManifesto(m, ""); rigaStato = "Fermato. Il pezzo già scaricato resta: premi di nuovo per riprendere."; Invalidate(); });
                }
                catch (Exception e)
                {
                    Registro.Scrivi("installazione non riuscita: " + e);
                    Sulla(delegate { Imposta(StatoLauncher.Errore, "Non ci sono riuscito: " + e.Message, -1, true); });
                }
            });
        }

        void Verifica()
        {
            if (manifesto == null || stato == StatoLauncher.Lavoro)
            {
                return;
            }
            Manifesto m = manifesto;
            StatoLauncher prima = stato;
            aggiornatore.Annulla = false;
            Imposta(StatoLauncher.Lavoro, "Controllo i file...", 0, false);
            ThreadPool.QueueUserWorkItem(delegate
            {
                List<string> sbagliati = null;
                string errore = "";
                try
                {
                    sbagliati = aggiornatore.Verifica(m, Avanza);
                }
                catch (Exception e)
                {
                    errore = e.Message;
                }
                Sulla(delegate
                {
                    if (sbagliati == null)
                    {
                        Imposta(StatoLauncher.Errore, "Controllo non riuscito: " + errore, -1, true);
                    }
                    else if (sbagliati.Count == 0)
                    {
                        Imposta(prima == StatoLauncher.Lavoro ? StatoLauncher.Pronto : prima, "Tutti i " + m.File.Count + " file sono a posto.", -1, false);
                    }
                    else
                    {
                        Registro.Scrivi("file mancanti o diversi: " + string.Join(", ", sbagliati.ToArray()));
                        Imposta(StatoLauncher.DaAggiornare, sbagliati.Count + " file mancano o sono rovinati: premi RIPARA per rimetterli (elenco nel Registro).", -1, true);
                        principale.Testo = "RIPARA";
                    }
                });
            });
        }

        void Gioca()
        {
            string exe = aggiornatore.Eseguibile(manifesto);
            if (!File.Exists(exe))
            {
                Imposta(StatoLauncher.Errore, "Non trovo il gioco (" + exe + "): reinstallalo.", -1, true);
                return;
            }
            string server = config.Server.Length > 0 ? config.Server : (manifesto != null ? manifesto.Server : "");
            string pagina = IndirizzoStato();
            List<string> argomenti = new List<string>();
            if (manifesto != null && manifesto.Argomenti.Length > 0)
            {
                argomenti.Add(manifesto.Argomenti);
            }
            if (config.ArgomentiInPiu.Length > 0)
            {
                argomenti.Add(config.ArgomentiInPiu);
            }
            if (server.Length > 0)
            {
                argomenti.Add("-ValdorsoServer=" + server);
            }
            if (pagina.Length > 0)
            {
                argomenti.Add("-ValdorsoStato=" + pagina);
            }
            argomenti.Add("-DalLauncher");
            try
            {
                ProcessStartInfo avvio = new ProcessStartInfo(exe, string.Join(" ", argomenti.ToArray()));
                avvio.WorkingDirectory = Path.GetDirectoryName(exe);
                avvio.UseShellExecute = false;
                Process.Start(avvio);
                Registro.Scrivi("gioco avviato: " + exe + " " + avvio.Arguments);
                if (config.ChiudiDopoAvvio)
                {
                    Close();
                }
                else
                {
                    WindowState = FormWindowState.Minimized;
                }
            }
            catch (Exception e)
            {
                Registro.Scrivi("gioco non avviato: " + e);
                Imposta(StatoLauncher.Errore, "Il gioco non parte: " + e.Message, -1, true);
            }
        }

        string IndirizzoStato()
        {
            if (config.Stato.Length > 0)
            {
                return config.Stato;
            }
            if (manifesto != null && manifesto.Stato.Length > 0)
            {
                return manifesto.Stato;
            }
            string server = config.Server.Length > 0 ? config.Server : (manifesto != null ? manifesto.Server : "");
            if (server.Length == 0)
            {
                return "";
            }
            int duePunti = server.LastIndexOf(':');
            string host = duePunti > 0 && !server.StartsWith("[") ? server.Substring(0, duePunti) : server;
            return "http://" + host + ":7780/stato";
        }

        void ChiediStato()
        {
            string url = IndirizzoStato();
            if (url.Length == 0 || Interlocked.CompareExchange(ref statoInViaggio, 1, 0) != 0)
            {
                return;
            }
            ThreadPool.QueueUserWorkItem(delegate
            {
                StatoMondo letto = null;
                try
                {
                    letto = StatoMondo.DaJson(Rete.Pagina(url, 5000));
                }
                catch (Exception)
                {
                }
                Interlocked.Exchange(ref statoInViaggio, 0);
                Sulla(delegate
                {
                    rigaMondo = letto != null ? letto.Riga() : "La valle dorme: il server non risponde.";
                    notiziaServer = letto != null ? letto.Notizia : "";
                    Invalidate();
                });
            });
        }

        void Avanza(string fase, long fatti, long totale, double bytePerSecondo)
        {
            Sulla(delegate
            {
                double parte = totale > 0 ? Math.Min(1.0, (double)fatti / totale) : -1;
                string testo = fase;
                if (parte >= 0)
                {
                    testo += ": " + Math.Floor(parte * 100).ToString(CultureInfo.InvariantCulture) + "%";
                }
                if (bytePerSecondo > 0 && totale > 0)
                {
                    testo += " · " + Byte(fatti) + " di " + Byte(totale) + " · " + Byte((long)bytePerSecondo) + "/s";
                    double restano = (totale - fatti) / bytePerSecondo;
                    if (restano > 1 && restano < 48 * 3600)
                    {
                        testo += " · mancano " + Durata(restano);
                    }
                }
                rigaStato = testo;
                avanzamento = parte;
                Invalidate();
            });
        }

        void Imposta(StatoLauncher nuovo, string riga, double parte, bool errore)
        {
            stato = nuovo;
            rigaStato = riga;
            avanzamento = parte;
            erroreRiga = errore;
            switch (nuovo)
            {
                case StatoLauncher.DaInstallare: principale.Testo = "INSTALLA"; principale.Attivo = true; break;
                case StatoLauncher.DaAggiornare: principale.Testo = "AGGIORNA"; principale.Attivo = true; break;
                case StatoLauncher.Pronto:
                case StatoLauncher.SenzaRete: principale.Testo = "GIOCA"; principale.Attivo = true; break;
                case StatoLauncher.Lavoro: principale.Testo = "FERMA"; principale.Attivo = true; break;
                case StatoLauncher.Errore: principale.Testo = "RIPROVA"; principale.Attivo = true; break;
                default: principale.Testo = "ATTENDI"; principale.Attivo = false; break;
            }
            Invalidate();
        }

        void Sulla(MethodInvoker azione)
        {
            if (IsDisposed)
            {
                return;
            }
            try
            {
                if (InvokeRequired)
                {
                    BeginInvoke(azione);
                }
                else
                {
                    azione();
                }
            }
            catch (ObjectDisposedException)
            {
            }
            catch (InvalidOperationException)
            {
            }
        }

        static string Byte(long n)
        {
            if (n >= 1L << 30)
            {
                return (n / (double)(1L << 30)).ToString("0.0", CultureInfo.GetCultureInfo("it-IT")) + " GB";
            }
            if (n >= 1L << 20)
            {
                return (n / (double)(1L << 20)).ToString("0.0", CultureInfo.GetCultureInfo("it-IT")) + " MB";
            }
            return Math.Max(1, n / 1024).ToString(CultureInfo.InvariantCulture) + " KB";
        }

        static string Durata(double secondi)
        {
            if (secondi < 90)
            {
                return ((int)secondi).ToString(CultureInfo.InvariantCulture) + " s";
            }
            if (secondi < 5400)
            {
                return ((int)(secondi / 60)).ToString(CultureInfo.InvariantCulture) + " min";
            }
            return (secondi / 3600).ToString("0.0", CultureInfo.GetCultureInfo("it-IT")) + " ore";
        }

        // ------------------------------------------------------------------------------------ disegno

        float Battito()
        {
            // due colpi e una pausa ogni 1,6 s, come il Cuore del gioco
            double t = (DateTime.UtcNow - inizio).TotalSeconds % 1.6;
            double a = Math.Exp(-Math.Pow((t - 0.10) / 0.07, 2));
            double b = 0.75 * Math.Exp(-Math.Pow((t - 0.42) / 0.07, 2));
            return (float)Math.Min(1.0, a + b);
        }

        protected override void OnPaint(PaintEventArgs e)
        {
            Graphics g = e.Graphics;
            g.SmoothingMode = SmoothingMode.AntiAlias;
            g.TextRenderingHint = TextRenderingHint.AntiAliasGridFit;
            g.InterpolationMode = InterpolationMode.HighQualityBicubic;
            float W = ClientSize.Width;
            float H = ClientSize.Height;

            DisegnaSfondo(g, W, H);
            DisegnaTitolo(g);
            DisegnaNotizie(g, W, H);
            DisegnaMondo(g, W);
            DisegnaBarra(g, W, H);
            foreach (Pulsante p in pulsanti)
            {
                if (p.Visibile)
                {
                    DisegnaPulsante(g, p);
                }
            }
            // il filo d'oro intorno alla finestra
            using (Pen filo = new Pen(Color.FromArgb(150, 120, 95, 45), Math.Max(1f, S)))
            {
                g.DrawRectangle(filo, 0.5f, 0.5f, W - 1.5f, H - 1.5f);
            }
        }

        void DisegnaSfondo(Graphics g, float W, float H)
        {
            if (sfondo != null)
            {
                // "copri": l'immagine riempie tutta la finestra, tagliando i bordi se serve
                float scala = Math.Max(W / sfondo.Width, H / sfondo.Height);
                float w = sfondo.Width * scala;
                float h = sfondo.Height * scala;
                g.DrawImage(sfondo, (W - w) / 2, (H - h) / 2, w, h);
            }
            // a sinistra scuro per leggere, in basso la barra
            using (LinearGradientBrush velo = new LinearGradientBrush(new RectangleF(0, 0, W * 0.62f, H), Color.FromArgb(235, Fondo), Color.FromArgb(0, Fondo), LinearGradientMode.Horizontal))
            {
                g.FillRectangle(velo, 0, 0, W * 0.62f, H);
            }
            using (LinearGradientBrush basso = new LinearGradientBrush(new RectangleF(0, H - 170 * S, W, 170 * S), Color.FromArgb(0, Fondo), Color.FromArgb(235, Fondo), LinearGradientMode.Vertical))
            {
                g.FillRectangle(basso, 0, H - 170 * S + 1, W, 170 * S);
            }
        }

        void DisegnaTitolo(Graphics g)
        {
            using (Font titolo = Carattere(famigliaTitolo, 74, FontStyle.Bold))
            using (Font motto = Carattere(famigliaCorsivo, 22, FontStyle.Italic))
            using (SolidBrush ombra = new SolidBrush(Color.FromArgb(200, 0, 0, 0)))
            {
                PointF dove = new PointF(46 * S, 34 * S);
                g.DrawString("VALDORSO", titolo, ombra, dove.X + 2 * S, dove.Y + 3 * S);
                RectangleF area = new RectangleF(dove.X, dove.Y, 620 * S, 100 * S);
                using (LinearGradientBrush oro = new LinearGradientBrush(area, OroChiaro, Color.FromArgb(160, 118, 38), LinearGradientMode.Vertical))
                {
                    g.DrawString("VALDORSO", titolo, oro, dove);
                }
                SizeF misura = g.MeasureString("VALDORSO", titolo);
                float y = dove.Y + misura.Height - 6 * S;
                using (SolidBrush sec = new SolidBrush(Secondario))
                {
                    g.DrawString("Ogni gesto lascia un segno. Scegli chi diventare.", motto, sec, dove.X + 4 * S, y);
                }
                // filo con il rombo
                float ly = y + 42 * S;
                using (Pen linea = new Pen(Color.FromArgb(140, Oro), Math.Max(1f, S)))
                {
                    g.DrawLine(linea, dove.X + 4 * S, ly, dove.X + 190 * S, ly);
                    g.DrawLine(linea, dove.X + 214 * S, ly, dove.X + 400 * S, ly);
                }
                Rombo(g, dove.X + 202 * S, ly, 5 * S, Oro);
            }
        }

        void DisegnaNotizie(Graphics g, float W, float H)
        {
            float x = 52 * S;
            float y = 210 * S;
            float larghezza = 560 * S;
            using (Font etichetta = Carattere(famigliaTitolo, 14, FontStyle.Bold))
            using (Font titoloNotizia = Carattere(famigliaTitolo, 19, FontStyle.Bold))
            using (Font data = Carattere(famigliaCorsivo, 15, FontStyle.Italic))
            using (Font testo = Carattere(famigliaTesto, 18, FontStyle.Regular))
            using (SolidBrush oro = new SolidBrush(Color.FromArgb(220, Oro)))
            using (SolidBrush perg = new SolidBrush(Pergamena))
            using (SolidBrush sec = new SolidBrush(Secondario))
            {
                g.DrawString("NOTIZIE DALLA VALLE", etichetta, oro, x, y);
                y += 34 * S;
                List<Notizia> elenco = new List<Notizia>();
                if (notiziaServer.Length > 0)
                {
                    Notizia n = new Notizia();
                    n.Titolo = "Dal server, adesso";
                    n.Testo = notiziaServer;
                    elenco.Add(n);
                }
                if (manifesto != null)
                {
                    elenco.AddRange(manifesto.Notizie);
                }
                if (elenco.Count == 0)
                {
                    g.DrawString(manifesto == null ? "Le notizie arrivano appena il launcher raggiunge il sito." : "Nessuna notizia, per ora: la valle è tranquilla.", testo, sec, new RectangleF(x, y, larghezza, 60 * S));
                    return;
                }
                float limite = H - 190 * S;
                foreach (Notizia n in elenco)
                {
                    if (y > limite - 60 * S)
                    {
                        break;
                    }
                    Rombo(g, x + 5 * S, y + 13 * S, 4 * S, Brace);
                    g.DrawString(n.Titolo, titoloNotizia, perg, x + 18 * S, y);
                    if (n.Data.Length > 0)
                    {
                        SizeF t = g.MeasureString(n.Titolo, titoloNotizia);
                        g.DrawString(n.Data, data, sec, x + 26 * S + t.Width, y + 4 * S);
                    }
                    y += 30 * S;
                    RectangleF area = new RectangleF(x + 18 * S, y, larghezza - 18 * S, Math.Max(10 * S, limite - y));
                    SizeF quanto = g.MeasureString(n.Testo, testo, (int)area.Width);
                    float alto = Math.Min(quanto.Height, area.Height);
                    using (StringFormat formato = new StringFormat())
                    {
                        formato.Trimming = StringTrimming.EllipsisWord;
                        g.DrawString(n.Testo, testo, sec, new RectangleF(area.X, area.Y, area.Width, alto), formato);
                    }
                    y += alto + 16 * S;
                }
            }
        }

        void DisegnaMondo(Graphics g, float W)
        {
            float w = 360 * S;
            float x = W - w - 46 * S;
            float y = 70 * S;
            using (Font etichetta = Carattere(famigliaTitolo, 13, FontStyle.Bold))
            using (Font testo = Carattere(famigliaTesto, 18, FontStyle.Regular))
            using (SolidBrush fondo = new SolidBrush(Color.FromArgb(185, Pannelli)))
            using (Pen bordo = new Pen(Color.FromArgb(120, Oro), Math.Max(1f, S)))
            using (SolidBrush oro = new SolidBrush(Oro))
            using (SolidBrush perg = new SolidBrush(Pergamena))
            {
                string riga = rigaMondo.Length > 0 ? rigaMondo : (IndirizzoStato().Length > 0 ? "Chiedo al server..." : "Il server della valle non è ancora indicato.");
                SizeF misura = g.MeasureString(riga, testo, (int)(w - 36 * S));
                float h = misura.Height + 58 * S;
                RectangleF area = new RectangleF(x, y, w, h);
                using (GraphicsPath forma = Arrotondato(area, 6 * S))
                {
                    g.FillPath(fondo, forma);
                    g.DrawPath(bordo, forma);
                }
                g.DrawString("NELLA VALLE", etichetta, oro, x + 18 * S, y + 14 * S);
                g.DrawString(riga, testo, perg, new RectangleF(x + 18 * S, y + 40 * S, w - 36 * S, misura.Height + 4 * S));
                if (avvisoLauncher.Length > 0)
                {
                    using (Font piccolo = Carattere(famigliaCorsivo, 15, FontStyle.Italic))
                    using (SolidBrush brace = new SolidBrush(Brace))
                    {
                        g.DrawString(avvisoLauncher, piccolo, brace, new RectangleF(x, y + h + 8 * S, w, 40 * S));
                    }
                }
            }
        }

        void DisegnaBarra(Graphics g, float W, float H)
        {
            float x = 52 * S;
            float y = H - 98 * S;
            float w = W - 400 * S;
            using (Font testo = Carattere(famigliaTesto, 18, FontStyle.Regular))
            using (SolidBrush colore = new SolidBrush(erroreRiga ? Rosso : Pergamena))
            {
                g.DrawString(rigaStato, testo, colore, new RectangleF(x, y, w, 50 * S));
            }
            // la barra d'avanzamento: un filo scuro che si riempie d'oro, con la punta di brace che batte
            float by = H - 52 * S;
            using (SolidBrush vuota = new SolidBrush(Color.FromArgb(160, 40, 32, 24)))
            {
                g.FillRectangle(vuota, x, by, w, 6 * S);
            }
            if (avanzamento >= 0)
            {
                float pieno = (float)(w * avanzamento);
                if (pieno > 1)
                {
                    using (LinearGradientBrush oro = new LinearGradientBrush(new RectangleF(x, by, pieno + 1, 6 * S), Color.FromArgb(150, 110, 35), OroChiaro, LinearGradientMode.Horizontal))
                    {
                        g.FillRectangle(oro, x, by, pieno, 6 * S);
                    }
                }
                float b = Battito();
                using (SolidBrush alone = new SolidBrush(Color.FromArgb((int)(60 + 120 * b), Brace)))
                {
                    float r = (5 + 4 * b) * S;
                    g.FillEllipse(alone, x + pieno - r, by + 3 * S - r, 2 * r, 2 * r);
                }
            }
        }

        void DisegnaPulsante(Graphics g, Pulsante p)
        {
            if (p.Grande)
            {
                float b = p.Attivo ? Battito() : 0f;
                using (GraphicsPath forma = Arrotondato(p.Area, 4 * S))
                {
                    Color sopra = p.Attivo ? (p.Sopra ? Color.FromArgb(120, 82, 34) : Color.FromArgb(92, 60, 26)) : Color.FromArgb(50, 42, 34);
                    Color sotto = p.Attivo ? (p.Sopra ? Color.FromArgb(70, 44, 20) : Color.FromArgb(52, 34, 16)) : Color.FromArgb(34, 28, 22);
                    using (LinearGradientBrush fondo = new LinearGradientBrush(p.Area, sopra, sotto, LinearGradientMode.Vertical))
                    {
                        g.FillPath(fondo, forma);
                    }
                    using (Pen bordo = new Pen(p.Attivo ? Color.FromArgb((int)(170 + 85 * b), Oro) : Color.FromArgb(90, Oro), 1.5f * S))
                    {
                        g.DrawPath(bordo, forma);
                    }
                }
                using (Font f = Carattere(famigliaTitolo, 26, FontStyle.Bold))
                using (SolidBrush scritta = new SolidBrush(p.Attivo ? (p.Sopra ? OroChiaro : Oro) : Color.FromArgb(120, Secondario)))
                using (StringFormat centro = new StringFormat())
                {
                    centro.Alignment = StringAlignment.Center;
                    centro.LineAlignment = StringAlignment.Center;
                    g.DrawString(p.Testo, f, scritta, p.Area, centro);
                }
                return;
            }
            bool piccolo = p.Area.Width == p.Area.Height;
            using (Font f = piccolo ? Carattere(famigliaTesto, 20, FontStyle.Regular) : Carattere(famigliaTesto, 15, FontStyle.Regular))
            using (SolidBrush scritta = new SolidBrush(p.Sopra ? OroChiaro : Color.FromArgb(piccolo ? 230 : 200, Secondario)))
            using (StringFormat centro = new StringFormat())
            {
                centro.Alignment = StringAlignment.Center;
                centro.LineAlignment = StringAlignment.Center;
                if (piccolo && p.Sopra)
                {
                    using (SolidBrush alone = new SolidBrush(Color.FromArgb(90, 58, 44, 31)))
                    {
                        g.FillRectangle(alone, p.Area);
                    }
                }
                g.DrawString(p.Testo, f, scritta, p.Area, centro);
                if (!piccolo && p.Sopra)
                {
                    using (Pen sotto = new Pen(Color.FromArgb(160, Oro), Math.Max(1f, S)))
                    {
                        g.DrawLine(sotto, p.Area.Left + 3 * S, p.Area.Bottom - 2 * S, p.Area.Right - 3 * S, p.Area.Bottom - 2 * S);
                    }
                }
            }
        }

        static void Rombo(Graphics g, float cx, float cy, float r, Color colore)
        {
            using (SolidBrush pennello = new SolidBrush(colore))
            {
                g.FillPolygon(pennello, new PointF[] { new PointF(cx, cy - r), new PointF(cx + r, cy), new PointF(cx, cy + r), new PointF(cx - r, cy) });
            }
        }

        static GraphicsPath Arrotondato(RectangleF r, float raggio)
        {
            GraphicsPath p = new GraphicsPath();
            float d = raggio * 2;
            p.AddArc(r.X, r.Y, d, d, 180, 90);
            p.AddArc(r.Right - d, r.Y, d, d, 270, 90);
            p.AddArc(r.Right - d, r.Bottom - d, d, d, 0, 90);
            p.AddArc(r.X, r.Bottom - d, d, d, 90, 90);
            p.CloseFigure();
            return p;
        }

        protected override void Dispose(bool disposing)
        {
            if (disposing)
            {
                battito.Dispose();
                orologioStato.Dispose();
                if (sfondo != null)
                {
                    sfondo.Dispose();
                }
                caratteri.Dispose();
            }
            base.Dispose(disposing);
        }
    }

    public static class Programma
    {
        [DllImport("user32.dll")]
        static extern bool SetProcessDPIAware();

        [STAThread]
        public static void Main()
        {
            bool primo;
            using (Mutex unoSolo = new Mutex(true, "ValdorsoLauncher_unico", out primo))
            {
                if (!primo)
                {
                    MessageBox.Show("Il launcher di Valdorso è già aperto.", "Valdorso", MessageBoxButtons.OK, MessageBoxIcon.Information);
                    return;
                }
                try
                {
                    if (Environment.OSVersion.Version.Major >= 6)
                    {
                        SetProcessDPIAware();
                    }
                }
                catch (Exception)
                {
                }
                Registro.Scrivi("--- launcher " + Info.Versione + " aperto");
                Application.EnableVisualStyles();
                Application.SetCompatibleTextRenderingDefault(false);
                Application.ThreadException += delegate(object s, ThreadExceptionEventArgs e) { Registro.Scrivi("errore: " + e.Exception); };
                Application.Run(new Finestra());
            }
        }
    }
}
