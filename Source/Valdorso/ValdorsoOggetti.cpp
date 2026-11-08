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
		C.Add(OggSemplice(TEXT("Corda"), TEXT("Corda di canapa"),
			TEXT("Dieci braccia di corda. Non si sa mai."), 1.0f));
		{
			FValdorsoTipoOggetto Tessera = OggDaLeggere(OggRicordo(TEXT("Tessera"), TEXT("Tessera del colono"),
				TEXT("Il lasciapassare della carovana, con il timbro della capitana al passo. Senza, dal passo non si torna."), 0.02f),
				TEXT("«Colono della valle dell'Orso, anno 312 dopo il Crepuscolo. Visto al passo: L. Serra, capitana.»"));
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
		else if (N == TEXT("Ricordo_Libro"))
		{
			L.Add(OggRett(-0.6f, -0.8f, 0.6f, 0.8f));
			L.Add(OggLinea({ { -0.45f, -0.8f }, { -0.45f, 0.8f } }));
			L.Add(OggLinea({ { 0.08f, -0.35f }, { 0.08f, 0.5f } }));
			L.Add(OggLinea({ { -0.17f, 0.22f }, { 0.33f, 0.22f } }));
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
		Kit.Emplace(TEXT("Pane_Nero"), 3);
		Kit.Emplace(TEXT("Mela"), 2);
		Kit.Emplace(TEXT("Borraccia"), 1);
		Kit.Emplace(TEXT("Coltello"), 1);
		Kit.Emplace(TEXT("Acciarino"), 1);
		Kit.Emplace(TEXT("Rami_Secchi"), 4);
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
