// Valdorso - Le cose da raccogliere nella valle (v0.1.4, seconda parte). Scritto da Claude il 08/11/2026.
//
// Come funziona:
// - Nei boschi si trovano i rami secchi (per i falò, ValdorsoFalo.h) e i funghi; nella palude di Nonna Edda le erbe.
//   I posti li decide ValdorsoRaccolta::Punti(): sempre gli stessi (un numero fisso fa da seme), sparsi intorno ai
//   luoghi della valle (ValdorsoLuoghiDati.inl), lontani l'uno dall'altro almeno 6 m.
// - Il server mette una cosa al suo posto quando un colono ci arriva vicino (80 m): solo allora il World Partition ha
//   caricato il terreno lì, e una linea verso il basso trova dove poggiarla (UValdorsoRaccolteSubsystem). Chi passa
//   vede "E · Raccogli: Rami secchi"; raccolta, la cosa sparisce per tutti e ricresce dopo qualche minuto.
// - Com'è fatta: rami, ciuffi d'erba e funghi con le forme di serie di Unreal (cilindri, coni, sfere) colorate.
//   Niente da importare; i modelli veri arriveranno con gli alberi.
// - (10/11, v0.1.4) Sul fondo del lago le cozze (almeno 2 m sott'acqua: ci si tuffa con Ctrl). Ogni tanto dentro c'e'
//   una perla (la compra Bruno).
// Comando di console (server): Valdorso.Raccolta.Ricresci (tutto torna subito al suo posto).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Subsystems/WorldSubsystem.h"
#include "ValdorsoInterazione.h"
#include "ValdorsoRaccolta.generated.h"

class UMaterialInterface;
class USphereComponent;
class UStaticMesh;
class UStaticMeshComponent;

/** Un posto dove si raccoglie qualcosa. */
struct FValdorsoPuntoRaccolta
{
	/** L'oggetto (ValdorsoOggetti.h) e quanti se ne prendono. */
	FName Oggetto;
	int32 Quantita = 1;
	/** Dove (cm del mondo; Z è la quota del luogo: quella vera la trova la linea verso il basso). */
	FVector Posizione = FVector::ZeroVector;
	/** In quanti secondi ricresce. */
	float Ricrescita = 300.f;
	/** (10/11, v0.1.4) Sul fondo dell'acqua (le cozze del lago): ci si tuffa per prenderle. */
	bool bFondale = false;
};

namespace ValdorsoRaccolta
{
	/** A quale distanza (cm, in piano) da un colono il server mette le cose al loro posto. */
	inline constexpr float RaggioComparsa = 8000.f;
	/** Quanto lontani tra loro i posti (cm). */
	inline constexpr float DistanzaMinima = 600.f;

	/** Tutti i posti della valle, sempre gli stessi. */
	VALDORSO_API const TArray<FValdorsoPuntoRaccolta>& Punti();
}

UCLASS()
class VALDORSO_API AValdorsoRaccolta : public AActor, public IValdorsoInterattivo
{
	GENERATED_BODY()

public:
	AValdorsoRaccolta();

	/** Solo sul server, prima di FinishSpawning: cosa c'è qui. */
	void Prepara(FName InOggetto, int32 InQuantita, float InRicrescita);

	/** Solo sul server (console): torna subito. */
	void Ricresci();

	// IValdorsoInterattivo
	virtual FText TestoInterazione(const APawn* Chi) const override;
	virtual void Interagisci(APawn* Chi) override;
	virtual FVector PuntoInterazione() const override;

	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	/** Dà la forma giusta ai pezzi (su ogni PC, quando si sa cosa c'è). */
	void Sistema();
	UFUNCTION()
	void OnRep_Oggetto();
	UFUNCTION()
	void OnRep_Disponibile();
	void MostraDisponibile();

	UPROPERTY(VisibleAnywhere, Category = "Raccolta")
	TObjectPtr<USphereComponent> Zona;

	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> Pezzi;

	UPROPERTY()
	TObjectPtr<UStaticMesh> FormaSfera;

	UPROPERTY()
	TObjectPtr<UStaticMesh> FormaCilindro;

	UPROPERTY()
	TObjectPtr<UStaticMesh> FormaCono;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> MaterialeSemplice;

	UPROPERTY(ReplicatedUsing = OnRep_Oggetto)
	FName Oggetto;

	UPROPERTY(ReplicatedUsing = OnRep_Disponibile)
	bool bDisponibile = true;

	int32 Quantita = 1;
	float Ricrescita = 300.f;
	FTimerHandle TimerRicrescita;
	bool bSistemato = false;
};

/** Il server mette le cose da raccogliere al loro posto quando un colono ci arriva vicino. */
UCLASS()
class VALDORSO_API UValdorsoRaccolteSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;

	/** Per la console: tutto torna al suo posto. */
	void RicresciTutto();

private:
	/** Prova a mettere il posto Indice (falso se la linea verso il basso non trova il terreno: si riprova dopo). */
	bool Metti(int32 Indice);

	/** Per ogni posto: l'attore messo (nullptr finché non serve). */
	TArray<TWeakObjectPtr<AValdorsoRaccolta>> Messi;
	/** I posti dove la linea verso il basso trova altro che il paesaggio (un tetto, una roccia): si lasciano stare. */
	TSet<int32> Saltati;
	double ProssimoControllo = 0.0;
	bool bValle = false;
	bool bValleControllata = false;
};
