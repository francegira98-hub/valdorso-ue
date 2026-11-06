// Valdorso - Il sigillo della Corona sul passo (v0.1.3). Scritto da Claude il 06/11/2026.
// Dal Design ("La barriera del passo", idea di Fra del 28/09), approvato da Fra il 06/11 (passo B).
//
// Come funziona:
// - Dietro la sbarra del posto di guardia un velo luminoso attraversa la gola da parete a parete: le rune scorrono,
//   pulsa con il battito (due colpi e una pausa, come il frammento del Cuore), è più forte di notte e di notte ogni
//   tanto tremola per un istante (i segni del risveglio). Si sente un ronzio basso.
// - Un muro invisibile ferma i coloni (solo loro: telecamera, luce e colpi di vista passano). Poco prima del muro c'è
//   la soglia: chi la tocca viene respinto verso la valle con una spinta e un salto, resta fermo un attimo e legge
//   "Il sigillo della Corona ti respinge". Lo decide il server, che lo annota nel registro eventi; il PC di chi
//   viene respinto lo fa subito anche lui, così non si sente il ritardo della rete.
// - Il sigillo si può aprire (per le carovane del Balivo, per un evento dello staff, e un giorno quando "la Corona
//   toglie il sigillo"): aperto, il velo sparisce e si passa. Lo stato viaggia in rete.
// - La mesh del velo, la texture delle rune e il ronzio li fa Tools/Generatori/sigillo.py; crea_villaggio.py mette
//   l'attore nel livello con le misure giuste della gola (Larghezza, Altezza, Base, CentroTraverso).
// Comandi di console (server): Valdorso.Sigillo.Apri, Valdorso.Sigillo.Chiudi.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ValdorsoSigillo.generated.h"

class ACharacter;
class UAudioComponent;
class UBoxComponent;
class UMaterialInstanceDynamic;
class UPrimitiveComponent;
class UStaticMeshComponent;
class AValdorsoCielo;

namespace ValdorsoSigillo
{
	/**
	 * La spinta per chi tocca il sigillo: via dal velo, dalla parte dove si trova (di solito verso la valle, che è la X
	 * dell'attore), più un salto in alto. Sigillo: dove sta il sigillo; Punto: dove sta il colono.
	 */
	VALDORSO_API FVector Spinta(const FTransform& Sigillo, const FVector& Punto, float Forza, float Su);

	/** Quanto brilla il velo (1 = di giorno, a riposo): battito in due colpi ogni 1,6 s, più forte di notte. */
	VALDORSO_API float Luminosita(float Secondi, float Ora);
}

UCLASS()
class VALDORSO_API AValdorsoSigillo : public AActor
{
	GENERATED_BODY()

public:
	AValdorsoSigillo();

	/** Quanto è largo il velo, di traverso alla gola (cm). */
	UPROPERTY(EditAnywhere, Category = "Sigillo", meta = (ClampMin = "100"))
	float Larghezza = 6000.f;

	/** Quanto è alto il velo (cm), dalla Base in su. */
	UPROPERTY(EditAnywhere, Category = "Sigillo", meta = (ClampMin = "100"))
	float Altezza = 1500.f;

	/** Dove comincia il velo in basso, rispetto all'attore (cm, di solito negativo: il fondo della gola). */
	UPROPERTY(EditAnywhere, Category = "Sigillo")
	float Base = -200.f;

	/** Il mezzo del velo di traverso, rispetto all'attore (cm): la gola non è sempre simmetrica. */
	UPROPERTY(EditAnywhere, Category = "Sigillo")
	float CentroTraverso = 0.f;

	/** La spinta verso la valle (cm/s) e il salto (cm/s). */
	UPROPERTY(EditAnywhere, Category = "Sigillo", meta = (ClampMin = "0"))
	float ForzaSpinta = 900.f;

	UPROPERTY(EditAnywhere, Category = "Sigillo", meta = (ClampMin = "0"))
	float SpintaSu = 380.f;

	/** Quanto resta fermo chi viene respinto (secondi): un breve stordimento. */
	UPROPERTY(EditAnywhere, Category = "Sigillo", meta = (ClampMin = "0"))
	float SecondiStordimento = 1.0f;

	/** Il colore del velo (oro e azzurro arcano, moltiplicato per la luminosità). */
	UPROPERTY(EditAnywhere, Category = "Sigillo")
	FLinearColor Colore = FLinearColor(1.0f, 0.78f, 0.42f);

	UPROPERTY(VisibleAnywhere, Category = "Sigillo")
	TObjectPtr<UStaticMeshComponent> Velo;

	UPROPERTY(VisibleAnywhere, Category = "Sigillo")
	TObjectPtr<UBoxComponent> Muro;

	UPROPERTY(VisibleAnywhere, Category = "Sigillo")
	TObjectPtr<UBoxComponent> Soglia;

	UPROPERTY(VisibleAnywhere, Category = "Sigillo")
	TObjectPtr<UAudioComponent> Ronzio;

	/** Solo sul server: apre o chiude il sigillo (lo annota nel registro eventi). */
	void ImpostaAperto(bool bNuovo);

	bool EAperto() const { return bAperto; }

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(ReplicatedUsing = OnRep_Aperto)
	bool bAperto = false;

	UFUNCTION()
	void OnRep_Aperto();

	/** Mette muro, soglia, velo e ronzio secondo bAperto. */
	void ApplicaAperto();

	/** Le misure dei due box secondo Larghezza, Altezza, Base e CentroTraverso. */
	void SistemaMisure();

	UFUNCTION()
	void QuandoTocca(UPrimitiveComponent* Componente, AActor* Altro, UPrimitiveComponent* AltroComponente, int32 Indice,
		bool bDaSpinta, const FHitResult& Colpo);

	/** Ogni tanto: chi è ancora dentro la soglia (perché spinge contro il muro) viene respinto di nuovo. */
	void ControllaSoglia();

	void Respingi(ACharacter* Colono);

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> MaterialeVelo;

	TWeakObjectPtr<AValdorsoCielo> Cielo;
	TMap<TWeakObjectPtr<ACharacter>, double> UltimaSpinta;
	FTimerHandle TimerSoglia;
	double ProssimoTremito = 0.0;
	double ProssimaRicercaCielo = 0.0;
	double FineTremito = 0.0;
};
