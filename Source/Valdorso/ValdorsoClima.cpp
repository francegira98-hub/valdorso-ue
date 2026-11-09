// Valdorso - Il caldo e il freddo, e le ore del giorno (vedi il .h). Scritto da Claude il 09/11/2026.

#include "ValdorsoClima.h"

#define LOCTEXT_NAMESPACE "ValdorsoClima"

namespace ValdorsoClima
{
	float MediaStagione(const FString& Stagione)
	{
		if (Stagione == TEXT("inverno"))
		{
			return 1.f;
		}
		if (Stagione == TEXT("primavera"))
		{
			return 11.f;
		}
		if (Stagione == TEXT("estate"))
		{
			return 21.f;
		}
		return 10.f;  // autunno (e se non si sa)
	}

	float Aria(float Ora, const FString& Stagione, float QuotaCm, float Nuvole, float Pioggia, float Nebbia)
	{
		const float Media = MediaStagione(Stagione);
		// il giro del giorno: il più fresco verso le 3, il più caldo verso le 15; le nuvole lo appiattiscono
		const float Escursione = 6.f * (1.f - 0.6f * FMath::Clamp(Nuvole, 0.f, 1.f));
		const float Angolo = (FMath::Fmod(FMath::Max(0.f, Ora), 24.f) - 15.f) / 24.f * 2.f * PI;
		const float Giorno = Escursione * FMath::Cos(Angolo);
		// la quota: un grado e mezzo ogni 100 m sopra la piazza (la valle è piccola: il gioco lo fa sentire di più)
		const float Quota = -1.5f * FMath::Max(0.f, QuotaCm - 5500.f) / 10000.f;
		const float Tempo = -3.f * FMath::Clamp(Pioggia, 0.f, 1.f) - 1.f * FMath::Clamp(Nebbia, 0.f, 1.f);
		return Media + Giorno + Quota + Tempo;
	}

	float Acqua(const FString& Stagione)
	{
		if (Stagione == TEXT("inverno"))
		{
			return 4.f;
		}
		if (Stagione == TEXT("primavera"))
		{
			return 10.f;
		}
		if (Stagione == TEXT("estate"))
		{
			return 19.f;
		}
		return 12.f;
	}

	float Percepita(float GradiAria, float Bagnato, bool bInAcqua, float GradiAcqua, bool bMantello, bool bTetto, bool bFuoco)
	{
		if (bInAcqua)
		{
			// nell'acqua si sente l'acqua, e un poco più fredda (si perde calore)
			return GradiAcqua - 2.f;
		}
		float Gradi = GradiAria;
		// bagnati si sente più freddo
		Gradi -= 6.f * FMath::Clamp(Bagnato, 0.f, 1.f);
		// il mantello e il tetto riparano dal freddo (non dal caldo)
		if (bMantello && Gradi < 18.f)
		{
			Gradi += 4.f;
		}
		if (bTetto && Gradi < 15.f)
		{
			Gradi += 3.f;
		}
		// accanto al falò si sta caldi (ma non si arrostisce)
		if (bFuoco)
		{
			Gradi = FMath::Max(Gradi + 12.f, 18.f);
			Gradi = FMath::Min(Gradi, 26.f);
		}
		return Gradi;
	}

	EValdorsoSensazione Sensazione(float Gradi)
	{
		if (Gradi < SogliaGelo)
		{
			return EValdorsoSensazione::Gelo;
		}
		if (Gradi < SogliaFreddo)
		{
			return EValdorsoSensazione::Freddo;
		}
		if (Gradi < SogliaFresco)
		{
			return EValdorsoSensazione::Fresco;
		}
		if (Gradi < SogliaMite)
		{
			return EValdorsoSensazione::Mite;
		}
		if (Gradi < SogliaCaldo)
		{
			return EValdorsoSensazione::Caldo;
		}
		return EValdorsoSensazione::Afa;
	}

	FText Nome(EValdorsoSensazione Quale)
	{
		switch (Quale)
		{
		case EValdorsoSensazione::Gelo: return LOCTEXT("Gelo", "Gelo");
		case EValdorsoSensazione::Freddo: return LOCTEXT("Freddo", "Freddo");
		case EValdorsoSensazione::Fresco: return LOCTEXT("Fresco", "Fresco");
		case EValdorsoSensazione::Mite: return LOCTEXT("Mite", "Mite");
		case EValdorsoSensazione::Caldo: return LOCTEXT("Caldo", "Caldo");
		default: return LOCTEXT("Afa", "Afa");
		}
	}

	float FrenoTemperatura(float Gradi)
	{
		switch (Sensazione(Gradi))
		{
		case EValdorsoSensazione::Gelo: return 0.5f;
		case EValdorsoSensazione::Freddo: return 0.25f;
		case EValdorsoSensazione::Afa: return 0.25f;
		default: return 0.f;
		}
	}

	float FrenoTotale(float FrenoBagnato, float FrenoCaldoFreddo)
	{
		return FMath::Clamp(FMath::Max(0.f, FrenoBagnato) + FMath::Max(0.f, FrenoCaldoFreddo), 0.f, 0.75f);
	}

	FText OraCanonica(float Ora)
	{
		const int32 Tre = FMath::Clamp(FMath::FloorToInt(FMath::Fmod(FMath::Max(0.f, Ora), 24.f) / 3.f), 0, 7);
		switch (Tre)
		{
		case 0: return LOCTEXT("Mattutino", "Mattutino");
		case 1: return LOCTEXT("Lodi", "Lodi");
		case 2: return LOCTEXT("Prima", "Prima");
		case 3: return LOCTEXT("Terza", "Terza");
		case 4: return LOCTEXT("Sesta", "Sesta");
		case 5: return LOCTEXT("Nona", "Nona");
		case 6: return LOCTEXT("Vespro", "Vespro");
		default: return LOCTEXT("Compieta", "Compieta");
		}
	}

	FString OraEMinuti(float Ora)
	{
		const int32 MinutiTotali = FMath::Clamp(FMath::FloorToInt(FMath::Fmod(FMath::Max(0.f, Ora), 24.f) * 60.f), 0, 24 * 60 - 1);
		return FString::Printf(TEXT("%02d:%02d"), MinutiTotali / 60, MinutiTotali % 60);
	}
}

#undef LOCTEXT_NAMESPACE
