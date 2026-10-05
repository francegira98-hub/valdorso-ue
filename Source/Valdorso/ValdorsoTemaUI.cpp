// Valdorso - Il tema "Oro e brace" (vedi il .h).

#include "ValdorsoTemaUI.h"
#include "Engine/FontFace.h"
#include "Styling/CoreStyle.h"
#include "Valdorso.h"

namespace
{
	/** Cerca un Font Face importato in Content/UI/Font (con il trattino o con il trattino basso). */
	UFontFace* CaricaFaccia(const FString& Nome)
	{
		const TArray<FString> Nomi = { Nome, Nome.Replace(TEXT("-"), TEXT("_")) };
		for (const FString& N : Nomi)
		{
			const FString Percorso = FString::Printf(TEXT("/Game/UI/Font/%s.%s"), *N, *N);
			if (UFontFace* Faccia = LoadObject<UFontFace>(nullptr, *Percorso, nullptr, LOAD_NoWarn | LOAD_Quiet))
			{
				return Faccia;
			}
		}
		UE_LOG(LogValdorso, Warning, TEXT("[Valdorso] Tema: carattere %s non trovato in Content/UI/Font"), *Nome);
		return nullptr;
	}
}

FValdorsoCaratteri::FValdorsoCaratteri()
{
	Cinzel = Crea({
		{ TEXT("Regular"), TEXT("Cinzel-Regular") },
		{ TEXT("Bold"),    TEXT("Cinzel-Bold") },
		{ TEXT("Black"),   TEXT("Cinzel-Black") } });

	Garamond = Crea({
		{ TEXT("Regular"),  TEXT("EBGaramond-Regular") },
		{ TEXT("Italic"),   TEXT("EBGaramond-Italic") },
		{ TEXT("SemiBold"), TEXT("EBGaramond-SemiBold") } });

	// (05/10) Si importa con Content/Python/importa_registro.py; finché non c'è, il Registro usa EB Garamond corsivo.
	Tangerine = Crea({
		{ TEXT("Regular"), TEXT("Tangerine-Regular") },
		{ TEXT("Bold"),    TEXT("Tangerine-Bold") } });
}

TSharedPtr<FStandaloneCompositeFont> FValdorsoCaratteri::Crea(const TArray<TPair<FName, FString>>& Facce)
{
	TSharedPtr<FStandaloneCompositeFont> Carattere = MakeShared<FStandaloneCompositeFont>();
	for (const TPair<FName, FString>& Faccia : Facce)
	{
		UFontFace* Oggetto = CaricaFaccia(Faccia.Value);
		if (Oggetto == nullptr)
		{
			return nullptr;
		}
		FTypefaceEntry& Voce = Carattere->DefaultTypeface.Fonts.AddDefaulted_GetRef();
		Voce.Name = Faccia.Key;
		Voce.Font = FFontData(Oggetto);
	}
	return Carattere;
}

FSlateFontInfo FValdorsoCaratteri::Titolo(float Dimensione, FName Peso) const
{
	if (Cinzel.IsValid())
	{
		return FSlateFontInfo(Cinzel, Dimensione, Peso);
	}
	return FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), Dimensione);
}

FSlateFontInfo FValdorsoCaratteri::Testo(float Dimensione, FName Stile) const
{
	if (Garamond.IsValid())
	{
		return FSlateFontInfo(Garamond, Dimensione, Stile);
	}
	return FCoreStyle::GetDefaultFontStyle(Stile == TEXT("Italic") ? TEXT("Italic") : TEXT("Regular"), Dimensione);
}

FSlateFontInfo FValdorsoCaratteri::Calligrafia(float Dimensione, FName Peso) const
{
	if (Tangerine.IsValid())
	{
		return FSlateFontInfo(Tangerine, Dimensione, Peso);
	}
	return Testo(Dimensione * 0.62f, TEXT("Italic"));
}
