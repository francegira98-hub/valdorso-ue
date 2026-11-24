// Valdorso - Le tre monete di Aurelia (v0.2.7.1 "Le rifiniture", 20/11/2026; deciso da Fra, nomi dalle Cronache).
// Scritto da Claude.
//
// 1 Aureus (oro) = 20 Lucio (argento); 1 Lucio = 10 ghiande (rame). Le batte la Zecca della Corona, ad Aurelia.
// Nel gioco i conti si tengono in LUCIO: FValdorsoZaino::Monete, i prezzi delle botteghe, le paghe della Gilda, il soldo
// e le multe sono numeri di Lucio (sono gli stessi numeri che fino alla v0.2.7 si chiamavano "Aureus": cosi' nessun
// salvataggio e nessun prezzo cambia). L'Aureus e' solo un modo di scrivere le somme grosse (da 20 Lucio in su).
// Le ghiande (20/11 sera) stanno in un campo a parte dello zaino, FValdorsoZaino::Ghiande, sempre da 0 a 9: dieci
// ghiande diventano un Lucio da sole (ValdorsoZaino::Sistema). Si ricevono di resto quando si paga un prezzo piccolo
// (la mela di Bruno, il boccale di sidro) e si pagano con Bastano / Togli / Aggiungi qui sotto.
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

	/** Per i posti stretti (la riga delle monete nello zaino): come Testo, ma quando le monete sono di tutti e tre i
	 *  tipi si accorcia: "2 Au · 7 Lu · 5 gh". */
	inline FString Breve(int64 Lucio, int32 Ghiande = 0)
	{
		const int64 Tutte = FMath::Max<int64>(0, Lucio * GhiandePerLucio + Ghiande);
		const int64 Oro = Tutte / (static_cast<int64>(LucioPerAureus) * GhiandePerLucio);
		const int64 Argento = (Tutte / GhiandePerLucio) % LucioPerAureus;
		const int64 Rame = Tutte % GhiandePerLucio;
		if (Oro > 0 && Argento > 0 && Rame > 0)
		{
			return FString::Printf(TEXT("%lld Au · %lld Lu · %lld gh"), Oro, Argento, Rame);
		}
		return Testo(Lucio, Ghiande);
	}

	/** Vero se con quei Lucio e quelle ghiande si paga quel prezzo. */
	inline bool Bastano(int32 Lucio, int32 Ghiande, int32 CostoLucio, int32 CostoGhiande)
	{
		return static_cast<int64>(Lucio) * GhiandePerLucio + Ghiande >= static_cast<int64>(CostoLucio) * GhiandePerLucio + CostoGhiande;
	}

	/** Paga quel prezzo (il resto torna in ghiande). Se non bastano, resta a zero: prima si chiede Bastano. */
	inline void Togli(int32& Lucio, int32& Ghiande, int32 CostoLucio, int32 CostoGhiande)
	{
		const int64 Resta = FMath::Max<int64>(0, static_cast<int64>(Lucio) * GhiandePerLucio + Ghiande
			- (static_cast<int64>(FMath::Max(0, CostoLucio)) * GhiandePerLucio + FMath::Max(0, CostoGhiande)));
		Lucio = static_cast<int32>(FMath::Min<int64>(Resta / GhiandePerLucio, MAX_int32));
		Ghiande = static_cast<int32>(Resta % GhiandePerLucio);
	}

	/** Aggiunge una somma (dieci ghiande fanno un Lucio). */
	inline void Aggiungi(int32& Lucio, int32& Ghiande, int32 PiuLucio, int32 PiuGhiande)
	{
		const int64 Tutte = static_cast<int64>(FMath::Max(0, Lucio)) * GhiandePerLucio + FMath::Max(0, Ghiande)
			+ static_cast<int64>(FMath::Max(0, PiuLucio)) * GhiandePerLucio + FMath::Max(0, PiuGhiande);
		Lucio = static_cast<int32>(FMath::Min<int64>(Tutte / GhiandePerLucio, MAX_int32));
		Ghiande = static_cast<int32>(Tutte % GhiandePerLucio);
	}

	/** La stessa cosa come FText (per FText::Format). */
	inline FText Scritto(int64 Lucio, int32 Ghiande = 0)
	{
		return FText::FromString(Testo(Lucio, Ghiande));
	}
}
