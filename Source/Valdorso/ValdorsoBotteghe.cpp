// Valdorso - Le botteghe (v0.1.4, 10/11/2026). Scritto da Claude. Vedi ValdorsoBotteghe.h.

#include "ValdorsoBotteghe.h"
#include "ValdorsoPNG.h"
#include "Valdorso.h"
#include "ValdorsoFalo.h"
#include "ValdorsoOggetti.h"
#include "ValdorsoQualita.h"
#include "ValdorsoZaino.h"
#include "ValdorsoZainoDati.h"
#include "Components/SphereComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

#define LOCTEXT_NAMESPACE "ValdorsoBotteghe"

namespace
{
	FValdorsoRicetta BottegaRicetta(const TCHAR* Id, EValdorsoTipoBanco Banco, const TCHAR* Verbo, const TCHAR* Risultato, int32 Quanti = 1)
	{
		FValdorsoRicetta R;
		R.Id = Id;
		R.Banco = Banco;
		R.Verbo = Verbo;
		R.Risultato = Risultato;
		R.QuantiRisultato = Quanti;
		return R;
	}

	FValdorsoRicetta BottegaServe(FValdorsoRicetta R, const TCHAR* A, int32 QA, const TCHAR* B = nullptr, int32 QB = 0,
		const TCHAR* C = nullptr, int32 QC = 0)
	{
		R.Serve[0] = { A, QA };
		R.Serve[1] = { B, QB };
		R.Serve[2] = { C, QC };
		return R;
	}

	FValdorsoRicetta BottegaCompra(const TCHAR* Id, EValdorsoTipoBanco Banco, const TCHAR* Oggetto, int32 Prezzo)
	{
		FValdorsoRicetta R = BottegaRicetta(Id, Banco, TEXT("Compra"), Oggetto);
		R.Costo = Prezzo;
		return R;
	}

	/** (14/11, v0.2.3) Una cosa fatta da una bottega: con il grado e la provenienza. */
	FValdorsoRicetta BottegaDi(FValdorsoRicetta R, const TCHAR* Provenienza, uint8 Qualita = 0)
	{
		R.Provenienza = Provenienza;
		R.Qualita = Qualita;
		return R;
	}

	/** (14/11, v0.2.3) Il colono forgia (o fa) da se': il grado lo decide la sua mano. */
	FValdorsoRicetta BottegaForgia(FValdorsoRicetta R)
	{
		R.bForgia = true;
		return R;
	}

	/** (14/11, v0.2.3) La riparazione dell'arma in mano (1 ferro, 2 legno), per tanti Aureus. */
	FValdorsoRicetta BottegaRipara(const TCHAR* Id, EValdorsoTipoBanco Banco, uint8 Materiale, int32 Prezzo)
	{
		FValdorsoRicetta R = BottegaRicetta(Id, Banco, TEXT("Ripara"), nullptr, 0);
		R.Ripara = Materiale;
		R.Costo = Prezzo;
		return R;
	}

	FValdorsoRicetta BottegaVendi(const TCHAR* Id, EValdorsoTipoBanco Banco, const TCHAR* Oggetto, int32 Prezzo)
	{
		FValdorsoRicetta R = BottegaServe(BottegaRicetta(Id, Banco, TEXT("Vendi"), nullptr, 0), Oggetto, 1);
		R.Guadagno = Prezzo;
		return R;
	}

	TArray<FValdorsoRicetta> BottegaCreaRicette()
	{
		using B = EValdorsoTipoBanco;
		TArray<FValdorsoRicetta> T;
		// --- il fuoco (un falo' acceso; anche la locanda ha il suo)
		T.Add(BottegaServe(BottegaRicetta(TEXT("Cuoci_Carne"), B::Fuoco, TEXT("Cuoci"), TEXT("Carne_Cotta")), TEXT("Carne_Cruda"), 1));
		T.Add(BottegaServe(BottegaRicetta(TEXT("Cuoci_Funghi"), B::Fuoco, TEXT("Cuoci"), TEXT("Funghi_Arrosto")), TEXT("Funghi"), 2));
		T.Add(BottegaServe(BottegaRicetta(TEXT("Cuoci_Stufato"), B::Fuoco, TEXT("Cuoci"), TEXT("Stufato")),
			TEXT("Carne_Cruda"), 2, TEXT("Funghi"), 1));
		// --- la conceria: le pelli diventano cuoio e vestiti caldi; le pelli si vendono
		T.Add(BottegaServe(BottegaRicetta(TEXT("Concia_Cinghiale"), B::Conceria, TEXT("Concia"), TEXT("Cuoio"), 2), TEXT("Pelle_Cinghiale"), 1));
		T.Add(BottegaServe(BottegaRicetta(TEXT("Concia_Cervo"), B::Conceria, TEXT("Concia"), TEXT("Cuoio"), 2), TEXT("Pelle_Cervo"), 1));
		{
			FValdorsoRicetta R = BottegaServe(BottegaRicetta(TEXT("Mantello_Lupo"), B::Conceria, TEXT("Cuci"), TEXT("Mantello_Lupo")),
				TEXT("Pelle_Lupo"), 2, TEXT("Cuoio"), 1);
			R.Costo = 5;
			T.Add(R);
		}
		{
			FValdorsoRicetta R = BottegaServe(BottegaRicetta(TEXT("Mantello_Orso"), B::Conceria, TEXT("Cuci"), TEXT("Mantello_Orso")),
				TEXT("Pelle_Orso"), 1, TEXT("Cuoio"), 2);
			R.Costo = 10;
			T.Add(R);
		}
		T.Add(BottegaServe(BottegaRicetta(TEXT("Cappuccio_Volpe"), B::Conceria, TEXT("Cuci"), TEXT("Cappuccio_Volpe")),
			TEXT("Pelle_Volpe"), 2, TEXT("Cuoio"), 1));
		T.Add(BottegaServe(BottegaRicetta(TEXT("Guanti_Lepre"), B::Conceria, TEXT("Cuci"), TEXT("Guanti_Lepre")),
			TEXT("Pelle_Lepre"), 3, TEXT("Cuoio"), 1));
		T.Add(BottegaVendi(TEXT("Vendi_Pelle_Lupo"), B::Conceria, TEXT("Pelle_Lupo"), 8));
		T.Add(BottegaVendi(TEXT("Vendi_Pelle_Cinghiale"), B::Conceria, TEXT("Pelle_Cinghiale"), 7));
		T.Add(BottegaVendi(TEXT("Vendi_Pelle_Orso"), B::Conceria, TEXT("Pelle_Orso"), 40));
		T.Add(BottegaVendi(TEXT("Vendi_Pelle_Cervo"), B::Conceria, TEXT("Pelle_Cervo"), 10));
		T.Add(BottegaVendi(TEXT("Vendi_Pelle_Volpe"), B::Conceria, TEXT("Pelle_Volpe"), 12));
		T.Add(BottegaVendi(TEXT("Vendi_Pelle_Lepre"), B::Conceria, TEXT("Pelle_Lepre"), 2));
		// --- la fucina di Marta: armi e scudi di ferro; compra zanne, artigli e palchi (per i manici e i ciondoli)
		T.Add(BottegaDi(BottegaCompra(TEXT("Compra_Coltello"), B::Fucina, TEXT("Coltello"), 5), TEXT("Fattura di Marta")));
		T.Add(BottegaDi(BottegaCompra(TEXT("Compra_Pugnale"), B::Fucina, TEXT("Pugnale"), 15), TEXT("Fattura di Marta")));
		T.Add(BottegaDi(BottegaCompra(TEXT("Compra_Accetta"), B::Fucina, TEXT("Accetta"), 18), TEXT("Fattura di Marta")));
		T.Add(BottegaDi(BottegaCompra(TEXT("Compra_Martello"), B::Fucina, TEXT("Martello"), 12), TEXT("Fattura di Marta")));
		T.Add(BottegaDi(BottegaCompra(TEXT("Compra_Falcetto"), B::Fucina, TEXT("Falcetto"), 10), TEXT("Fattura di Marta")));
		T.Add(BottegaDi(BottegaCompra(TEXT("Compra_Lancia"), B::Fucina, TEXT("Lancia"), 25), TEXT("Fattura di Marta")));
		T.Add(BottegaDi(BottegaCompra(TEXT("Compra_Spada_Corta"), B::Fucina, TEXT("Spada_Corta"), 35), TEXT("Fattura di Marta")));
		T.Add(BottegaDi(BottegaCompra(TEXT("Compra_Mazza"), B::Fucina, TEXT("Mazza"), 40), TEXT("Fattura di Marta")));
		T.Add(BottegaDi(BottegaCompra(TEXT("Compra_Gladio"), B::Fucina, TEXT("Gladio"), 60), TEXT("Fattura di Marta")));
		T.Add(BottegaDi(BottegaCompra(TEXT("Compra_Ascia_Guerra"), B::Fucina, TEXT("Ascia_Guerra"), 70), TEXT("Fattura di Marta")));
		T.Add(BottegaDi(BottegaCompra(TEXT("Compra_Spada_Lunga"), B::Fucina, TEXT("Spada_Lunga"), 85), TEXT("Fattura di Marta")));
		T.Add(BottegaDi(BottegaCompra(TEXT("Compra_Brocchiere"), B::Fucina, TEXT("Brocchiere"), 20), TEXT("Fattura di Marta")));
		T.Add(BottegaDi(BottegaCompra(TEXT("Compra_Scudo_Aquilone"), B::Fucina, TEXT("Scudo_Aquilone"), 55), TEXT("Fattura di Marta")));
		T.Add(BottegaDi(BottegaCompra(TEXT("Compra_Scudo_Legione"), B::Fucina, TEXT("Scudo_Legione"), 90), TEXT("Fattura di Marta")));
		T.Add(BottegaVendi(TEXT("Vendi_Zanne_Lupo"), B::Fucina, TEXT("Zanne_Lupo"), 3));
		T.Add(BottegaVendi(TEXT("Vendi_Zanne_Cinghiale"), B::Fucina, TEXT("Zanne_Cinghiale"), 4));
		T.Add(BottegaVendi(TEXT("Vendi_Artigli_Orso"), B::Fucina, TEXT("Artigli_Orso"), 15));
		// (11/11, v0.1.7) la grotta del nord: il piccone per la frana; le monete dei nani si vendono per il metallo
		T.Add(BottegaDi(BottegaCompra(TEXT("Compra_Piccone"), B::Fucina, TEXT("Piccone"), 14), TEXT("Fattura di Marta")));
		T.Add(BottegaVendi(TEXT("Vendi_Moneta_Nani"), B::Fucina, TEXT("Moneta_Nani"), 6));
		// (11/11) il cuore del guardiano della forgia: Marta lo paga bene (e non dice per farci cosa)
		T.Add(BottegaVendi(TEXT("Vendi_Cuore_di_Pietra"), B::Fucina, TEXT("Cuore_di_Pietra"), 45));
		// --- il falegname: legno
		T.Add(BottegaDi(BottegaCompra(TEXT("Compra_Clava"), B::Falegname, TEXT("Clava"), 6), TEXT("Fattura del falegname")));
		// (12/11, v0.2.0) la carovana di Sibilla: sale, spezie, stoffa e una lanterna; compra perle, ferro e pelli fini
		T.Add(BottegaCompra(TEXT("Carovana_Sale"), B::Carovana, TEXT("Sale"), 3));
		T.Add(BottegaCompra(TEXT("Carovana_Spezie"), B::Carovana, TEXT("Spezie"), 14));
		T.Add(BottegaCompra(TEXT("Carovana_Stoffa"), B::Carovana, TEXT("Stoffa_Fine"), 20));
		T.Add(BottegaVendi(TEXT("Carovana_Vendi_Perla"), B::Carovana, TEXT("Perla"), 28));
		T.Add(BottegaVendi(TEXT("Carovana_Vendi_Lingotto"), B::Carovana, TEXT("Lingotto_Ferro"), 11));
		T.Add(BottegaVendi(TEXT("Carovana_Vendi_Chiodi"), B::Carovana, TEXT("Chiodi"), 1));
		// (11/11, v0.1.9 "Mani e materiali") lavorare i materiali: assi, ferro, chiodi, attrezzi, pietra
		T.Add(BottegaServe(BottegaRicetta(TEXT("Sega_Assi"), B::Falegname, TEXT("Sega"), TEXT("Asse"), 2), TEXT("Legna"), 1));
		T.Add(BottegaServe(BottegaRicetta(TEXT("Fai_Secchio"), B::Falegname, TEXT("Fai"), TEXT("Secchio")), TEXT("Asse"), 2, TEXT("Chiodi"), 3));
		T.Add(BottegaVendi(TEXT("Vendi_Legna"), B::Falegname, TEXT("Legna"), 2));
		T.Add(BottegaVendi(TEXT("Vendi_Asse"), B::Falegname, TEXT("Asse"), 2));
		T.Add(BottegaServe(BottegaRicetta(TEXT("Fondi_Ferro"), B::Fucina, TEXT("Fondi"), TEXT("Lingotto_Ferro")), TEXT("Ferro_Grezzo"), 2, TEXT("Legna"), 1));
		T.Add(BottegaServe(BottegaRicetta(TEXT("Batti_Chiodi"), B::Fucina, TEXT("Batti"), TEXT("Chiodi"), 12), TEXT("Lingotto_Ferro"), 1));
		T.Add(BottegaForgia(BottegaServe(BottegaRicetta(TEXT("Forgia_Accetta"), B::Fucina, TEXT("Forgia"), TEXT("Accetta")), TEXT("Lingotto_Ferro"), 1, TEXT("Asse"), 1)));
		T.Add(BottegaForgia(BottegaServe(BottegaRicetta(TEXT("Forgia_Piccone"), B::Fucina, TEXT("Forgia"), TEXT("Piccone")), TEXT("Lingotto_Ferro"), 2, TEXT("Asse"), 1)));
		T.Add(BottegaServe(BottegaRicetta(TEXT("Squadra_Pietra"), B::Fucina, TEXT("Squadra"), TEXT("Pietra_Squadrata")), TEXT("Pietra_Grezza"), 2));
		T.Add(BottegaVendi(TEXT("Vendi_Ferro_Grezzo"), B::Fucina, TEXT("Ferro_Grezzo"), 3));
		T.Add(BottegaVendi(TEXT("Vendi_Lingotto"), B::Fucina, TEXT("Lingotto_Ferro"), 9));
		// (11/11, v0.2 "L'acqua della valle") il secchio, e Bruno compra l'acqua della cascata (per la birra)
		T.Add(BottegaCompra(TEXT("Compra_Secchio"), B::Falegname, TEXT("Secchio"), 5));
		{
			// il secchio torna indietro vuoto
			FValdorsoRicetta Acqua = BottegaServe(BottegaRicetta(TEXT("Vendi_Acqua_Pura"), B::Locanda, TEXT("Vendi"), TEXT("Secchio"), 1),
				TEXT("Secchio_Acqua_Pura"), 1);
			Acqua.Guadagno = 4;
			T.Add(Acqua);
		}
		T.Add(BottegaDi(BottegaCompra(TEXT("Compra_Bastone_Pastore"), B::Falegname, TEXT("Bastone_Pastore"), 6), TEXT("Fattura del falegname")));
		T.Add(BottegaDi(BottegaCompra(TEXT("Compra_Bastone_Ferrato"), B::Falegname, TEXT("Bastone_Ferrato"), 12), TEXT("Fattura del falegname")));
		T.Add(BottegaDi(BottegaCompra(TEXT("Compra_Forcone"), B::Falegname, TEXT("Forcone"), 10), TEXT("Fattura del falegname")));
		T.Add(BottegaDi(BottegaCompra(TEXT("Compra_Scudo_Legno"), B::Falegname, TEXT("Scudo_Legno"), 15), TEXT("Fattura del falegname")));
		T.Add(BottegaDi(BottegaCompra(TEXT("Compra_Scudo_Rotondo"), B::Falegname, TEXT("Scudo_Rotondo"), 30), TEXT("Fattura del falegname")));
		T.Add(BottegaVendi(TEXT("Vendi_Palchi_Cervo"), B::Falegname, TEXT("Palchi_Cervo"), 8));
		T.Add(BottegaVendi(TEXT("Vendi_Rami"), B::Falegname, TEXT("Rami_Secchi"), 1));
		// (14/11, v0.2.3 "Armi vere") Marta vende anche armi buone (verdi) e ripara il ferro; il colono forgia da se' le armi
		// (il grado lo decide la sua abilita' "Bottega"); il falegname vende l'arco e le frecce, ripara il legno, e insegna a
		// farli
		T.Add(BottegaDi(BottegaCompra(TEXT("Compra_Spada_Corta_Buona"), B::Fucina, TEXT("Spada_Corta"), 75), TEXT("Fattura di Marta"), 1));
		T.Add(BottegaDi(BottegaCompra(TEXT("Compra_Pugnale_Buono"), B::Fucina, TEXT("Pugnale"), 35), TEXT("Fattura di Marta"), 1));
		T.Add(BottegaDi(BottegaCompra(TEXT("Compra_Mazza_Buona"), B::Fucina, TEXT("Mazza"), 85), TEXT("Fattura di Marta"), 1));
		T.Add(BottegaRipara(TEXT("Ripara_Ferro"), B::Fucina, 1, 6));
		T.Add(BottegaForgia(BottegaServe(BottegaRicetta(TEXT("Forgia_Coltello"), B::Fucina, TEXT("Forgia"), TEXT("Coltello")), TEXT("Lingotto_Ferro"), 1)));
		T.Add(BottegaForgia(BottegaServe(BottegaRicetta(TEXT("Forgia_Pugnale"), B::Fucina, TEXT("Forgia"), TEXT("Pugnale")), TEXT("Lingotto_Ferro"), 1, TEXT("Cuoio"), 1)));
		T.Add(BottegaForgia(BottegaServe(BottegaRicetta(TEXT("Forgia_Lancia"), B::Fucina, TEXT("Forgia"), TEXT("Lancia")), TEXT("Lingotto_Ferro"), 1, TEXT("Asse"), 2)));
		T.Add(BottegaForgia(BottegaServe(BottegaRicetta(TEXT("Forgia_Spada_Corta"), B::Fucina, TEXT("Forgia"), TEXT("Spada_Corta")), TEXT("Lingotto_Ferro"), 2, TEXT("Cuoio"), 1)));
		T.Add(BottegaForgia(BottegaServe(BottegaRicetta(TEXT("Forgia_Mazza"), B::Fucina, TEXT("Forgia"), TEXT("Mazza")), TEXT("Lingotto_Ferro"), 2, TEXT("Asse"), 1)));
		T.Add(BottegaForgia(BottegaServe(BottegaRicetta(TEXT("Forgia_Gladio"), B::Fucina, TEXT("Forgia"), TEXT("Gladio")), TEXT("Lingotto_Ferro"), 3, TEXT("Cuoio"), 1)));
		T.Add(BottegaForgia(BottegaServe(BottegaRicetta(TEXT("Forgia_Ascia_Guerra"), B::Fucina, TEXT("Forgia"), TEXT("Ascia_Guerra")), TEXT("Lingotto_Ferro"), 3, TEXT("Asse"), 1)));
		T.Add(BottegaForgia(BottegaServe(BottegaRicetta(TEXT("Forgia_Spada_Lunga"), B::Fucina, TEXT("Forgia"), TEXT("Spada_Lunga")), TEXT("Lingotto_Ferro"), 4, TEXT("Cuoio"), 1)));
		T.Add(BottegaForgia(BottegaServe(BottegaRicetta(TEXT("Forgia_Brocchiere"), B::Fucina, TEXT("Forgia"), TEXT("Brocchiere")), TEXT("Lingotto_Ferro"), 2)));
		T.Add(BottegaDi(BottegaCompra(TEXT("Compra_Arco"), B::Falegname, TEXT("Arco"), 28), TEXT("Fattura del falegname")));
		{
			// le frecce si comprano a dieci
			FValdorsoRicetta Mazzo = BottegaCompra(TEXT("Compra_Frecce_10"), B::Falegname, TEXT("Frecce"), 9);
			Mazzo.QuantiRisultato = 10;
			T.Add(Mazzo);
		}
		T.Add(BottegaRipara(TEXT("Ripara_Legno"), B::Falegname, 2, 3));
		T.Add(BottegaForgia(BottegaServe(BottegaRicetta(TEXT("Fai_Arco"), B::Falegname, TEXT("Fai"), TEXT("Arco")), TEXT("Asse"), 2, TEXT("Corda"), 1)));
		T.Add(BottegaServe(BottegaRicetta(TEXT("Fai_Frecce"), B::Falegname, TEXT("Fai"), TEXT("Frecce"), 10), TEXT("Asse"), 1, TEXT("Chiodi"), 2));
		T.Add(BottegaForgia(BottegaServe(BottegaRicetta(TEXT("Fai_Scudo_Legno"), B::Falegname, TEXT("Fai"), TEXT("Scudo_Legno")), TEXT("Asse"), 3, TEXT("Chiodi"), 4)));
		// --- la locanda di Bruno: da mangiare (e il suo fuoco); compra la carne e i funghi
		T.Add(BottegaCompra(TEXT("Compra_Zuppa"), B::Locanda, TEXT("Zuppa_Bruno"), 3));
		T.Add(BottegaCompra(TEXT("Compra_Pane"), B::Locanda, TEXT("Pane_Nero"), 2));
		T.Add(BottegaCompra(TEXT("Compra_Mela"), B::Locanda, TEXT("Mela"), 1));
		T.Add(BottegaCompra(TEXT("Compra_Formaggio"), B::Locanda, TEXT("Formaggio"), 3));
		T.Add(BottegaServe(BottegaRicetta(TEXT("Locanda_Stufato"), B::Locanda, TEXT("Cuoci"), TEXT("Stufato")),
			TEXT("Carne_Cruda"), 2, TEXT("Funghi"), 1));
		T.Add(BottegaServe(BottegaRicetta(TEXT("Locanda_Carne"), B::Locanda, TEXT("Cuoci"), TEXT("Carne_Cotta")), TEXT("Carne_Cruda"), 1));
		T.Add(BottegaVendi(TEXT("Vendi_Carne"), B::Locanda, TEXT("Carne_Cruda"), 2));
		T.Add(BottegaVendi(TEXT("Vendi_Funghi"), B::Locanda, TEXT("Funghi"), 1));
		T.Add(BottegaVendi(TEXT("Vendi_Cozze"), B::Locanda, TEXT("Cozze"), 1));
		T.Add(BottegaVendi(TEXT("Vendi_Perla"), B::Locanda, TEXT("Perla"), 20));
		return T;
	}

	FText BottegaNome(const TCHAR* Id)
	{
		const FValdorsoTipoOggetto* Tipo = Id ? ValdorsoOggetti::Trova(Id) : nullptr;
		return FText::FromString(Tipo ? FString(Tipo->Nome) : FString(Id ? Id : TEXT("?")));
	}
}

namespace ValdorsoBotteghe
{
	TArrayView<const FValdorsoRicetta> Ricette()
	{
		static const TArray<FValdorsoRicetta> Tabella = BottegaCreaRicette();
		return Tabella;
	}

	const FValdorsoRicetta* Trova(FName Id)
	{
		for (const FValdorsoRicetta& R : Ricette())
		{
			if (Id == FName(R.Id))
			{
				return &R;
			}
		}
		return nullptr;
	}

	TArray<const FValdorsoRicetta*> Del(EValdorsoTipoBanco Banco)
	{
		TArray<const FValdorsoRicetta*> Lista;
		for (const FValdorsoRicetta& R : Ricette())
		{
			if (R.Banco == Banco)
			{
				Lista.Add(&R);
			}
		}
		return Lista;
	}

	bool ManoDaRiparare(const FValdorsoZaino& Zaino, const FValdorsoRicetta& Ricetta, bool& bOutSinistra)
	{
		bOutSinistra = false;
		if (Ricetta.Ripara == 0)
		{
			return false;
		}
		const bool bLegno = Ricetta.Ripara == 2;
		for (const bool bSinistra : { false, true })
		{
			const FName Id = bSinistra ? Zaino.Sinistra : Zaino.Destra;
			const int32 Posto = ValdorsoZaino::PostoDaRiparare(Zaino, Id);
			if (Zaino.Posti.IsValidIndex(Posto) && ValdorsoQualita::DiLegno(Id) == bLegno)
			{
				bOutSinistra = bSinistra;
				return true;
			}
		}
		return false;
	}

	FText Manca(const FValdorsoZaino& Zaino, const FValdorsoRicetta& Ricetta)
	{
		// (14/11, v0.2.3) la riparazione: serve l'arma (o lo scudo) in mano, consumata, del materiale giusto
		if (Ricetta.Ripara != 0)
		{
			bool bSinistra = false;
			if (!ManoDaRiparare(Zaino, Ricetta, bSinistra))
			{
				return Ricetta.Ripara == 2 ? LOCTEXT("NienteLegno", "Impugna l'arma di legno (o lo scudo) da riparare")
					: LOCTEXT("NienteFerro", "Impugna l'arma di ferro (o lo scudo) da riparare");
			}
			if (Zaino.Monete < Ricetta.Costo)
			{
				return FText::Format(LOCTEXT("ServeMoneteRipara", "Ti servono {0} Aureus"), FText::AsNumber(Ricetta.Costo));
			}
			return FText::GetEmpty();
		}
		for (const FValdorsoIngrediente& Ingrediente : Ricetta.Serve)
		{
			if (Ingrediente.Id && Ingrediente.Quanti > 0 && ValdorsoZaino::Conta(Zaino, Ingrediente.Id) < Ingrediente.Quanti)
			{
				return FText::Format(LOCTEXT("ServeOggetto", "Ti serve: {0} × {1}"), FText::AsNumber(Ingrediente.Quanti), BottegaNome(Ingrediente.Id));
			}
		}
		if (Zaino.Monete < Ricetta.Costo)
		{
			return FText::Format(LOCTEXT("ServeMonete", "Ti servono {0} Aureus"), FText::AsNumber(Ricetta.Costo));
		}
		if (Ricetta.Risultato)
		{
			// c'e' posto? si prova su una copia
			FValdorsoZaino Prova = Zaino;
			for (const FValdorsoIngrediente& Ingrediente : Ricetta.Serve)
			{
				if (Ingrediente.Id && Ingrediente.Quanti > 0)
				{
					ValdorsoZaino::TogliOggetto(Prova, Ingrediente.Id, Ingrediente.Quanti);
				}
			}
			if (ValdorsoZaino::AggiungiCon(Prova, Ricetta.Risultato, Ricetta.QuantiRisultato, Ricetta.Qualita,
				Ricetta.Provenienza ? FString(Ricetta.Provenienza) : FString()) < Ricetta.QuantiRisultato)
			{
				return LOCTEXT("ZainoPieno", "Lo zaino è pieno");
			}
		}
		return FText::GetEmpty();
	}

	bool Fai(FValdorsoZaino& Zaino, const FValdorsoRicetta& Ricetta)
	{
		return FaiCon(Zaino, Ricetta, Ricetta.Qualita, Ricetta.Provenienza ? FString(Ricetta.Provenienza) : FString());
	}

	bool FaiCon(FValdorsoZaino& Zaino, const FValdorsoRicetta& Ricetta, uint8 Qualita, const FString& Provenienza)
	{
		if (!Manca(Zaino, Ricetta).IsEmpty())
		{
			return false;
		}
		// (14/11, v0.2.3) la riparazione: l'arma in mano torna nuova
		if (Ricetta.Ripara != 0)
		{
			bool bSinistra = false;
			if (!ManoDaRiparare(Zaino, Ricetta, bSinistra) || !ValdorsoZaino::Ripara(Zaino, bSinistra))
			{
				return false;
			}
			Zaino.Monete = FMath::Max(0, Zaino.Monete - Ricetta.Costo);
			return true;
		}
		for (const FValdorsoIngrediente& Ingrediente : Ricetta.Serve)
		{
			if (Ingrediente.Id && Ingrediente.Quanti > 0)
			{
				ValdorsoZaino::TogliOggetto(Zaino, Ingrediente.Id, Ingrediente.Quanti);
			}
		}
		Zaino.Monete = FMath::Max(0, Zaino.Monete - Ricetta.Costo) + FMath::Max(0, Ricetta.Guadagno);
		if (Ricetta.Risultato)
		{
			ValdorsoZaino::AggiungiCon(Zaino, Ricetta.Risultato, Ricetta.QuantiRisultato, Qualita, Provenienza);
		}
		return true;
	}

	FText Titolo(EValdorsoTipoBanco Banco)
	{
		switch (Banco)
		{
		case EValdorsoTipoBanco::Fuoco: return LOCTEXT("TitoloFuoco", "Al fuoco");
		case EValdorsoTipoBanco::Conceria: return LOCTEXT("TitoloConceria", "La conceria");
		case EValdorsoTipoBanco::Fucina: return LOCTEXT("TitoloFucina", "La fucina di Marta");
		case EValdorsoTipoBanco::Falegname: return LOCTEXT("TitoloFalegname", "Il falegname");
		case EValdorsoTipoBanco::Carovana: return LOCTEXT("TitoloCarovana", "La carovana di Sibilla");
		default: return LOCTEXT("TitoloLocanda", "La locanda di Bruno");
		}
	}

	FText Invito(EValdorsoTipoBanco Banco)
	{
		switch (Banco)
		{
		case EValdorsoTipoBanco::Fuoco: return LOCTEXT("InvitoFuoco", "Cucina al fuoco");
		case EValdorsoTipoBanco::Conceria: return LOCTEXT("InvitoConceria", "Lavora alla conceria");
		case EValdorsoTipoBanco::Fucina: return LOCTEXT("InvitoFucina", "Compra da Marta");
		case EValdorsoTipoBanco::Falegname: return LOCTEXT("InvitoFalegname", "Compra dal falegname");
		case EValdorsoTipoBanco::Carovana: return LOCTEXT("InvitoCarovana", "Guarda le merci di Sibilla");
		default: return LOCTEXT("InvitoLocanda", "Ordina da Bruno");
		}
	}

	FText TestoServe(const FValdorsoRicetta& Ricetta)
	{
		TArray<FString> Parti;
		for (const FValdorsoIngrediente& Ingrediente : Ricetta.Serve)
		{
			if (Ingrediente.Id && Ingrediente.Quanti > 0)
			{
				Parti.Add(Ingrediente.Quanti > 1 ? FString::Printf(TEXT("%d × %s"), Ingrediente.Quanti, *BottegaNome(Ingrediente.Id).ToString())
					: BottegaNome(Ingrediente.Id).ToString());
			}
		}
		if (Ricetta.Costo > 0)
		{
			Parti.Add(FString::Printf(TEXT("%d Aureus"), Ricetta.Costo));
		}
		return FText::FromString(FString::Join(Parti, TEXT(", ")));
	}

	FText TestoDa(const FValdorsoRicetta& Ricetta)
	{
		// (14/11, v0.2.3) la riparazione e le cose di un grado
		if (Ricetta.Ripara != 0)
		{
			return Ricetta.Ripara == 2 ? LOCTEXT("RiparaLegno", "Riparare l'arma (o lo scudo) di legno che hai in mano")
				: LOCTEXT("RiparaFerro", "Riparare l'arma (o lo scudo) di ferro che hai in mano");
		}
		if (Ricetta.Risultato && Ricetta.Qualita > 0 && Ricetta.Guadagno <= 0)
		{
			return FText::FromString(FString::Printf(TEXT("%s (%s)"), *BottegaNome(Ricetta.Risultato).ToString(),
				*FString(ValdorsoQualita::Nome(Ricetta.Qualita)).ToLower()));
		}
		// (11/11, v0.2) Aureus e qualcosa indietro (l'acqua venduta: torna il secchio vuoto)
		if (Ricetta.Risultato && Ricetta.Guadagno > 0)
		{
			return FText::FromString(FString::Printf(TEXT("%d Aureus e %s"), Ricetta.Guadagno, *BottegaNome(Ricetta.Risultato).ToString().ToLower()));
		}
		if (Ricetta.Risultato)
		{
			return Ricetta.QuantiRisultato > 1
				? FText::FromString(FString::Printf(TEXT("%d × %s"), Ricetta.QuantiRisultato, *BottegaNome(Ricetta.Risultato).ToString()))
				: BottegaNome(Ricetta.Risultato);
		}
		return FText::FromString(FString::Printf(TEXT("%d Aureus"), Ricetta.Guadagno));
	}

	bool BancoVicino(const APawn* Chi, EValdorsoTipoBanco Banco)
	{
		UWorld* Mondo = Chi ? Chi->GetWorld() : nullptr;
		if (!Mondo)
		{
			return false;
		}
		if (Banco == EValdorsoTipoBanco::Fuoco && AValdorsoFalo::CaldoVicino(Mondo, Chi->GetActorLocation()))
		{
			return true;
		}
		// (12/11) la carovana: Sibilla, se c'e'
		if (Banco == EValdorsoTipoBanco::Carovana)
		{
			for (TActorIterator<AValdorsoPNG> It(Mondo); It; ++It)
			{
				if (It->Dialogo == FName(TEXT("Sibilla")) && !It->Via() && FVector::Dist(It->GetActorLocation(), Chi->GetActorLocation()) <= Raggio * 3.f)
				{
					return true;
				}
			}
			return false;
		}
		for (TActorIterator<AValdorsoBanco> It(Mondo); It; ++It)
		{
			if (It->Banco == Banco && FVector::Dist(It->GetActorLocation(), Chi->GetActorLocation()) <= Raggio)
			{
				return true;
			}
		}
		return false;
	}
}

AValdorsoBanco::AValdorsoBanco()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = false;
	Presa = CreateDefaultSubobject<USphereComponent>(TEXT("Presa"));
	Presa->InitSphereRadius(60.f);
	Presa->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Presa->SetCollisionResponseToAllChannels(ECR_Overlap);
	Presa->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	Presa->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
	Presa->SetCanEverAffectNavigation(false);
	Presa->SetGenerateOverlapEvents(false);
	RootComponent = Presa;
}

FText AValdorsoBanco::TestoInterazione(const APawn* Chi) const
{
	return ValdorsoBotteghe::Invito(Banco);
}

void AValdorsoBanco::Interagisci(APawn* Chi)
{
	const APlayerController* Controllore = Chi ? Cast<APlayerController>(Chi->GetController()) : nullptr;
	if (UValdorsoZainoComponent* Zaino = UValdorsoZainoComponent::Di(Controllore))
	{
		Zaino->ClientApriBottega(static_cast<uint8>(Banco));
	}
}

#undef LOCTEXT_NAMESPACE
