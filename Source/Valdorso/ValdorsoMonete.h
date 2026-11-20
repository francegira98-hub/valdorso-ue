// Valdorso - Le tre monete di Aurelia (v0.2.7.1 "Le rifiniture", 20/11/2026; deciso da Fra, nomi dalle Cronache).
// Scritto da Claude.
//
// 1 Aureus (oro) = 20 Lucio (argento); 1 Lucio = 10 ghiande (rame). Le batte la Zecca della Corona, ad Aurelia.
// Nel gioco i conti si tengono in LUCIO: FValdorsoZaino::Monete, i prezzi delle botteghe, le paghe della Gilda, il soldo
// e le multe sono numeri di Lucio (sono gli stessi numeri che fino alla v0.2.7 si chiamavano "Aureus": cosi' nessun
// salvataggio e nessun prezzo cambia). L'Aureus per ora e' solo un modo di scrivere le somme grosse (da 20 Lucio in su);
// le ghiande arriveranno con i prezzi piccoli (un campo a parte nello zaino).
// Ogni somma che il giocatore legge passa da qui: ValdorsoMonete::Testo(25) = "1 Aureus e 5 Lucio".

#pragma once

#include "CoreMinimal.h"

namespace ValdorsoMonete
{
	inline constexpr int32 LucioPerAureus = 20;
	inline constexpr int32 GhiandePerLucio = 10;

	/** Una somma in Lucio (e, se ci sono, ghiande) scritta per esteso: "3 Lucio", "1 Aureus", "2 Aureus e 5 Lucio",
	 *  "1 Aureus, 5 Lucio e 3 ghiande". Zero = "0 Lucio". I negativi con il meno davanti. */
	inline FString Testo(int64 Lucio, int32 Ghiande = 0)
	{
		int64 Tutte = Lucio * GhiandePerLucio + Ghiande;
		const bool bMeno = Tutte < 0;
		if (bMeno)
		{
			Tutte = -Tutte;
		}
		const int64 Oro = Tutte / (static_cast<int64>(LucioPerAureus) * GhiandePerLucio);
		const int64 Argento = (Tutte / GhiandePerLucio) % LucioPerAureus;
		const int64 Rame = Tutte % GhiandePerLucio;
		TArray<FString> Parti;
		if (Oro > 0)
		{
			Parti.Add(FString::Printf(TEXT("%lld Aureus"), Oro));
		}
		if (Argento > 0 || (Oro == 0 && Rame == 0))
		{
			Parti.Add(FString::Printf(TEXT("%lld Lucio"), Argento));
		}
		if (Rame > 0)
		{
			Parti.Add(Rame == 1 ? FString(TEXT("1 ghianda")) : FString::Printf(TEXT("%lld ghiande"), Rame));
		}
		FString Scritta;
		for (int32 i = 0; i < Parti.Num(); ++i)
		{
			Scritta += (i == 0 ? TEXT("") : (i == Parti.Num() - 1 ? TEXT(" e ") : TEXT(", "))) + Parti[i];
		}
		return bMeno ? TEXT("-") + Scritta : Scritta;
	}

	/** La stessa cosa come FText (per FText::Format). */
	inline FText Scritto(int64 Lucio, int32 Ghiande = 0)
	{
		return FText::FromString(Testo(Lucio, Ghiande));
	}
}
