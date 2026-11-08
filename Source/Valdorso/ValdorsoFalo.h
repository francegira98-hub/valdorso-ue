// Valdorso - Falò e bivacchi (v0.1.4 "Pericoli", seconda parte). Scritto da Claude il 08/11/2026.
//
// Come funziona:
// - Con l'acciarino nello zaino (clic destro, o il suo tasto nella barra) e due fasci di rami secchi, il colono accende
//   un falò un metro e mezzo davanti a sé: per terra, all'aperto (non sotto un tetto), fuori dall'acqua, non addosso
//   a un altro fuoco. Lo decide il server (AValdorsoFalo::Accendi), che consuma i rami.
// - Il falò dura cinque minuti; nell'ultimo mezzo minuto si abbassa e poi si spegne. Ogni colono ne tiene acceso uno
//   solo: accendendone un altro, il primo si spegne. Lo vedono tutti (l'attore viaggia in rete).
// - Al fuoco (entro 4 m) ci si asciuga in mezzo minuto e, anche bagnati, la stamina torna piena velocità
//   (ValdorsoBagnato.h).
// - Com'è fatto: un cerchio di sassi, tre ciocchi incrociati, le braci e tre lingue di fuoco incrociate (M_Fuoco2, lo
//   stesso dei bracieri del menu), con una luce che trema. Tutto con le forme di serie di Unreal: niente da importare.
// Comando di console (sul server, o giocando da soli): Valdorso.Falo (accende un falò davanti a ogni colono, senza
// rami, per le prove).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ValdorsoFalo.generated.h"

class AController;
class APawn;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UPointLightComponent;
class UStaticMesh;
class UStaticMeshComponent;

namespace ValdorsoFaloRegole
{
	/** Quanto dura un falò (secondi) e in quanti secondi, alla fine, si spegne. */
	inline constexpr float Durata = 300.f;
	inline constexpr float Spegnimento = 30.f;
	/** Fin dove scalda e asciuga (cm). */
	inline constexpr float RaggioCaldo = 400.f;
	/** Quanto lontano dal colono si accende (cm) e quanto lontano deve stare da un altro fuoco. */
	inline constexpr float Davanti = 150.f;
	inline constexpr float DistanzaMinima = 300.f;

	/** Quanto è vivo un fuoco (0..1) quando gli restano Rimasti secondi: 1 fino all'ultimo mezzo minuto, poi cala. */
	VALDORSO_API float Vita(float Rimasti);
}

UCLASS()
class VALDORSO_API AValdorsoFalo : public AActor
{
	GENERATED_BODY()

public:
	AValdorsoFalo();

	/**
	 * Solo sul server: Chi accende un falò davanti a sé. Restituisce un testo vuoto se l'ha acceso, altrimenti il
	 * motivo da dire al giocatore ("Qui sotto un tetto no: il fumo..."). Non tocca lo zaino (i rami li toglie chi chiama).
	 */
	static FText Accendi(APawn* Chi);

	/** Vero se in quel punto c'è un falò acceso vicino (entro RaggioCaldo). */
	static bool CaldoVicino(const UWorld* Mondo, const FVector& Dove);

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	/** Secondi che restano (dall'orologio del server, uguale su tutti i PC). */
	float Rimasti() const;
	UStaticMeshComponent* Pezzo(const TCHAR* Nome, UStaticMesh* Forma, const FVector& Dove, const FRotator& Giro, const FVector& Scala);

	UPROPERTY(VisibleAnywhere, Category = "Falo")
	TObjectPtr<USceneComponent> Radice;

	UPROPERTY(VisibleAnywhere, Category = "Falo")
	TObjectPtr<UPointLightComponent> Luce;

	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> Sassi;

	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> Ciocchi;

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> Braci;

	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> Fiamme;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> MaterialeSemplice;

	UPROPERTY()
	TArray<TObjectPtr<UMaterialInstanceDynamic>> FiammeVive;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> BraciVive;

	/** Quando si spegne, sull'orologio del server. */
	UPROPERTY(Replicated)
	double SpentoAlle = 0.0;

	/** Chi l'ha acceso (solo sul server: uno a testa). */
	TWeakObjectPtr<AController> Acceso;

	float Tempo = 0.f;
	float Seme = 0.f;
};
