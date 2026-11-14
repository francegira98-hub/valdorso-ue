// Valdorso - La qualita' degli oggetti e le armi vere (vedi il .h). Scritto da Claude il 14/11/2026.

#include "ValdorsoQualita.h"
#include "ValdorsoOggetti.h"
#include "ValdorsoZainoDati.h"

namespace
{
	struct FQualitaFamiglia
	{
		const TCHAR* Id;
		EValdorsoFamigliaArma Famiglia;
		/** Di legno: la ripara il falegname. */
		bool bLegno;
	};

	// tutte le armi e gli scudi che si impugnano (ValdorsoOggetti.cpp, "le armi"), con la loro famiglia
	const FQualitaFamiglia QualitaArmi[] = {
		{ TEXT("Coltello"), EValdorsoFamigliaArma::Pugnali, false },
		{ TEXT("Pugnale"), EValdorsoFamigliaArma::Pugnali, false },
		{ TEXT("Falcetto"), EValdorsoFamigliaArma::Pugnali, false },
		{ TEXT("Pugnale_Nani"), EValdorsoFamigliaArma::Pugnali, false },
		{ TEXT("Spada_Corta"), EValdorsoFamigliaArma::Spade, false },
		{ TEXT("Spada_Lunga"), EValdorsoFamigliaArma::Spade, false },
		{ TEXT("Gladio"), EValdorsoFamigliaArma::Spade, false },
		{ TEXT("Accetta"), EValdorsoFamigliaArma::Asce, false },
		{ TEXT("Ascia_Guerra"), EValdorsoFamigliaArma::Asce, false },
		{ TEXT("Martello"), EValdorsoFamigliaArma::Mazze, false },
		{ TEXT("Mazza"), EValdorsoFamigliaArma::Mazze, false },
		{ TEXT("Clava"), EValdorsoFamigliaArma::Mazze, true },
		{ TEXT("Piccone"), EValdorsoFamigliaArma::Mazze, false },
		{ TEXT("Martello_Nani"), EValdorsoFamigliaArma::Mazze, false },
		{ TEXT("Lancia"), EValdorsoFamigliaArma::Aste, false },
		{ TEXT("Forcone"), EValdorsoFamigliaArma::Aste, true },
		{ TEXT("Bastone_Pastore"), EValdorsoFamigliaArma::Aste, true },
		{ TEXT("Bastone_Ferrato"), EValdorsoFamigliaArma::Aste, true },
		{ TEXT("Scudo_Legno"), EValdorsoFamigliaArma::Scudo, true },
		{ TEXT("Scudo_Rotondo"), EValdorsoFamigliaArma::Scudo, true },
		{ TEXT("Brocchiere"), EValdorsoFamigliaArma::Scudo, false },
		{ TEXT("Scudo_Aquilone"), EValdorsoFamigliaArma::Scudo, false },
		{ TEXT("Scudo_Legione"), EValdorsoFamigliaArma::Scudo, false },
		{ TEXT("Arco"), EValdorsoFamigliaArma::Arco, true },
	};

	const FQualitaFamiglia* QualitaTrova(FName Id)
	{
		if (Id.IsNone())
		{
			return nullptr;
		}
		for (const FQualitaFamiglia& A : QualitaArmi)
		{
			if (Id == FName(A.Id))
			{
				return &A;
			}
		}
		return nullptr;
	}

	float QualitaScala(uint8 Qualita, float C, float B, float R, float E, float L)
	{
		switch (FMath::Min<uint8>(Qualita, ValdorsoQualita::Massima))
		{
		case ValdorsoQualita::Buona: return B;
		case ValdorsoQualita::Rara: return R;
		case ValdorsoQualita::Epica: return E;
		case ValdorsoQualita::Leggendaria: return L;
		default: return C;
		}
	}
}

namespace ValdorsoQualita
{
	const TCHAR* Nome(uint8 Qualita)
	{
		switch (FMath::Min<uint8>(Qualita, Massima))
		{
		case Buona: return TEXT("Buona");
		case Rara: return TEXT("Rara");
		case Epica: return TEXT("Epica");
		case Leggendaria: return TEXT("Leggendaria");
		default: return TEXT("Comune");
		}
	}

	FLinearColor Colore(uint8 Qualita)
	{
		// gli stessi delle storie del Battito (mattoni.json: comune #9a8f7a, insolita #5f8a5a, rara #4a6fa5, epica #7d4a9e,
		// leggendaria #c8962e)
		switch (FMath::Min<uint8>(Qualita, Massima))
		{
		case Buona: return FLinearColor(FColor(0x5f, 0x8a, 0x5a));
		case Rara: return FLinearColor(FColor(0x4a, 0x6f, 0xa5));
		case Epica: return FLinearColor(FColor(0x7d, 0x4a, 0x9e));
		case Leggendaria: return FLinearColor(FColor(0xc8, 0x96, 0x2e));
		default: return FLinearColor(FColor(0x9a, 0x8f, 0x7a));
		}
	}

	float Danno(uint8 Qualita) { return QualitaScala(Qualita, 1.f, 1.1f, 1.2f, 1.35f, 1.5f); }
	float Parata(uint8 Qualita) { return QualitaScala(Qualita, 1.f, 1.05f, 1.1f, 1.15f, 1.2f); }
	float Durata(uint8 Qualita) { return QualitaScala(Qualita, 1.f, 1.25f, 1.5f, 2.f, 3.f); }
	float Ristoro(uint8 Qualita) { return QualitaScala(Qualita, 1.f, 1.15f, 1.3f, 1.5f, 1.75f); }
	float Calore(uint8 Qualita) { return QualitaScala(Qualita, 1.f, 1.15f, 1.3f, 1.45f, 1.6f); }

	EValdorsoFamigliaArma Famiglia(FName Id)
	{
		const FQualitaFamiglia* A = QualitaTrova(Id);
		return A ? A->Famiglia : EValdorsoFamigliaArma::Nessuna;
	}

	FName NomeFamiglia(EValdorsoFamigliaArma Famiglia)
	{
		switch (Famiglia)
		{
		case EValdorsoFamigliaArma::Pugnali: return FName(TEXT("Pugnali"));
		case EValdorsoFamigliaArma::Spade: return FName(TEXT("Spade"));
		case EValdorsoFamigliaArma::Asce: return FName(TEXT("Asce"));
		case EValdorsoFamigliaArma::Mazze: return FName(TEXT("Mazze"));
		case EValdorsoFamigliaArma::Aste: return FName(TEXT("Aste"));
		case EValdorsoFamigliaArma::Scudo: return FName(TEXT("Scudo"));
		case EValdorsoFamigliaArma::Arco: return FName(TEXT("Arco"));
		default: return FName(TEXT("Lotta"));
		}
	}

	bool DiLegno(FName Id)
	{
		const FQualitaFamiglia* A = QualitaTrova(Id);
		return A && A->bLegno;
	}

	int32 DurataMassima(FName Id, uint8 Qualita)
	{
		float Base = 0.f;
		switch (Famiglia(Id))
		{
		case EValdorsoFamigliaArma::Pugnali: Base = 120.f; break;
		case EValdorsoFamigliaArma::Spade: Base = 160.f; break;
		case EValdorsoFamigliaArma::Asce: Base = 150.f; break;
		case EValdorsoFamigliaArma::Mazze: Base = 200.f; break;
		case EValdorsoFamigliaArma::Aste: Base = 140.f; break;
		case EValdorsoFamigliaArma::Scudo: Base = 180.f; break;
		case EValdorsoFamigliaArma::Arco: Base = 150.f; break;
		default: return 0;
		}
		return FMath::RoundToInt(Base * Durata(Qualita));
	}

	bool Rovinata(const FValdorsoPostoZaino& Posto)
	{
		const int32 Massimo = Posto.Vuoto() ? 0 : DurataMassima(Posto.Id, Posto.Qualita);
		return Massimo > 0 && Posto.Usura >= Massimo;
	}

	bool Consumata(const FValdorsoPostoZaino& Posto)
	{
		return !Posto.Vuoto() && DurataMassima(Posto.Id, Posto.Qualita) > 0 && Posto.Usura > 0;
	}

	float ForzaDelPosto(const FValdorsoPostoZaino& Posto, bool bParata)
	{
		if (Posto.Vuoto())
		{
			return 1.f;
		}
		const float PerGrado = bParata ? Parata(Posto.Qualita) : Danno(Posto.Qualita);
		return Rovinata(Posto) ? PerGrado * FattoreRovinata : PerGrado;
	}

	uint8 Forgiata(float PuntiBottega, float Caso)
	{
		const float Bravura = FMath::Clamp(PuntiBottega, 0.f, 100.f) / 100.f;
		const float Punteggio = 0.6f * Bravura + 0.4f * FMath::Clamp(Caso, 0.f, 1.f);
		if (PuntiBottega >= 75.f && Punteggio >= 0.88f)
		{
			return Epica;
		}
		if (PuntiBottega >= 50.f && Punteggio >= 0.72f)
		{
			return Rara;
		}
		if (Punteggio >= 0.45f)
		{
			return Buona;
		}
		return Comune;
	}

	FString TestoStato(const FValdorsoPostoZaino& Posto)
	{
		const int32 Massimo = Posto.Vuoto() ? 0 : DurataMassima(Posto.Id, Posto.Qualita);
		if (Massimo <= 0)
		{
			return FString();
		}
		if (Rovinata(Posto))
		{
			return DiLegno(Posto.Id) ? TEXT("Rovinata: colpisce la meta'. Portala al falegname.")
				: TEXT("Rovinata: colpisce la meta'. Portala a Marta, alla fucina.");
		}
		const bool bScudo = Famiglia(Posto.Id) == EValdorsoFamigliaArma::Scudo;
		return FString::Printf(TEXT("Stato: %d %s su %d"), Massimo - FMath::Max(0, Posto.Usura), bScudo ? TEXT("parate") : TEXT("colpi"), Massimo);
	}

	FString TestoQualita(const FValdorsoPostoZaino& Posto)
	{
		if (Posto.Vuoto() || (Posto.Qualita == Comune && Posto.Provenienza.IsEmpty()))
		{
			return FString();
		}
		if (Posto.Provenienza.IsEmpty())
		{
			return Nome(Posto.Qualita);
		}
		return FString::Printf(TEXT("%s · %s"), Nome(Posto.Qualita), *Posto.Provenienza);
	}
}
