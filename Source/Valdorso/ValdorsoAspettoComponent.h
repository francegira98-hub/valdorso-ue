// Valdorso - L'aspetto del colono nel mondo: il corpo MetaHuman sopra il manichino e i cursori del volto.
// Scritto da Claude il 28/10/2026 (v0.1.2, passo 4.3/4.4).
//
// Come funziona:
// - Il manichino (la Mesh del personaggio) resta e fa tutto: animazioni, abilità, rete. Diventa solo invisibile.
// - Su ogni PC (mai sul server dedicato) si fa nascere il Blueprint MetaHuman (di serie BP_MH_Prova), si attacca il
//   suo Body alla Mesh del manichino e gli si dà l'animazione che copia la posa (ABP_CorpoSegue, Copy Pose From Mesh).
//   Il volto (Face) segue il corpo come nel Blueprint MetaHuman.
// - L'aspetto (FValdorsoAspetto) lo decide il server, che lo legge dal Registro del personaggio, e arriva a tutti in rete:
//   i cursori del volto vanno nelle variabili Float dell'animazione del volto (ABP_Face_PostProcess) con lo stesso nome
//   dell'Id del catalogo; il nodo Volto Valdorso fa il resto.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ValdorsoAspetto.h"
#include "ValdorsoAspettoComponent.generated.h"

class USkeletalMeshComponent;
class UAnimInstance;

UCLASS(ClassGroup = (Valdorso), meta = (BlueprintSpawnableComponent))
class VALDORSO_API UValdorsoAspettoComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UValdorsoAspettoComponent();

	/** Il Blueprint MetaHuman da usare come corpo (per ora uno solo; poi la testa scelta nel Registro). */
	UPROPERTY(EditDefaultsOnly, Category = "Aspetto")
	TSoftClassPtr<AActor> ClasseMetaHuman;

	/** L'animazione del corpo MetaHuman che copia la posa del manichino (Copy Pose From Mesh, Use Attached Parent). */
	UPROPERTY(EditDefaultsOnly, Category = "Aspetto")
	TSoftClassPtr<UAnimInstance> AnimazioneCorpo;

	/** Spento = resta il manichino grigio (utile per le prove). */
	UPROPERTY(EditDefaultsOnly, Category = "Aspetto")
	bool bUsaMetaHuman = true;

	/** Solo sul server: l'aspetto del personaggio (dal Registro). Arriva a tutti e si applica al volto. */
	void ImpostaAspetto(const FValdorsoAspetto& Nuovo);

	const FValdorsoAspetto& GetAspetto() const { return Aspetto; }

	/** Solo per le prove, su questo PC: cambia un cursore del volto (-1..1) senza toccare l'aspetto vero. */
	void ProvaCursore(FName Id, float Valore);

	/** Il volto MetaHuman (nullptr se non c'è). */
	USkeletalMeshComponent* GetVolto() const { return Volto.Get(); }

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UPROPERTY(ReplicatedUsing = OnRep_Aspetto)
	FValdorsoAspetto Aspetto;

	UFUNCTION()
	void OnRep_Aspetto();

	/** Fa nascere il MetaHuman e lo attacca al manichino. */
	void CreaCorpo();

	/** Scrive i cursori del volto nell'animazione del volto. */
	void ApplicaVolto();

	UPROPERTY(Transient)
	TObjectPtr<AActor> CorpoMetaHuman;

	TWeakObjectPtr<USkeletalMeshComponent> Volto;

	/** I cursori cambiati con ProvaCursore (vincono sull'aspetto). */
	TMap<FName, float> CursoriProva;
};
