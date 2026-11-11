// Valdorso - Il baule del colono (v0.2.0 "Mani e materiali", 12/11/2026). Scritto da Claude.
//
// Da Bruno, alla locanda, ogni colono ha il suo baule: "Qui le cose di chi e' arrivato restano al sicuro. Il baule e'
// tuo, la chiave ce l'ho io, e io non apro niente." Un baule solo nella stanza (AValdorsoBaule, lo mette
// Content/Python/nuovi/crea_volti.py), ma dentro ognuno trova le sue cose: il contenuto sta nel suo zaino
// (FValdorsoZaino::Baule, 30 posti), si salva con il personaggio e arriva solo al suo PC.
// - E davanti al baule: si apre la schermata (SValdorsoBaule.h): a sinistra lo zaino, a destra il baule; "Metti" e
//   "Prendi" chiedono al server (UValdorsoZainoComponent::ServerDeposita / ServerRitira), che controlla che il colono
//   sia davvero li' davanti. Le cose nel baule non pesano. La tessera no: va sempre con te.
// - Le regole (quanto entra, le pile) stanno in ValdorsoZainoDati.h (Deposita, Ritira), provate dai test.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ValdorsoInterazione.h"
#include "ValdorsoBaule.generated.h"

class USphereComponent;

UCLASS()
class VALDORSO_API AValdorsoBaule : public AActor, public IValdorsoInterattivo
{
	GENERATED_BODY()

public:
	AValdorsoBaule();

	/** Quanto si puo' stare lontani dal baule per usarlo (cm). */
	static constexpr float Raggio = 420.f;

	/** Vero se quel colono ha un baule vicino. */
	static bool Vicino(const APawn* Colono);

	virtual FText TestoInterazione(const APawn* Chi) const override;
	virtual void Interagisci(APawn* Chi) override;
	virtual FVector PuntoInterazione() const override { return GetActorLocation() + FVector(0.f, 0.f, 60.f); }

private:
	UPROPERTY(VisibleAnywhere, Category = "Baule")
	TObjectPtr<USphereComponent> Presa;
};
