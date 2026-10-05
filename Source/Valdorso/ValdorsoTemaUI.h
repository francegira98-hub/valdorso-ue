// Valdorso - Il tema "Oro e brace": colori, caratteri e battito del Cuore, usati da tutte le schermate.
// Cambiando qui, cambia tutto il gioco (decisione del 26/09: un unico tema per tutte le interfacce).

#pragma once

#include "CoreMinimal.h"
#include "Fonts/SlateFontInfo.h"
#include "Fonts/CompositeFont.h"

namespace ValdorsoTema
{
	/** Un colore scritto come nel documento (esadecimale, per esempio "D4AF37"). */
	inline FLinearColor Hex(const TCHAR* Codice) { return FLinearColor(FColor::FromHex(FString(Codice))); }

	inline FLinearColor Fondo()            { return Hex(TEXT("0E0B09")); }
	inline FLinearColor Pannelli()         { return Hex(TEXT("15110D")); }
	inline FLinearColor Pulsanti()         { return Hex(TEXT("1C1612")); }
	inline FLinearColor PulsanteSopra()    { return Hex(TEXT("3A2C1F")); }
	inline FLinearColor PulsantePremuto()  { return Hex(TEXT("5A4430")); }
	inline FLinearColor Oro()              { return Hex(TEXT("D4AF37")); }
	inline FLinearColor OroChiaro()        { return Hex(TEXT("F3DC8C")); }
	inline FLinearColor Pergamena()        { return Hex(TEXT("E8D9B5")); }
	inline FLinearColor TestoSecondario()  { return Hex(TEXT("C9B99A")); }
	inline FLinearColor Brace()            { return Hex(TEXT("D9772B")); }
	inline FLinearColor RossoSangue()      { return Hex(TEXT("8E2B25")); }
	inline FLinearColor BluArcano()        { return Hex(TEXT("3E6FA8")); }
	inline FLinearColor VerdeVita()        { return Hex(TEXT("5E8C3A")); }

	// L'inchiostro sulla pergamena del Registro (passo 4.2b, 05/10): scuro per i testi, tenue per le note,
	// rosso "di rubrica" (come i titoli dei registri antichi) per nomi, scelte ed errori, oro scuro per i rombi.
	inline FLinearColor Inchiostro()       { return Hex(TEXT("3A2414")); }
	inline FLinearColor InchiostroTenue()  { return Hex(TEXT("6E5034")); }
	inline FLinearColor Rubrica()          { return Hex(TEXT("8E2B25")); }
	inline FLinearColor OroScuro()         { return Hex(TEXT("7A5A12")); }

	/**
	 * Il battito del Cuore del Mondo: due colpi e una pausa (come il frammento del tempio).
	 * Restituisce un numero da 0 (quiete) a 1 (colpo). Usa l'orologio del PC, così menu e frammento battono insieme.
	 */
	inline float Battito(double Secondi)
	{
		const double Periodo = 1.6;
		const double P = FMath::Fmod(Secondi, Periodo);
		auto Colpo = [](double X, double Centro)
		{
			const double D = (X - Centro) / 0.07;
			return static_cast<double>(FMath::Exp(static_cast<float>(-D * D)));
		};
		const double Valore = Colpo(P, 0.0) + Colpo(P, Periodo) + 0.75 * Colpo(P, 0.32);
		return static_cast<float>(FMath::Clamp(Valore, 0.0, 1.0));
	}
}

/**
 * I caratteri di Valdorso: Cinzel per titoli, pulsanti e nomi; EB Garamond per testi e dialoghi;
 * Tangerine per la calligrafia del sacerdote nel Registro (05/10, scelta da Fra).
 * Si caricano dalla cartella Content/UI/Font (licenza OFL, copie della licenza in Licenze/).
 * Se un file manca, si usa il carattere di serie di Unreal (e il Registro output lo dice).
 */
class VALDORSO_API FValdorsoCaratteri
{
public:
	FValdorsoCaratteri();

	/** Cinzel. Peso: "Regular", "Bold" o "Black". */
	FSlateFontInfo Titolo(float Dimensione, FName Peso = TEXT("Bold")) const;

	/** EB Garamond. Stile: "Regular", "Italic" o "SemiBold". */
	FSlateFontInfo Testo(float Dimensione, FName Stile = TEXT("Regular")) const;

	/** Tangerine, la mano del sacerdote. Peso: "Regular" o "Bold". Se manca, EB Garamond corsivo un poco più piccolo. */
	FSlateFontInfo Calligrafia(float Dimensione, FName Peso = TEXT("Bold")) const;

	/** Vero se la calligrafia è caricata (per scegliere la grandezza: Tangerine è molto più piccola a pari punti). */
	bool HaCalligrafia() const { return Tangerine.IsValid(); }

private:
	static TSharedPtr<FStandaloneCompositeFont> Crea(const TArray<TPair<FName, FString>>& Facce);

	TSharedPtr<FStandaloneCompositeFont> Cinzel;
	TSharedPtr<FStandaloneCompositeFont> Garamond;
	TSharedPtr<FStandaloneCompositeFont> Tangerine;
};
