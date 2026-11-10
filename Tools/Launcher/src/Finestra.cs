// Valdorso - Il launcher: la finestra "Oro e brace". Scritto da Claude il 07/11/2026; (08/11) le cinque schede, i
// contenuti dal sito, la pausa, lo spazio sul disco, le opzioni salvate dalla finestra.
//
// Una finestra senza bordi di Windows (si trascina dalla parte alta), con lo sfondo della valle al tramonto, il titolo
// e le schede (La valle, Note delle patch, Roadmap, Cronache, Opzioni), e in basso la barra con l'avanzamento e il
// pulsante grande: INSTALLA, AGGIORNA, PAUSA, RIPRENDI o GIOCA. Tutto è disegnato qui (niente controlli di Windows,
// tranne la casella dell'indirizzo del server nelle Opzioni), con i colori del gioco (ValdorsoTemaUI.h).
// Le schede sono in Pagine.cs. Per non ridisegnare tutto 20 volte al secondo, la pagina si disegna una volta in
// un'immagine (paginaPronta) e si rifà solo quando cambia qualcosa; sopra si disegnano le cose che si muovono.

using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.Drawing.Imaging;
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
        InPausa,
        Errore,
        SenzaRete
    }

    public enum Scheda
    {
        Valle,
        Patch,
        Roadmap,
        Cronache,
        Opzioni
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
        /// <summary>Per le schede: quale (-1 per gli altri pulsanti).</summary>
        public int Scheda = -1;
        /// <summary>"chiudi", "riduci" o "" (i due in alto a destra si disegnano a linee).</summary>
        public string Segno = "";
    }

    /// <summary>Una parte cliccabile dentro la pagina (un collegamento, una versione, un interruttore).</summary>
    public class Zona
    {
        public RectangleF Area;
        public string Chiave = "";
        public Action Azione;
    }

    public partial class Finestra : Form
    {
        // i colori del tema "Oro e brace" (ValdorsoTemaUI.h)
        static readonly Color Fondo = Color.FromArgb(14, 11, 9);
        static readonly Color Pannelli = Color.FromArgb(21, 17, 13);
        static readonly Color PulsanteSopra = Color.FromArgb(58, 44, 31);
        static readonly Color Oro = Color.FromArgb(212, 175, 55);
        static readonly Color OroChiaro = Color.FromArgb(243, 220, 140);
        static readonly Color Pergamena = Color.FromArgb(232, 217, 181);
        static readonly Color Secondario = Color.FromArgb(201, 185, 154);
        static readonly Color Brace = Color.FromArgb(217, 119, 43);
        static readonly Color Rosso = Color.FromArgb(200, 80, 62);
        static readonly Color Verde = Color.FromArgb(120, 160, 80);

        // le misure della finestra (in punti a 96 dpi; si moltiplicano per S)
        const float Larghezza = 1180;
        const float Altezza = 720;
        const float Margine = 40;
        const float SottoTitolo = 100;   // la riga d'oro sotto il titolo e le schede
        const float AltezzaBarra = 150;  // la barra in basso

        [DllImport("user32.dll")]
        static extern bool ReleaseCapture();

        [DllImport("user32.dll")]
        static extern IntPtr SendMessage(IntPtr hWnd, int msg, int wParam, int lParam);

        readonly Configurazione config;
        readonly Aggiornatore aggiornatore;
        PrivateFontCollection caratteri = new PrivateFontCollection();
        readonly float S;
        Image sfondo;
        FontFamily famigliaTitolo;
        FontFamily famigliaTesto;
        FontFamily famigliaCorsivo;
        readonly Dictionary<string, Font> fontPronti = new Dictionary<string, Font>();
        readonly List<Pulsante> pulsanti = new List<Pulsante>();
        Pulsante principale;
        readonly System.Windows.Forms.Timer battito = new System.Windows.Forms.Timer();
        readonly System.Windows.Forms.Timer orologioStato = new System.Windows.Forms.Timer();
        readonly System.Windows.Forms.Timer orologio = new System.Windows.Forms.Timer();
        readonly DateTime inizio = DateTime.UtcNow;

        // le schede e la pagina disegnata
        Scheda scheda = Scheda.Valle;
        readonly float[] scorrimento = new float[5];
        readonly float[] altezzaPagina = new float[5];
        RectangleF areaScorre = RectangleF.Empty;
        Bitmap sfondoPronto;
        Scheda sfondoProntoPer = (Scheda)(-1);
        Bitmap paginaPronta;
        bool paginaSporca = true;
        List<Zona> zone = new List<Zona>();
        readonly List<RectangleF> animate = new List<RectangleF>();
        string zonaSopra = "";
        TextBox casellaServer;

        // cosa sa il launcher
        StatoLauncher stato = StatoLauncher.Avvio;
        Fase faseLavoro = Fase.Scarico;
        Manifesto manifesto;
        Contenuti contenuti = new Contenuti();
        readonly Dictionary<string, Image> immagini = new Dictionary<string, Image>();
        string versioneInstallata = "";
        string rigaStato = "Cerco la valle...";
        string rigaDettaglio = "";
        double avanzamento = -1;
        bool erroreRiga;
        bool ripara;
        int statoInViaggio;
        int contenutiInViaggio;
        StatoMondo mondo;
        DateTime mondoLetto = DateTime.MinValue;
        bool mondoChiesto;
        string ultimoMinuto = "";
        int voce;
        int versioneScelta;
        bool roadmapVista;

        public Finestra(string schedaIniziale)
        {
            config = Configurazione.Carica();
            aggiornatore = new Aggiornatore(config);

            float dpi;
            using (Graphics g = CreateGraphics())
            {
                dpi = Math.Max(1f, g.DpiX / 96f);
            }
            // la finestra deve stare nello schermo anche sui portatili piccoli
            Rectangle schermo = Screen.PrimaryScreen.WorkingArea;
            float entra = Math.Min((schermo.Width - 16) / Larghezza, (schermo.Height - 16) / Altezza);
            S = Math.Max(0.6f, Math.Min(dpi, entra));

            Text = "Valdorso";
            FormBorderStyle = FormBorderStyle.None;
            StartPosition = FormStartPosition.CenterScreen;
            ClientSize = new Size((int)(Larghezza * S), (int)(Altezza * S));
            BackColor = Fondo;
            DoubleBuffered = true;
            SetStyle(ControlStyles.AllPaintingInWmPaint | ControlStyles.UserPaint | ControlStyles.OptimizedDoubleBuffer | ControlStyles.ResizeRedraw, true);
            KeyPreview = true;

            CaricaRisorse();
            CreaPulsanti();
            CreaCaselle();
            contenuti = Archivio.CaricaLocali();
            for (int i = 0; i < 5; i++)
            {
                if (string.Equals(schedaIniziale, ((Scheda)i).ToString(), StringComparison.OrdinalIgnoreCase))
                {
                    scheda = (Scheda)i;
                }
            }
            MostraCaselle();

            battito.Interval = 50;
            battito.Tick += delegate { Anima(); };
            battito.Start();
            orologioStato.Interval = 30000;
            orologioStato.Tick += delegate { ChiediStato(); };
            orologioStato.Start();
            orologio.Interval = 1000;
            orologio.Tick += delegate { OgniSecondo(); };
            orologio.Start();

            MouseMove += SuMouseMove;
            MouseDown += SuMouseDown;
            MouseWheel += SuRotella;
            MouseLeave += delegate
            {
                foreach (Pulsante p in pulsanti)
                {
                    p.Sopra = false;
                }
                CambiaZonaSopra("");
                Cursor = Cursors.Default;
                Invalidate();
            };
            KeyDown += SuTasto;
            Shown += delegate
            {
                Controlla();
                AggiornaContenuti();
            };
            FormClosing += SuChiusura;
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
            sfondo = CaricaImmagine(Path.Combine(Cartelle.Risorse, "sfondo.jpg")) ?? CaricaImmagine(Path.Combine(Cartelle.Risorse, "sfondo.png"));
            CaricaCaratteri();
        }

        /// <summary>Un'immagine dal disco, copiata in memoria (così il file non resta bloccato). Null se non va.</summary>
        static Image CaricaImmagine(string percorso)
        {
            try
            {
                if (percorso != null && File.Exists(percorso))
                {
                    using (Image letta = Image.FromFile(percorso))
                    {
                        return new Bitmap(letta);
                    }
                }
            }
            catch (Exception e)
            {
                Registro.Scrivi("immagine non caricata (" + percorso + "): " + e.Message);
            }
            return null;
        }

        /// <summary>Cinzel ed EB Garamond dalla cartella risorse (o Georgia, se mancano o se lo chiede l'opzione).</summary>
        void CaricaCaratteri()
        {
            foreach (Font f in fontPronti.Values)
            {
                f.Dispose();
            }
            fontPronti.Clear();
            altezzeRiga.Clear();
            caratteri.Dispose();
            caratteri = new PrivateFontCollection();
            if (config.CaratteriPropri)
            {
                foreach (string file in new string[] { "Cinzel.ttf", "EBGaramond.ttf", "EBGaramond-Italic.ttf" })
                {
                    try
                    {
                        string percorso = Path.Combine(Cartelle.Risorse, file);
                        if (File.Exists(percorso))
                        {
                            caratteri.AddFontFile(percorso);
                        }
                    }
                    catch (Exception e)
                    {
                        Registro.Scrivi("carattere " + file + " non caricato: " + e.Message);
                    }
                }
            }
            famigliaTitolo = Famiglia("Cinzel");
            famigliaTesto = Famiglia("EB Garamond");
            // il corsivo è lo stesso EB Garamond: il file Italic aggiunge lo stile alla famiglia
            famigliaCorsivo = famigliaTesto;
        }

        FontFamily Famiglia(string nome)
        {
            foreach (FontFamily f in caratteri.Families)
            {
                if (f.Name.StartsWith(nome, StringComparison.OrdinalIgnoreCase))
                {
                    return f;
                }
            }
            try
            {
                return new FontFamily("Georgia");
            }
            catch (Exception)
            {
                return FontFamily.GenericSerif;
            }
        }

        /// <summary>Un carattere della misura chiesta (in punti a 96 dpi). Si tengono da parte: non si buttano.</summary>
        Font Carattere(FontFamily f, float punti, FontStyle stile)
        {
            string chiave = f.Name + "|" + punti.ToString(CultureInfo.InvariantCulture) + "|" + (int)stile + "|" + (f == famigliaCorsivo ? "c" : "");
            Font pronto;
            if (fontPronti.TryGetValue(chiave, out pronto))
            {
                return pronto;
            }
            FontStyle vero = stile;
            if (!f.IsStyleAvailable(vero))
            {
                vero = FontStyle.Regular;
                if (!f.IsStyleAvailable(vero))
                {
                    vero = f.IsStyleAvailable(FontStyle.Italic) ? FontStyle.Italic : FontStyle.Bold;
                }
            }
            pronto = new Font(f, punti * S, vero, GraphicsUnit.Pixel);
            fontPronti[chiave] = pronto;
            return pronto;
        }

        Font Titolo(float punti)
        {
            return Carattere(famigliaTitolo, punti, FontStyle.Bold);
        }

        Font Testo(float punti)
        {
            return Carattere(famigliaTesto, punti, FontStyle.Regular);
        }

        Font Corsivo(float punti)
        {
            return Carattere(famigliaCorsivo, punti, FontStyle.Italic);
        }

        Image ImmagineContenuto(string nome)
        {
            if (string.IsNullOrEmpty(nome))
            {
                return null;
            }
            Image img;
            if (immagini.TryGetValue(nome, out img))
            {
                return img;
            }
            string percorso = Archivio.Immagine(nome);
            if (percorso == null)
            {
                return null;
            }
            img = CaricaImmagine(percorso);
            if (img != null)
            {
                immagini[nome] = img;
            }
            return img;
        }

        // ------------------------------------------------------------------------------------ pulsanti

        void CreaPulsanti()
        {
            pulsanti.Clear();
            float W = ClientSize.Width;
            float H = ClientSize.Height;

            principale = new Pulsante();
            principale.Grande = true;
            principale.Area = new RectangleF(W - 300 * S, H - 124 * S, 260 * S, 74 * S);
            principale.Azione = Principale;
            pulsanti.Add(principale);

            // in alto a destra: chiudi e riduci
            Pulsante chiudi = Piccolo(W - 46 * S, 10 * S, 34 * S, "chiudi", delegate { Close(); });
            chiudi.Suggerimento = "Chiudi";
            Pulsante riduci = Piccolo(W - 84 * S, 10 * S, 34 * S, "riduci", delegate { WindowState = FormWindowState.Minimized; });
            riduci.Suggerimento = "Riduci a icona";

            // le schede, da destra a sinistra
            string[] nomi = { "LA VALLE", "NOTE DELLE PATCH", "ROADMAP", "CRONACHE", "OPZIONI" };
            float destra = W - 112 * S;
            using (Graphics g = CreateGraphics())
            {
                Font f = Titolo(13.5f);
                for (int i = nomi.Length - 1; i >= 0; i--)
                {
                    SizeF m = g.MeasureString(nomi[i], f);
                    Pulsante p = new Pulsante();
                    p.Testo = nomi[i];
                    p.Scheda = i;
                    p.Area = new RectangleF(destra - m.Width, 34 * S, m.Width, 40 * S);
                    int quale = i;
                    p.Azione = delegate { CambiaScheda((Scheda)quale); };
                    pulsanti.Add(p);
                    destra -= m.Width + 30 * S;
                }
            }

            // in basso a sinistra: i collegamenti
            float x = Margine * S;
            float y = H - 26 * S;
            if (config.Sito.Length > 0)
            {
                Collegamento(ref x, y, "Il sito", delegate { Apri(config.Sito); });
            }
            if (config.Discord.Length > 0)
            {
                Collegamento(ref x, y, "Discord", delegate { Apri(config.Discord); });
            }
            Collegamento(ref x, y, "Cartella del gioco", ApriCartellaGioco);
            Collegamento(ref x, y, "Registro", delegate { Apri(Registro.File); });
        }

        Pulsante Piccolo(float x, float y, float lato, string segno, Action azione)
        {
            Pulsante p = new Pulsante();
            p.Area = new RectangleF(x, y, lato, lato);
            p.Segno = segno;
            p.Azione = azione;
            pulsanti.Add(p);
            return p;
        }

        Pulsante Collegamento(ref float x, float y, string testo, Action azione)
        {
            Pulsante p = new Pulsante();
            using (Graphics g = CreateGraphics())
            {
                SizeF misura = g.MeasureString(testo, Testo(15));
                p.Area = new RectangleF(x, y - misura.Height / 2, misura.Width + 6 * S, misura.Height);
                x += misura.Width + 24 * S;
            }
            p.Testo = testo;
            p.Azione = azione;
            pulsanti.Add(p);
            return p;
        }

        /// <summary>La casella dell'indirizzo del server (Opzioni): l'unico controllo vero di Windows.</summary>
        void CreaCaselle()
        {
            casellaServer = new TextBox();
            casellaServer.BorderStyle = BorderStyle.None;
            casellaServer.BackColor = Color.FromArgb(30, 24, 19);
            casellaServer.ForeColor = Pergamena;
            try
            {
                casellaServer.Font = new Font("Georgia", 15 * S, FontStyle.Regular, GraphicsUnit.Pixel);
            }
            catch (Exception)
            {
            }
            casellaServer.Text = config.Server;
            casellaServer.Visible = false;
            casellaServer.Leave += delegate { SalvaServer(); };
            casellaServer.KeyDown += delegate(object s, KeyEventArgs e)
            {
                if (e.KeyCode == Keys.Enter)
                {
                    e.SuppressKeyPress = true;
                    SalvaServer();
                    ActiveControl = null;
                    Focus();
                }
            };
            Controls.Add(casellaServer);
        }

        void MostraCaselle()
        {
            if (casellaServer != null)
            {
                casellaServer.Visible = scheda == Scheda.Opzioni;
                if (!casellaServer.Visible && ContainsFocus && casellaServer.Focused)
                {
                    ActiveControl = null;
                }
            }
        }

        void SalvaServer()
        {
            string nuovo = casellaServer.Text.Trim();
            if (nuovo == config.Server)
            {
                return;
            }
            config.Server = nuovo;
            SalvaOpzioni();
            mondo = null;
            mondoChiesto = false;
            ChiediStato();
        }

        void SalvaOpzioni()
        {
            if (!config.Salva())
            {
                rigaDettaglio = "Le opzioni valgono fino alla chiusura: non riesco a salvarle (vedi il Registro).";
            }
            Ridisegna();
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

        void ApriCartellaGioco()
        {
            try
            {
                Directory.CreateDirectory(config.PercorsoGioco);
            }
            catch (Exception)
            {
            }
            Apri(config.PercorsoGioco);
        }

        void CambiaScheda(Scheda nuova)
        {
            if (scheda == nuova)
            {
                // un secondo clic sulla scheda torna in cima
                scorrimento[(int)nuova] = 0;
            }
            scheda = nuova;
            if (nuova == Scheda.Roadmap && !roadmapVista)
            {
                // la prima volta la roadmap si apre sull'ultima tappa fatta (le prime si leggono scorrendo in su)
                roadmapVista = true;
                ancoraRoadmap = true;
            }
            MostraCaselle();
            Ridisegna();
        }

        /// <summary>La pagina va ridisegnata (è cambiato qualcosa).</summary>
        void Ridisegna()
        {
            paginaSporca = true;
            Invalidate();
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

        Zona ZonaSotto(Point punto)
        {
            for (int i = zone.Count - 1; i >= 0; i--)
            {
                if (zone[i].Area.Contains(punto))
                {
                    return zone[i];
                }
            }
            return null;
        }

        void CambiaZonaSopra(string chiave)
        {
            if (chiave != zonaSopra)
            {
                zonaSopra = chiave;
                Ridisegna();
            }
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
            Zona z = sotto == null ? ZonaSotto(e.Location) : null;
            CambiaZonaSopra(z != null ? z.Chiave : "");
            Cursor = (sotto != null && sotto.Attivo && sotto.Azione != null) || z != null ? Cursors.Hand : Cursors.Default;
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
            if (casellaServer.Focused)
            {
                // un clic fuori dalla casella la salva
                ActiveControl = null;
                SalvaServer();
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
            Zona z = ZonaSotto(e.Location);
            if (z != null)
            {
                if (z.Azione != null)
                {
                    z.Azione();
                }
                return;
            }
            // la parte alta della finestra la trascina
            if (e.Y < SottoTitolo * S)
            {
                ReleaseCapture();
                SendMessage(Handle, 0xA1, 0x2, 0);
            }
        }

        void SuRotella(object sender, MouseEventArgs e)
        {
            Scorri(-e.Delta / 120f * 70 * S);
        }

        void Scorri(float quanto)
        {
            int k = (int)scheda;
            float massimo = Math.Max(0, altezzaPagina[k] - areaScorre.Height);
            float nuovo = Math.Max(0, Math.Min(massimo, scorrimento[k] + quanto));
            if (Math.Abs(nuovo - scorrimento[k]) > 0.5f)
            {
                scorrimento[k] = nuovo;
                Ridisegna();
            }
        }

        void SuTasto(object sender, KeyEventArgs e)
        {
            if (casellaServer.Focused)
            {
                return;
            }
            if (e.KeyCode == Keys.Enter && principale.Attivo)
            {
                Principale();
            }
            else if (e.KeyCode == Keys.Escape && stato != StatoLauncher.Lavoro)
            {
                Close();
            }
            else if (e.KeyCode == Keys.PageDown || e.KeyCode == Keys.Down)
            {
                Scorri(e.KeyCode == Keys.Down ? 50 * S : areaScorre.Height * 0.85f);
            }
            else if (e.KeyCode == Keys.PageUp || e.KeyCode == Keys.Up)
            {
                Scorri(e.KeyCode == Keys.Up ? -50 * S : -areaScorre.Height * 0.85f);
            }
            else if (e.Control && e.KeyCode >= Keys.D1 && e.KeyCode <= Keys.D5)
            {
                CambiaScheda((Scheda)(e.KeyCode - Keys.D1));
            }
        }

        void SuChiusura(object sender, FormClosingEventArgs e)
        {
            if (aggiornatore.Scambio)
            {
                // a metà dello scambio delle cartelle non ci si ferma: è questione di un attimo
                e.Cancel = true;
                rigaDettaglio = "Un attimo: sto mettendo al suo posto la versione nuova.";
                Invalidate();
                return;
            }
            if (casellaServer.Focused)
            {
                SalvaServer();
            }
            aggiornatore.Ferma();
        }

        // ------------------------------------------------------------------------------------ cosa fa

        /// <summary>All'apertura (e con RIPROVA): legge la versione installata e il manifesto, poi decide cosa si può fare.</summary>
        void Controlla()
        {
            Imposta(StatoLauncher.Controllo, "Cerco la valle...", -1, false);
            aggiornatore.Ripristina();
            versioneInstallata = aggiornatore.VersioneInstallata();
            ThreadPool.QueueUserWorkItem(delegate
            {
                Manifesto letto = null;
                string errore = "";
                try
                {
                    letto = Manifesto.DaJson(Rete.Pagina(config.Manifesto, 12000));
                    Registro.Scrivi("manifesto letto: versione " + letto.Versione + ", " + letto.Parti.Count + " parti, installata " + (versioneInstallata.Length > 0 ? versioneInstallata : "nessuna"));
                }
                catch (Exception e)
                {
                    errore = e.Message;
                    Registro.Scrivi("manifesto non letto (" + config.Manifesto + "): " + e.Message);
                }
                Sulla(delegate { DopoIlManifesto(letto, errore); });
            });
        }

        bool Installato
        {
            get { return versioneInstallata.Length > 0 && File.Exists(aggiornatore.Eseguibile(manifesto)); }
        }

        void DopoIlManifesto(Manifesto letto, string errore)
        {
            if (letto != null)
            {
                manifesto = letto;
            }
            ripara = false;
            if (manifesto == null)
            {
                if (Installato)
                {
                    Imposta(StatoLauncher.SenzaRete, "Non raggiungo il sito degli aggiornamenti: puoi giocare con la versione che hai (v" + versioneInstallata + ").", -1, false);
                }
                else
                {
                    Imposta(StatoLauncher.Errore, "Non raggiungo il sito degli aggiornamenti" + (errore.Length > 0 ? " (" + errore + ")" : "") + ". Controlla la rete e riprova.", -1, true);
                }
            }
            else if (!Installato)
            {
                long resta = aggiornatore.DaScaricare(manifesto);
                string misura = manifesto.BytePacchetto > 0 ? " Da scaricare: " + Misure.Byte(resta) + (resta < manifesto.BytePacchetto ? " (il resto c'è già)" : "") + "." : "";
                Imposta(StatoLauncher.DaInstallare, "Valdorso v" + manifesto.Versione + Nome(manifesto) + " è pronto da installare." + misura, -1, false);
            }
            else if (Versioni.Confronta(versioneInstallata, manifesto.Versione) < 0)
            {
                Imposta(StatoLauncher.DaAggiornare, "C'è la versione nuova: v" + manifesto.Versione + Nome(manifesto) + " (hai la v" + versioneInstallata + "). Le note sono nella scheda Note delle patch.", -1, false);
            }
            else
            {
                Imposta(StatoLauncher.Pronto, "Pronto a partire · v" + versioneInstallata + Nome(manifesto), -1, false);
            }
            ChiediStato();
            Ridisegna();
        }

        static string Nome(Manifesto m)
        {
            return m != null && m.NomeVersione.Length > 0 ? " «" + m.NomeVersione + "»" : "";
        }

        /// <summary>C'è un launcher più nuovo di questo sul sito?</summary>
        bool LauncherNuovo
        {
            get { return manifesto != null && manifesto.VersioneLauncher.Length > 0 && Versioni.Confronta(Info.Versione, manifesto.VersioneLauncher) < 0; }
        }

        void ScaricaLauncher()
        {
            if (manifesto != null && manifesto.UrlLauncher.Length > 0)
            {
                Apri(manifesto.UrlLauncher);
            }
            else
            {
                Apri(config.Sito);
            }
        }

        void Principale()
        {
            switch (stato)
            {
                case StatoLauncher.DaInstallare:
                case StatoLauncher.DaAggiornare:
                case StatoLauncher.InPausa:
                    Installa();
                    break;
                case StatoLauncher.Pronto:
                case StatoLauncher.SenzaRete:
                    if (ValleAperta())
                    {
                        Gioca();
                    }
                    else
                    {
                        ChiediStato();
                    }
                    break;
                case StatoLauncher.Lavoro:
                    aggiornatore.Ferma();
                    principale.Attivo = false;
                    rigaStato = faseLavoro == Fase.Scarico ? "Metto in pausa... (quello che è già scaricato resta)" : "Mi fermo...";
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
            if (m == null)
            {
                Controlla();
                return;
            }
            string spazio = aggiornatore.ControllaSpazio(m);
            if (spazio.Length > 0)
            {
                Imposta(StatoLauncher.Errore, spazio, -1, true);
                return;
            }
            aggiornatore.Annulla = false;
            faseLavoro = Fase.Scarico;
            Imposta(StatoLauncher.Lavoro, "Comincio...", avanzamento >= 0 ? avanzamento : 0, false);
            rigaDettaglio = "";
            ThreadPool.QueueUserWorkItem(delegate
            {
                try
                {
                    aggiornatore.Installa(m, Avanza);
                    Sulla(delegate
                    {
                        versioneInstallata = aggiornatore.VersioneInstallata();
                        ripara = false;
                        rigaDettaglio = "";
                        Imposta(StatoLauncher.Pronto, "Installato! Pronto a partire · v" + versioneInstallata + Nome(m), -1, false);
                    });
                }
                catch (OperationCanceledException)
                {
                    Sulla(delegate
                    {
                        if (faseLavoro == Fase.Scarico)
                        {
                            stato = StatoLauncher.InPausa;
                            rigaStato = "In pausa. Premi RIPRENDI per continuare da dove eri (anche dopo aver chiuso il launcher).";
                            erroreRiga = false;
                            principale.Attivo = true;
                            Ridisegna();
                        }
                        else
                        {
                            double prima = avanzamento;
                            DopoIlManifesto(m, "");
                            rigaStato = "Fermato. Premi di nuovo per riprendere: le parti già scaricate restano.";
                            avanzamento = prima >= 0 && faseLavoro == Fase.Controllo ? prima : -1;
                            rigaDettaglio = "";
                            Ridisegna();
                        }
                    });
                }
                catch (SpazioInsufficiente e)
                {
                    Sulla(delegate { Imposta(StatoLauncher.Errore, e.Message, -1, true); rigaDettaglio = ""; });
                }
                catch (Exception e)
                {
                    Registro.Scrivi("installazione non riuscita: " + e);
                    Sulla(delegate { Imposta(StatoLauncher.Errore, "Non ci sono riuscito: " + e.Message, -1, true); rigaDettaglio = "Premi RIPROVA: riprende da dove si era fermato. I dettagli sono nel Registro."; });
                }
            });
        }

        void Verifica()
        {
            if (manifesto == null || stato == StatoLauncher.Lavoro || !Installato)
            {
                return;
            }
            if (manifesto.File.Count == 0)
            {
                rigaDettaglio = "Il sito non ha l'elenco dei file di questa versione: non posso controllarli.";
                Ridisegna();
                return;
            }
            if (Versioni.Confronta(versioneInstallata, manifesto.Versione) != 0)
            {
                rigaDettaglio = "Prima aggiorna: l'elenco dei file sul sito è quello della v" + manifesto.Versione + ".";
                Ridisegna();
                return;
            }
            Manifesto m = manifesto;
            aggiornatore.Annulla = false;
            faseLavoro = Fase.Verifica;
            Imposta(StatoLauncher.Lavoro, "Controllo i file...", 0, false);
            rigaDettaglio = "";
            ThreadPool.QueueUserWorkItem(delegate
            {
                List<string> sbagliati = null;
                string errore = "";
                bool fermato = false;
                try
                {
                    sbagliati = aggiornatore.Verifica(m, Avanza);
                }
                catch (OperationCanceledException)
                {
                    fermato = true;
                }
                catch (Exception e)
                {
                    errore = e.Message;
                }
                Sulla(delegate
                {
                    rigaDettaglio = "";
                    if (fermato)
                    {
                        DopoIlManifesto(m, "");
                        rigaStato = "Controllo fermato.";
                    }
                    else if (sbagliati == null)
                    {
                        Imposta(StatoLauncher.Errore, "Controllo non riuscito: " + errore, -1, true);
                    }
                    else if (sbagliati.Count == 0)
                    {
                        DopoIlManifesto(m, "");
                        rigaStato = "Tutti i " + m.File.Count + " file sono a posto. Pronto a partire · v" + versioneInstallata + Nome(m);
                    }
                    else
                    {
                        Registro.Scrivi("file mancanti o diversi: " + string.Join(", ", sbagliati.ToArray()));
                        Imposta(StatoLauncher.DaAggiornare, sbagliati.Count + (sbagliati.Count == 1 ? " file manca o è rovinato" : " file mancano o sono rovinati") + ": premi RIPARA per rimetterli (l'elenco è nel Registro).", -1, true);
                        ripara = true;
                    }
                    Ridisegna();
                });
            });
        }

        void Gioca()
        {
            string exe = aggiornatore.Eseguibile(manifesto);
            if (!File.Exists(exe))
            {
                Imposta(StatoLauncher.Errore, "Non trovo il gioco (" + exe + "): premi RIPROVA e installalo di nuovo.", -1, true);
                return;
            }
            string server = IndirizzoServer();
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

        string IndirizzoServer()
        {
            return config.Server.Length > 0 ? config.Server : (manifesto != null ? manifesto.Server : "");
        }

        string IndirizzoStato()
        {
            if (config.Stato.Length > 0)
            {
                return config.Stato;
            }
            // se l'indirizzo del server l'ha scritto il giocatore, la pagina dello stato è sullo stesso PC
            if (config.Server.Length == 0 && manifesto != null && manifesto.Stato.Length > 0)
            {
                return manifesto.Stato;
            }
            string server = IndirizzoServer().Trim();
            if (server.Length == 0)
            {
                return "";
            }
            int doppi = server.IndexOf("://", StringComparison.Ordinal);
            if (doppi >= 0)
            {
                server = server.Substring(doppi + 3);
            }
            server = server.TrimEnd('/');
            string host;
            if (server.StartsWith("["))
            {
                int chiusa = server.IndexOf(']');
                host = chiusa > 0 ? server.Substring(0, chiusa + 1) : server;
            }
            else
            {
                int duePunti = server.LastIndexOf(':');
                host = duePunti > 0 ? server.Substring(0, duePunti) : server;
            }
            return host.Length > 0 ? "http://" + host + ":7780/stato" : "";
        }

        void ChiediStato()
        {
            string url = IndirizzoStato();
            if (url.Length == 0)
            {
                mondo = null;
                mondoChiesto = true;
                AggiornaAttivo();
                Ridisegna();
                return;
            }
            if (Interlocked.CompareExchange(ref statoInViaggio, 1, 0) != 0)
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
                    mondo = letto;
                    mondoLetto = DateTime.UtcNow;
                    mondoChiesto = true;
                    AggiornaAttivo();
                    Ridisegna();
                });
            });
        }

        /// <summary>L'ora della valle adesso: quella detta dal server più il tempo passato (un giorno della valle = 120
        /// minuti veri, cioè 12 minuti della valle ogni minuto vero). Ogni mezzo minuto il server la corregge.</summary>
        double OraDellaValle()
        {
            if (mondo == null || mondo.Ora < 0)
            {
                return -1;
            }
            double passati = Math.Min(2.0, (DateTime.UtcNow - mondoLetto).TotalMinutes);
            return (mondo.Ora + passati * 0.2) % 24.0;
        }

        /// <summary>Scarica dal sito notizie, note, roadmap, cronache e le immagini che mancano.</summary>
        void AggiornaContenuti()
        {
            string cartella = config.CartellaContenuti;
            if (cartella.Length == 0 || Interlocked.CompareExchange(ref contenutiInViaggio, 1, 0) != 0)
            {
                return;
            }
            ThreadPool.QueueUserWorkItem(delegate
            {
                int arrivati = Archivio.ScaricaDalSito(cartella);
                Contenuti nuovi = Archivio.CaricaLocali();
                foreach (Notizia n in nuovi.Notizie)
                {
                    if (n.Immagine.Length > 0 && Archivio.Immagine(n.Immagine) == null)
                    {
                        Archivio.ScaricaImmagine(cartella, n.Immagine);
                    }
                }
                Registro.Scrivi("contenuti dal sito: " + arrivati + " file su " + Archivio.Nomi.Length);
                Interlocked.Exchange(ref contenutiInViaggio, 0);
                Sulla(delegate
                {
                    contenuti = nuovi;
                    foreach (Image img in immagini.Values)
                    {
                        img.Dispose();
                    }
                    immagini.Clear();
                    versioneScelta = Math.Min(versioneScelta, Math.Max(0, contenuti.Patch.Count - 1));
                    Ridisegna();
                });
            });
        }

        void Avanza(Fase fase, string testo, long fatti, long totale, double bytePerSecondo)
        {
            Sulla(delegate
            {
                if (stato != StatoLauncher.Lavoro)
                {
                    return;
                }
                faseLavoro = fase;
                double parte = totale > 0 ? Math.Min(1.0, (double)fatti / totale) : -1;
                string riga = testo;
                if (parte >= 0)
                {
                    riga += ": " + Math.Floor(parte * 100).ToString(CultureInfo.InvariantCulture) + "%";
                }
                rigaStato = riga;
                if (fase == Fase.Scarico && totale > 0)
                {
                    string dettaglio = Misure.Byte(fatti) + " di " + Misure.Byte(totale);
                    if (bytePerSecondo > 0)
                    {
                        dettaglio += " · " + Misure.Byte((long)bytePerSecondo) + "/s";
                        double restano = (totale - fatti) / bytePerSecondo;
                        if (restano > 1 && restano < 72 * 3600)
                        {
                            dettaglio += " · mancano circa " + Misure.Durata(restano);
                        }
                    }
                    rigaDettaglio = dettaglio;
                }
                else if (fase == Fase.Installo)
                {
                    rigaDettaglio = "Scompatto il gioco accanto a quello vecchio: si scambiano solo alla fine.";
                }
                else if (fase == Fase.Controllo)
                {
                    rigaDettaglio = "Controllo che i pezzi scaricati siano giusti (impronta SHA-256).";
                }
                else
                {
                    rigaDettaglio = "";
                }
                avanzamento = parte;
                Invalidate(RettangoloBarra());
            });
        }

        void Imposta(StatoLauncher nuovo, string riga, double parte, bool errore)
        {
            stato = nuovo;
            rigaStato = riga;
            avanzamento = parte;
            erroreRiga = errore;
            AggiornaAttivo();
            Ridisegna();
        }

        /// <summary>(10/11, deciso da Fra) Si gioca solo con il server acceso: se la valle dorme (il server non risponde)
        /// o e' chiusa per lavori, GIOCA resta spento. Niente partite in locale dal launcher.</summary>
        bool ValleAperta()
        {
            return mondo != null && mondo.Aperto;
        }

        void AggiornaAttivo()
        {
            bool pronto = stato == StatoLauncher.Pronto || stato == StatoLauncher.SenzaRete;
            principale.Attivo = stato != StatoLauncher.Avvio && stato != StatoLauncher.Controllo && (!pronto || ValleAperta());
        }

        /// <summary>La scritta del pulsante grande e la riga piccola sotto.</summary>
        string Etichetta(out string sotto)
        {
            sotto = "";
            switch (stato)
            {
                case StatoLauncher.DaInstallare:
                    if (manifesto != null && manifesto.BytePacchetto > 0)
                    {
                        sotto = Misure.Byte(aggiornatore.DaScaricare(manifesto)) + " da scaricare";
                    }
                    return "INSTALLA";
                case StatoLauncher.DaAggiornare:
                    if (manifesto != null)
                    {
                        sotto = "v" + manifesto.Versione + (manifesto.BytePacchetto > 0 ? " · " + Misure.Byte(aggiornatore.DaScaricare(manifesto)) : "");
                    }
                    return ripara ? "RIPARA" : "AGGIORNA";
                case StatoLauncher.Pronto:
                case StatoLauncher.SenzaRete:
                    if (!ValleAperta())
                    {
                        // il server e' spento (o chiuso per lavori): si aspetta, il launcher ricontrolla da solo
                        sotto = !mondoChiesto ? "cerco il server..." : (mondo != null ? "la valle è chiusa per lavori" : "la valle dorme: il server è spento");
                        return "DORME";
                    }
                    sotto = versioneInstallata.Length > 0 ? "v" + versioneInstallata + (stato == StatoLauncher.SenzaRete ? " · senza rete" : "") : "";
                    return "GIOCA";
                case StatoLauncher.Lavoro:
                    string quanto = avanzamento >= 0 ? " " + Math.Floor(avanzamento * 100).ToString(CultureInfo.InvariantCulture) + "%" : "";
                    switch (faseLavoro)
                    {
                        case Fase.Scarico: sotto = "scarico..." + quanto; return "PAUSA";
                        case Fase.Controllo: sotto = "controllo..." + quanto; return "FERMA";
                        case Fase.Installo: sotto = "installo..." + quanto; return "FERMA";
                        default: sotto = "verifico..." + quanto; return "FERMA";
                    }
                case StatoLauncher.InPausa:
                    sotto = (avanzamento >= 0 ? Math.Floor(avanzamento * 100).ToString(CultureInfo.InvariantCulture) + "% · " : "") + "in pausa";
                    return "RIPRENDI";
                case StatoLauncher.Errore:
                    return "RIPROVA";
                default:
                    sotto = "cerco la valle...";
                    return "ATTENDI";
            }
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

        // ------------------------------------------------------------------------------------ il tempo che passa

        float Battito()
        {
            // due colpi e una pausa ogni 1,6 s, come il Cuore del gioco
            double t = (DateTime.UtcNow - inizio).TotalSeconds % 1.6;
            double a = Math.Exp(-Math.Pow((t - 0.10) / 0.07, 2));
            double b = 0.75 * Math.Exp(-Math.Pow((t - 0.42) / 0.07, 2));
            return (float)Math.Min(1.0, a + b);
        }

        Rectangle RettangoloBarra()
        {
            return new Rectangle(0, (int)(ClientSize.Height - AltezzaBarra * S), ClientSize.Width, (int)(AltezzaBarra * S) + 1);
        }

        void Anima()
        {
            if (WindowState == FormWindowState.Minimized)
            {
                return;
            }
            Invalidate(RettangoloBarra());
            foreach (RectangleF r in animate)
            {
                Invalidate(Rectangle.Inflate(Rectangle.Round(r), 2, 2));
            }
        }

        void OgniSecondo()
        {
            // l'ora della valle cammina anche tra una domanda al server e l'altra
            string minuto = StatoMondo.OraLeggibile(OraDellaValle());
            if (scheda == Scheda.Valle && minuto != ultimoMinuto)
            {
                ultimoMinuto = minuto;
                Ridisegna();
            }
            // le voci dalla locanda cambiano ogni 12 secondi
            int ora = (int)((DateTime.UtcNow - inizio).TotalSeconds / 12);
            if (ora != voce)
            {
                voce = ora;
                if (scheda == Scheda.Valle && contenuti.Voci.Count > 1)
                {
                    Ridisegna();
                }
            }
        }

        // ------------------------------------------------------------------------------------ disegno

        protected override void OnPaint(PaintEventArgs e)
        {
            Graphics g = e.Graphics;
            float W = ClientSize.Width;
            float H = ClientSize.Height;
            if (W < 10 || H < 10)
            {
                return;
            }
            Scheda tipo = scheda == Scheda.Valle ? Scheda.Valle : Scheda.Patch;
            if (sfondoPronto == null || sfondoPronto.Width != (int)W || sfondoPronto.Height != (int)H || sfondoProntoPer != tipo)
            {
                if (sfondoPronto != null)
                {
                    sfondoPronto.Dispose();
                }
                sfondoPronto = PreparaSfondo((int)W, (int)H, tipo == Scheda.Valle);
                sfondoProntoPer = tipo;
            }
            g.CompositingMode = CompositingMode.SourceCopy;
            g.DrawImageUnscaled(sfondoPronto, 0, 0);
            g.CompositingMode = CompositingMode.SourceOver;

            if (paginaSporca || paginaPronta == null || paginaPronta.Width != (int)W || paginaPronta.Height != (int)H)
            {
                PreparaPagina((int)W, (int)H);
            }
            g.DrawImageUnscaled(paginaPronta, 0, 0);

            g.SmoothingMode = SmoothingMode.AntiAlias;
            g.TextRenderingHint = TextRenderingHint.AntiAliasGridFit;
            g.InterpolationMode = InterpolationMode.HighQualityBicubic;
            DisegnaAnimazioni(g);
            DisegnaTitolo(g, W);
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

        /// <summary>Lo sfondo con i veli, una volta sola: nella Valle il dipinto si vede bene, nelle altre schede è più scuro.</summary>
        Bitmap PreparaSfondo(int W, int H, bool valle)
        {
            Bitmap b = new Bitmap(W, H, PixelFormat.Format32bppPArgb);
            using (Graphics g = Graphics.FromImage(b))
            {
                g.InterpolationMode = InterpolationMode.HighQualityBicubic;
                g.Clear(Fondo);
                if (sfondo != null)
                {
                    // "copri": l'immagine riempie tutta la finestra, tagliando i bordi se serve
                    float scala = Math.Max((float)W / sfondo.Width, (float)H / sfondo.Height);
                    float w = sfondo.Width * scala;
                    float h = sfondo.Height * scala;
                    g.DrawImage(sfondo, (W - w) / 2, (H - h) / 2, w, h);
                }
                if (valle)
                {
                    // a sinistra scuro per leggere le notizie, poi sfuma verso il dipinto
                    float largo = W * 0.8f;
                    using (LinearGradientBrush velo = new LinearGradientBrush(new RectangleF(-1, 0, largo + 2, H), Color.FromArgb(236, Fondo), Color.FromArgb(0, Fondo), LinearGradientMode.Horizontal))
                    {
                        Blend sfuma = new Blend();
                        sfuma.Positions = new float[] { 0f, 0.55f, 0.75f, 1f };
                        sfuma.Factors = new float[] { 0f, 0.12f, 0.45f, 1f };
                        velo.Blend = sfuma;
                        g.FillRectangle(velo, 0, 0, largo, H);
                    }
                }
                else
                {
                    using (SolidBrush velo = new SolidBrush(Color.FromArgb(214, Fondo)))
                    {
                        g.FillRectangle(velo, 0, 0, W, H);
                    }
                }
                // in alto un velo per il titolo e le schede, in basso per la barra
                using (LinearGradientBrush alto = new LinearGradientBrush(new RectangleF(0, 0, W, 130 * S), Color.FromArgb(200, Fondo), Color.FromArgb(0, Fondo), LinearGradientMode.Vertical))
                {
                    g.FillRectangle(alto, 0, 0, W, 130 * S);
                }
                float hb = (AltezzaBarra + 30) * S;
                using (LinearGradientBrush basso = new LinearGradientBrush(new RectangleF(0, H - hb, W, hb), Color.FromArgb(0, Fondo), Color.FromArgb(245, Fondo), LinearGradientMode.Vertical))
                {
                    g.FillRectangle(basso, 0, H - hb + 1, W, hb);
                }
            }
            return b;
        }

        void PreparaPagina(int W, int H)
        {
            if (paginaPronta == null || paginaPronta.Width != W || paginaPronta.Height != H)
            {
                if (paginaPronta != null)
                {
                    paginaPronta.Dispose();
                }
                paginaPronta = new Bitmap(W, H, PixelFormat.Format32bppPArgb);
            }
            // prima di disegnare: se la pagina chiede di essere rifatta (la roadmap che si sposta), resta sporca
            paginaSporca = false;
            List<Zona> nuove = new List<Zona>();
            animate.Clear();
            using (Graphics g = Graphics.FromImage(paginaPronta))
            {
                g.Clear(Color.Transparent);
                g.SmoothingMode = SmoothingMode.AntiAlias;
                // sul trasparente il ClearType viene male: si usa l'antialias semplice
                g.TextRenderingHint = TextRenderingHint.AntiAlias;
                g.InterpolationMode = InterpolationMode.HighQualityBicubic;
                zone = nuove;
                try
                {
                    DisegnaPagina(g, W, H);
                }
                catch (Exception e)
                {
                    // una pagina fatta male (un JSON strano) non deve rompere la finestra
                    Registro.Scrivi("pagina " + scheda + " non disegnata: " + e);
                }
            }
            PosizionaCaselle();
        }

        void DisegnaTitolo(Graphics g, float W)
        {
            Font titolo = Titolo(40);
            PointF dove = new PointF(Margine * S, 16 * S);
            using (SolidBrush ombra = new SolidBrush(Color.FromArgb(200, 0, 0, 0)))
            {
                g.DrawString("VALDORSO", titolo, ombra, dove.X + 2 * S, dove.Y + 2 * S);
            }
            RectangleF area = new RectangleF(dove.X, dove.Y, 400 * S, 56 * S);
            using (LinearGradientBrush oro = new LinearGradientBrush(area, OroChiaro, Color.FromArgb(160, 118, 38), LinearGradientMode.Vertical))
            {
                g.DrawString("VALDORSO", titolo, oro, dove);
            }
            using (SolidBrush sec = new SolidBrush(Secondario))
            {
                g.DrawString("Ogni gesto lascia un segno. Scegli chi diventare.", Corsivo(15.5f), sec, dove.X + 3 * S, dove.Y + 52 * S);
            }
            // la riga d'oro che sfuma ai lati, con il rombo di brace in mezzo
            float y = SottoTitolo * S;
            RectangleF linea = new RectangleF(Margine * S, y, W - 2 * Margine * S, Math.Max(1f, S));
            using (LinearGradientBrush filo = new LinearGradientBrush(linea, Color.FromArgb(0, Oro), Color.FromArgb(150, Oro), LinearGradientMode.Horizontal))
            {
                filo.SetBlendTriangularShape(0.5f);
                g.FillRectangle(filo, linea);
            }
        }

        void DisegnaBarra(Graphics g, float W, float H)
        {
            float x = Margine * S;
            float w = W - Margine * S - 340 * S;
            using (SolidBrush colore = new SolidBrush(erroreRiga ? Rosso : Pergamena))
            {
                g.DrawString(rigaStato, Testo(18), colore, new RectangleF(x, H - 124 * S, w, 48 * S));
            }
            // la barra d'avanzamento: un filo scuro che si riempie d'oro, con la punta di brace che batte
            float by = H - 70 * S;
            float alta = 6 * S;
            using (SolidBrush vuota = new SolidBrush(Color.FromArgb(170, 44, 35, 26)))
            {
                g.FillRectangle(vuota, x, by, w, alta);
            }
            if (avanzamento >= 0)
            {
                float pieno = (float)(w * avanzamento);
                bool ferma = stato == StatoLauncher.InPausa || stato == StatoLauncher.Errore;
                if (pieno > 1)
                {
                    Color da = ferma ? Color.FromArgb(110, 90, 50) : Color.FromArgb(150, 110, 35);
                    Color a = ferma ? Color.FromArgb(170, 150, 100) : OroChiaro;
                    using (LinearGradientBrush oro = new LinearGradientBrush(new RectangleF(x, by, pieno + 1, alta), da, a, LinearGradientMode.Horizontal))
                    {
                        g.FillRectangle(oro, x, by, pieno, alta);
                    }
                }
                if (!ferma)
                {
                    float b = Battito();
                    using (SolidBrush alone = new SolidBrush(Color.FromArgb((int)(60 + 120 * b), Brace)))
                    {
                        float r = (5 + 4 * b) * S;
                        g.FillEllipse(alone, x + pieno - r, by + alta / 2 - r, 2 * r, 2 * r);
                    }
                }
            }
            if (rigaDettaglio.Length > 0)
            {
                using (SolidBrush sec = new SolidBrush(Color.FromArgb(220, Secondario)))
                {
                    g.DrawString(rigaDettaglio, Corsivo(15), sec, new RectangleF(x, by + 12 * S, w, 24 * S));
                }
            }
            // in basso a destra: le versioni
            string versioni = "launcher " + Info.Versione + (versioneInstallata.Length > 0 ? " · gioco v" + versioneInstallata : "");
            using (SolidBrush sec = new SolidBrush(Color.FromArgb(150, Secondario)))
            using (StringFormat destra = new StringFormat())
            {
                destra.Alignment = StringAlignment.Far;
                destra.LineAlignment = StringAlignment.Center;
                g.DrawString(versioni, Corsivo(14), sec, new RectangleF(W - 400 * S, H - 40 * S, 360 * S, 28 * S), destra);
            }
        }

        void DisegnaPulsante(Graphics g, Pulsante p)
        {
            if (p.Grande)
            {
                DisegnaPrincipale(g, p);
                return;
            }
            if (p.Scheda >= 0)
            {
                bool attiva = (int)scheda == p.Scheda;
                Color colore = attiva ? OroChiaro : (p.Sopra ? Pergamena : Color.FromArgb(205, Secondario));
                using (SolidBrush scritta = new SolidBrush(colore))
                using (StringFormat centro = new StringFormat())
                {
                    centro.Alignment = StringAlignment.Center;
                    centro.LineAlignment = StringAlignment.Center;
                    g.DrawString(p.Testo, Titolo(13.5f), scritta, p.Area, centro);
                }
                if (attiva || p.Sopra)
                {
                    float y = p.Area.Bottom - 4 * S;
                    RectangleF filo = new RectangleF(p.Area.X - 4 * S, y, p.Area.Width + 8 * S, Math.Max(1f, 1.5f * S));
                    using (LinearGradientBrush oro = new LinearGradientBrush(filo, Color.FromArgb(0, Oro), Color.FromArgb(attiva ? 230 : 110, Oro), LinearGradientMode.Horizontal))
                    {
                        oro.SetBlendTriangularShape(0.5f);
                        g.FillRectangle(oro, filo);
                    }
                    if (attiva)
                    {
                        Rombo(g, p.Area.X + p.Area.Width / 2, y + 0.75f * S, 3.5f * S, Brace);
                    }
                }
                return;
            }
            if (p.Segno.Length > 0)
            {
                if (p.Sopra)
                {
                    using (SolidBrush alone = new SolidBrush(p.Segno == "chiudi" ? Color.FromArgb(150, 120, 40, 30) : Color.FromArgb(120, PulsanteSopra)))
                    {
                        g.FillRectangle(alone, p.Area);
                    }
                }
                using (Pen linea = new Pen(p.Sopra ? OroChiaro : Color.FromArgb(230, Secondario), 1.6f * S))
                {
                    float cx = p.Area.X + p.Area.Width / 2;
                    float cy = p.Area.Y + p.Area.Height / 2;
                    float r = 6 * S;
                    if (p.Segno == "chiudi")
                    {
                        g.DrawLine(linea, cx - r, cy - r, cx + r, cy + r);
                        g.DrawLine(linea, cx - r, cy + r, cx + r, cy - r);
                    }
                    else
                    {
                        g.DrawLine(linea, cx - r, cy + 2 * S, cx + r, cy + 2 * S);
                    }
                }
                return;
            }
            using (SolidBrush scritta = new SolidBrush(p.Sopra ? OroChiaro : Color.FromArgb(200, Secondario)))
            using (StringFormat centro = new StringFormat())
            {
                centro.Alignment = StringAlignment.Center;
                centro.LineAlignment = StringAlignment.Center;
                g.DrawString(p.Testo, Testo(15), scritta, p.Area, centro);
                if (p.Sopra)
                {
                    using (Pen sotto = new Pen(Color.FromArgb(160, Oro), Math.Max(1f, S)))
                    {
                        g.DrawLine(sotto, p.Area.Left + 3 * S, p.Area.Bottom - 2 * S, p.Area.Right - 3 * S, p.Area.Bottom - 2 * S);
                    }
                }
            }
        }

        /// <summary>Il pulsante grande: d'oro pieno quando si può giocare, scuro con il bordo d'oro negli altri casi.</summary>
        void DisegnaPrincipale(Graphics g, Pulsante p)
        {
            string sotto;
            string testo = Etichetta(out sotto);
            bool gioca = testo == "GIOCA" && p.Attivo;
            float b = p.Attivo ? Battito() : 0f;
            RectangleF a = p.Area;
            // l'alone di brace che batte dietro il pulsante
            if (p.Attivo)
            {
                for (int i = 3; i >= 1; i--)
                {
                    float gonfia = i * 3 * S * (0.6f + 0.6f * b);
                    using (GraphicsPath alone = Arrotondato(RectangleF.Inflate(a, gonfia, gonfia), 6 * S + gonfia))
                    using (SolidBrush colore = new SolidBrush(Color.FromArgb((int)((gioca ? 26 : 16) * (0.5f + b)), gioca ? Oro : Brace)))
                    {
                        g.FillPath(colore, alone);
                    }
                }
            }
            using (GraphicsPath forma = Arrotondato(a, 5 * S))
            {
                Color alto, basso;
                if (gioca)
                {
                    alto = p.Sopra ? Color.FromArgb(250, 228, 150) : Color.FromArgb(236, 204, 108);
                    basso = p.Sopra ? Color.FromArgb(196, 146, 40) : Color.FromArgb(170, 124, 30);
                }
                else if (p.Attivo)
                {
                    alto = p.Sopra ? Color.FromArgb(120, 82, 34) : Color.FromArgb(92, 60, 26);
                    basso = p.Sopra ? Color.FromArgb(70, 44, 20) : Color.FromArgb(52, 34, 16);
                }
                else
                {
                    alto = Color.FromArgb(50, 42, 34);
                    basso = Color.FromArgb(34, 28, 22);
                }
                // i pennelli sfumati un po' più grandi della figura: GDI+ ripete la sfumatura e sul bordo farebbe una riga
                using (LinearGradientBrush fondo = new LinearGradientBrush(RectangleF.Inflate(a, 0, 2), alto, basso, LinearGradientMode.Vertical))
                {
                    g.FillPath(fondo, forma);
                }
                // il riflesso in alto
                RectangleF luce = new RectangleF(a.X + 3 * S, a.Y + 2 * S, a.Width - 6 * S, a.Height * 0.45f);
                using (GraphicsPath parte = Arrotondato(luce, 4 * S))
                using (LinearGradientBrush riflesso = new LinearGradientBrush(RectangleF.Inflate(luce, 0, 2), Color.FromArgb(gioca ? 70 : 28, 255, 245, 210), Color.FromArgb(0, 255, 245, 210), LinearGradientMode.Vertical))
                {
                    g.FillPath(riflesso, parte);
                }
                Color bordo = gioca ? Color.FromArgb(255, 250, 230, 160) : (p.Attivo ? Color.FromArgb((int)(170 + 85 * b), Oro) : Color.FromArgb(90, Oro));
                using (Pen linea = new Pen(bordo, 1.5f * S))
                {
                    g.DrawPath(linea, forma);
                }
            }
            Color scritta = gioca ? Color.FromArgb(40, 24, 8) : (p.Attivo ? (p.Sopra ? OroChiaro : Oro) : Color.FromArgb(120, Secondario));
            using (SolidBrush pennello = new SolidBrush(scritta))
            using (StringFormat centro = new StringFormat())
            {
                centro.Alignment = StringAlignment.Center;
                centro.LineAlignment = StringAlignment.Center;
                float dimensione = testo.Length > 7 ? 23 : 27;
                RectangleF alto = sotto.Length > 0 ? new RectangleF(a.X, a.Y + 2 * S, a.Width, a.Height * 0.62f) : a;
                g.DrawString(testo, Titolo(dimensione), pennello, alto, centro);
                if (sotto.Length > 0)
                {
                    using (SolidBrush piccolo = new SolidBrush(Color.FromArgb(gioca ? 200 : 210, gioca ? Color.FromArgb(60, 36, 10) : Secondario)))
                    {
                        g.DrawString(sotto, Corsivo(14.5f), piccolo, new RectangleF(a.X, a.Y + a.Height * 0.56f, a.Width, a.Height * 0.36f), centro);
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
            float d = Math.Max(1f, Math.Min(raggio * 2, Math.Min(r.Width, r.Height)));
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
                orologio.Dispose();
                if (sfondo != null)
                {
                    sfondo.Dispose();
                }
                if (sfondoPronto != null)
                {
                    sfondoPronto.Dispose();
                }
                if (paginaPronta != null)
                {
                    paginaPronta.Dispose();
                }
                foreach (Image img in immagini.Values)
                {
                    img.Dispose();
                }
                foreach (Font f in fontPronti.Values)
                {
                    f.Dispose();
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
        public static void Main(string[] argomenti)
        {
            // "--scheda=roadmap" apre il launcher su una scheda (per le prove e i collegamenti)
            string schedaIniziale = "";
            foreach (string a in argomenti)
            {
                if (a.StartsWith("--scheda=", StringComparison.OrdinalIgnoreCase))
                {
                    schedaIniziale = a.Substring(9);
                }
            }
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
                    if (Environment.OSVersion.Platform == PlatformID.Win32NT && Environment.OSVersion.Version.Major >= 6)
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
                AppDomain.CurrentDomain.UnhandledException += delegate(object s, UnhandledExceptionEventArgs e) { Registro.Scrivi("errore grave: " + e.ExceptionObject); };
                Application.Run(new Finestra(schedaIniziale));
            }
        }
    }
}
