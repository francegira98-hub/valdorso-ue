// Valdorso - La chat di prossimità: quello che si vede (v0.1.4). Scritto da Claude il 08/11/2026.
//
// SValdorsoPannelloChat  in basso a sinistra, le ultime 8 frasi ("Nome: testo"; chi grida d'oro chiaro, gli avvisi di
//                        questo PC in corsivo tenue) su un velo scuro; ognuna sfuma via dopo 20 secondi. Si nasconde con
//                        le schermate aperte, nella modalità foto e nel taccuino. Mentre si scrive si rivedono tutte.
// SValdorsoRigaChat      la riga per scrivere, sotto le frasi: "Dici" e il campo nello stile "Oro e brace". Invio manda,
//                        Esc annulla (li prende lei prima del campo); al massimo 200 caratteri. Se perde la tastiera
//                        (per esempio si apre una schermata dal server) si chiude da sola.
// Le mette e le toglie UValdorsoChatComponent (ValdorsoChat.h).

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Styling/SlateBrush.h"

class APlayerController;
class UValdorsoChatComponent;
class FValdorsoCaratteri;
class FValdorsoStileAccesso;
class SEditableTextBox;
class SVerticalBox;

DECLARE_DELEGATE_OneParam(FValdorsoSuFraseChat, const FString& /*Testo*/);

class VALDORSO_API SValdorsoPannelloChat : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SValdorsoPannelloChat) {}
		SLATE_ARGUMENT(TWeakObjectPtr<APlayerController>, Controllore)
		SLATE_ARGUMENT(TWeakObjectPtr<UValdorsoChatComponent>, Chat)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	/** Quanto è largo il pannello (pixel). */
	static constexpr float Larghezza = 460.f;

private:
	EActiveTimerReturnType Controlla(double Tempo, float Delta);
	/** Rifà le righe (quando arriva una frase o si apre e chiude la riga). */
	void Rifai();
	/** Quanto si vede la frase numero Indice adesso (con la sfumatura dello schermo). */
	float PresenzaFrase(int32 Indice) const;

	TWeakObjectPtr<APlayerController> Controllore;
	TWeakObjectPtr<UValdorsoChatComponent> Chat;
	TSharedPtr<FValdorsoCaratteri> Caratteri;
	TSharedPtr<SVerticalBox> Lista;
	FSlateBrush Velo;
	int32 CambiamentiVisti = -1;
	/** 0 con le schermate aperte, 1 in gioco. */
	float PresenzaSchermo = 0.f;
	/** La frase più visibile (per il velo dietro). */
	float PresenzaMassima = 0.f;
};

class VALDORSO_API SValdorsoRigaChat : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SValdorsoRigaChat) {}
		SLATE_EVENT(FValdorsoSuFraseChat, OnInvia)
		SLATE_EVENT(FSimpleDelegate, OnAnnulla)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	/** Il campo dove si scrive (per dargli la tastiera). */
	TSharedPtr<SWidget> Campo() const;

	virtual FReply OnPreviewKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;

private:
	EActiveTimerReturnType Controlla(double Tempo, float Delta);
	void Annulla();

	TSharedPtr<FValdorsoStileAccesso> Stile;
	TSharedPtr<SEditableTextBox> Casella;
	FValdorsoSuFraseChat SuInvio;
	FSimpleDelegate SuAnnulla;
	/** Quando si è aperta (l'orologio dell'interfaccia; -1 finché non lo sa). */
	double ApertaDa = -1.0;
	/** Già mandata o annullata: niente doppioni. */
	bool bFinita = false;
};
