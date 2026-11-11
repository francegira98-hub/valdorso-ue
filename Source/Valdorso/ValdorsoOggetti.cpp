// Valdorso - Gli oggetti della valle (vedi il .h). Scritto da Claude il 08/11/2026.

#include "ValdorsoOggetti.h"
#include "ValdorsoRegistro.h"

namespace
{
	using FLineeOggetto = TArray<TArray<FVector2D>>;

	FValdorsoTipoOggetto OggCibo(const TCHAR* Id, const TCHAR* Nome, const TCHAR* Descrizione, float Peso, int32 Pila,
		float Salute, float Stamina)
	{
		FValdorsoTipoOggetto T;
		T.Id = Id;
		T.Nome = Nome;
		T.Descrizione = Descrizione;
		T.Peso = Peso;
		T.Pila = Pila;
		T.Uso = EValdorsoUso::Mangia;
		T.Salute = Salute;
		T.Stamina = Stamina;
		return T;
	}

	FValdorsoTipoOggetto OggSemplice(const TCHAR* Id, const TCHAR* Nome, const TCHAR* Descrizione, float Peso, int32 Pila = 1)
	{
		FValdorsoTipoOggetto T;
		T.Id = Id;
		T.Nome = Nome;
		T.Descrizione = Descrizione;
		T.Peso = Peso;
		T.Pila = Pila;
		return T;
	}

	FValdorsoTipoOggetto OggRicordo(const TCHAR* Id, const TCHAR* Nome, const TCHAR* Descrizione, float Peso)
	{
		FValdorsoTipoOggetto T = OggSemplice(Id, Nome, Descrizione, Peso);
		T.bRicordo = true;
		return T;
	}

	FValdorsoTipoOggetto OggDaLeggere(FValdorsoTipoOggetto T, const TCHAR* Lettura)
	{
		T.Uso = EValdorsoUso::Leggi;
		T.Lettura = Lettura;
		return T;
	}

	TArray<FValdorsoTipoOggetto> OggCreaCatalogo()
	{
		TArray<FValdorsoTipoOggetto> C;

		// --- Da mangiare, bere e curarsi -----------------------------------------------------------------------
		C.Add(OggCibo(TEXT("Pane_Nero"), TEXT("Pane nero"),
			TEXT("Pane di segale, duro il giusto. Sazia e rimette in forze."), 0.3f, 10, 10.f, 30.f));
		C.Add(OggCibo(TEXT("Formaggio"), TEXT("Formaggio di malga"),
			TEXT("Una forma piccola, stagionata in quota. Si taglia col coltello e dura settimane."), 0.4f, 5, 15.f, 20.f));
		C.Add(OggCibo(TEXT("Mela"), TEXT("Mela"),
			TEXT("Una mela rossa degli orti del villaggio, un po' ammaccata."), 0.2f, 10, 5.f, 15.f));
		{
			FValdorsoTipoOggetto Borraccia = OggSemplice(TEXT("Borraccia"), TEXT("Borraccia"),
				TEXT("Di pelle, cucita a mano: cinque sorsi. Si riempie entrando nell'acqua del lago o del fiume."), 0.5f);
			Borraccia.Uso = EValdorsoUso::Bevi;
			Borraccia.Stamina = 20.f;
			Borraccia.Equilibrio = 10.f;
			Borraccia.Cariche = 5;
			C.Add(Borraccia);
		}
		{
			FValdorsoTipoOggetto Erbe = OggSemplice(TEXT("Erbe_Palude"), TEXT("Erbe della palude"),
				TEXT("Menta d'acqua e achillea, legate con un filo d'erba da Nonna Edda. Per le ferite piccole."), 0.1f, 10);
			Erbe.Uso = EValdorsoUso::Cura;
			Erbe.Salute = 25.f;
			C.Add(Erbe);
		}

		// --- Quello che ogni colono porta -------------------------------------------------------------------
		C.Add(OggSemplice(TEXT("Coltello"), TEXT("Coltello"),
			TEXT("Un coltello da tasca con il manico di corno. Taglia pane, corda e poco altro."), 0.3f));
		{
			FValdorsoTipoOggetto Acciarino = OggSemplice(TEXT("Acciarino"), TEXT("Acciarino e pietra focaia"),
				TEXT("Per accendere un falò all'aperto, con due fasci di rami secchi. Al fuoco ci si asciuga e ci si scalda."), 0.2f);
			Acciarino.Uso = EValdorsoUso::Accendi;
			C.Add(Acciarino);
		}
		C.Add(OggSemplice(TEXT("Rami_Secchi"), TEXT("Rami secchi"),
			TEXT("Un fascio di rami caduti, legato con un giunco. Si trovano nei boschi; due fanno un falò."), 0.5f, 10));
		C.Add(OggCibo(TEXT("Funghi"), TEXT("Funghi di bosco"),
			TEXT("Porcini e chiodini, quelli che Gaspare dice che si possono mangiare. Gli altri lasciali stare."), 0.1f, 10, 5.f, 10.f));
		C.Add(OggSemplice(TEXT("Mantello"), TEXT("Mantello di lana"),
			TEXT("Grezzo, pesante e caldo. Finché lo porti con te, la pioggia ti bagna la metà."), 1.5f));
		C.Add(OggDaLeggere(OggSemplice(TEXT("Mappa_Valle"), TEXT("Mappa della valle"),
			TEXT("La valle dell'Orso su un foglio piegato in tre, con il villaggio, il lago e il passo. Senza, il tasto M non apre niente."), 0.1f),
			TEXT("La mappa della valle: si apre con il tasto M.")));
		C.Add(OggSemplice(TEXT("Corda"), TEXT("Corda di canapa"),
			TEXT("Dieci braccia di corda. Non si sa mai."), 1.0f));
		{
			FValdorsoTipoOggetto Tessera = OggRicordo(TEXT("Tessera"), TEXT("Tessera del colono"),
				TEXT("Il lasciapassare della carovana, con il tuo ritratto e il timbro della capitana. Senza, dal passo non si torna."), 0.02f);
			// (09/11) si apre come un foglio vero, e si può mostrare a chi si ha davanti
			Tessera.Uso = EValdorsoUso::Documento;
			C.Add(Tessera);
		}

		// --- Gli attrezzi del vecchio mestiere (Registro: "Cosa facevi prima?") -------------------------------------
		C.Add(OggSemplice(TEXT("Falcetto"), TEXT("Falcetto"),
			TEXT("Il falcetto del vecchio podere, con la lama consumata dalla cote."), 0.6f));
		C.Add(OggSemplice(TEXT("Bastone_Pastore"), TEXT("Bastone da pastore"),
			TEXT("Di nocciolo, con il manico ricurvo. Le pecore lo conoscevano meglio di te."), 1.2f));
		C.Add(OggSemplice(TEXT("Martello"), TEXT("Martello di bottega"),
			TEXT("Il martello del tuo vecchio maestro. Il manico ha la forma della sua mano."), 0.8f));
		C.Add(OggSemplice(TEXT("Pugnale"), TEXT("Pugnale d'ordinanza"),
			TEXT("Quello che resta dell'equipaggiamento della Corona: un pugnale con il marchio del reggimento."), 0.5f));
		C.Add(OggSemplice(TEXT("Candeliere"), TEXT("Candeliere d'ottone"),
			TEXT("Dalla vecchia casa dei signori. Te l'hanno regalato, dici. Quasi."), 0.7f));
		C.Add(OggDaLeggere(OggSemplice(TEXT("Quaderno"), TEXT("Quaderno di appunti"),
			TEXT("Gli appunti dell'accademia, fitti e storti. A metà, un disegno di runa che non hai mai capito."), 0.3f),
			TEXT("«Le rune degli elementi stanno sulla linea di base; i verbi sono un gesto. Ur non entra nelle parole.»")));
		C.Add(OggSemplice(TEXT("Medaglietta"), TEXT("Medaglietta del tempio"),
			TEXT("Una medaglietta di stagno con il Cuore inciso. Le davano agli orfani del tempio."), 0.05f));
		C.Add(OggSemplice(TEXT("Bilancia"), TEXT("Bilancia da mercante"),
			TEXT("Due piatti, un giogo e i pesi in un sacchetto. Non mente mai, se la tieni dritta."), 1.0f));
		C.Add(OggSemplice(TEXT("Trappola"), TEXT("Trappola a laccio"),
			TEXT("Filo di ottone e un picchetto. Per lepri e conigli, non per quello che vive nel bosco fondo."), 0.6f));
		C.Add(OggSemplice(TEXT("Rete"), TEXT("Rete da pesca"),
			TEXT("Una rete piccola, rattoppata più volte. Nel lago della valle i pesci sono grossi."), 1.5f));

		// --- I ricordi (Registro: "Cosa porti con te?"): non si gettano, non si perdono morendo --------------------
		C.Add(OggRicordo(TEXT("Ricordo_Anello"), TEXT("L'anello di tua madre"),
			TEXT("Un anello d'argento consumato. Si può dare in pegno e poi riscattare."), 0.02f));
		C.Add(OggRicordo(TEXT("Ricordo_Lettera"), TEXT("Una lettera mai aperta"),
			TEXT("Il sigillo è intatto. Dentro c'è una storia, quando troverai il coraggio."), 0.02f));
		C.Add(OggRicordo(TEXT("Ricordo_Spada"), TEXT("La spada spezzata di tuo padre"),
			TEXT("Due pezzi avvolti in un panno. Un fabbro potrebbe riforgiarla."), 1.8f));
		C.Add(OggRicordo(TEXT("Ricordo_Amuleto"), TEXT("Un amuleto di legno"),
			TEXT("Intagliato a forma di zampa. Stretto in mano, la paura pesa meno."), 0.05f));
		C.Add(OggRicordo(TEXT("Ricordo_Mappa"), TEXT("Una mappa strappata"),
			TEXT("Mezza mappa della valle, con un segno. L'altra metà è da qualche parte."), 0.05f));
		C.Add(OggDaLeggere(OggRicordo(TEXT("Ricordo_Libro"), TEXT("Un libro di preghiere"),
			TEXT("Le pagine sanno di cera. Il primo tomo della luce."), 0.6f),
			TEXT("«Due colpi e una pausa: così batte il Cuore, così batte chi lo ascolta.»")));
		// --- (09/11, v0.1.4) Le armi provvisorie, le prede dei lupi, il fondo del lago ----------------------------
		{
			FValdorsoTipoOggetto Spada = OggSemplice(TEXT("Spada_Corta"), TEXT("Spada corta"),
				TEXT("Una lama dritta da un braccio, la guardia di ferro e il manico di cuoio. Da impugnare nella destra."), 1.4f);
			C.Add(Spada);
		}
		C.Add(OggSemplice(TEXT("Scudo_Legno"), TEXT("Scudo di legno"),
			TEXT("Assi di faggio inchiodate, il bordo e l'umbone di ferro. Si porta nella sinistra: ferma i colpi che arrivano davanti."), 3.5f));
		C.Add(OggSemplice(TEXT("Pelle_Lupo"), TEXT("Pelle di lupo"),
			TEXT("Grigia, folta, ancora da conciare. Alla conceria ne fanno un mantello che tiene caldo."), 2.0f, 5));
		C.Add(OggSemplice(TEXT("Zanne_Lupo"), TEXT("Zanne di lupo"),
			TEXT("Due zanne lunghe come un dito. Gaspare dice che portano fortuna a chi le ha prese da sé."), 0.05f, 20));
		{
			FValdorsoTipoOggetto Mantello = OggSemplice(TEXT("Mantello_Lupo"), TEXT("Mantello di pelle di lupo"),
				TEXT("Pesante e caldissimo. Finché lo porti con te senti il freddo molto meno, e la pioggia ti bagna la metà."), 2.5f);
			Mantello.Calore = 6.f;
			C.Add(Mantello);
		}
		C.Add(OggCibo(TEXT("Cozze"), TEXT("Cozze di lago"),
			TEXT("Raccolte sul fondo del lago, nere e chiuse. Si mangiano anche crude, se hai coraggio."), 0.2f, 10, 8.f, 10.f));
		C.Add(OggSemplice(TEXT("Perla"), TEXT("Perla di lago"),
			TEXT("Piccola, grigia, quasi tonda. Ogni tanto ce n'è una tra le cozze del fondo. Bruno la compra bene."), 0.01f, 20));

		// --- (10/11, v0.1.4) L'armeria provvisoria: armi della valle, della Corona e dell'Impero, e gli scudi ------
		C.Add(OggSemplice(TEXT("Accetta"), TEXT("Accetta"),
			TEXT("Per la legna, ma anche no. Il ferro è corto e pesante: colpisce forte e da vicino."), 1.2f));
		C.Add(OggSemplice(TEXT("Ascia_Guerra"), TEXT("Ascia da guerra"),
			TEXT("Il manico lungo un braccio e mezzo, la lama a mezzaluna. Faticosa, ma chi la prende in pieno non si rialza in fretta."), 2.6f));
		C.Add(OggSemplice(TEXT("Mazza"), TEXT("Mazza ferrata"),
			TEXT("Una testa di ferro a sei alette su un manico di frassino. Non taglia: rompe, e fa perdere l'equilibrio."), 2.2f));
		C.Add(OggSemplice(TEXT("Clava"), TEXT("Clava di quercia"),
			TEXT("Un ramo nodoso, tagliato e lisciato dove si tiene. L'arma di chi non ha di meglio."), 1.6f));
		C.Add(OggSemplice(TEXT("Lancia"), TEXT("Lancia"),
			TEXT("Due metri di frassino e una punta a foglia. Tiene lontano chi ti viene addosso: lupi compresi."), 2.0f));
		C.Add(OggSemplice(TEXT("Forcone"), TEXT("Forcone"),
			TEXT("Tre denti di ferro per il fieno. Nelle mani giuste, il primo arnese della milizia di un villaggio."), 1.8f));
		C.Add(OggSemplice(TEXT("Bastone_Ferrato"), TEXT("Bastone ferrato"),
			TEXT("Un bastone da viaggio con le punte rivestite di ferro. Colpisce lontano e fa barcollare."), 1.5f));
		C.Add(OggSemplice(TEXT("Spada_Lunga"), TEXT("Spada lunga"),
			TEXT("La spada dei cavalieri della Corona: la lama lunga, l'elsa a croce e il pomo tondo. Pesa, ma arriva lontano."), 1.9f));
		C.Add(OggSemplice(TEXT("Gladio"), TEXT("Gladio"),
			TEXT("La spada corta delle legioni dell'Impero: larga, appuntita, fatta per colpire di punta da dietro lo scudo."), 1.2f));
		C.Add(OggSemplice(TEXT("Brocchiere"), TEXT("Brocchiere"),
			TEXT("Uno scudo piccolo di ferro, da pugno. Leggero: devia i colpi, ma ne ferma meno di uno grande."), 1.6f));
		C.Add(OggSemplice(TEXT("Scudo_Rotondo"), TEXT("Scudo rotondo dipinto"),
			TEXT("Tondo, di tavole incollate, dipinto di rosso e d'oro come gli scudi della milizia di Aurelia."), 4.0f));
		C.Add(OggSemplice(TEXT("Scudo_Aquilone"), TEXT("Scudo a mandorla"),
			TEXT("Lungo, stretto in basso: copre dalla spalla al ginocchio. Lo portano i cavalieri della Corona."), 5.0f));
		C.Add(OggSemplice(TEXT("Scudo_Legione"), TEXT("Scudo della legione"),
			TEXT("Lo scudo ricurvo delle legioni dell'Impero, con l'umbone di bronzo. Ferma quasi tutto, ma pesa come un bambino."), 7.0f));

		// --- (10/11) Quello che danno gli animali (si scuoiano col coltello: E sull'animale a terra) ----------------
		C.Add(OggCibo(TEXT("Carne_Cruda"), TEXT("Carne cruda"),
			TEXT("Un taglio di carne fresca. Cruda sazia poco e non fa bene: al fuoco sarà un'altra cosa."), 0.5f, 10, -2.f, 15.f));
		C.Add(OggSemplice(TEXT("Pelle_Cinghiale"), TEXT("Pelle di cinghiale"),
			TEXT("Dura e setolosa. Il conciatore ne fa cuoio per stivali e cinghie."), 3.0f, 5));
		C.Add(OggSemplice(TEXT("Zanne_Cinghiale"), TEXT("Zanne di cinghiale"),
			TEXT("Due zanne ricurve, bianche e affilate. I ragazzi del villaggio le portano al collo."), 0.1f, 20));
		C.Add(OggSemplice(TEXT("Pelle_Orso"), TEXT("Pelle d'orso"),
			TEXT("Enorme, bruna, pesante. Chi la porta al villaggio non paga da bere per un mese."), 8.0f, 2));
		C.Add(OggSemplice(TEXT("Artigli_Orso"), TEXT("Artigli d'orso"),
			TEXT("Cinque artigli lunghi come un dito, neri in punta. Dicono che l'orso della collina sia il guardiano della valle."), 0.2f, 20));
		C.Add(OggSemplice(TEXT("Pelle_Cervo"), TEXT("Pelle di cervo"),
			TEXT("Morbida e fulva. Se ne fanno guanti, borse e le copertine dei libri del tempio."), 2.5f, 5));
		C.Add(OggSemplice(TEXT("Palchi_Cervo"), TEXT("Palchi di cervo"),
			TEXT("Le corna ramificate di un cervo adulto. Si intagliano: manici, pettini, bottoni."), 1.5f, 4));
		C.Add(OggSemplice(TEXT("Pelle_Lepre"), TEXT("Pelle di lepre"),
			TEXT("Piccola e morbidissima. Ne servono tante per un colletto."), 0.2f, 20));
		C.Add(OggSemplice(TEXT("Pelle_Volpe"), TEXT("Pelle di volpe"),
			TEXT("Rossa, con la coda bianca in punta. I mercanti della carovana la pagano bene."), 0.6f, 10));

		// --- (10/11) Dal fuoco, dalla conceria e dalla locanda (ValdorsoBotteghe.h) --------------------------------
		C.Add(OggCibo(TEXT("Carne_Cotta"), TEXT("Carne arrostita"),
			TEXT("Cotta sulla brace finché il grasso sfrigola. Rimette in forze come poche altre cose."), 0.4f, 10, 15.f, 35.f));
		C.Add(OggCibo(TEXT("Funghi_Arrosto"), TEXT("Funghi arrosto"),
			TEXT("Infilzati su un ramo e girati sul fuoco. Sanno di bosco e di fumo."), 0.1f, 10, 8.f, 15.f));
		C.Add(OggCibo(TEXT("Stufato"), TEXT("Stufato di carne e funghi"),
			TEXT("In una ciotola di legno, ancora caldo. Bruno dice che è la ricetta di sua madre."), 0.6f, 5, 30.f, 50.f));
		C.Add(OggCibo(TEXT("Zuppa_Bruno"), TEXT("Zuppa calda di Bruno"),
			TEXT("Orzo, cipolle e lardo. Una scodella di questa e anche la notte più fredda sembra meno lunga."), 0.5f, 5, 20.f, 40.f));
		C.Add(OggSemplice(TEXT("Cuoio"), TEXT("Cuoio conciato"),
			TEXT("Pelle conciata e battuta, pronta da tagliare. Serve per mantelli, cinghie e guanti."), 0.6f, 10));
		{
			FValdorsoTipoOggetto Mantello = OggSemplice(TEXT("Mantello_Orso"), TEXT("Mantello di pelle d'orso"),
				TEXT("Enorme e pesantissimo. Con questo addosso il freddo della valle quasi non si sente."), 4.0f);
			Mantello.Calore = 8.f;
			C.Add(Mantello);
		}
		{
			FValdorsoTipoOggetto Cappuccio = OggSemplice(TEXT("Cappuccio_Volpe"), TEXT("Cappuccio di volpe"),
				TEXT("Rosso, con la coda che ricade sulla spalla. Tiene calde le orecchie e fa voltare la gente."), 0.6f);
			Cappuccio.Calore = 2.f;
			C.Add(Cappuccio);
		}
		{
			FValdorsoTipoOggetto Guanti = OggSemplice(TEXT("Guanti_Lepre"), TEXT("Guanti foderati"),
				TEXT("Guanti di cuoio foderati di pelo di lepre. Le dita ringraziano."), 0.3f);
			Guanti.Calore = 1.f;
			C.Add(Guanti);
		}

		// (10/11) il foglio di un incarico staccato dalla bacheca della Gilda (il numero dell'incarico sta nel posto
		// dello zaino, FValdorsoPostoZaino::Numero). Non si getta: si rinuncia da Lucia (ValdorsoGildaDati.h).
		{
			FValdorsoTipoOggetto Foglio = OggRicordo(TEXT("Foglio_Incarico"), TEXT("Foglio della Gilda"),
				TEXT("Un incarico staccato dalla bacheca della Gilda, con il sigillo di ceralacca. Si porta a Lucia, al bancone."), 0.01f);
			Foglio.Uso = EValdorsoUso::Leggi;
			Foglio.Lettura = TEXT("Un incarico della Gilda: portalo a Lucia, al bancone.");
			C.Add(Foglio);
		}

		// (11/11, v0.1.5) la tessera della carovana: dalle Cronache (Capitolo V), la tessera di legno con il numero inciso e il
		// segno della Corona bruciato a fuoco, quella del viaggio e del passo (il foglio di carta e' quella del villaggio)
		C.Add(OggDaLeggere(OggRicordo(TEXT("Tessera_Carovana"), TEXT("Tessera della carovana"),
			TEXT("Una tavoletta di legno con il tuo numero inciso e il segno della Corona bruciato a fuoco. Mastro Ilario te l'ha data con il registro; al passo la capitana Livia l'ha confrontata con la lista."),
			0.05f), TEXT("La tessera della carovana: il tuo numero, inciso nel legno, e la corona bruciata a fuoco. Il ricordo del viaggio.")));

		// (11/11, v0.1.7) la grotta del nord: il piccone per la frana, le monete e il pugnale dei nani, la storia di Lucia
		C.Add(OggSemplice(TEXT("Piccone"), TEXT("Piccone"),
			TEXT("Ferro di Marta su un manico di frassino. Per la roccia, per la frana del nord e, se serve, per chi ti viene addosso."), 3.f));
		C.Add(OggSemplice(TEXT("Moneta_Nani"), TEXT("Moneta dei nani"),
			TEXT("Squadrata, con un foro nel mezzo e segni che nessuno sa leggere. Marta, alla fucina, le compra per il metallo."), 0.02f, 50));
		C.Add(OggSemplice(TEXT("Pugnale_Nani"), TEXT("Pugnale dei nani"),
			TEXT("Corto, pesante, la lama scura che non ha preso ruggine in mille anni. Sul pomo, un segno squadrato."), 0.6f));
		// (11/11) la forgia dei nani: il martello e il cuore del guardiano
		C.Add(OggSemplice(TEXT("Martello_Nani"), TEXT("Martello dei nani"),
			TEXT("Il martello del guardiano della forgia: una testa di ferro nero grande come un pane, il manico fasciato d'ottone. Pesa, e si sente."), 4.5f));
		C.Add(OggSemplice(TEXT("Cuore_di_Pietra"), TEXT("Cuore di pietra"),
			TEXT("Una pietra grande come un pugno, liscia, tiepida. Dentro, se la tieni all'orecchio, qualcosa batte piano. Marta la vuole vedere."), 0.5f, 10));
		// (11/11, v0.2 "L'acqua della valle") il secchio e l'acqua: quale acqua, lo decide dove la prendi
		{
			FValdorsoTipoOggetto Secchio = OggSemplice(TEXT("Secchio"), TEXT("Secchio"),
				TEXT("Un secchio di doghe cerchiate di ferro, con il manico di corda. Entra nell'acqua fino alle ginocchia e riempilo: la pozza della cascata da' l'acqua pura, il lago e il fiume la chiara, le gallerie allagate dei nani la torbida."), 1.2f);
			Secchio.Uso = EValdorsoUso::Raccogli;
			C.Add(Secchio);
			struct FAcqua { const TCHAR* Id; const TCHAR* Nome; const TCHAR* Descrizione; float Salute; float Stamina; float Equilibrio; };
			const FAcqua Acque[] = {
				{ TEXT("Secchio_Acqua_Pura"), TEXT("Secchio d'acqua pura"),
					TEXT("Acqua della cascata, fredda e trasparente: sa di pietra e di neve. La migliore della valle: Bruno la paga per la birra."), 3.f, 25.f, 12.f },
				{ TEXT("Secchio_Acqua_Chiara"), TEXT("Secchio d'acqua chiara"),
					TEXT("Acqua del lago o del fiume: buona da bere, buona per cucinare. Ogni tanto ci nuota dentro qualcosa."), 0.f, 20.f, 8.f },
				{ TEXT("Secchio_Acqua_Torbida"), TEXT("Secchio d'acqua torbida"),
					TEXT("Acqua scura, che sa di foglie marce e di ferro. Si beve se proprio si deve, e disseta poco. A qualcuno, nella palude, potrebbe servire."), 0.f, 8.f, 0.f },
			};
			for (const FAcqua& A : Acque)
			{
				FValdorsoTipoOggetto Pieno = OggSemplice(A.Id, A.Nome, A.Descrizione, 9.f);
				Pieno.Uso = EValdorsoUso::Bevi;
				Pieno.Salute = A.Salute;
				Pieno.Stamina = A.Stamina;
				Pieno.Equilibrio = A.Equilibrio;
				Pieno.Cariche = 8;
				Pieno.Vuoto = TEXT("Secchio");
				C.Add(Pieno);
			}
		}
		// (11/11, v0.1.8 "Le storie della valle") i ricordi delle storie dei volti nuovi (ValdorsoStorie.h)
		C.Add(OggRicordo(TEXT("Fischietto_di_Livia"), TEXT("Il fischietto della capitana"),
			TEXT("Un fischietto d'ottone consumato, con una S incisa. La capitana Livia l'ha usato otto anni sui confini. \"Se fischi tre volte, chi ha servito con me si volta.\""), 0.05f));
		C.Add(OggRicordo(TEXT("Sasso_di_Tobia"), TEXT("Il sasso dell'orsetto"),
			TEXT("Un sasso liscio di fiume, con la forma di un orso che dorme. Tobia l'ha tenuto sotto il cuscino per due anni: adesso e' tuo, perche' tu sei andato a vedere."), 0.2f));
		C.Add(OggRicordo(TEXT("Chiodo_Runico"), TEXT("Il chiodo che canta"),
			TEXT("Un chiodo di ferro battuto da Marta sull'incudine antica, la notte che canto'. Sulla testa c'e' un segno che nessuno le ha insegnato. Se lo stringi, e' tiepido."), 0.1f));
		C.Add(OggRicordo(TEXT("Candela_della_Veglia"), TEXT("La candela della veglia"),
			TEXT("Un mozzicone di cera d'api, consumato fino in fondo accanto al frammento del Cuore. Padre Anselmo ne da' uno a chi ha vegliato con lui fino all'alba."), 0.1f));
		C.Add(OggDaLeggere(OggRicordo(TEXT("Mappa_Gallerie"), TEXT("Due pagine della mappa delle gallerie"),
			TEXT("Le due pagine che Gisella lego' alla corda nell'autunno del 309: la galleria della frana, la sala delle colonne, una scala che scende. Poi il foglio finisce."),
			0.02f), TEXT("La galleria della frana scende verso nord fino a una sala con sei colonne. A ovest, un campo. A nord, una scala che scende. Poi, il bordo strappato.")));
		C.Add(OggDaLeggere(OggRicordo(TEXT("Taccuino_Gisella"), TEXT("Il taccuino di Gisella"),
			TEXT("Un taccuino di pelle gonfio d'umidita', pieno di mappe disegnate a carboncino. Ai bordi, draghi."), 0.2f),
			TEXT("\"Giorno quattro. Abbiamo sentito i martelli. Non sono martelli.\" Va riportato a Lucia, alla Gilda.")));
		C.Add(OggRicordo(TEXT("Lanterna_Cecco"), TEXT("La lanterna di Cecco"),
			TEXT("Una lanterna da minatore con il vetro rotto e il nome di Cecco inciso sul manico. Va riportata a Lucia, alla Gilda."), 0.8f));
		C.Add(OggDaLeggere(OggRicordo(TEXT("Anello_Ranieri"), TEXT("L'anello di Ranieri"),
			TEXT("D'argento, con la lanterna della Gilda incisa e una R. Lo portava al collo il capitano delle ossa."), 0.02f),
			TEXT("Dentro l'anello, piccolissimo: \"a L., che tiene la corda\". Va riportato a Lucia, alla Gilda.")));

		// le armi: chi si impugna e come colpisce (il pugno: 10 di danno, 30 di equilibrio, 10 di stamina, 90 cm)
		struct FArmaDati
		{
			const TCHAR* Id;
			EValdorsoPosto Posto;
			float Danno;
			float Equilibrio;
			float Stamina;
			float Portata;
			uint8 Tipo;
			float Parata;
		};
		static const FArmaDati Armi[] = {
			{ TEXT("Coltello"), EValdorsoPosto::ManoDestra, 12.f, 15.f, 2.f, 100.f, 1, 0.f },
			{ TEXT("Pugnale"), EValdorsoPosto::ManoDestra, 15.f, 18.f, 3.f, 110.f, 1, 0.f },
			{ TEXT("Falcetto"), EValdorsoPosto::ManoDestra, 14.f, 20.f, 4.f, 115.f, 1, 0.f },
			{ TEXT("Martello"), EValdorsoPosto::ManoDestra, 16.f, 40.f, 6.f, 105.f, 2, 0.f },
			{ TEXT("Bastone_Pastore"), EValdorsoPosto::ManoDestra, 11.f, 45.f, 5.f, 170.f, 2, 0.f },
			{ TEXT("Spada_Corta"), EValdorsoPosto::ManoDestra, 22.f, 25.f, 7.f, 150.f, 1, 0.f },
			{ TEXT("Scudo_Legno"), EValdorsoPosto::ManoSinistra, 6.f, 50.f, 6.f, 90.f, 4, 0.6f },
			// (10/11) l'armeria: Danno, Equilibrio, Stamina, Portata (cm), Tipo (1 lama, 2 botta, 4 scudo), Parata
			{ TEXT("Accetta"), EValdorsoPosto::ManoDestra, 18.f, 30.f, 5.f, 115.f, 1, 0.f },
			{ TEXT("Ascia_Guerra"), EValdorsoPosto::ManoDestra, 27.f, 45.f, 9.f, 150.f, 1, 0.f },
			{ TEXT("Mazza"), EValdorsoPosto::ManoDestra, 21.f, 55.f, 7.f, 120.f, 2, 0.f },
			{ TEXT("Clava"), EValdorsoPosto::ManoDestra, 15.f, 45.f, 6.f, 120.f, 2, 0.f },
			{ TEXT("Lancia"), EValdorsoPosto::ManoDestra, 20.f, 25.f, 6.f, 230.f, 1, 0.f },
			{ TEXT("Forcone"), EValdorsoPosto::ManoDestra, 13.f, 30.f, 5.f, 200.f, 1, 0.f },
			{ TEXT("Bastone_Ferrato"), EValdorsoPosto::ManoDestra, 14.f, 50.f, 5.f, 190.f, 2, 0.f },
			{ TEXT("Spada_Lunga"), EValdorsoPosto::ManoDestra, 28.f, 30.f, 9.f, 175.f, 1, 0.f },
			{ TEXT("Gladio"), EValdorsoPosto::ManoDestra, 24.f, 22.f, 6.f, 140.f, 1, 0.f },
			{ TEXT("Brocchiere"), EValdorsoPosto::ManoSinistra, 8.f, 40.f, 4.f, 80.f, 4, 0.4f },
			{ TEXT("Scudo_Rotondo"), EValdorsoPosto::ManoSinistra, 7.f, 50.f, 6.f, 90.f, 4, 0.65f },
			{ TEXT("Scudo_Aquilone"), EValdorsoPosto::ManoSinistra, 7.f, 55.f, 7.f, 90.f, 4, 0.75f },
			{ TEXT("Scudo_Legione"), EValdorsoPosto::ManoSinistra, 9.f, 65.f, 8.f, 90.f, 4, 0.85f },
			// (11/11, v0.1.7) il piccone (una botta: conta come le mazze) e il pugnale dei nani
			{ TEXT("Piccone"), EValdorsoPosto::ManoDestra, 15.f, 40.f, 7.f, 120.f, 2, 0.f },
			{ TEXT("Pugnale_Nani"), EValdorsoPosto::ManoDestra, 19.f, 22.f, 3.f, 115.f, 1, 0.f },
			// (11/11) il martello dei nani: la botta piu' forte della valle, lento e pesante
			{ TEXT("Martello_Nani"), EValdorsoPosto::ManoDestra, 32.f, 65.f, 11.f, 135.f, 2, 0.f },
		};
		for (const FArmaDati& A : Armi)
		{
			for (FValdorsoTipoOggetto& Tipo : C)
			{
				if (Tipo.Id == FName(A.Id))
				{
					Tipo.Uso = EValdorsoUso::Impugna;
					Tipo.Posto = A.Posto;
					Tipo.DannoArma = A.Danno;
					Tipo.EquilibrioArma = A.Equilibrio;
					Tipo.StaminaArma = A.Stamina;
					Tipo.PortataArma = A.Portata;
					Tipo.TipoColpo = A.Tipo;
					Tipo.Parata = A.Parata;
				}
			}
		}
		return C;
	}

	// --- I disegni: poche linee a penna ---------------------------------------------------------------------

	/** Un arco di ellisse (gradi, 0 = a destra, in senso antiorario). */
	TArray<FVector2D> OggArco(float Cx, float Cy, float Rx, float Ry, float Da, float A, int32 Passi = 16)
	{
		TArray<FVector2D> P;
		P.Reserve(Passi + 1);
		for (int32 i = 0; i <= Passi; ++i)
		{
			const float Gradi = FMath::Lerp(Da, A, static_cast<float>(i) / Passi);
			const float R = FMath::DegreesToRadians(Gradi);
			P.Add(FVector2D(Cx + Rx * FMath::Cos(R), Cy + Ry * FMath::Sin(R)));
		}
		return P;
	}

	TArray<FVector2D> OggCerchio(float Cx, float Cy, float R, int32 Passi = 20)
	{
		return OggArco(Cx, Cy, R, R, 0.f, 360.f, Passi);
	}

	TArray<FVector2D> OggRett(float X0, float Y0, float X1, float Y1)
	{
		return { FVector2D(X0, Y0), FVector2D(X1, Y0), FVector2D(X1, Y1), FVector2D(X0, Y1), FVector2D(X0, Y0) };
	}

	TArray<FVector2D> OggLinea(std::initializer_list<FVector2D> Punti)
	{
		return TArray<FVector2D>(Punti);
	}

	FLineeOggetto OggDisegnoDi(FName Id)
	{
		const FString N = Id.ToString();
		FLineeOggetto L;
		if (N == TEXT("Pane_Nero"))
		{
			L.Add(OggArco(0.f, -0.35f, 0.8f, 0.75f, 0.f, 180.f));
			L.Add(OggLinea({ { -0.8f, -0.35f }, { 0.8f, -0.35f } }));
			L.Add(OggLinea({ { -0.4f, 0.05f }, { -0.2f, 0.3f } }));
			L.Add(OggLinea({ { -0.05f, 0.05f }, { 0.15f, 0.3f } }));
			L.Add(OggLinea({ { 0.3f, 0.0f }, { 0.48f, 0.22f } }));
		}
		else if (N == TEXT("Formaggio"))
		{
			L.Add(OggLinea({ { -0.8f, -0.5f }, { 0.8f, -0.5f }, { 0.8f, 0.1f }, { -0.8f, -0.1f }, { -0.8f, -0.5f } }));
			L.Add(OggLinea({ { -0.8f, -0.1f }, { 0.5f, 0.55f }, { 0.8f, 0.1f } }));
			L.Add(OggCerchio(-0.3f, -0.3f, 0.1f, 10));
			L.Add(OggCerchio(0.35f, -0.22f, 0.07f, 10));
			L.Add(OggCerchio(0.1f, 0.22f, 0.07f, 10));
		}
		else if (N == TEXT("Mela"))
		{
			L.Add(OggArco(0.f, -0.1f, 0.65f, 0.62f, 100.f, 440.f, 24));
			L.Add(OggLinea({ { 0.f, 0.45f }, { 0.08f, 0.85f } }));
			L.Add(OggLinea({ { 0.08f, 0.7f }, { 0.45f, 0.82f }, { 0.12f, 0.6f } }));
		}
		else if (N == TEXT("Borraccia"))
		{
			L.Add(OggArco(0.f, -0.25f, 0.62f, 0.6f, 65.f, 475.f, 24));
			L.Add(OggRett(-0.18f, 0.32f, 0.18f, 0.62f));
			L.Add(OggRett(-0.24f, 0.62f, 0.24f, 0.78f));
			L.Add(OggArco(0.f, -0.25f, 0.42f, 0.4f, 200.f, 340.f, 10));
		}
		else if (N == TEXT("Erbe_Palude"))
		{
			L.Add(OggLinea({ { 0.f, -0.85f }, { 0.f, 0.75f } }));
			L.Add(OggLinea({ { 0.f, -0.85f }, { -0.45f, 0.6f } }));
			L.Add(OggLinea({ { 0.f, -0.85f }, { 0.45f, 0.6f } }));
			L.Add(OggLinea({ { -0.12f, -0.4f }, { 0.12f, -0.4f } }));
			L.Add(OggLinea({ { 0.f, 0.3f }, { -0.25f, 0.45f } }));
			L.Add(OggLinea({ { 0.f, 0.05f }, { 0.25f, 0.2f } }));
			L.Add(OggLinea({ { -0.3f, 0.15f }, { -0.5f, 0.2f } }));
			L.Add(OggLinea({ { 0.32f, 0.3f }, { 0.55f, 0.3f } }));
		}
		else if (N == TEXT("Mappa_Valle"))
		{
			// un foglio piegato in tre, con il fiume e un segno
			L.Add(OggLinea({ { -0.85f, -0.6f }, { -0.3f, -0.45f }, { 0.3f, -0.6f }, { 0.85f, -0.45f }, { 0.85f, 0.6f }, { 0.3f, 0.45f },
				{ -0.3f, 0.6f }, { -0.85f, 0.45f }, { -0.85f, -0.6f } }));
			L.Add(OggLinea({ { -0.3f, -0.45f }, { -0.3f, 0.6f } }));
			L.Add(OggLinea({ { 0.3f, -0.6f }, { 0.3f, 0.45f } }));
			L.Add(OggLinea({ { -0.7f, 0.25f }, { -0.45f, 0.05f }, { -0.1f, 0.1f }, { 0.15f, -0.15f }, { 0.5f, -0.1f }, { 0.7f, -0.3f } }));
			L.Add(OggCerchio(0.55f, 0.25f, 0.1f, 10));
		}
		else if (N == TEXT("Rami_Secchi"))
		{
			L.Add(OggLinea({ { -0.85f, -0.3f }, { 0.8f, 0.25f } }));
			L.Add(OggLinea({ { -0.8f, 0.2f }, { 0.85f, -0.25f } }));
			L.Add(OggLinea({ { -0.75f, -0.05f }, { 0.85f, 0.02f } }));
			L.Add(OggLinea({ { 0.35f, 0.1f }, { 0.55f, 0.4f } }));
			L.Add(OggLinea({ { -0.4f, -0.12f }, { -0.55f, -0.45f } }));
			// il giunco che li lega
			L.Add(OggArco(0.f, 0.f, 0.12f, 0.32f, 0.f, 360.f, 12));
		}
		else if (N == TEXT("Funghi"))
		{
			L.Add(OggArco(-0.2f, 0.05f, 0.5f, 0.42f, 0.f, 180.f, 14));
			L.Add(OggLinea({ { -0.7f, 0.05f }, { 0.3f, 0.05f } }));
			L.Add(OggLinea({ { -0.32f, 0.05f }, { -0.36f, -0.7f }, { -0.04f, -0.7f }, { -0.08f, 0.05f } }));
			L.Add(OggArco(0.45f, -0.25f, 0.3f, 0.25f, 0.f, 180.f, 10));
			L.Add(OggLinea({ { 0.15f, -0.25f }, { 0.75f, -0.25f } }));
			L.Add(OggLinea({ { 0.38f, -0.25f }, { 0.36f, -0.75f }, { 0.54f, -0.75f }, { 0.52f, -0.25f } }));
		}
		else if (N == TEXT("Coltello"))
		{
			L.Add(OggLinea({ { -0.85f, -0.05f }, { 0.25f, -0.05f }, { 0.25f, 0.15f }, { -0.55f, 0.15f }, { -0.85f, -0.05f } }));
			L.Add(OggRett(0.25f, -0.12f, 0.85f, 0.18f));
			L.Add(OggLinea({ { 0.4f, 0.03f }, { 0.7f, 0.03f } }));
		}
		else if (N == TEXT("Acciarino"))
		{
			L.Add(OggArco(-0.2f, 0.f, 0.5f, 0.4f, 90.f, 270.f, 12));
			L.Add(OggLinea({ { -0.2f, 0.4f }, { 0.35f, 0.4f } }));
			L.Add(OggLinea({ { -0.2f, -0.4f }, { 0.35f, -0.4f } }));
			L.Add(OggLinea({ { 0.45f, -0.15f }, { 0.8f, -0.3f }, { 0.75f, 0.2f }, { 0.5f, 0.25f }, { 0.45f, -0.15f } }));
		}
		else if (N == TEXT("Mantello"))
		{
			L.Add(OggLinea({ { -0.25f, 0.6f }, { -0.75f, -0.8f }, { 0.75f, -0.8f }, { 0.25f, 0.6f } }));
			L.Add(OggArco(0.f, 0.6f, 0.25f, 0.25f, 0.f, 180.f, 10));
			L.Add(OggLinea({ { -0.1f, 0.35f }, { -0.3f, -0.8f } }));
			L.Add(OggLinea({ { 0.12f, 0.35f }, { 0.3f, -0.8f } }));
		}
		else if (N == TEXT("Corda"))
		{
			L.Add(OggArco(0.f, 0.f, 0.75f, 0.5f, 0.f, 360.f, 24));
			L.Add(OggArco(0.f, 0.f, 0.5f, 0.32f, 0.f, 360.f, 20));
			L.Add(OggArco(0.f, 0.f, 0.25f, 0.15f, 0.f, 360.f, 14));
			L.Add(OggLinea({ { 0.75f, 0.f }, { 0.9f, -0.6f } }));
		}
		else if (N == TEXT("Tessera"))
		{
			L.Add(OggRett(-0.8f, -0.55f, 0.8f, 0.55f));
			L.Add(OggLinea({ { -0.6f, 0.3f }, { 0.2f, 0.3f } }));
			L.Add(OggLinea({ { -0.6f, 0.08f }, { 0.1f, 0.08f } }));
			L.Add(OggLinea({ { -0.6f, -0.14f }, { 0.f, -0.14f } }));
			L.Add(OggCerchio(0.45f, -0.2f, 0.2f, 14));
			L.Add(OggCerchio(0.45f, -0.2f, 0.09f, 10));
		}
		else if (N == TEXT("Falcetto"))
		{
			L.Add(OggArco(0.1f, 0.15f, 0.6f, 0.6f, -20.f, 200.f, 18));
			L.Add(OggArco(0.1f, 0.05f, 0.42f, 0.42f, 0.f, 180.f, 14));
			L.Add(OggLinea({ { -0.48f, -0.05f }, { -0.7f, -0.85f } }));
		}
		else if (N == TEXT("Bastone_Pastore"))
		{
			L.Add(OggLinea({ { 0.1f, -0.9f }, { 0.1f, 0.45f } }));
			L.Add(OggArco(-0.15f, 0.45f, 0.25f, 0.3f, 0.f, 200.f, 12));
		}
		else if (N == TEXT("Martello"))
		{
			L.Add(OggRett(-0.6f, 0.3f, 0.6f, 0.7f));
			L.Add(OggRett(-0.1f, -0.9f, 0.1f, 0.3f));
		}
		else if (N == TEXT("Pugnale"))
		{
			L.Add(OggLinea({ { -0.1f, -0.1f }, { 0.f, 0.9f }, { 0.1f, -0.1f } }));
			L.Add(OggLinea({ { -0.45f, -0.1f }, { 0.45f, -0.1f } }));
			L.Add(OggRett(-0.08f, -0.65f, 0.08f, -0.1f));
			L.Add(OggCerchio(0.f, -0.75f, 0.1f, 10));
		}
		else if (N == TEXT("Candeliere"))
		{
			L.Add(OggLinea({ { -0.5f, -0.85f }, { 0.5f, -0.85f } }));
			L.Add(OggArco(0.f, -0.85f, 0.4f, 0.2f, 0.f, 180.f, 10));
			L.Add(OggLinea({ { 0.f, -0.65f }, { 0.f, 0.05f } }));
			L.Add(OggLinea({ { -0.3f, 0.05f }, { 0.3f, 0.05f } }));
			L.Add(OggRett(-0.12f, 0.05f, 0.12f, 0.55f));
			L.Add(OggLinea({ { 0.f, 0.62f }, { 0.08f, 0.75f }, { 0.f, 0.9f }, { -0.08f, 0.75f }, { 0.f, 0.62f } }));
		}
		else if (N == TEXT("Quaderno"))
		{
			L.Add(OggRett(-0.6f, -0.8f, 0.6f, 0.8f));
			L.Add(OggLinea({ { -0.42f, -0.8f }, { -0.42f, 0.8f } }));
			L.Add(OggLinea({ { -0.25f, 0.45f }, { 0.4f, 0.45f } }));
			L.Add(OggLinea({ { -0.25f, 0.2f }, { 0.4f, 0.2f } }));
			L.Add(OggLinea({ { -0.25f, -0.05f }, { 0.25f, -0.05f } }));
		}
		else if (N == TEXT("Medaglietta"))
		{
			L.Add(OggCerchio(0.f, -0.2f, 0.5f, 20));
			L.Add(OggCerchio(0.f, 0.45f, 0.14f, 10));
			L.Add(OggLinea({ { -0.14f, 0.45f }, { -0.6f, 0.9f } }));
			L.Add(OggLinea({ { 0.14f, 0.45f }, { 0.6f, 0.9f } }));
			// il Cuore inciso: due colpi
			L.Add(OggArco(-0.12f, -0.12f, 0.12f, 0.12f, 0.f, 180.f, 8));
			L.Add(OggArco(0.12f, -0.12f, 0.12f, 0.12f, 0.f, 180.f, 8));
			L.Add(OggLinea({ { -0.24f, -0.12f }, { 0.f, -0.45f }, { 0.24f, -0.12f } }));
		}
		else if (N == TEXT("Bilancia"))
		{
			L.Add(OggLinea({ { 0.f, -0.85f }, { 0.f, 0.7f } }));
			L.Add(OggLinea({ { -0.35f, -0.85f }, { 0.35f, -0.85f } }));
			L.Add(OggLinea({ { -0.75f, 0.55f }, { 0.75f, 0.55f } }));
			L.Add(OggLinea({ { -0.75f, 0.55f }, { -0.9f, -0.1f } }));
			L.Add(OggLinea({ { -0.75f, 0.55f }, { -0.6f, -0.1f } }));
			L.Add(OggLinea({ { 0.75f, 0.55f }, { 0.6f, -0.1f } }));
			L.Add(OggLinea({ { 0.75f, 0.55f }, { 0.9f, -0.1f } }));
			L.Add(OggArco(-0.75f, -0.1f, 0.18f, 0.12f, 180.f, 360.f, 8));
			L.Add(OggArco(0.75f, -0.1f, 0.18f, 0.12f, 180.f, 360.f, 8));
		}
		else if (N == TEXT("Trappola"))
		{
			L.Add(OggCerchio(-0.2f, 0.2f, 0.45f, 20));
			L.Add(OggLinea({ { 0.2f, 0.f }, { 0.55f, -0.45f } }));
			L.Add(OggLinea({ { 0.45f, -0.4f }, { 0.75f, -0.9f }, { 0.75f, -0.3f } }));
		}
		else if (N == TEXT("Rete"))
		{
			L.Add(OggArco(0.f, 0.f, 0.75f, 0.75f, 0.f, 360.f, 24));
			for (int32 i = -2; i <= 2; ++i)
			{
				const float K = i * 0.28f;
				const float Mezzo = FMath::Sqrt(FMath::Max(0.f, 0.75f * 0.75f - K * K));
				L.Add(OggLinea({ { K, -Mezzo }, { K, Mezzo } }));
				L.Add(OggLinea({ { -Mezzo, K }, { Mezzo, K } }));
			}
		}
		else if (N == TEXT("Ricordo_Anello"))
		{
			L.Add(OggArco(0.f, -0.2f, 0.5f, 0.5f, 0.f, 360.f, 22));
			L.Add(OggArco(0.f, -0.2f, 0.38f, 0.38f, 0.f, 360.f, 18));
			L.Add(OggLinea({ { 0.f, 0.3f }, { 0.2f, 0.5f }, { 0.f, 0.75f }, { -0.2f, 0.5f }, { 0.f, 0.3f } }));
		}
		else if (N == TEXT("Ricordo_Lettera"))
		{
			L.Add(OggRett(-0.8f, -0.5f, 0.8f, 0.5f));
			L.Add(OggLinea({ { -0.8f, 0.5f }, { 0.f, -0.05f }, { 0.8f, 0.5f } }));
			L.Add(OggCerchio(0.f, -0.05f, 0.16f, 12));
		}
		else if (N == TEXT("Ricordo_Spada"))
		{
			L.Add(OggLinea({ { -0.08f, -0.25f }, { -0.08f, 0.2f }, { 0.04f, 0.28f }, { 0.08f, 0.15f }, { 0.08f, -0.25f } }));
			L.Add(OggLinea({ { 0.2f, 0.42f }, { 0.25f, 0.85f }, { 0.36f, 0.9f }, { 0.36f, 0.48f }, { 0.2f, 0.42f } }));
			L.Add(OggLinea({ { -0.4f, -0.25f }, { 0.4f, -0.25f } }));
			L.Add(OggRett(-0.07f, -0.7f, 0.07f, -0.25f));
			L.Add(OggCerchio(0.f, -0.8f, 0.1f, 10));
		}
		else if (N == TEXT("Ricordo_Amuleto"))
		{
			L.Add(OggArco(0.f, -0.35f, 0.4f, 0.32f, 0.f, 360.f, 18));
			L.Add(OggCerchio(-0.45f, 0.1f, 0.13f, 10));
			L.Add(OggCerchio(-0.16f, 0.32f, 0.13f, 10));
			L.Add(OggCerchio(0.16f, 0.32f, 0.13f, 10));
			L.Add(OggCerchio(0.45f, 0.1f, 0.13f, 10));
			L.Add(OggLinea({ { -0.16f, 0.45f }, { -0.3f, 0.9f } }));
			L.Add(OggLinea({ { 0.16f, 0.45f }, { 0.3f, 0.9f } }));
		}
		else if (N == TEXT("Ricordo_Mappa"))
		{
			L.Add(OggLinea({ { 0.3f, 0.65f }, { -0.8f, 0.65f }, { -0.8f, -0.65f }, { 0.3f, -0.65f }, { 0.45f, -0.4f }, { 0.25f, -0.15f },
				{ 0.48f, 0.1f }, { 0.28f, 0.38f }, { 0.3f, 0.65f } }));
			L.Add(OggLinea({ { -0.6f, -0.4f }, { -0.45f, -0.3f } }));
			L.Add(OggLinea({ { -0.35f, -0.2f }, { -0.25f, 0.f } }));
			L.Add(OggLinea({ { -0.2f, 0.12f }, { -0.05f, 0.22f } }));
			L.Add(OggLinea({ { 0.f, 0.25f }, { 0.18f, 0.45f } }));
			L.Add(OggLinea({ { 0.f, 0.45f }, { 0.18f, 0.25f } }));
		}
		else if (N == TEXT("Spada_Corta"))
		{
			L.Add(OggLinea({ { -0.75f, -0.75f }, { 0.55f, 0.55f }, { 0.8f, 0.85f }, { 0.65f, 0.5f }, { -0.6f, -0.8f } }));
			L.Add(OggLinea({ { -0.65f, -0.35f }, { -0.35f, -0.65f } }));
			L.Add(OggCerchio(-0.82f, -0.82f, 0.08f, 8));
		}
		else if (N == TEXT("Scudo_Legno"))
		{
			L.Add(OggCerchio(0.f, 0.f, 0.8f, 24));
			L.Add(OggCerchio(0.f, 0.f, 0.22f, 12));
			L.Add(OggLinea({ { -0.45f, -0.66f }, { -0.45f, 0.66f } }));
			L.Add(OggLinea({ { 0.45f, -0.66f }, { 0.45f, 0.66f } }));
		}
		else if (N == TEXT("Pelle_Lupo"))
		{
			L.Add(OggLinea({ { -0.2f, 0.8f }, { 0.2f, 0.8f }, { 0.35f, 0.5f }, { 0.8f, 0.45f }, { 0.5f, 0.1f }, { 0.55f, -0.5f },
				{ 0.85f, -0.75f }, { 0.25f, -0.6f }, { 0.f, -0.85f }, { -0.25f, -0.6f }, { -0.85f, -0.75f }, { -0.55f, -0.5f },
				{ -0.5f, 0.1f }, { -0.8f, 0.45f }, { -0.35f, 0.5f }, { -0.2f, 0.8f } }));
		}
		else if (N == TEXT("Zanne_Lupo"))
		{
			L.Add(OggArco(-0.5f, 0.6f, 0.25f, 1.3f, 270.f, 330.f, 8));
			L.Add(OggArco(0.5f, 0.6f, 0.25f, 1.3f, 210.f, 270.f, 8));
			L.Add(OggLinea({ { -0.6f, 0.6f }, { 0.6f, 0.6f } }));
		}
		else if (N == TEXT("Mantello_Lupo"))
		{
			L.Add(OggLinea({ { -0.3f, 0.75f }, { 0.3f, 0.75f }, { 0.75f, -0.8f }, { -0.75f, -0.8f }, { -0.3f, 0.75f } }));
			L.Add(OggLinea({ { -0.55f, -0.8f }, { -0.45f, -0.6f }, { -0.3f, -0.8f }, { -0.15f, -0.6f }, { 0.f, -0.8f }, { 0.15f, -0.6f },
				{ 0.3f, -0.8f }, { 0.45f, -0.6f }, { 0.55f, -0.8f } }));
			L.Add(OggCerchio(0.f, 0.62f, 0.09f, 8));
		}
		else if (N == TEXT("Cozze"))
		{
			L.Add(OggArco(0.f, 0.f, 0.75f, 0.38f, 0.f, 360.f, 20));
			L.Add(OggLinea({ { -0.75f, 0.f }, { 0.75f, 0.f } }));
			L.Add(OggArco(-0.1f, 0.f, 0.45f, 0.22f, 20.f, 160.f, 8));
		}
		else if (N == TEXT("Perla"))
		{
			L.Add(OggCerchio(0.f, 0.f, 0.45f, 18));
			L.Add(OggArco(-0.12f, 0.12f, 0.18f, 0.18f, 100.f, 200.f, 6));
		}
		// (10/11) l'armeria e le prede: disegni a penna, con l'impugnatura in basso a sinistra come la spada corta
		else if (N == TEXT("Accetta"))
		{
			L.Add(OggLinea({ { -0.75f, -0.8f }, { 0.35f, 0.5f } }));
			L.Add(OggLinea({ { -0.68f, -0.86f }, { 0.42f, 0.44f } }));
			L.Add(OggLinea({ { 0.2f, 0.62f }, { 0.75f, 0.5f }, { 0.62f, 0.05f }, { 0.4f, 0.32f } }));
		}
		else if (N == TEXT("Ascia_Guerra"))
		{
			L.Add(OggLinea({ { -0.85f, -0.85f }, { 0.55f, 0.55f } }));
			L.Add(OggArco(0.45f, 0.45f, 0.42f, 0.42f, -45.f, 135.f, 12));
			L.Add(OggLinea({ { 0.75f, 0.15f }, { 0.15f, 0.75f } }));
		}
		else if (N == TEXT("Mazza"))
		{
			L.Add(OggLinea({ { -0.8f, -0.8f }, { 0.3f, 0.3f } }));
			L.Add(OggCerchio(0.45f, 0.45f, 0.25f, 12));
			L.Add(OggLinea({ { 0.45f, 0.1f }, { 0.45f, 0.8f } }));
			L.Add(OggLinea({ { 0.1f, 0.45f }, { 0.8f, 0.45f } }));
		}
		else if (N == TEXT("Clava"))
		{
			L.Add(OggLinea({ { -0.8f, -0.75f }, { -0.1f, 0.05f }, { 0.25f, 0.65f }, { 0.7f, 0.8f }, { 0.8f, 0.4f }, { 0.4f, 0.0f }, { -0.7f, -0.85f } }));
			L.Add(OggCerchio(0.4f, 0.45f, 0.06f, 6));
		}
		else if (N == TEXT("Lancia"))
		{
			L.Add(OggLinea({ { -0.9f, -0.9f }, { 0.5f, 0.5f } }));
			L.Add(OggLinea({ { 0.5f, 0.5f }, { 0.55f, 0.75f }, { 0.9f, 0.9f }, { 0.75f, 0.55f }, { 0.5f, 0.5f } }));
		}
		else if (N == TEXT("Forcone"))
		{
			L.Add(OggLinea({ { -0.9f, -0.9f }, { 0.3f, 0.3f } }));
			L.Add(OggLinea({ { 0.1f, 0.5f }, { 0.5f, 0.1f } }));
			L.Add(OggLinea({ { 0.1f, 0.5f }, { 0.5f, 0.9f } }));
			L.Add(OggLinea({ { 0.3f, 0.3f }, { 0.7f, 0.7f } }));
			L.Add(OggLinea({ { 0.5f, 0.1f }, { 0.9f, 0.5f } }));
		}
		else if (N == TEXT("Bastone_Ferrato"))
		{
			L.Add(OggLinea({ { -0.85f, -0.85f }, { 0.85f, 0.85f } }));
			L.Add(OggLinea({ { -0.85f, -0.7f }, { -0.7f, -0.85f } }));
			L.Add(OggLinea({ { 0.7f, 0.85f }, { 0.85f, 0.7f } }));
			L.Add(OggLinea({ { -0.62f, -0.48f }, { -0.48f, -0.62f } }));
			L.Add(OggLinea({ { 0.48f, 0.62f }, { 0.62f, 0.48f } }));
		}
		else if (N == TEXT("Spada_Lunga"))
		{
			L.Add(OggLinea({ { -0.6f, -0.6f }, { 0.75f, 0.75f }, { 0.9f, 0.9f }, { 0.8f, 0.68f }, { -0.48f, -0.62f } }));
			L.Add(OggLinea({ { -0.72f, -0.3f }, { -0.3f, -0.72f } }));
			L.Add(OggCerchio(-0.82f, -0.82f, 0.09f, 8));
		}
		else if (N == TEXT("Gladio"))
		{
			L.Add(OggLinea({ { -0.55f, -0.7f }, { 0.35f, 0.25f }, { 0.75f, 0.75f }, { 0.25f, 0.35f }, { -0.7f, -0.55f } }));
			L.Add(OggLinea({ { -0.75f, -0.4f }, { -0.4f, -0.75f } }));
			L.Add(OggCerchio(-0.85f, -0.85f, 0.11f, 8));
		}
		else if (N == TEXT("Brocchiere"))
		{
			L.Add(OggCerchio(0.f, 0.f, 0.55f, 20));
			L.Add(OggCerchio(0.f, 0.f, 0.2f, 12));
			L.Add(OggCerchio(0.f, 0.f, 0.08f, 8));
		}
		else if (N == TEXT("Scudo_Rotondo"))
		{
			L.Add(OggCerchio(0.f, 0.f, 0.82f, 24));
			L.Add(OggCerchio(0.f, 0.f, 0.68f, 24));
			L.Add(OggCerchio(0.f, 0.f, 0.18f, 12));
			L.Add(OggLinea({ { 0.f, 0.68f }, { 0.f, -0.68f } }));
			L.Add(OggLinea({ { -0.68f, 0.f }, { 0.68f, 0.f } }));
		}
		else if (N == TEXT("Scudo_Aquilone"))
		{
			L.Add(OggLinea({ { -0.55f, 0.75f }, { 0.55f, 0.75f }, { 0.6f, 0.2f }, { 0.f, -0.9f }, { -0.6f, 0.2f }, { -0.55f, 0.75f } }));
			L.Add(OggLinea({ { 0.f, 0.75f }, { 0.f, -0.9f } }));
			L.Add(OggLinea({ { -0.55f, 0.35f }, { 0.55f, 0.35f } }));
		}
		else if (N == TEXT("Scudo_Legione"))
		{
			L.Add(OggRett(-0.55f, -0.85f, 0.55f, 0.85f));
			L.Add(OggCerchio(0.f, 0.f, 0.2f, 12));
			L.Add(OggLinea({ { -0.4f, 0.55f }, { -0.15f, 0.25f } }));
			L.Add(OggLinea({ { 0.4f, 0.55f }, { 0.15f, 0.25f } }));
			L.Add(OggLinea({ { -0.4f, -0.55f }, { -0.15f, -0.25f } }));
			L.Add(OggLinea({ { 0.4f, -0.55f }, { 0.15f, -0.25f } }));
		}
		else if (N == TEXT("Carne_Cruda"))
		{
			L.Add(OggArco(0.f, 0.f, 0.75f, 0.5f, 0.f, 360.f, 20));
			L.Add(OggArco(0.15f, 0.05f, 0.3f, 0.18f, 0.f, 360.f, 12));
			L.Add(OggLinea({ { -0.55f, -0.2f }, { -0.2f, -0.35f } }));
		}
		else if (N == TEXT("Pelle_Cinghiale") || N == TEXT("Pelle_Orso") || N == TEXT("Pelle_Cervo") || N == TEXT("Pelle_Volpe")
			|| N == TEXT("Pelle_Lepre"))
		{
			// la stessa pelle distesa del lupo, piu' larga o piu' piccola, con un segno per specie
			const float K = N == TEXT("Pelle_Orso") ? 1.0f : (N == TEXT("Pelle_Lepre") ? 0.55f : (N == TEXT("Pelle_Volpe") ? 0.75f : 0.9f));
			TArray<FVector2D> Bordo = OggLinea({ { -0.2f, 0.8f }, { 0.2f, 0.8f }, { 0.35f, 0.5f }, { 0.8f, 0.45f }, { 0.5f, 0.1f },
				{ 0.55f, -0.5f }, { 0.85f, -0.75f }, { 0.25f, -0.6f }, { 0.f, -0.85f }, { -0.25f, -0.6f }, { -0.85f, -0.75f },
				{ -0.55f, -0.5f }, { -0.5f, 0.1f }, { -0.8f, 0.45f }, { -0.35f, 0.5f }, { -0.2f, 0.8f } });
			for (FVector2D& P : Bordo)
			{
				P *= K;
			}
			L.Add(Bordo);
			if (N == TEXT("Pelle_Cinghiale"))
			{
				L.Add(OggLinea({ { 0.f, 0.6f }, { 0.f, -0.6f } }));   // la cresta di setole
			}
			else if (N == TEXT("Pelle_Volpe"))
			{
				L.Add(OggLinea({ { 0.f, -0.64f }, { 0.1f, -0.95f } })); // la coda
			}
			else if (N == TEXT("Pelle_Cervo"))
			{
				L.Add(OggCerchio(-0.2f, 0.f, 0.06f, 6));
				L.Add(OggCerchio(0.2f, 0.2f, 0.06f, 6));
				L.Add(OggCerchio(0.15f, -0.25f, 0.06f, 6));
			}
		}
		else if (N == TEXT("Zanne_Cinghiale"))
		{
			L.Add(OggArco(-0.15f, -0.2f, 0.6f, 0.75f, 180.f, 290.f, 10));
			L.Add(OggArco(0.15f, -0.2f, 0.6f, 0.75f, 250.f, 360.f, 10));
		}
		else if (N == TEXT("Artigli_Orso"))
		{
			for (int32 i = 0; i < 4; ++i)
			{
				const float X = -0.6f + i * 0.4f;
				L.Add(OggLinea({ { X, -0.6f }, { X + 0.05f, 0.3f }, { X + 0.2f, 0.7f } }));
			}
		}
		else if (N == TEXT("Palchi_Cervo"))
		{
			L.Add(OggLinea({ { 0.f, -0.85f }, { -0.15f, 0.f }, { -0.45f, 0.75f } }));
			L.Add(OggLinea({ { -0.15f, 0.f }, { -0.6f, 0.15f } }));
			L.Add(OggLinea({ { -0.3f, 0.4f }, { 0.05f, 0.7f } }));
			L.Add(OggLinea({ { 0.f, -0.85f }, { 0.2f, -0.1f }, { 0.5f, 0.7f } }));
			L.Add(OggLinea({ { 0.2f, -0.1f }, { 0.65f, 0.05f } }));
		}
		// (10/11) cibi cotti, cuoio e vestiti
		else if (N == TEXT("Carne_Cotta"))
		{
			L.Add(OggArco(0.1f, 0.f, 0.65f, 0.45f, 0.f, 360.f, 20));
			L.Add(OggLinea({ { -0.85f, -0.6f }, { -0.45f, -0.25f } }));
			L.Add(OggCerchio(-0.88f, -0.65f, 0.08f, 6));
			L.Add(OggLinea({ { -0.2f, -0.15f }, { 0.1f, 0.15f } }));
			L.Add(OggLinea({ { 0.15f, -0.2f }, { 0.45f, 0.1f } }));
		}
		else if (N == TEXT("Funghi_Arrosto"))
		{
			L.Add(OggLinea({ { -0.85f, -0.85f }, { 0.85f, 0.85f } }));
			L.Add(OggArco(-0.25f, -0.15f, 0.3f, 0.22f, 0.f, 180.f, 10));
			L.Add(OggArco(0.3f, 0.4f, 0.3f, 0.22f, 0.f, 180.f, 10));
		}
		else if (N == TEXT("Stufato") || N == TEXT("Zuppa_Bruno"))
		{
			L.Add(OggArco(0.f, 0.f, 0.8f, 0.7f, 180.f, 360.f, 16));
			L.Add(OggLinea({ { -0.8f, 0.f }, { 0.8f, 0.f } }));
			L.Add(OggArco(-0.25f, 0.35f, 0.12f, 0.2f, 180.f, 360.f, 6));
			L.Add(OggArco(0.2f, 0.45f, 0.12f, 0.2f, 0.f, 180.f, 6));
			if (N == TEXT("Stufato"))
			{
				L.Add(OggLinea({ { 0.45f, -0.1f }, { 0.85f, 0.7f } }));
			}
		}
		else if (N == TEXT("Cuoio"))
		{
			L.Add(OggRett(-0.7f, -0.5f, 0.7f, 0.5f));
			L.Add(OggArco(0.7f, 0.f, 0.2f, 0.5f, 270.f, 450.f, 8));
			L.Add(OggLinea({ { -0.5f, -0.3f }, { 0.4f, -0.3f } }));
		}
		else if (N == TEXT("Mantello_Orso"))
		{
			L.Add(OggLinea({ { -0.35f, 0.75f }, { 0.35f, 0.75f }, { 0.85f, -0.85f }, { -0.85f, -0.85f }, { -0.35f, 0.75f } }));
			L.Add(OggCerchio(-0.25f, 0.82f, 0.1f, 8));
			L.Add(OggCerchio(0.25f, 0.82f, 0.1f, 8));
			L.Add(OggLinea({ { -0.4f, -0.3f }, { 0.4f, -0.3f } }));
		}
		else if (N == TEXT("Cappuccio_Volpe"))
		{
			L.Add(OggArco(0.f, -0.1f, 0.6f, 0.7f, 0.f, 180.f, 14));
			L.Add(OggLinea({ { -0.6f, -0.1f }, { -0.6f, -0.5f }, { 0.6f, -0.5f }, { 0.6f, -0.1f } }));
			L.Add(OggLinea({ { 0.6f, -0.3f }, { 0.85f, -0.6f }, { 0.75f, -0.9f } }));
		}
		else if (N == TEXT("Foglio_Incarico"))
		{
			// un foglio con le righe, l'angolo strappato in alto (dove era appeso) e il sigillo in basso
			L.Add(OggLinea({ { -0.55f, -0.85f }, { 0.55f, -0.85f }, { 0.55f, 0.6f }, { 0.35f, 0.85f }, { -0.55f, 0.85f }, { -0.55f, -0.85f } }));
			L.Add(OggLinea({ { 0.35f, 0.85f }, { 0.4f, 0.65f }, { 0.55f, 0.6f } }));
			L.Add(OggLinea({ { -0.35f, 0.5f }, { 0.25f, 0.5f } }));
			L.Add(OggLinea({ { -0.35f, 0.25f }, { 0.35f, 0.25f } }));
			L.Add(OggLinea({ { -0.35f, 0.f }, { 0.35f, 0.f } }));
			L.Add(OggLinea({ { -0.35f, -0.25f }, { 0.1f, -0.25f } }));
			L.Add(OggCerchio(0.2f, -0.55f, 0.18f, 12));
		}
		else if (N == TEXT("Guanti_Lepre"))
		{
			L.Add(OggLinea({ { -0.4f, -0.8f }, { -0.4f, 0.3f }, { -0.25f, 0.7f }, { -0.1f, 0.3f }, { 0.05f, 0.75f }, { 0.2f, 0.3f },
				{ 0.35f, 0.65f }, { 0.45f, 0.2f }, { 0.75f, 0.35f }, { 0.45f, -0.3f }, { 0.45f, -0.8f }, { -0.4f, -0.8f } }));
			L.Add(OggLinea({ { -0.4f, -0.55f }, { 0.45f, -0.55f } }));
		}
		else if (N == TEXT("Ricordo_Libro"))
		{
			L.Add(OggRett(-0.6f, -0.8f, 0.6f, 0.8f));
			L.Add(OggLinea({ { -0.45f, -0.8f }, { -0.45f, 0.8f } }));
			L.Add(OggLinea({ { 0.08f, -0.35f }, { 0.08f, 0.5f } }));
			L.Add(OggLinea({ { -0.17f, 0.22f }, { 0.33f, 0.22f } }));
		}
		// (11/11) la tessera della carovana, il piccone, la grotta del nord
		else if (N == TEXT("Tessera_Carovana"))
		{
			L.Add(OggRett(-0.75f, -0.45f, 0.75f, 0.45f));
			L.Add(OggCerchio(-0.62f, 0.32f, 0.06f, 8));
			// la corona bruciata a fuoco e il numero inciso
			L.Add(OggLinea({ { -0.4f, -0.05f }, { -0.4f, 0.22f }, { -0.25f, 0.08f }, { -0.1f, 0.28f }, { 0.05f, 0.08f }, { 0.2f, 0.22f },
				{ 0.2f, -0.05f }, { -0.4f, -0.05f } }));
			L.Add(OggLinea({ { -0.45f, -0.25f }, { 0.55f, -0.25f } }));
		}
		else if (N == TEXT("Piccone"))
		{
			L.Add(OggLinea({ { -0.1f, -0.85f }, { 0.12f, 0.55f } }));
			L.Add(OggLinea({ { -0.75f, 0.35f }, { -0.3f, 0.62f }, { 0.12f, 0.68f }, { 0.55f, 0.62f }, { 0.85f, 0.4f } }));
			L.Add(OggLinea({ { -0.75f, 0.35f }, { -0.25f, 0.5f }, { 0.12f, 0.55f }, { 0.5f, 0.5f }, { 0.85f, 0.4f } }));
		}
		else if (N == TEXT("Moneta_Nani"))
		{
			L.Add(OggRett(-0.6f, -0.6f, 0.6f, 0.6f));
			L.Add(OggRett(-0.15f, -0.15f, 0.15f, 0.15f));
			L.Add(OggLinea({ { -0.45f, 0.4f }, { -0.3f, 0.4f }, { -0.3f, 0.28f } }));
			L.Add(OggLinea({ { 0.3f, -0.28f }, { 0.3f, -0.4f }, { 0.45f, -0.4f } }));
		}
		else if (N == TEXT("Pugnale_Nani"))
		{
			L.Add(OggLinea({ { -0.85f, 0.f }, { -0.45f, 0.f } }));
			L.Add(OggRett(-0.5f, -0.22f, -0.38f, 0.22f));
			L.Add(OggLinea({ { -0.38f, -0.14f }, { 0.6f, -0.1f }, { 0.85f, 0.f }, { 0.6f, 0.1f }, { -0.38f, 0.14f } }));
			L.Add(OggRett(-0.95f, -0.08f, -0.85f, 0.08f));
		}
		else if (N == TEXT("Martello_Nani"))
		{
			L.Add(OggLinea({ { -0.1f, -0.9f }, { -0.1f, 0.35f } }));
			L.Add(OggRett(-0.6f, 0.35f, 0.4f, 0.85f));
			L.Add(OggLinea({ { -0.6f, 0.6f }, { 0.4f, 0.6f } }));
			L.Add(OggLinea({ { -0.2f, -0.6f }, { 0.f, -0.6f } }));
			L.Add(OggLinea({ { -0.2f, -0.45f }, { 0.f, -0.45f } }));
		}
		else if (N == TEXT("Secchio") || N.StartsWith(TEXT("Secchio_Acqua_")))
		{
			// il secchio: un trapezio con il manico; pieno, la linea dell'acqua (torbida: con le onde)
			L.Add(OggLinea({ { -0.55f, -0.45f }, { -0.4f, 0.75f }, { 0.4f, 0.75f }, { 0.55f, -0.45f }, { -0.55f, -0.45f } }));
			L.Add(OggLinea({ { -0.55f, -0.45f }, { -0.35f, -0.85f }, { 0.35f, -0.85f }, { 0.55f, -0.45f } }));
			if (N != TEXT("Secchio"))
			{
				L.Add(N.EndsWith(TEXT("Torbida")) ? OggLinea({ { -0.5f, -0.25f }, { -0.25f, -0.32f }, { 0.f, -0.22f }, { 0.25f, -0.32f }, { 0.5f, -0.25f } })
					: OggLinea({ { -0.51f, -0.27f }, { 0.51f, -0.27f } }));
			}
		}
		else if (N == TEXT("Fischietto_di_Livia"))
		{
			L.Add(OggRett(-0.8f, -0.2f, 0.3f, 0.2f));
			L.Add(OggCerchio(0.55f, 0.f, 0.3f, 12));
			L.Add(OggLinea({ { -0.8f, 0.f }, { -0.95f, 0.f } }));
		}
		else if (N == TEXT("Sasso_di_Tobia"))
		{
			L.Add(OggCerchio(0.f, 0.1f, 0.6f, 16));
			L.Add(OggCerchio(-0.4f, -0.45f, 0.15f, 8));
			L.Add(OggCerchio(0.4f, -0.45f, 0.15f, 8));
		}
		else if (N == TEXT("Chiodo_Runico"))
		{
			L.Add(OggRett(-0.35f, -0.85f, 0.35f, -0.65f));
			L.Add(OggLinea({ { 0.f, -0.65f }, { 0.f, 0.9f } }));
			L.Add(OggLinea({ { -0.15f, -0.8f }, { 0.f, -0.7f }, { 0.15f, -0.8f } }));
		}
		else if (N == TEXT("Candela_della_Veglia"))
		{
			L.Add(OggRett(-0.25f, -0.2f, 0.25f, 0.8f));
			L.Add(OggLinea({ { 0.f, -0.2f }, { 0.f, -0.4f } }));
			L.Add(OggLinea({ { 0.f, -0.45f }, { -0.12f, -0.65f }, { 0.f, -0.85f }, { 0.12f, -0.65f }, { 0.f, -0.45f } }));
		}
		else if (N == TEXT("Cuore_di_Pietra"))
		{
			L.Add(OggCerchio(0.f, 0.f, 0.6f, 14));
			L.Add(OggCerchio(0.f, 0.f, 0.22f, 8));
			L.Add(OggLinea({ { -0.6f, 0.f }, { -0.3f, 0.f } }));
			L.Add(OggLinea({ { 0.3f, 0.f }, { 0.6f, 0.f } }));
		}
		else if (N == TEXT("Mappa_Gallerie"))
		{
			L.Add(OggLinea({ { -0.8f, -0.7f }, { 0.75f, -0.7f }, { 0.8f, -0.2f }, { 0.65f, 0.05f }, { 0.8f, 0.35f }, { 0.7f, 0.7f },
				{ -0.8f, 0.7f }, { -0.8f, -0.7f } }));
			L.Add(OggLinea({ { -0.55f, -0.5f }, { -0.55f, 0.f }, { -0.1f, 0.f }, { -0.1f, 0.45f }, { 0.35f, 0.45f } }));
			L.Add(OggRett(-0.3f, -0.15f, 0.1f, 0.15f));
		}
		else if (N == TEXT("Taccuino_Gisella"))
		{
			L.Add(OggRett(-0.55f, -0.75f, 0.55f, 0.75f));
			L.Add(OggLinea({ { -0.4f, -0.75f }, { -0.4f, 0.75f } }));
			L.Add(OggLinea({ { -0.2f, 0.35f }, { 0.35f, 0.35f } }));
			L.Add(OggLinea({ { -0.2f, 0.1f }, { 0.3f, 0.1f } }));
			L.Add(OggLinea({ { -0.2f, -0.15f }, { 0.35f, -0.15f } }));
			L.Add(OggLinea({ { 0.55f, 0.1f }, { 0.75f, 0.1f }, { 0.75f, -0.05f }, { 0.55f, -0.05f } }));
		}
		else if (N == TEXT("Lanterna_Cecco"))
		{
			L.Add(OggArco(0.f, 0.72f, 0.22f, 0.15f, 0.f, 180.f, 10));
			L.Add(OggRett(-0.38f, 0.5f, 0.38f, 0.62f));
			L.Add(OggLinea({ { -0.3f, 0.5f }, { -0.35f, -0.55f }, { 0.35f, -0.55f }, { 0.3f, 0.5f } }));
			L.Add(OggRett(-0.42f, -0.72f, 0.42f, -0.55f));
			L.Add(OggLinea({ { -0.1f, 0.2f }, { 0.15f, -0.05f }, { -0.05f, -0.25f } }));
		}
		else if (N == TEXT("Anello_Ranieri"))
		{
			L.Add(OggCerchio(0.f, -0.15f, 0.55f, 22));
			L.Add(OggCerchio(0.f, -0.15f, 0.38f, 20));
			L.Add(OggRett(-0.18f, 0.38f, 0.18f, 0.62f));
			L.Add(OggLinea({ { 0.f, 0.62f }, { -0.25f, 0.85f } }));
			L.Add(OggLinea({ { 0.f, 0.62f }, { 0.25f, 0.85f } }));
		}
		return L;
	}

	FLineeOggetto OggCreaDisegnoMoneta()
	{
		FLineeOggetto L;
		L.Add(OggCerchio(0.f, 0.f, 0.8f, 24));
		L.Add(OggCerchio(0.f, 0.f, 0.62f, 22));
		// la corona della Corona di Aurelia, piccola, nel mezzo
		L.Add(OggLinea({ { -0.35f, -0.2f }, { -0.35f, 0.15f }, { -0.18f, 0.f }, { 0.f, 0.3f }, { 0.18f, 0.f }, { 0.35f, 0.15f },
			{ 0.35f, -0.2f }, { -0.35f, -0.2f } }));
		return L;
	}
}

namespace ValdorsoOggetti
{
	TArrayView<const FValdorsoTipoOggetto> Catalogo()
	{
		static const TArray<FValdorsoTipoOggetto> Tutti = OggCreaCatalogo();
		return Tutti;
	}

	const FValdorsoTipoOggetto* Trova(FName Id)
	{
		if (Id.IsNone())
		{
			return nullptr;
		}
		for (const FValdorsoTipoOggetto& Tipo : Catalogo())
		{
			if (Tipo.Id == Id)
			{
				return &Tipo;
			}
		}
		return nullptr;
	}

	const TArray<TArray<FVector2D>>& Disegno(FName Id)
	{
		static const TMap<FName, FLineeOggetto> Disegni = []()
		{
			TMap<FName, FLineeOggetto> Tutti;
			for (const FValdorsoTipoOggetto& Tipo : Catalogo())
			{
				Tutti.Add(Tipo.Id, OggDisegnoDi(Tipo.Id));
			}
			return Tutti;
		}();
		static const FLineeOggetto Vuoto;
		const FLineeOggetto* Trovato = Disegni.Find(Id);
		return Trovato ? *Trovato : Vuoto;
	}

	const TArray<TArray<FVector2D>>& DisegnoMoneta()
	{
		static const FLineeOggetto Moneta = OggCreaDisegnoMoneta();
		return Moneta;
	}

	FName AttrezzoDelMestiere(const FString& Mestiere)
	{
		struct FAttrezzo
		{
			const TCHAR* Mestiere;
			const TCHAR* Oggetto;
		};
		static const FAttrezzo Attrezzi[] = {
			{ TEXT("contadino"), TEXT("Falcetto") },
			{ TEXT("pastore"), TEXT("Bastone_Pastore") },
			{ TEXT("apprendista"), TEXT("Martello") },
			{ TEXT("soldato"), TEXT("Pugnale") },
			{ TEXT("servo"), TEXT("Candeliere") },
			{ TEXT("allievo"), TEXT("Quaderno") },
			{ TEXT("orfano"), TEXT("Medaglietta") },
			{ TEXT("mercante"), TEXT("Bilancia") },
			{ TEXT("cacciatore"), TEXT("Trappola") },
			{ TEXT("marinaio"), TEXT("Rete") },
		};
		for (const FAttrezzo& Voce : Attrezzi)
		{
			if (Mestiere == Voce.Mestiere)
			{
				return FName(Voce.Oggetto);
			}
		}
		return NAME_None;
	}

	FName OggettoDelRicordo(const FString& Ricordo)
	{
		// lo stesso Id che dà il Registro (ValdorsoRegistro::OggettoDelRicordo), se è nel catalogo
		const FName Id = ValdorsoRegistro::OggettoDelRicordo(Ricordo).Id;
		return Trova(Id) ? Id : NAME_None;
	}

	TArray<TPair<FName, int32>> Partenza(const FValdorsoRegistro* Registro)
	{
		TArray<TPair<FName, int32>> Kit;
		Kit.Emplace(TEXT("Tessera"), 1);
		// (11/11, v0.1.5) e quella di legno della carovana, il ricordo del viaggio (dalle Cronache, Capitolo V)
		Kit.Emplace(TEXT("Tessera_Carovana"), 1);
		Kit.Emplace(TEXT("Pane_Nero"), 3);
		Kit.Emplace(TEXT("Mela"), 2);
		Kit.Emplace(TEXT("Borraccia"), 1);
		Kit.Emplace(TEXT("Coltello"), 1);
		Kit.Emplace(TEXT("Acciarino"), 1);
		Kit.Emplace(TEXT("Rami_Secchi"), 4);
		Kit.Emplace(TEXT("Mappa_Valle"), 1);
		Kit.Emplace(TEXT("Mantello"), 1);
		if (Registro)
		{
			const FName Attrezzo = AttrezzoDelMestiere(Registro->Mestiere);
			if (!Attrezzo.IsNone())
			{
				Kit.Emplace(Attrezzo, 1);
			}
			const FName Ricordo = OggettoDelRicordo(Registro->Ricordo);
			if (!Ricordo.IsNone())
			{
				Kit.Emplace(Ricordo, 1);
			}
		}
		return Kit;
	}

	int32 MonetePartenza(const FValdorsoRegistro* Registro)
	{
		return (Registro && Registro->Motivo == TEXT("debito")) ? MoneteDebito : MoneteIniziali;
	}

	bool DonoDelSegno(FName IdDialogo, FName Segno, FName& OutOggetto, int32& OutQuantita)
	{
		struct FDono
		{
			const TCHAR* Dialogo;
			const TCHAR* Segno;
			const TCHAR* Oggetto;
			int32 Quantita;
		};
		static const FDono Doni[] = {
			{ TEXT("Bruno"), TEXT("PrimaScodella"), TEXT("Pane_Nero"), 1 },
			{ TEXT("NonnaEdda"), TEXT("DovutoRispetto"), TEXT("Erbe_Palude"), 2 },
			// (11/11, v0.1.8) le storie dei volti nuovi (ValdorsoStorie.h)
			{ TEXT("Livia"), TEXT("LetteraArrivata"), TEXT("Fischietto_di_Livia"), 1 },
			{ TEXT("Tobia"), TEXT("SassoTrovato"), TEXT("Sasso_di_Tobia"), 1 },
			{ TEXT("Marta"), TEXT("Canta"), TEXT("Chiodo_Runico"), 1 },
		};
		for (const FDono& Dono : Doni)
		{
			if (IdDialogo == FName(Dono.Dialogo) && Segno == FName(Dono.Segno))
			{
				OutOggetto = FName(Dono.Oggetto);
				OutQuantita = Dono.Quantita;
				return true;
			}
		}
		return false;
	}

	float FattoreVelocita(float PesoKg)
	{
		return PesoKg > SovraccaricoKg ? 0.6f : 1.f;
	}

	bool PuoCorrere(float PesoKg)
	{
		return PesoKg <= PortataKg;
	}

	FString TestoPeso(float PesoKg)
	{
		FString Testo = FString::Printf(TEXT("%.1f kg"), FMath::Max(0.f, PesoKg));
		return Testo.Replace(TEXT("."), TEXT(","));
	}
}
