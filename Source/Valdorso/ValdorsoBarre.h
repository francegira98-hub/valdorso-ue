// Valdorso - Le barre e la barra delle azioni (v0.1.4 "Pericoli", prima parte). Scritto da Claude il 08/11/2026.
//
// Come funziona:
// - UValdorsoBarreComponent sta sul PlayerController (uno per giocatore) e lavora solo sul PC di chi gioca: mette sullo
//   schermo, in basso al centro, le quattro barre sottili del colono (salute color brace, stamina d'oro, mana blu
//   arcano, equilibrio color pergamena; SValdorsoBarre) e sopra la barra delle azioni con gli 8 posti (tasti 1-8;
//   SValdorsoBarraAzioni); a tutto schermo il velo scuro della caduta (SValdorsoVeloCaduta). Vedi SValdorsoBarre.h.
// - Ogni barra si vede quando cambia o non è piena; 3 secondi dopo essere tornata piena sfuma via. Tutto si nasconde
//   quando c'è una schermata aperta (si vede il mouse), nella modalità foto, nel taccuino e senza il colono (come la
//   bussola). Valdorso.Barre.Numeri 1 scrive i numeri accanto alle barre.
// - La barra delle azioni: (08/11, seconda parte) negli 8 posti si mettono gli oggetti dello zaino (ValdorsoZaino.h:
//   pane, borraccia, erbe...; si sceglie nello zaino, tasto I) e si salvano con lo zaino nel file del personaggio.
//   Premendo un tasto da 1 a 8 il posto si illumina e il suo oggetto si usa; un posto vuoto dice "Nessuna azione".
//   Le abilità arriveranno più avanti negli stessi posti.
// - I tasti 1-8 li collega AValdorsoPlayerController (solo in gioco, senza schermate aperte: dentro il dialogo, il
//   taccuino e la modalità foto i numeri li prendono quelle schermate, che hanno la tastiera tutta per sé).

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ValdorsoBarre.generated.h"

class APlayerController;
class SWidget;
class SValdorsoBarraAzioni;
class UValdorsoZainoComponent;

/** Lo stato di una barra sullo schermo (quanto è piena, da quando è piena, quanto si vede). */
struct FValdorsoStatoBarra
{
	/** Quanto è piena (0..1); -1 finché non è mai stata letta. */
	float Frazione = -1.f;
	/** La scia del danno: resta un poco indietro quando la barra scende, poi la raggiunge. */
	float Scia = 0.f;
	/** L'ultima volta (secondi) in cui è cambiata o non era piena. */
	double UltimoCambio = 0.0;
	/** Quanto si vede (0 nascosta, 1 tutta). */
	float Presenza = 0.f;
};

namespace ValdorsoBarre
{
	/** Quanti secondi resta una barra dopo essere tornata piena. */
	inline constexpr double SecondiDopoPiena = 3.0;
	/** In quanti secondi una barra compare o sparisce. */
	inline constexpr float SecondiSfumatura = 0.6f;
	/** Quanti posti ha la barra delle azioni (tasti da 1 a 8). */
	inline constexpr int32 NumeroAzioni = 8;
	/** Quanto scende la scia del danno, in barre al secondo. */
	inline constexpr float VelocitaScia = 0.5f;

	/** Quanto è piena una barra (0..1); senza massimo vale piena. */
	VALDORSO_API float Frazione(float Valore, float Massimo);

	/** Vero se la barra è piena (o quasi). */
	VALDORSO_API bool Piena(float InFrazione);

	/** Vero se la barra deve vedersi adesso: non è piena, o è cambiata da meno di SecondiDopoPiena. */
	VALDORSO_API bool DaMostrare(const FValdorsoStatoBarra& Stato, double Adesso);

	/** Un passo: legge il valore nuovo, aggiorna la scia e fa sfumare la presenza verso quella voluta. */
	VALDORSO_API void Aggiorna(FValdorsoStatoBarra& Stato, float Valore, float Massimo, double Adesso, float Delta);

	/** "73 / 100" (senza decimali). */
	VALDORSO_API FString TestoNumeri(float Valore, float Massimo);

	/**
	 * Vero se le scritte del gioco (barre, azioni, chat) possono vedersi su questo PC: c'è il colono e non c'è niente
	 * di aperto sopra (né mouse, né modalità foto, né taccuino). Come la bussola.
	 */
	VALDORSO_API bool SchermoLibero(const APlayerController* Controllore);
}

UCLASS(ClassGroup = (Valdorso))
class VALDORSO_API UValdorsoBarreComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UValdorsoBarreComponent();

	/** Il componente delle barre di quel PlayerController (nullptr se non ce l'ha). */
	static UValdorsoBarreComponent* Di(const APlayerController* Controllore);

	/** Le azioni che non vengono dallo zaino (le abilità, più avanti): NAME_None = vuoto. */
	TArray<FName> Azioni;

	/** L'azione di un posto (Indice da 0 a 7): l'oggetto messo lì nello zaino, se no Azioni; NAME_None = vuoto. */
	FName Azione(int32 Indice) const;

	/** Lo zaino dello stesso giocatore (nullptr se non c'è). */
	const UValdorsoZainoComponent* ZainoDelGiocatore() const;

	/** Il tasto di un posto (Indice da 0 a 7): il posto si illumina e il suo oggetto si usa; se è vuoto, "Nessuna azione". */
	void PremiAzione(int32 Indice);

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void MettiSchermo();
	void TogliSchermo();

	TSharedPtr<SWidget> Hud;
	TSharedPtr<SWidget> Velo;
	TSharedPtr<SValdorsoBarraAzioni> BarraAzioni;
};
