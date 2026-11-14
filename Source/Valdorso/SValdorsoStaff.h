// Valdorso - La finestra dello staff (F9): le schede (vedi ValdorsoStaff.h). Scritto da Claude il 10/11/2026.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Styling/SlateBrush.h"

class APlayerController;
class FValdorsoCaratteri;
class SBox;
class UValdorsoStaffComponent;
struct FValdorsoStaffVista;

class VALDORSO_API SValdorsoStaff : public SCompoundWidget
{
public:
	enum class EScheda : uint8
	{
		Coloni,
		Prove,
		Bacheche,
		Mondo,
		Giocatori,
		Registro,
		Permessi,
	};

	SLATE_BEGIN_ARGS(SValdorsoStaff) {}
		SLATE_ARGUMENT(TWeakObjectPtr<APlayerController>, Controllore)
		SLATE_EVENT(FSimpleDelegate, OnChiudi)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	virtual bool SupportsKeyboardFocus() const override { return true; }
	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

private:
	void Rifai();
	bool SchedaVisibile(EScheda Quale) const;
	TSharedRef<SWidget> Linguette();
	TSharedRef<SWidget> Coloni();
	TSharedRef<SWidget> Prove();
	TSharedRef<SWidget> Bacheche();
	TSharedRef<SWidget> Mondo();
	TSharedRef<SWidget> Giocatori();
	TSharedRef<SWidget> Registro();
	TSharedRef<SWidget> Permessi();

	/** Un pulsante che manda un'azione al server (Valore letto al momento del clic). */
	TSharedRef<SWidget> Pulsante(const FText& Testo, const FString& Azione, TFunction<FString()> Chi, TFunction<FString()> Valore, bool bAttivo = true);
	/** Una casella di testo che scrive in Dove (la stringa resta anche quando la scheda si rifa'). */
	TSharedRef<SWidget> Casella(FString& Dove, const FText& Suggerimento, float Larga = 200.f);
	TSharedRef<SWidget> Titoletto(const FText& Testo);
	TSharedRef<SWidget> Riga(const FString& Testo, const FLinearColor& Colore);
	FString Scelto() const;

	TWeakObjectPtr<APlayerController> Controllore;
	TWeakObjectPtr<UValdorsoStaffComponent> Staff;
	FSimpleDelegate OnChiudi;
	TSharedPtr<FValdorsoCaratteri> Caratteri;
	TSharedPtr<SBox> Contenuto;
	TSharedPtr<SBox> BoxLinguette;
	FSlateBrush Fondo;
	FSlateBrush Velo;
	FSlateBrush Riquadro;
	FSlateBrush RiquadroScelto;
	EScheda Scheda = EScheda::Coloni;
	int32 Visto = -1;
	bool bDaRifare = false;

	/** Le caselle (restano tra un aggiornamento e l'altro). */
	FString ColonoScelto;
	FString TestoPunti;
	FString TestoOggetto;
	FString TestoMonete;
	/** (11/11) "Lame 40": l'abilita' e i punti. */
	FString TestoAbilita;
	FString TestoProclama;
	/** (13/11) L'annuncio dello staff (il Banditore della Corona). */
	FString TestoAnnuncio;
	FString TestoIncarico;
	FString TestoCassa;
	FString TestoOra;
	FString TestoGiorno;
	FString TestoFiltro;
	/** I permessi spuntati, per colono (prima di "Salva"). */
	TMap<FString, int32> Bozze;
	/** I permessi che diceva il server quando la bozza e' nata. */
	TMap<FString, int32> BozzeDi;
};
