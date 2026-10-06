// Valdorso - L'aspetto del colono (v0.1.2, passo 4.4, parte dei dati). Scritto da Claude il 06/10/2026.

#include "ValdorsoAspetto.h"

namespace ValdorsoAspetto
{
	namespace
	{
		using G = EValdorsoGruppoAspetto;

		// Il catalogo, versione 1. Le quantità delle scelte sono provvisorie: crescono con i contenuti (teste base fatte
		// nel MetaHuman Creator, acconciature, barbe). Aggiungere voci in fondo e alzare VersioneCatalogo.
		const FValdorsoVoceAspetto AspettoVoci[] = {
			// Il volto: la testa base e i cursori sopra.
			{ TEXT("Testa"), TEXT("Volto"), G::Volto, false, 16, 16, 0 },
			{ TEXT("NasoLunghezza"), TEXT("Naso, lunghezza"), G::Volto, true, 0, 0, Mezzo },
			{ TEXT("NasoLarghezza"), TEXT("Naso, larghezza"), G::Volto, true, 0, 0, Mezzo },
			{ TEXT("Mascella"), TEXT("Mascella"), G::Volto, true, 0, 0, Mezzo },
			{ TEXT("Mento"), TEXT("Mento"), G::Volto, true, 0, 0, Mezzo },
			{ TEXT("Zigomi"), TEXT("Zigomi"), G::Volto, true, 0, 0, Mezzo },
			{ TEXT("OcchiDistanza"), TEXT("Occhi, distanza"), G::Volto, true, 0, 0, Mezzo },
			{ TEXT("OcchiGrandezza"), TEXT("Occhi, grandezza"), G::Volto, true, 0, 0, Mezzo },
			{ TEXT("Labbra"), TEXT("Labbra"), G::Volto, true, 0, 0, Mezzo },
			{ TEXT("Orecchie"), TEXT("Orecchie"), G::Volto, true, 0, 0, Mezzo },
			{ TEXT("Sopracciglia"), TEXT("Sopracciglia"), G::Volto, false, 6, 6, 0 },
			// La pelle e gli occhi.
			{ TEXT("Carnagione"), TEXT("Carnagione"), G::Pelle, true, 0, 0, Mezzo },
			{ TEXT("Lentiggini"), TEXT("Lentiggini"), G::Pelle, true, 0, 0, 0 },
			{ TEXT("ColoreOcchi"), TEXT("Colore degli occhi"), G::Pelle, false, 16, 16, 0 },
			// Capelli e barba (la barba solo per gli uomini: per le donne c'è solo "nessuna").
			{ TEXT("Acconciatura"), TEXT("Capelli"), G::Capelli, false, 12, 12, 0 },
			{ TEXT("ColoreCapelli"), TEXT("Colore dei capelli"), G::Capelli, false, 12, 12, 0 },
			{ TEXT("Barba"), TEXT("Barba e baffi"), G::Capelli, false, 8, 1, 0 },
			// Il corpo.
			{ TEXT("Corporatura"), TEXT("Corporatura"), G::Corpo, true, 0, 0, Mezzo },
			{ TEXT("Muscoli"), TEXT("Muscoli"), G::Corpo, true, 0, 0, Mezzo },
			{ TEXT("Altezza"), TEXT("Altezza"), G::Corpo, true, 0, 0, Mezzo },
			// (06/10, chiesti da Fra) I segni: cicatrici, tatuaggi e segni particolari. La prima scelta è sempre "nessuno".
			// Cicatrici: sopracciglio, guancia, labbro, naso, mento, collo, sull'occhio, sul braccio.
			{ TEXT("Cicatrice"), TEXT("Cicatrice"), G::Segni, false, 9, 9, 0 },
			{ TEXT("CicatriceVisibile"), TEXT("Cicatrice, quanto si vede"), G::Segni, true, 0, 0, Mezzo },
			// Tatuaggi della valle: rune, spirali, nodo, animali, il segno dell'Orso, linee dei clan, fiori, stelle.
			{ TEXT("Tatuaggio"), TEXT("Tatuaggio"), G::Segni, false, 9, 9, 0 },
			// Dove: braccio sinistro, braccio destro, petto, schiena, collo, mano, volto.
			{ TEXT("TatuaggioPosto"), TEXT("Tatuaggio, dove"), G::Segni, false, 7, 7, 0 },
			// Colore: inchiostro nero, blu di guado, rosso ocra, verde, bianco di cenere.
			{ TEXT("TatuaggioColore"), TEXT("Tatuaggio, colore"), G::Segni, false, 5, 5, 0 },
			{ TEXT("TatuaggioGrandezza"), TEXT("Tatuaggio, grandezza"), G::Segni, true, 0, 0, Mezzo },
			// Segni particolari: neo, voglia, occhi di due colori, ciocca bianca, dente mancante, orecchio spezzato,
			// bruciatura, naso rotto.
			{ TEXT("SegnoParticolare"), TEXT("Segno particolare"), G::Segni, false, 9, 9, 0 },
		};

		constexpr int32 AspettoNumero = static_cast<int32>(UE_ARRAY_COUNT(AspettoVoci));

		bool AspettoUomo(const FString& Sesso)
		{
			return Sesso == TEXT("uomo");
		}

		uint8 AspettoValoreSicuro(const FValdorsoAspetto& Aspetto, int32 i)
		{
			if (NonScelto(Aspetto) || !Aspetto.Valori.IsValidIndex(i))
			{
				return AspettoVoci[i].Predefinito;
			}
			return Aspetto.Valori[i];
		}
	}

	TArrayView<const FValdorsoVoceAspetto> Voci()
	{
		return MakeArrayView(AspettoVoci);
	}

	int32 Indice(const TCHAR* Id)
	{
		for (int32 i = 0; i < AspettoNumero; ++i)
		{
			if (FCString::Strcmp(AspettoVoci[i].Id, Id) == 0)
			{
				return i;
			}
		}
		return INDEX_NONE;
	}

	int32 Possibili(const FValdorsoVoceAspetto& Voce, const FString& Sesso)
	{
		if (Voce.bCursore)
		{
			return 256;
		}
		if (Sesso.IsEmpty())
		{
			return FMath::Max<int32>(Voce.SceltePerUomo, Voce.SceltePerDonna);
		}
		return AspettoUomo(Sesso) ? Voce.SceltePerUomo : Voce.SceltePerDonna;
	}

	FValdorsoAspetto Predefinito()
	{
		FValdorsoAspetto Aspetto;
		Aspetto.Versione = VersioneCatalogo;
		for (const FValdorsoVoceAspetto& Voce : AspettoVoci)
		{
			Aspetto.Valori.Add(Voce.Predefinito);
		}
		return Aspetto;
	}

	bool NonScelto(const FValdorsoAspetto& Aspetto)
	{
		return Aspetto.Versione == 0 && Aspetto.Valori.Num() == 0;
	}

	FString Problema(const FValdorsoAspetto& Aspetto, const FString& Sesso)
	{
		if (NonScelto(Aspetto))
		{
			return FString();
		}
		// Il server non si fida del gioco: versione, numero di valori e ogni scelta dentro il catalogo.
		if (Aspetto.Versione != VersioneCatalogo || Aspetto.Valori.Num() != AspettoNumero)
		{
			return TEXT("L'aspetto non è valido: aggiorna il gioco.");
		}
		for (int32 i = 0; i < AspettoNumero; ++i)
		{
			if (Aspetto.Valori[i] >= Possibili(AspettoVoci[i], Sesso))
			{
				return FString::Printf(TEXT("Una scelta dell'aspetto non è valida (%s)."), AspettoVoci[i].Nome);
			}
		}
		return FString();
	}

	uint8 Valore(const FValdorsoAspetto& Aspetto, const TCHAR* Id)
	{
		const int32 i = Indice(Id);
		return i == INDEX_NONE ? 0 : AspettoValoreSicuro(Aspetto, i);
	}

	void Imposta(FValdorsoAspetto& Aspetto, const TCHAR* Id, uint8 Nuovo)
	{
		const int32 i = Indice(Id);
		if (i == INDEX_NONE)
		{
			return;
		}
		if (NonScelto(Aspetto))
		{
			Aspetto = Predefinito();
		}
		if (Aspetto.Valori.IsValidIndex(i))
		{
			Aspetto.Valori[i] = Nuovo;
		}
	}

	float Cursore(const FValdorsoAspetto& Aspetto, const TCHAR* Id)
	{
		const float V = static_cast<float>(Valore(Aspetto, Id));
		return FMath::Clamp((V - static_cast<float>(Mezzo)) / 127.f, -1.f, 1.f);
	}

	void Casuale(FValdorsoAspetto& Aspetto, const FString& Sesso, FRandomStream& Dadi)
	{
		Aspetto = Predefinito();
		for (int32 i = 0; i < AspettoNumero; ++i)
		{
			const FValdorsoVoceAspetto& Voce = AspettoVoci[i];
			if (Voce.bCursore)
			{
				// La media di tre tiri: quasi sempre vicino al mezzo, ogni tanto un tratto marcato.
				const int32 Somma = Dadi.RandRange(0, 255) + Dadi.RandRange(0, 255) + Dadi.RandRange(0, 255);
				Aspetto.Valori[i] = static_cast<uint8>(Somma / 3);
			}
			else
			{
				Aspetto.Valori[i] = static_cast<uint8>(Dadi.RandRange(0, FMath::Max(1, Possibili(Voce, Sesso)) - 1));
			}
		}
		// Le lentiggini sono rare.
		const int32 Lentiggini = Indice(TEXT("Lentiggini"));
		Aspetto.Valori[Lentiggini] = Dadi.FRand() < 0.25f ? static_cast<uint8>(Dadi.RandRange(40, 220)) : static_cast<uint8>(0);
		// Anche i segni sono rari: la maggior parte dei coloni arriva senza.
		auto ForseNessuno = [&Aspetto, &Dadi](const TCHAR* Id, float Probabilita)
		{
			if (Dadi.FRand() >= Probabilita)
			{
				Aspetto.Valori[Indice(Id)] = 0;
			}
		};
		ForseNessuno(TEXT("Cicatrice"), 0.2f);
		ForseNessuno(TEXT("Tatuaggio"), 0.15f);
		ForseNessuno(TEXT("SegnoParticolare"), 0.3f);
	}

	void Adatta(FValdorsoAspetto& Aspetto, const FString& Sesso)
	{
		if (NonScelto(Aspetto))
		{
			return;
		}
		FValdorsoAspetto Nuovo = Predefinito();
		// Stesso catalogo (o uno più vecchio con meno voci in fondo): si tengono i valori che ci sono.
		const int32 Da = FMath::Min(Aspetto.Valori.Num(), Nuovo.Valori.Num());
		for (int32 i = 0; i < Da; ++i)
		{
			Nuovo.Valori[i] = Aspetto.Valori[i];
		}
		for (int32 i = 0; i < AspettoNumero; ++i)
		{
			if (Nuovo.Valori[i] >= Possibili(AspettoVoci[i], Sesso))
			{
				Nuovo.Valori[i] = 0;
			}
		}
		Aspetto = MoveTemp(Nuovo);
	}

	bool Uguali(const FValdorsoAspetto& A, const FValdorsoAspetto& B)
	{
		for (int32 i = 0; i < AspettoNumero; ++i)
		{
			if (AspettoValoreSicuro(A, i) != AspettoValoreSicuro(B, i))
			{
				return false;
			}
		}
		// "Non scelto" e "scelto uguale a quello di partenza" restano diversi: la scelta va salvata.
		return NonScelto(A) == NonScelto(B);
	}
}
