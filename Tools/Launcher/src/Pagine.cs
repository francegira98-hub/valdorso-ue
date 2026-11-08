// Valdorso - Il launcher: le cinque schede (La valle, Note delle patch, Roadmap, Cronache, Opzioni). Scritto da
// Claude l'08/11/2026.
//
// Ogni scheda si disegna nell'immagine della pagina (Finestra.PreparaPagina): qui si scrive e si registrano le zone
// cliccabili; quello che si muove (il Cuore che batte, la tappa in corso) lo disegna DisegnaAnimazioni, sopra.
// Le pagine lunghe scorrono con la rotella (o con Pagina su e Pagina giù): la barretta d'oro a destra dice dove sei.

using System;
using System.Collections.Generic;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.Globalization;
using System.IO;
using System.Windows.Forms;

namespace Valdorso.Launcher
{
    public partial class Finestra
    {
        RectangleF visibile;
        RectangleF campoServer = RectangleF.Empty;
        readonly List<PointF> anelli = new List<PointF>();
        bool ancoraRoadmap;
        float posizioneUltimaFatta = -1;
        RectangleF cuore = RectangleF.Empty;

        // ------------------------------------------------------------------------------------ attrezzi

        readonly Dictionary<Font, float> altezzeRiga = new Dictionary<Font, float>();

        /// <summary>Quanto è alta una riga di questo carattere (misurata: Font.Height con i caratteri propri non va
        /// dappertutto).</summary>
        float AltezzaRiga(Graphics g, Font f)
        {
            float h;
            if (!altezzeRiga.TryGetValue(f, out h))
            {
                h = g.MeasureString("Àg", f).Height;
                altezzeRiga[f] = h;
            }
            return h;
        }

        float Alto(Graphics g, string testo, Font f, float w)
        {
            if (string.IsNullOrEmpty(testo))
            {
                return 0;
            }
            return g.MeasureString(testo, f, (int)Math.Max(10, w)).Height;
        }

        /// <summary>Scrive il testo andando a capo nella larghezza data; restituisce quanto è alto.</summary>
        float Scrivi(Graphics g, string testo, Font f, Color colore, float x, float y, float w)
        {
            if (string.IsNullOrEmpty(testo))
            {
                return 0;
            }
            float h = Alto(g, testo, f, w);
            if (y < visibile.Bottom && y + h > visibile.Top)
            {
                using (SolidBrush b = new SolidBrush(colore))
                {
                    g.DrawString(testo, f, b, new RectangleF(x, y, w + 2, h + 2));
                }
            }
            return h;
        }

        /// <summary>Una riga sola, tagliata con "..." se non ci sta.</summary>
        void Riga(Graphics g, string testo, Font f, Color colore, RectangleF r, bool destra)
        {
            using (SolidBrush b = new SolidBrush(colore))
            using (StringFormat formato = new StringFormat(StringFormatFlags.NoWrap))
            {
                formato.Trimming = StringTrimming.EllipsisCharacter;
                formato.LineAlignment = StringAlignment.Center;
                formato.Alignment = destra ? StringAlignment.Far : StringAlignment.Near;
                g.DrawString(testo ?? "", f, b, r, formato);
            }
        }

        float Largo(Graphics g, string testo, Font f)
        {
            return g.MeasureString(testo ?? "", f).Width;
        }

        /// <summary>Un'etichetta in maiuscoletto d'oro ("NOTIZIE DALLA VALLE") con il filo che va fino a destra.</summary>
        float Etichetta(Graphics g, string testo, float x, float y, float w, Color colore)
        {
            Font f = Titolo(12.5f);
            using (SolidBrush b = new SolidBrush(colore))
            {
                g.DrawString(testo, f, b, x, y);
            }
            float lw = Largo(g, testo, f);
            float ly = y + AltezzaRiga(g, f) * 0.55f;
            if (w > lw + 20 * S)
            {
                RectangleF filo = new RectangleF(x + lw + 8 * S, ly, w - lw - 8 * S, Math.Max(1f, S));
                using (LinearGradientBrush oro = new LinearGradientBrush(filo, Color.FromArgb(110, colore), Color.FromArgb(0, colore), LinearGradientMode.Horizontal))
                {
                    g.FillRectangle(oro, filo);
                }
            }
            return AltezzaRiga(g, f) + 8 * S;
        }

        void Pannello(Graphics g, RectangleF r, int opacita)
        {
            using (GraphicsPath forma = Arrotondato(r, 6 * S))
            using (SolidBrush fondo = new SolidBrush(Color.FromArgb(opacita, Pannelli)))
            using (Pen bordo = new Pen(Color.FromArgb(105, Oro), Math.Max(1f, S)))
            {
                g.FillPath(fondo, forma);
                g.DrawPath(bordo, forma);
            }
        }

        /// <summary>Registra una zona cliccabile (solo la parte che si vede).</summary>
        void Zona(RectangleF r, string chiave, Action azione)
        {
            RectangleF dentro = RectangleF.Intersect(r, visibile);
            if (dentro.Width <= 0 || dentro.Height <= 0)
            {
                return;
            }
            Zona z = new Zona();
            z.Area = dentro;
            z.Chiave = chiave;
            z.Azione = azione;
            zone.Add(z);
        }

        bool Sopra(string chiave)
        {
            return zonaSopra == chiave;
        }

        /// <summary>Un collegamento nel testo: d'oro, sottolineato quando ci passa sopra il mouse.</summary>
        float Collega(Graphics g, string testo, float x, float y, string chiave, Action azione)
        {
            Font f = Corsivo(16);
            bool sopra = Sopra(chiave);
            SizeF m = g.MeasureString(testo + "  »", f);
            using (SolidBrush b = new SolidBrush(sopra ? OroChiaro : Oro))
            {
                g.DrawString(testo + "  »", f, b, x, y);
            }
            if (sopra)
            {
                using (Pen p = new Pen(Color.FromArgb(160, Oro), Math.Max(1f, S)))
                {
                    g.DrawLine(p, x + 3 * S, y + m.Height - 3 * S, x + m.Width - 4 * S, y + m.Height - 3 * S);
                }
            }
            Zona(new RectangleF(x, y, m.Width, m.Height), chiave, azione);
            return m.Height;
        }

        /// <summary>Un pulsantino a contorno d'oro (Opzioni, notizie).</summary>
        void Bottoncino(Graphics g, RectangleF r, string testo, string chiave, Action azione, bool attivo)
        {
            bool sopra = attivo && Sopra(chiave);
            using (GraphicsPath forma = Arrotondato(r, 4 * S))
            using (SolidBrush fondo = new SolidBrush(sopra ? Color.FromArgb(220, PulsanteSopra) : Color.FromArgb(200, 28, 22, 18)))
            using (Pen bordo = new Pen(attivo ? Color.FromArgb(sopra ? 230 : 150, Oro) : Color.FromArgb(70, Oro), Math.Max(1f, S)))
            {
                g.FillPath(fondo, forma);
                g.DrawPath(bordo, forma);
            }
            using (SolidBrush b = new SolidBrush(attivo ? (sopra ? OroChiaro : Pergamena) : Color.FromArgb(120, Secondario)))
            using (StringFormat centro = new StringFormat())
            {
                centro.Alignment = StringAlignment.Center;
                centro.LineAlignment = StringAlignment.Center;
                centro.Trimming = StringTrimming.EllipsisCharacter;
                centro.FormatFlags = StringFormatFlags.NoWrap;
                g.DrawString(testo, Testo(16), b, r, centro);
            }
            if (attivo)
            {
                Zona(r, chiave, azione);
            }
        }

        /// <summary>Un "segno di spunta" disegnato a linee (i caratteri non sempre hanno ✔).</summary>
        void Spunta(Graphics g, float cx, float cy, float r, Color colore, float spessore)
        {
            using (Pen p = new Pen(colore, spessore))
            {
                p.StartCap = LineCap.Round;
                p.EndCap = LineCap.Round;
                p.LineJoin = LineJoin.Round;
                g.DrawLines(p, new PointF[] { new PointF(cx - r, cy), new PointF(cx - r * 0.3f, cy + r * 0.7f), new PointF(cx + r, cy - r * 0.75f) });
            }
        }

        /// <summary>Un elenco di voci con il rombo, in una o più colonne pareggiate (si legge dall'alto in basso).</summary>
        float Elenco(Graphics g, List<string> voci, Font f, Color colore, Color puntino, float x, float y, float w, int colonne)
        {
            if (voci == null || voci.Count == 0)
            {
                return 0;
            }
            colonne = Math.Max(1, Math.Min(colonne, voci.Count));
            float spazio = 26 * S;
            float cw = (w - spazio * (colonne - 1)) / colonne;
            float rientro = 18 * S;
            float tra = 5 * S;
            List<float> alte = new List<float>();
            float somma = 0;
            foreach (string v in voci)
            {
                float h = Alto(g, v, f, cw - rientro) + tra;
                alte.Add(h);
                somma += h;
            }
            float obiettivo = somma / colonne;
            int colonna = 0;
            float cy = y;
            float massimo = 0;
            for (int i = 0; i < voci.Count; i++)
            {
                if (colonna < colonne - 1 && cy - y > 0 && cy - y + alte[i] * 0.5f > obiettivo)
                {
                    massimo = Math.Max(massimo, cy - y);
                    colonna++;
                    cy = y;
                }
                float cx = x + colonna * (cw + spazio);
                Rombo(g, cx + 5 * S, cy + AltezzaRiga(g, f) * 0.55f, 3.2f * S, puntino);
                Scrivi(g, voci[i], f, colore, cx + rientro, cy, cw - rientro);
                cy += alte[i];
            }
            return Math.Max(massimo, cy - y);
        }

        /// <summary>Una runa a linee d'oro, con il bagliore di brace sotto. mezza = mezza altezza in pixel.</summary>
        void DisegnaRuna(Graphics g, int indice, float cx, float cy, float mezza, Color colore, float spessore, bool bagliore)
        {
            if (indice < 0 || indice >= Rune.Linee.Length)
            {
                return;
            }
            float[][] linee = Rune.Linee[indice];
            List<PointF[]> tratti = new List<PointF[]>();
            foreach (float[] l in linee)
            {
                PointF[] punti = new PointF[l.Length / 2];
                for (int k = 0; k < punti.Length; k++)
                {
                    punti[k] = new PointF(cx + l[2 * k] * mezza, cy - l[2 * k + 1] * mezza);
                }
                if (punti.Length >= 2)
                {
                    tratti.Add(punti);
                }
            }
            if (bagliore)
            {
                using (Pen alone = new Pen(Color.FromArgb(45, Brace), spessore * 3.2f))
                {
                    alone.StartCap = LineCap.Round;
                    alone.EndCap = LineCap.Round;
                    alone.LineJoin = LineJoin.Round;
                    foreach (PointF[] t in tratti)
                    {
                        g.DrawLines(alone, t);
                    }
                }
            }
            using (Pen p = new Pen(colore, spessore))
            {
                p.StartCap = LineCap.Round;
                p.EndCap = LineCap.Round;
                p.LineJoin = LineJoin.Round;
                foreach (PointF[] t in tratti)
                {
                    g.DrawLines(p, t);
                }
            }
        }

        /// <summary>Una parola di rune ("Aeth-Var"): le rune in fila unite dal trattino a metà altezza, come nel gioco.
        /// Restituisce quanto è larga.</summary>
        float DisegnaParola(Graphics g, string parola, float x, float cy, float mezza, Color colore, float spessore)
        {
            string[] nomi = (parola ?? "").Split('-');
            const float passo = 2.0f + 0.9f;
            float larga = (nomi.Length * 2.0f + (nomi.Length - 1) * 0.9f) * mezza;
            float cx0 = x + mezza;
            for (int k = 0; k < nomi.Length; k++)
            {
                float cx = cx0 + k * passo * mezza;
                DisegnaRuna(g, Rune.Indice(nomi[k]), cx, cy, mezza, colore, spessore, false);
                if (k < nomi.Length - 1)
                {
                    using (Pen p = new Pen(colore, spessore))
                    {
                        p.StartCap = LineCap.Round;
                        p.EndCap = LineCap.Round;
                        g.DrawLine(p, cx + 0.95f * mezza, cy, cx + 1.95f * mezza, cy);
                    }
                }
            }
            return larga;
        }

        /// <summary>Un'immagine che copre il rettangolo (taglia i bordi), con gli angoli tondi.</summary>
        void Copri(Graphics g, Image img, RectangleF r, float raggio)
        {
            float scala = Math.Max(r.Width / img.Width, r.Height / img.Height);
            float w = img.Width * scala;
            float h = img.Height * scala;
            GraphicsState prima = g.Save();
            using (GraphicsPath forma = Arrotondato(r, raggio))
            {
                g.SetClip(forma, CombineMode.Intersect);
                g.DrawImage(img, r.X + (r.Width - w) / 2, r.Y + (r.Height - h) / 2, w, h);
            }
            g.Restore(prima);
        }

        void BarraScorrimento(Graphics g, RectangleF area, float totale, float scorso)
        {
            if (totale <= area.Height + 1)
            {
                return;
            }
            float x = area.Right - 4 * S;
            using (SolidBrush binario = new SolidBrush(Color.FromArgb(40, Oro)))
            {
                g.FillRectangle(binario, x, area.Top, 2 * S, area.Height);
            }
            float h = Math.Max(30 * S, area.Height * area.Height / totale);
            float y = area.Top + (area.Height - h) * (scorso / Math.Max(1, totale - area.Height));
            using (SolidBrush cursore = new SolidBrush(Color.FromArgb(170, Oro)))
            {
                g.FillRectangle(cursore, x - S, y, 4 * S, h);
            }
        }

        // ------------------------------------------------------------------------------------ la pagina

        void DisegnaPagina(Graphics g, int W, int H)
        {
            anelli.Clear();
            cuore = RectangleF.Empty;
            campoServer = RectangleF.Empty;
            float alto = (SottoTitolo + 18) * S;
            float basso = H - 140 * S;
            RectangleF area = new RectangleF(Margine * S, alto, W - 2 * Margine * S, basso - alto);
            visibile = area;
            switch (scheda)
            {
                case Scheda.Valle: PaginaValle(g, area); break;
                case Scheda.Patch: PaginaPatch(g, area); break;
                case Scheda.Roadmap: PaginaScorre(g, area, PaginaRoadmap); break;
                case Scheda.Cronache: PaginaScorre(g, area, PaginaCronache); break;
                case Scheda.Opzioni: PaginaOpzioni(g, area); break;
            }
            foreach (PointF a in anelli)
            {
                animate.Add(new RectangleF(a.X - 20 * S, a.Y - 20 * S, 40 * S, 40 * S));
            }
            if (!cuore.IsEmpty)
            {
                animate.Add(RectangleF.Inflate(cuore, 30 * S, 30 * S));
            }
        }

        delegate float Contenuto(Graphics g, float x, float y, float w);

        /// <summary>Una pagina che scorre: disegna il contenuto spostato in su di quanto si è scorso.</summary>
        void PaginaScorre(Graphics g, RectangleF area, Contenuto contenuto)
        {
            int k = (int)scheda;
            areaScorre = area;
            scorrimento[k] = Math.Max(0, Math.Min(scorrimento[k], Math.Max(0, altezzaPagina[k] - area.Height)));
            GraphicsState prima = g.Save();
            g.SetClip(area);
            visibile = area;
            float h = contenuto(g, area.X, area.Y - scorrimento[k], area.Width - 18 * S);
            g.Restore(prima);
            altezzaPagina[k] = h + 10 * S;
            if (scheda == Scheda.Roadmap && ancoraRoadmap && posizioneUltimaFatta > 0)
            {
                ancoraRoadmap = false;
                scorrimento[k] = Math.Max(0, Math.Min(posizioneUltimaFatta - 12 * S, altezzaPagina[k] - area.Height));
                paginaSporca = true;
                Invalidate();
            }
            BarraScorrimento(g, area, altezzaPagina[k], scorrimento[k]);
        }

        // ------------------------------------------------------------------------------------ La valle

        void PaginaValle(Graphics g, RectangleF area)
        {
            float wDestra = 372 * S;
            float xDestra = area.Right - wDestra;
            float alto = area.Top;
            if (LauncherNuovo)
            {
                RectangleF fascia = new RectangleF(area.X, alto, area.Width, 38 * S);
                bool sopra = Sopra("launcher");
                using (GraphicsPath forma = Arrotondato(fascia, 5 * S))
                using (LinearGradientBrush fondo = new LinearGradientBrush(fascia, Color.FromArgb(230, 92, 50, 18), Color.FromArgb(200, 40, 26, 14), LinearGradientMode.Horizontal))
                using (Pen bordo = new Pen(Color.FromArgb(sopra ? 255 : 190, Brace), Math.Max(1f, S)))
                {
                    g.FillPath(fondo, forma);
                    g.DrawPath(bordo, forma);
                }
                Rombo(g, fascia.X + 18 * S, fascia.Y + fascia.Height / 2, 5 * S, OroChiaro);
                Riga(g, "C'è un launcher nuovo, la versione " + manifesto.VersioneLauncher + " (tu hai la " + Info.Versione + "): clicca qui per scaricarlo. Le opzioni restano.",
                    Testo(16.5f), sopra ? OroChiaro : Pergamena, new RectangleF(fascia.X + 34 * S, fascia.Y, fascia.Width - 44 * S, fascia.Height), false);
                Zona(fascia, "launcher", ScaricaLauncher);
                alto += 50 * S;
            }
            RectangleF sinistra = new RectangleF(area.X, alto, xDestra - area.X - 36 * S, area.Bottom - alto);
            PaginaScorre(g, sinistra, Notizie);
            visibile = area;
            ColonnaMondo(g, xDestra, alto, wDestra, area.Bottom);
        }

        float Notizie(Graphics g, float x, float y, float w)
        {
            float inizio = y;
            y += Etichetta(g, "NOTIZIE DALLA VALLE", x, y, w, Oro);
            List<Notizia> elenco = contenuti.Notizie.Count > 0 ? contenuti.Notizie : (manifesto != null ? manifesto.Notizie : new List<Notizia>());
            if (elenco.Count == 0)
            {
                y += Scrivi(g, "Le notizie arrivano appena il launcher raggiunge il sito.", Testo(17), Secondario, x, y + 4 * S, w);
                return y - inizio;
            }
            for (int i = 0; i < elenco.Count; i++)
            {
                Notizia n = elenco[i];
                Image img = ImmagineContenuto(n.Immagine);
                y += 6 * S;
                if (img != null)
                {
                    // la notizia con l'immagine: una cartolina con il titolo sopra
                    float hi = Math.Min(220 * S, w * 0.34f);
                    RectangleF cartolina = new RectangleF(x, y, w, hi);
                    if (cartolina.Bottom > visibile.Top && cartolina.Top < visibile.Bottom)
                    {
                        Copri(g, img, cartolina, 6 * S);
                        RectangleF velo = new RectangleF(x, y + hi * 0.35f, w, hi * 0.65f);
                        using (GraphicsPath forma = Arrotondato(cartolina, 6 * S))
                        {
                            GraphicsState prima = g.Save();
                            g.SetClip(forma, CombineMode.Intersect);
                            using (LinearGradientBrush b = new LinearGradientBrush(new RectangleF(velo.X, velo.Y - 1, velo.Width, velo.Height + 2), Color.FromArgb(0, Fondo), Color.FromArgb(230, Fondo), LinearGradientMode.Vertical))
                            {
                                g.FillRectangle(b, velo);
                            }
                            g.Restore(prima);
                            using (Pen bordo = new Pen(Color.FromArgb(110, Oro), Math.Max(1f, S)))
                            {
                                g.DrawPath(bordo, forma);
                            }
                        }
                        float ty = y + hi - 48 * S;
                        float tx = x + 18 * S;
                        if (n.Etichetta.Length > 0)
                        {
                            tx += Cartellino(g, n.Etichetta.ToUpperInvariant(), tx, ty + 6 * S, Brace) + 10 * S;
                        }
                        using (SolidBrush ombra = new SolidBrush(Color.FromArgb(200, 0, 0, 0)))
                        {
                            g.DrawString(n.Titolo, Titolo(21), ombra, tx + 1.5f * S, ty + 1.5f * S);
                        }
                        Riga(g, n.Titolo, Titolo(21), OroChiaro, new RectangleF(tx, ty, x + w - tx - 10 * S, 34 * S), false);
                        if (n.Data.Length > 0)
                        {
                            Riga(g, Date.Leggibile(n.Data), Corsivo(15), Pergamena, new RectangleF(x, y + 8 * S, w - 14 * S, 22 * S), true);
                        }
                    }
                    y += hi + 10 * S;
                }
                else
                {
                    Rombo(g, x + 5 * S, y + 13 * S, 4 * S, Brace);
                    float tx = x + 18 * S;
                    Font ft = Titolo(i == 0 ? 19 : 16.5f);
                    float tw = Math.Min(Largo(g, n.Titolo, ft), w - 160 * S);
                    Riga(g, n.Titolo, ft, Pergamena, new RectangleF(tx, y, tw + 4 * S, AltezzaRiga(g, ft) + 2), false);
                    float dx = tx + tw + 10 * S;
                    if (n.Etichetta.Length > 0)
                    {
                        dx += Cartellino(g, n.Etichetta.ToUpperInvariant(), dx, y + AltezzaRiga(g, ft) * 0.18f, Brace) + 8 * S;
                    }
                    if (n.Data.Length > 0)
                    {
                        Riga(g, Date.Leggibile(n.Data), Corsivo(15), Secondario, new RectangleF(dx, y, x + w - dx, AltezzaRiga(g, ft) + 2), false);
                    }
                    y += AltezzaRiga(g, ft) + 4 * S;
                }
                float rientro = img != null ? 0 : 18 * S;
                y += Scrivi(g, n.Testo, Testo(i == 0 ? 17.5f : 16.5f), i == 0 ? Pergamena : Secondario, x + rientro, y, w - rientro);
                if (n.Link.Length > 0)
                {
                    y += 2 * S;
                    y += CollegamentoNotizia(g, n.Link, x + rientro, y, "notizia" + i);
                }
                y += 14 * S;
                if (i < elenco.Count - 1)
                {
                    using (Pen filo = new Pen(Color.FromArgb(38, Oro), Math.Max(1f, S)))
                    {
                        g.DrawLine(filo, x, y, x + w, y);
                    }
                    y += 6 * S;
                }
            }
            return y - inizio;
        }

        /// <summary>"#patch", "#roadmap", "#cronache", "#opzioni" aprono una scheda; gli indirizzi web il browser.</summary>
        float CollegamentoNotizia(Graphics g, string link, float x, float y, string chiave)
        {
            string l = link.Trim();
            string minuscolo = l.ToLowerInvariant();
            if (minuscolo == "#patch") return Collega(g, "Leggi le note della patch", x, y, chiave, delegate { CambiaScheda(Scheda.Patch); });
            if (minuscolo == "#roadmap") return Collega(g, "Guarda la roadmap", x, y, chiave, delegate { CambiaScheda(Scheda.Roadmap); });
            if (minuscolo == "#cronache") return Collega(g, "Leggi le Cronache", x, y, chiave, delegate { CambiaScheda(Scheda.Cronache); });
            if (minuscolo == "#opzioni") return Collega(g, "Apri le opzioni", x, y, chiave, delegate { CambiaScheda(Scheda.Opzioni); });
            if (minuscolo.StartsWith("http://") || minuscolo.StartsWith("https://"))
            {
                return Collega(g, "Leggi tutto", x, y, chiave, delegate { Apri(l); });
            }
            return 0;
        }

        /// <summary>Un cartellino ("PATCH", "IN CORSO"): restituisce quanto è largo.</summary>
        float Cartellino(Graphics g, string testo, float x, float y, Color colore)
        {
            Font f = Titolo(10.5f);
            SizeF m = g.MeasureString(testo, f);
            RectangleF r = new RectangleF(x, y, m.Width + 10 * S, m.Height + 2 * S);
            using (GraphicsPath forma = Arrotondato(r, 3 * S))
            using (SolidBrush fondo = new SolidBrush(Color.FromArgb(60, colore)))
            using (Pen bordo = new Pen(Color.FromArgb(200, colore), Math.Max(1f, S)))
            {
                g.FillPath(fondo, forma);
                g.DrawPath(bordo, forma);
            }
            using (SolidBrush b = new SolidBrush(Color.FromArgb(255, Math.Min(255, colore.R + 30), Math.Min(255, colore.G + 30), Math.Min(255, colore.B + 30))))
            using (StringFormat centro = new StringFormat())
            {
                centro.Alignment = StringAlignment.Center;
                centro.LineAlignment = StringAlignment.Center;
                g.DrawString(testo, f, b, r, centro);
            }
            return r.Width;
        }

        void ColonnaMondo(Graphics g, float x, float y, float w, float basso)
        {
            // 1. nella valle, adesso
            float h1 = 222 * S;
            RectangleF p1 = new RectangleF(x, y, w, h1);
            Pannello(g, p1, 205);
            Etichetta(g, "NELLA VALLE", x + 18 * S, y + 14 * S, w - 36 * S, Oro);
            double ora = OraDellaValle();
            PointF centroCielo = new PointF(x + w - 78 * S, y + 100 * S);
            bool sveglia = mondo != null && mondo.Aperto;
            Cielo(g, centroCielo, 52 * S, sveglia ? ora : -1);
            float tx = x + 18 * S;
            float tw = w - 160 * S;
            if (sveglia && ora >= 0)
            {
                using (SolidBrush b = new SolidBrush(OroChiaro))
                {
                    g.DrawString(StatoMondo.OraLeggibile(ora), Titolo(34), b, tx - 2 * S, y + 40 * S);
                }
                Riga(g, StatoMondo.Momento(ora), Corsivo(16.5f), Secondario, new RectangleF(tx, y + 88 * S, tw, 24 * S), false);
                List<string> pezzi = new List<string>();
                if (mondo.Tempo.Length > 0) pezzi.Add(mondo.Tempo.ToLowerInvariant());
                if (mondo.Stagione.Length > 0) pezzi.Add(mondo.Stagione);
                Riga(g, string.Join(" · ", pezzi.ToArray()), Testo(17.5f), Pergamena, new RectangleF(tx, y + 112 * S, tw + 30 * S, 26 * S), false);
                string coloni = mondo.Coloni == 1 ? "un colono nella valle" : (mondo.Coloni > 1 ? mondo.Coloni + " coloni nella valle" : "nessun colono in giro");
                if (mondo.Posti > 0)
                {
                    coloni += " · " + mondo.Posti + " posti";
                }
                Riga(g, coloni, Testo(16.5f), Secondario, new RectangleF(tx, y + 138 * S, w - 36 * S, 24 * S), false);
                if (mondo.Notizia.Length > 0)
                {
                    using (SolidBrush b = new SolidBrush(Color.FromArgb(235, 236, 170, 110)))
                    using (StringFormat f = new StringFormat())
                    {
                        f.Trimming = StringTrimming.EllipsisWord;
                        g.DrawString("«" + mondo.Notizia + "»", Corsivo(15.5f), b, new RectangleF(tx, y + 164 * S, w - 36 * S, 46 * S), f);
                    }
                }
            }
            else
            {
                using (SolidBrush b = new SolidBrush(OroChiaro))
                {
                    g.DrawString(mondo != null && !mondo.Aperto ? "Chiusa per lavori" : "La valle dorme", Titolo(23), b, tx - 1 * S, y + 46 * S);
                }
                string perche;
                string indirizzo = IndirizzoStato();
                if (mondo != null && !mondo.Aperto)
                {
                    perche = "Lo staff sta sistemando la valle: riapre presto. Intanto puoi aggiornare il gioco.";
                }
                else if (indirizzo.Length == 0)
                {
                    perche = "Il server non è ancora acceso. Quando lo sarà, qui vedrai l'ora della valle, il tempo e chi c'è.";
                }
                else if (!mondoChiesto)
                {
                    perche = "Chiedo al server...";
                }
                else
                {
                    perche = "Il server non risponde: forse è spento per la notte. Riprovo da solo ogni mezzo minuto.";
                }
                using (SolidBrush b = new SolidBrush(Secondario))
                using (StringFormat f = new StringFormat())
                {
                    f.Trimming = StringTrimming.EllipsisWord;
                    g.DrawString(perche, Testo(16.5f), b, new RectangleF(tx, y + 88 * S, w - 150 * S, h1 - 100 * S), f);
                }
            }
            y += h1 + 12 * S;

            // 3. la tua copia (prima si misura: le voci dalla locanda prendono quello che resta)
            float h3 = 128 * S;
            float h2 = basso - y - h3 - 12 * S;
            if (h2 >= 70 * S && contenuti.Voci.Count > 0)
            {
                RectangleF p2 = new RectangleF(x, y, w, Math.Min(h2, 104 * S));
                Pannello(g, p2, 185);
                Etichetta(g, "VOCI DALLA LOCANDA", x + 18 * S, y + 12 * S, w - 36 * S, Color.FromArgb(230, Brace));
                string v = contenuti.Voci[((voce % contenuti.Voci.Count) + contenuti.Voci.Count) % contenuti.Voci.Count];
                using (SolidBrush b = new SolidBrush(Pergamena))
                using (StringFormat f = new StringFormat())
                {
                    f.Trimming = StringTrimming.EllipsisWord;
                    g.DrawString("«" + v + "»", Corsivo(16), b, new RectangleF(x + 18 * S, y + 36 * S, w - 36 * S, p2.Height - 42 * S), f);
                }
                y += p2.Height + 12 * S;
            }
            h3 = Math.Max(100 * S, basso - y);
            RectangleF p3 = new RectangleF(x, y, w, h3);
            Pannello(g, p3, 185);
            Etichetta(g, "LA TUA COPIA", x + 18 * S, y + 12 * S, w - 36 * S, Oro);
            float ry = y + 38 * S;
            string tua = versioneInstallata.Length > 0 ? "v" + versioneInstallata : "non ancora installata";
            string sito = manifesto != null ? "v" + manifesto.Versione + Nome(manifesto) + (manifesto.Data.Length > 0 ? " · " + Date.Corta(manifesto.Data) : "") : "il sito non risponde";
            VoceInfo(g, "Installata", tua, x + 18 * S, ry, w - 36 * S);
            VoceInfo(g, "Sul sito", sito, x + 18 * S, ry + 23 * S, w - 36 * S);
            if (ry + 70 * S < y + h3)
            {
                float lx = x + 18 * S;
                float ly = ry + 50 * S;
                Font f = Corsivo(16);
                float l1 = Largo(g, "Note della patch  »", f);
                Collega(g, "Note della patch", lx, ly, "copia_patch", delegate { CambiaScheda(Scheda.Patch); });
                Collega(g, "Roadmap", lx + l1 + 18 * S, ly, "copia_roadmap", delegate { CambiaScheda(Scheda.Roadmap); });
            }
        }

        void VoceInfo(Graphics g, string chiave, string valore, float x, float y, float w)
        {
            Font f = Testo(16);
            float kw = 92 * S;
            Riga(g, chiave, Corsivo(15), Secondario, new RectangleF(x, y, kw, 22 * S), false);
            Riga(g, valore, f, Pergamena, new RectangleF(x + kw, y, w - kw, 22 * S), false);
        }

        /// <summary>Il cielo della valle: un arco con il sole di giorno (6-18) e la luna di notte. ora &lt; 0: la valle dorme.</summary>
        void Cielo(Graphics g, PointF c, float r, double ora)
        {
            // l'orizzonte e l'arco del cielo
            using (Pen orizzonte = new Pen(Color.FromArgb(120, Oro), Math.Max(1f, S)))
            {
                g.DrawLine(orizzonte, c.X - r - 10 * S, c.Y, c.X + r + 10 * S, c.Y);
            }
            using (Pen arco = new Pen(Color.FromArgb(70, Oro), Math.Max(1f, S)))
            {
                arco.DashStyle = DashStyle.Dot;
                g.DrawArc(arco, c.X - r, c.Y - r, 2 * r, 2 * r, 180, 180);
            }
            // i monti in basso, come nello sfondo
            using (SolidBrush monti = new SolidBrush(Color.FromArgb(200, 30, 22, 20)))
            {
                g.FillPolygon(monti, new PointF[]
                {
                    new PointF(c.X - r - 10 * S, c.Y), new PointF(c.X - r * 0.6f, c.Y - r * 0.28f), new PointF(c.X - r * 0.3f, c.Y - r * 0.1f),
                    new PointF(c.X + r * 0.1f, c.Y - r * 0.38f), new PointF(c.X + r * 0.55f, c.Y - r * 0.12f), new PointF(c.X + r * 0.8f, c.Y - r * 0.22f),
                    new PointF(c.X + r + 10 * S, c.Y)
                });
            }
            if (ora < 0)
            {
                // la valle dorme: la luna bassa e qualche stella
                Luna(g, c.X + r * 0.45f, c.Y - r * 0.62f, 9 * S, 120);
                Stella(g, c.X - r * 0.5f, c.Y - r * 0.75f, 70);
                Stella(g, c.X - r * 0.1f, c.Y - r * 0.95f, 50);
                Stella(g, c.X + r * 0.05f, c.Y - r * 0.55f, 40);
                return;
            }
            // il sole dalle 6 alle 19 (così al tramonto si vede basso a ovest), la luna dalle 19 alle 6
            bool giorno = ora >= 6 && ora < 19;
            double t = giorno ? (ora - 6) / 13.0 : ((ora + 5) % 24) / 11.0;
            double angolo = Math.PI * (1 - t);
            float sx = c.X + (float)(Math.Cos(angolo) * r);
            float sy = c.Y - (float)(Math.Sin(angolo) * r);
            if (giorno)
            {
                for (int i = 3; i >= 1; i--)
                {
                    float a = (6 + i * 5) * S;
                    using (SolidBrush alone = new SolidBrush(Color.FromArgb(28, Brace)))
                    {
                        g.FillEllipse(alone, sx - a, sy - a, 2 * a, 2 * a);
                    }
                }
                using (SolidBrush sole = new SolidBrush(Color.FromArgb(250, 228, 150)))
                {
                    g.FillEllipse(sole, sx - 7 * S, sy - 7 * S, 14 * S, 14 * S);
                }
            }
            else
            {
                Luna(g, sx, sy, 9 * S, 230);
                Stella(g, c.X - r * 0.55f, c.Y - r * 0.8f, 90);
                Stella(g, c.X + r * 0.6f, c.Y - r * 0.9f, 70);
                Stella(g, c.X - r * 0.05f, c.Y - r * 0.6f, 60);
            }
        }

        void Luna(Graphics g, float cx, float cy, float r, int alfa)
        {
            using (SolidBrush luce = new SolidBrush(Color.FromArgb(alfa, 236, 226, 200)))
            {
                g.FillEllipse(luce, cx - r, cy - r, 2 * r, 2 * r);
            }
            // la falce: un cerchio scuro spostato
            using (SolidBrush ombra = new SolidBrush(Color.FromArgb(Math.Min(255, alfa + 20), Pannelli)))
            {
                g.FillEllipse(ombra, cx - r + r * 0.55f, cy - r - r * 0.25f, 2 * r, 2 * r);
            }
        }

        void Stella(Graphics g, float x, float y, int alfa)
        {
            using (SolidBrush b = new SolidBrush(Color.FromArgb(alfa, Pergamena)))
            {
                g.FillEllipse(b, x - 1.2f * S, y - 1.2f * S, 2.4f * S, 2.4f * S);
            }
        }

        // ------------------------------------------------------------------------------------ Note delle patch

        void PaginaPatch(Graphics g, RectangleF area)
        {
            List<VersionePatch> elenco = contenuti.Patch;
            if (elenco.Count == 0)
            {
                Scrivi(g, "Le note delle patch arrivano appena il launcher raggiunge il sito.", Testo(18), Secondario, area.X, area.Y, area.Width);
                return;
            }
            versioneScelta = Math.Max(0, Math.Min(versioneScelta, elenco.Count - 1));
            // a sinistra le versioni
            float lw = 250 * S;
            float y = area.Y;
            y += Etichetta(g, "VERSIONI", area.X, y, lw, Oro);
            for (int i = 0; i < elenco.Count; i++)
            {
                VersionePatch v = elenco[i];
                RectangleF r = new RectangleF(area.X, y, lw, 62 * S);
                if (r.Bottom > area.Bottom)
                {
                    break;
                }
                bool scelta = i == versioneScelta;
                bool sopra = Sopra("versione" + i);
                if (scelta || sopra)
                {
                    using (GraphicsPath forma = Arrotondato(r, 5 * S))
                    using (SolidBrush fondo = new SolidBrush(Color.FromArgb(scelta ? 210 : 110, scelta ? Pannelli : PulsanteSopra)))
                    {
                        g.FillPath(fondo, forma);
                    }
                }
                if (scelta)
                {
                    using (SolidBrush barra = new SolidBrush(Oro))
                    {
                        g.FillRectangle(barra, r.X, r.Y + 8 * S, 3 * S, r.Height - 16 * S);
                    }
                }
                Riga(g, "v" + v.Versione, Titolo(18), scelta ? OroChiaro : Pergamena, new RectangleF(r.X + 16 * S, r.Y + 6 * S, lw - 30 * S, 28 * S), false);
                if (v.Versione == versioneInstallata)
                {
                    float vw = Largo(g, "v" + v.Versione, Titolo(18));
                    Cartellino(g, "LA TUA", r.X + 22 * S + vw, r.Y + 12 * S, Verde);
                }
                string sotto = v.Nome + (v.Data.Length > 0 ? " · " + Date.Corta(v.Data) : "");
                Riga(g, sotto, Corsivo(15), Secondario, new RectangleF(r.X + 16 * S, r.Y + 34 * S, lw - 30 * S, 22 * S), false);
                int quale = i;
                Zona(r, "versione" + i, delegate
                {
                    versioneScelta = quale;
                    scorrimento[(int)Scheda.Patch] = 0;
                    Ridisegna();
                });
                y += r.Height + 4 * S;
            }
            // a destra le note della versione scelta
            float dx = area.X + lw + 34 * S;
            RectangleF destra = new RectangleF(dx, area.Y, area.Right - dx, area.Height);
            PaginaScorre(g, destra, NotePatch);
        }

        float NotePatch(Graphics g, float x, float y, float w)
        {
            VersionePatch v = contenuti.Patch[versioneScelta];
            float inizio = y;
            string titolo = "v" + v.Versione + (v.Nome.Length > 0 ? " · " + v.Nome : "");
            Font ft = Titolo(30);
            RectangleF tr = new RectangleF(x, y, w, AltezzaRiga(g, ft) + 4 * S);
            using (LinearGradientBrush oro = new LinearGradientBrush(new RectangleF(tr.X, tr.Y - 1, tr.Width, tr.Height + 2), OroChiaro, Color.FromArgb(170, 128, 40), LinearGradientMode.Vertical))
            {
                g.DrawString(titolo, ft, oro, x - 2 * S, y);
            }
            y += AltezzaRiga(g, ft) + 2 * S;
            if (v.Data.Length > 0)
            {
                y += Scrivi(g, "Uscita il " + Date.Leggibile(v.Data), Corsivo(16), Secondario, x, y, w);
            }
            y += 10 * S;
            if (v.Sommario.Length > 0)
            {
                y += Scrivi(g, v.Sommario, Testo(18), Pergamena, x, y, w) + 14 * S;
            }
            foreach (GruppoPatch gr in v.Gruppi)
            {
                Color colore = gr.Titolo.StartsWith("Sistem", StringComparison.OrdinalIgnoreCase) ? Oro : Brace;
                y += Etichetta(g, gr.Titolo.ToUpperInvariant(), x, y, w, colore);
                y += Elenco(g, gr.Voci, Testo(16.5f), Pergamena, colore, x, y, w, gr.Voci.Count > 5 ? 2 : 1) + 18 * S;
            }
            return y - inizio;
        }

        // ------------------------------------------------------------------------------------ Roadmap

        float PaginaRoadmap(Graphics g, float x, float y, float w)
        {
            float inizio = y;
            if (contenuti.RoadmapIntro.Length > 0)
            {
                y += Scrivi(g, contenuti.RoadmapIntro, Corsivo(17.5f), Secondario, x, y, w) + 16 * S;
            }
            float linea = x + 14 * S;
            float tx = x + 44 * S;
            float tw = w - 44 * S;
            List<PointF> nodi = new List<PointF>();
            List<string> stati = new List<string>();
            posizioneUltimaFatta = -1;
            foreach (Tappa t in contenuti.Tappe)
            {
                string s = t.Stato;
                if (s == "fatta")
                {
                    posizioneUltimaFatta = y - inizio;
                }
                bool fatta = s == "fatta";
                bool corso = s == "in_corso";
                bool prossima = s == "prossima";
                Font ft = Titolo(fatta ? 19 : 21);
                Color ct = corso ? OroChiaro : (fatta || prossima ? Pergamena : Secondario);
                string titolo = (t.Versione.Length > 0 ? "v" + t.Versione + " · " : "") + t.Nome;
                float lt = Largo(g, titolo, ft);
                Riga(g, titolo, ft, ct, new RectangleF(tx, y, Math.Min(lt + 4 * S, tw - 150 * S), AltezzaRiga(g, ft) + 2), false);
                nodi.Add(new PointF(linea, y + AltezzaRiga(g, ft) * 0.52f));
                stati.Add(s);
                string cartello = fatta ? "FATTA" + (t.Data.Length > 0 ? " · " + Date.Corta(t.Data).ToUpperInvariant() : "") : (corso ? "IN CORSO" : (prossima ? "PROSSIMA" : "PIÙ AVANTI"));
                Color cc = fatta ? Verde : (corso ? Brace : (prossima ? Oro : Color.FromArgb(150, 140, 120)));
                Cartellino(g, cartello, tx + Math.Min(lt, tw - 150 * S) + 14 * S, y + AltezzaRiga(g, ft) * 0.2f, cc);
                y += AltezzaRiga(g, ft) + 4 * S;
                if (t.Sommario.Length > 0)
                {
                    y += Scrivi(g, t.Sommario, Corsivo(16.5f), corso ? Pergamena : Secondario, tx, y, tw) + 6 * S;
                }
                int colonne = tw > 900 * S ? 3 : 2;
                y += Elenco(g, t.Voci, Testo(fatta ? 15 : 16), fatta ? Secondario : Pergamena, fatta ? Color.FromArgb(160, Oro) : (corso ? Brace : Oro), tx, y, tw, t.Voci.Count > 4 ? colonne : 1);
                y += 26 * S;
            }
            // la linea del tempo, poi i nodi sopra
            if (nodi.Count > 1)
            {
                using (Pen p = new Pen(Color.FromArgb(90, Oro), 2 * S))
                {
                    g.DrawLine(p, linea, nodi[0].Y, linea, nodi[nodi.Count - 1].Y);
                }
            }
            for (int i = 0; i < nodi.Count; i++)
            {
                PointF n = nodi[i];
                float r = 8 * S;
                if (stati[i] == "fatta")
                {
                    using (SolidBrush b = new SolidBrush(Oro))
                    {
                        g.FillEllipse(b, n.X - r, n.Y - r, 2 * r, 2 * r);
                    }
                    Spunta(g, n.X, n.Y, 4 * S, Fondo, 1.8f * S);
                }
                else if (stati[i] == "in_corso")
                {
                    using (SolidBrush b = new SolidBrush(Brace))
                    {
                        g.FillEllipse(b, n.X - r, n.Y - r, 2 * r, 2 * r);
                    }
                    if (visibile.Contains(n))
                    {
                        anelli.Add(n);
                    }
                }
                else
                {
                    bool prossima = stati[i] == "prossima";
                    using (SolidBrush b = new SolidBrush(Fondo))
                    using (Pen p = new Pen(prossima ? Oro : Color.FromArgb(130, Secondario), 2 * S))
                    {
                        float rr = prossima ? r : r * 0.75f;
                        g.FillEllipse(b, n.X - rr, n.Y - rr, 2 * rr, 2 * rr);
                        g.DrawEllipse(p, n.X - rr, n.Y - rr, 2 * rr, 2 * rr);
                    }
                }
            }
            if (contenuti.RoadmapDopo.Length > 0)
            {
                y += Etichetta(g, "E POI", x, y, w, Oro);
                y += Scrivi(g, contenuti.RoadmapDopo, Testo(17), Pergamena, x, y, w);
            }
            if (contenuti.Tappe.Count == 0 && contenuti.RoadmapIntro.Length == 0)
            {
                y += Scrivi(g, "La roadmap arriva appena il launcher raggiunge il sito.", Testo(18), Secondario, x, y, w);
            }
            return y - inizio;
        }

        // ------------------------------------------------------------------------------------ Cronache

        float PaginaCronache(Graphics g, float x, float y, float w)
        {
            float inizio = y;
            // la testata con l'orso
            Image img = ImmagineContenuto("cronache.jpg");
            float hi = img != null ? 190 * S : 70 * S;
            if (img != null)
            {
                RectangleF r = new RectangleF(x, y, w, hi);
                Copri(g, img, r, 6 * S);
                using (GraphicsPath forma = Arrotondato(r, 6 * S))
                using (Pen bordo = new Pen(Color.FromArgb(110, Oro), Math.Max(1f, S)))
                {
                    g.DrawPath(bordo, forma);
                }
            }
            float ty = y + hi - 66 * S;
            using (SolidBrush ombra = new SolidBrush(Color.FromArgb(200, 0, 0, 0)))
            {
                g.DrawString("CRONACHE DI VAL D'ORSO", Titolo(28), ombra, x + 24 * S + 2 * S, ty + 2 * S);
            }
            using (SolidBrush oro = new SolidBrush(OroChiaro))
            {
                g.DrawString("CRONACHE DI VAL D'ORSO", Titolo(28), oro, x + 24 * S, ty);
            }
            Riga(g, "Anno 312 dopo il Crepuscolo. Quello che si racconta nella valle, davanti al fuoco della locanda.", Corsivo(16.5f), Pergamena, new RectangleF(x + 26 * S, ty + 38 * S, w - 50 * S, 24 * S), false);
            y += hi + 22 * S;

            // i racconti a sinistra, il Cuore a destra
            float cw = w - 330 * S;
            float yRacconti = y;
            foreach (Capitolo c in contenuti.Capitoli)
            {
                y += Etichetta(g, c.Titolo.ToUpperInvariant(), x, y, cw, Oro);
                y += Scrivi(g, c.Testo, Testo(17.5f), Pergamena, x, y, cw) + 18 * S;
            }
            float bx = x + cw + 30 * S;
            float bw = w - cw - 30 * S;
            const string battito = "Nel Tempio del Cuore batte un frammento del Cuore: due colpi e una pausa, da più di trecento anni. Padre Anselmo dice che finché batte la valle è viva. Bruno, alla locanda, giura che una notte ha saltato un battito.";
            RectangleF box = new RectangleF(bx, yRacconti, bw, 132 * S + Alto(g, battito, Corsivo(15.5f), bw - 36 * S));
            Pannello(g, box, 190);
            Etichetta(g, "IL CUORE", bx + 18 * S, box.Y + 14 * S, bw - 36 * S, Brace);
            cuore = new RectangleF(bx + bw / 2 - 22 * S, box.Y + 56 * S, 44 * S, 44 * S);
            if (!visibile.IntersectsWith(cuore))
            {
                cuore = RectangleF.Empty;
            }
            Scrivi(g, battito, Corsivo(15.5f), Secondario, bx + 18 * S, box.Y + 118 * S, bw - 36 * S);
            y = Math.Max(y, box.Bottom + 20 * S);

            // l'alfabeto delle rune
            y += Etichetta(g, "L'ALFABETO DELLE RUNE", x, y, w, Oro);
            if (contenuti.RuneIntro.Length > 0)
            {
                y += Scrivi(g, contenuti.RuneIntro, Testo(17), Pergamena, x, y, w) + 12 * S;
            }
            string[] gruppi = { "Gli elementi: stanno sulla linea di base", "I verbi: sono un gesto", "Il custode" };
            int[][] quali = { new int[] { 0, 1, 2, 3, 4 }, new int[] { 5, 6, 7, 8, 9 }, new int[] { 10 } };
            float carta = (w - 4 * 14 * S) / 5;
            for (int k = 0; k < 3; k++)
            {
                y += Scrivi(g, gruppi[k], Corsivo(16), Secondario, x, y, w) + 6 * S;
                for (int j = 0; j < quali[k].Length; j++)
                {
                    int i = quali[k][j];
                    RectangleF c = new RectangleF(x + j * (carta + 14 * S), y, carta, 168 * S);
                    if (c.Bottom > visibile.Top && c.Top < visibile.Bottom)
                    {
                        Pannello(g, c, 200);
                        DisegnaRuna(g, i, c.X + c.Width / 2, c.Y + 62 * S, 36 * S, Oro, 3.2f * S, true);
                        using (StringFormat centro = new StringFormat())
                        using (SolidBrush b1 = new SolidBrush(OroChiaro))
                        using (SolidBrush b2 = new SolidBrush(Secondario))
                        {
                            centro.Alignment = StringAlignment.Center;
                            g.DrawString(Rune.Nomi[i], Titolo(16.5f), b1, new RectangleF(c.X, c.Y + 110 * S, c.Width, 26 * S), centro);
                            g.DrawString(Rune.Significati[i], Corsivo(15), b2, new RectangleF(c.X + 6 * S, c.Y + 134 * S, c.Width - 12 * S, 30 * S), centro);
                        }
                    }
                }
                if (k == 2)
                {
                    float ux = x + carta + 24 * S;
                    Scrivi(g, "Ur è la runa dell'Orso, il custode, e della porta. Si scrive da sola, sopra le altre: gli Antichi la incidevano sopra le porte che non si aprono. Nel gioco la trovi nelle rovine e in fondo alla grotta est.",
                        Testo(17), Pergamena, ux, y + 12 * S, w - (ux - x));
                }
                y += 168 * S + 14 * S;
            }
            // le parole
            if (contenuti.Parole.Count > 0)
            {
                y += 6 * S;
                y += Etichetta(g, "LE PAROLE CON UN NOME", x, y, w, Oro);
                y += Scrivi(g, "Un incantesimo è un elemento e un verbo; una terza runa lo piega. Queste sono le parole che le Cronache ricordano.", Corsivo(16), Secondario, x, y, w) + 10 * S;
                int colonne = 3;
                float pw = (w - (colonne - 1) * 20 * S) / colonne;
                for (int i = 0; i < contenuti.Parole.Count; i++)
                {
                    int col = i % colonne;
                    if (col == 0 && i > 0)
                    {
                        y += 50 * S;
                    }
                    ParolaRuna p = contenuti.Parole[i];
                    float px = x + col * (pw + 20 * S);
                    float larga = DisegnaParola(g, p.Rune, px + 4 * S, y + 20 * S, 12 * S, Oro, 2 * S);
                    float nx = px + Math.Max(larga, 3 * 2.9f * 12 * S - 0.9f * 12 * S) + 16 * S;
                    Riga(g, p.Nome, Titolo(14), Pergamena, new RectangleF(nx, y + 2 * S, px + pw - nx, 22 * S), false);
                    Riga(g, p.Rune, Corsivo(14.5f), Secondario, new RectangleF(nx, y + 22 * S, px + pw - nx, 22 * S), false);
                }
                y += 60 * S;
            }
            // i volti
            if (contenuti.Volti.Count > 0)
            {
                y += Etichetta(g, "I VOLTI DELLA VALLE", x, y, w, Oro);
                if (contenuti.VoltiIntro.Length > 0)
                {
                    y += Scrivi(g, contenuti.VoltiIntro, Corsivo(16), Secondario, x, y, w) + 10 * S;
                }
                int colonne = 3;
                float vw = (w - (colonne - 1) * 16 * S) / colonne;
                float rigaAlta = 0;
                for (int i = 0; i < contenuti.Volti.Count; i++)
                {
                    int col = i % colonne;
                    if (col == 0 && i > 0)
                    {
                        y += rigaAlta + 14 * S;
                        rigaAlta = 0;
                    }
                    if (col == 0)
                    {
                        // la riga è alta quanto la scheda più lunga
                        for (int j = i; j < Math.Min(i + colonne, contenuti.Volti.Count); j++)
                        {
                            rigaAlta = Math.Max(rigaAlta, 64 * S + Alto(g, contenuti.Volti[j].Testo, Testo(15.5f), vw - 32 * S));
                        }
                    }
                    Volto v = contenuti.Volti[i];
                    RectangleF c = new RectangleF(x + col * (vw + 16 * S), y, vw, rigaAlta);
                    if (c.Bottom > visibile.Top && c.Top < visibile.Bottom)
                    {
                        Pannello(g, c, 185);
                        Riga(g, v.Nome, Titolo(16), OroChiaro, new RectangleF(c.X + 16 * S, c.Y + 12 * S, c.Width - 32 * S, 24 * S), false);
                        Riga(g, v.Ruolo, Corsivo(15), Color.FromArgb(240, 230, 160, 100), new RectangleF(c.X + 16 * S, c.Y + 36 * S, c.Width - 32 * S, 22 * S), false);
                        Scrivi(g, v.Testo, Testo(15.5f), Secondario, c.X + 16 * S, c.Y + 60 * S, c.Width - 32 * S);
                    }
                }
                y += rigaAlta + 10 * S;
            }
            return y - inizio;
        }

        // ------------------------------------------------------------------------------------ Opzioni

        void PaginaOpzioni(Graphics g, RectangleF area)
        {
            float x = area.X;
            float y = area.Y;
            float w = 640 * S;
            bool libero = stato != StatoLauncher.Lavoro;

            y += Etichetta(g, "LA CARTELLA DEL GIOCO", x, y, w, Oro);
            RectangleF campo = new RectangleF(x, y, w - 130 * S, 38 * S);
            Campo(g, campo);
            using (SolidBrush b = new SolidBrush(Pergamena))
            using (StringFormat f = new StringFormat(StringFormatFlags.NoWrap))
            {
                f.Trimming = StringTrimming.EllipsisPath;
                f.LineAlignment = StringAlignment.Center;
                g.DrawString(config.PercorsoGioco, Testo(16), b, new RectangleF(campo.X + 12 * S, campo.Y, campo.Width - 20 * S, campo.Height), f);
            }
            Bottoncino(g, new RectangleF(campo.Right + 12 * S, campo.Y, 118 * S, campo.Height), "Scegli...", "cartella", ScegliCartella, libero);
            y += campo.Height + 6 * S;
            y += Scrivi(g, "Se la cambi, il gioco già installato resta dov'è: il launcher lo installa nella cartella nuova (e lì scarica gli aggiornamenti).", Corsivo(15), Secondario, x, y, w) + 18 * S;

            y += Etichetta(g, "L'INDIRIZZO DEL SERVER", x, y, w, Oro);
            campoServer = new RectangleF(x, y, w - 130 * S, 38 * S);
            Campo(g, campoServer);
            Bottoncino(g, new RectangleF(campoServer.Right + 12 * S, campoServer.Y, 118 * S, campoServer.Height), "Usa il sito", "server_vuoto", delegate
            {
                casellaServer.Text = "";
                SalvaServer();
            }, config.Server.Length > 0);
            y += campoServer.Height + 6 * S;
            string delSito = manifesto != null && manifesto.Server.Length > 0 ? manifesto.Server : "non ancora indicato";
            y += Scrivi(g, "Vuoto: si usa quello scritto sul sito (adesso: " + delSito + "). Scrivilo solo per provare un altro server, per esempio 192.168.1.20:7777, e premi Invio.", Corsivo(15), Secondario, x, y, w) + 18 * S;

            y += Etichetta(g, "COME SI COMPORTA", x, y, w, Oro);
            y += Interruttore(g, "Chiudi il launcher quando parte il gioco", config.ChiudiDopoAvvio, x, y, w, "chiudi", delegate
            {
                config.ChiudiDopoAvvio = !config.ChiudiDopoAvvio;
                SalvaOpzioni();
            });
            y += Interruttore(g, "Usa i caratteri di Windows (se al posto delle lettere vedi dei quadratini)", !config.CaratteriPropri, x, y, w, "caratteri", delegate
            {
                config.CaratteriPropri = !config.CaratteriPropri;
                SalvaOpzioni();
                CaricaCaratteri();
                CreaPulsanti();
                Ridisegna();
            });

            // a destra: i controlli e le informazioni
            float dx = x + w + 50 * S;
            float dw = area.Right - dx;
            float dy = area.Y;
            dy += Etichetta(g, "CONTROLLI", dx, dy, dw, Oro);
            bool puoiVerificare = libero && Installato && manifesto != null && manifesto.File.Count > 0;
            Bottoncino(g, new RectangleF(dx, dy, dw, 38 * S), "Controlla i file del gioco", "verifica", Verifica, puoiVerificare);
            dy += 46 * S;
            Bottoncino(g, new RectangleF(dx, dy, dw, 38 * S), "Ricarica notizie e note dal sito", "ricarica", delegate
            {
                AggiornaContenuti();
                if (stato != StatoLauncher.Lavoro && stato != StatoLauncher.InPausa)
                {
                    Controlla();
                }
            }, libero);
            dy += 46 * S;
            float meta = (dw - 10 * S) / 2;
            Bottoncino(g, new RectangleF(dx, dy, meta, 38 * S), "Cartella del gioco", "apri_gioco", ApriCartellaGioco, true);
            Bottoncino(g, new RectangleF(dx + meta + 10 * S, dy, meta, 38 * S), "Il registro", "apri_registro", delegate { Apri(Registro.File); }, true);
            dy += 58 * S;

            dy += Etichetta(g, "INFORMAZIONI", dx, dy, dw, Oro);
            List<string[]> info = new List<string[]>();
            info.Add(new string[] { "Launcher", Info.Versione + (LauncherNuovo ? " (sul sito c'è la " + manifesto.VersioneLauncher + ")" : "") });
            info.Add(new string[] { "Gioco", versioneInstallata.Length > 0 ? "v" + versioneInstallata : "non installato" });
            info.Add(new string[] { "Sul sito", manifesto != null ? "v" + manifesto.Versione + Nome(manifesto) + (manifesto.Parti.Count > 1 ? " · " + manifesto.Parti.Count + " parti" : "") + (manifesto.BytePacchetto > 0 ? " · " + Misure.Byte(manifesto.BytePacchetto) : "") : "non raggiunto" });
            info.Add(new string[] { "Manifesto", config.Manifesto });
            string notizie;
            info.Add(new string[] { "Notizie", contenuti.Origini.TryGetValue("notizie.json", out notizie) ? notizie : "nessuna" });
            string patch;
            info.Add(new string[] { "Note", contenuti.Origini.TryGetValue("patch.json", out patch) ? patch : "nessuna" });
            foreach (string[] voce in info)
            {
                if (dy + 22 * S > area.Bottom)
                {
                    break;
                }
                Riga(g, voce[0], Corsivo(15), Secondario, new RectangleF(dx, dy, 96 * S, 22 * S), false);
                Riga(g, voce[1], Testo(15.5f), Pergamena, new RectangleF(dx + 96 * S, dy, dw - 96 * S, 22 * S), false);
                dy += 24 * S;
            }
        }

        void Campo(Graphics g, RectangleF r)
        {
            using (GraphicsPath forma = Arrotondato(r, 4 * S))
            using (SolidBrush fondo = new SolidBrush(Color.FromArgb(255, 30, 24, 19)))
            using (Pen bordo = new Pen(Color.FromArgb(120, Oro), Math.Max(1f, S)))
            {
                g.FillPath(fondo, forma);
                g.DrawPath(bordo, forma);
            }
        }

        float Interruttore(Graphics g, string testo, bool acceso, float x, float y, float w, string chiave, Action azione)
        {
            float lato = 22 * S;
            bool sopra = Sopra(chiave);
            RectangleF box = new RectangleF(x, y + 3 * S, lato, lato);
            using (GraphicsPath forma = Arrotondato(box, 4 * S))
            using (SolidBrush fondo = new SolidBrush(acceso ? Oro : Color.FromArgb(255, 30, 24, 19)))
            using (Pen bordo = new Pen(sopra ? OroChiaro : Color.FromArgb(170, Oro), 1.4f * S))
            {
                g.FillPath(fondo, forma);
                g.DrawPath(bordo, forma);
            }
            if (acceso)
            {
                Spunta(g, box.X + lato / 2, box.Y + lato / 2, 6 * S, Fondo, 2.2f * S);
            }
            Font f = Testo(17);
            float h = Math.Max(lato + 6 * S, Alto(g, testo, f, w - lato - 14 * S));
            Scrivi(g, testo, f, sopra ? OroChiaro : Pergamena, x + lato + 12 * S, y + 2 * S, w - lato - 14 * S);
            Zona(new RectangleF(x, y, w, h), chiave, azione);
            return h + 8 * S;
        }

        void ScegliCartella()
        {
            using (FolderBrowserDialog scelta = new FolderBrowserDialog())
            {
                scelta.Description = "Dove installo Valdorso? (Se la cartella non è vuota, ci metto dentro una cartella \"Valdorso\".)";
                scelta.ShowNewFolderButton = true;
                try
                {
                    string ora = config.PercorsoGioco;
                    scelta.SelectedPath = Directory.Exists(ora) ? ora : Cartelle.Base;
                }
                catch (Exception)
                {
                }
                if (scelta.ShowDialog(this) != DialogResult.OK || string.IsNullOrEmpty(scelta.SelectedPath))
                {
                    return;
                }
                string cartella = Aggiornatore.CartellaSicura(scelta.SelectedPath);
                config.CartellaGioco = cartella;
                SalvaOpzioni();
                Registro.Scrivi("cartella del gioco: " + cartella);
                if (stato != StatoLauncher.Lavoro && stato != StatoLauncher.InPausa)
                {
                    Controlla();
                }
            }
        }

        void PosizionaCaselle()
        {
            if (casellaServer == null)
            {
                return;
            }
            if (scheda == Scheda.Opzioni && !campoServer.IsEmpty)
            {
                int h = casellaServer.PreferredHeight;
                casellaServer.Location = new Point((int)(campoServer.X + 12 * S), (int)(campoServer.Y + (campoServer.Height - h) / 2));
                casellaServer.Width = (int)(campoServer.Width - 24 * S);
                if (!casellaServer.Visible)
                {
                    casellaServer.Visible = true;
                }
            }
            else if (casellaServer.Visible)
            {
                casellaServer.Visible = false;
            }
        }

        // ------------------------------------------------------------------------------------ quello che si muove

        void DisegnaAnimazioni(Graphics g)
        {
            if (anelli.Count == 0 && cuore.IsEmpty)
            {
                return;
            }
            // solo dentro la parte che scorre: niente aloni sopra il titolo o sulla barra
            GraphicsState prima = g.Save();
            g.SetClip(areaScorre, CombineMode.Intersect);
            DisegnaAnimazioniDentro(g);
            g.Restore(prima);
        }

        void DisegnaAnimazioniDentro(Graphics g)
        {
            float b = Battito();
            foreach (PointF a in anelli)
            {
                float r = (10 + 8 * b) * S;
                using (Pen p = new Pen(Color.FromArgb((int)(40 + 150 * b), Brace), 2 * S))
                {
                    g.DrawEllipse(p, a.X - r, a.Y - r, 2 * r, 2 * r);
                }
            }
            if (!cuore.IsEmpty)
            {
                float cx = cuore.X + cuore.Width / 2;
                float cy = cuore.Y + cuore.Height / 2;
                for (int i = 4; i >= 1; i--)
                {
                    float r = (8 + i * 5 + 8 * b) * S;
                    using (SolidBrush alone = new SolidBrush(Color.FromArgb((int)((14 + 26 * b) * (5 - i) / 2.5f), Brace)))
                    {
                        g.FillEllipse(alone, cx - r, cy - r, 2 * r, 2 * r);
                    }
                }
                float rc = (11 + 4 * b) * S;
                using (GraphicsPath forma = new GraphicsPath())
                {
                    forma.AddEllipse(cx - rc, cy - rc, 2 * rc, 2 * rc);
                    using (PathGradientBrush brace = new PathGradientBrush(forma))
                    {
                        brace.CenterColor = Color.FromArgb(255, 255, 226, 160);
                        brace.SurroundColors = new Color[] { Color.FromArgb(255, 200, 90, 30) };
                        g.FillPath(brace, forma);
                    }
                }
            }
        }
    }
}
