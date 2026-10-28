// Valdorso - L'aspetto del colono nel mondo: il corpo MetaHuman sopra il manichino e i cursori del volto.
// Scritto da Claude il 28/10/2026 (v0.1.2, passo 4.3/4.4).
//
// Come funziona:
// - Il manichino (la Mesh del personaggio) resta e fa tutto: animazioni, abilità, rete. Diventa solo invisibile.
// - Su ogni PC (mai sul server dedicato) si fa nascere il Blueprint MetaHuman della testa scelta (voce "Testa" del
//   catalogo, tra le teste da uomo o da donna), si attacca il suo Body alla Mesh del manichino e gli si dà
//   l'animazione che copia la posa (ABP_CorpoSegue, Copy Pose From Mesh). Il volto (Face) segue il corpo.
// - L'aspetto (FValdorsoAspetto) e il sesso li decide il server, che li legge dal Registro del personaggio, e
//   arrivano a tutti in rete: i cursori del volto vanno nelle variabili Float dell'animazione del volto
//   (ABP_Face_PostProcess) con lo stesso nome dell'Id del catalogo; il nodo Volto Valdorso fa il resto.
// - Se la testa cambia (arriva dopo, o si cambia nella pagina Aspetto), il corpo si rifà.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ValdorsoAspetto.h"
#include "ValdorsoAspettoComponent.generated.h"

class USkeletalMeshComponent;
class UAnimInstance;
class USceneCaptureComponent2D;

UCLASS(ClassGroup = (Valdorso), meta = (BlueprintSpawnableComponent))
class VALDORSO_API UValdorsoAspettoComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UValdorsoAspettoComponent();

	/**
	 * Le teste base da uomo (Blueprint MetaHuman montati), nell'ordine delle scelte della voce "Testa".
	 * Le liste si cambiano solo qui nel C++ (costruttore): le leggono anche il Registro e il palco.
	 */
	UPROPERTY(VisibleDefaultsOnly, Category = "Aspetto")
	TArray<TSoftClassPtr<AActor>> TesteUomo;

	/** Le teste base da donna. */
	UPROPERTY(VisibleDefaultsOnly, Category = "Aspetto")
	TArray<TSoftClassPtr<AActor>> TesteDonna;

	/** La testa di riserva, se quella scelta non c'è su questo PC. */
	UPROPERTY(EditDefaultsOnly, Category = "Aspetto")
	TSoftClassPtr<AActor> ClasseMetaHuman;

	/** L'animazione del corpo MetaHuman che copia la posa del manichino (Copy Pose From Mesh, Use Attached Parent). */
	UPROPERTY(EditDefaultsOnly, Category = "Aspetto")
	TSoftClassPtr<UAnimInstance> AnimazioneCorpo;

	/**
	 * L'animazione del volto con il nodo Volto Valdorso (i cursori). Le teste montate dal Creator nuovo usano quella del
	 * plugin MetaHuman, senza il nostro nodo: si mette questa al suo posto.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Aspetto")
	TSoftClassPtr<UAnimInstance> AnimazioneVolto;

	/** Spento = resta il manichino grigio (utile per le prove). */
	UPROPERTY(EditDefaultsOnly, Category = "Aspetto")
	bool bUsaMetaHuman = true;

	/** Solo sul server: aspetto e sesso del personaggio (dal Registro). Arrivano a tutti e si vedono sul colono. */
	void ImpostaAspetto(const FValdorsoAspetto& Nuovo, const FString& Sesso);

	const FValdorsoAspetto& GetAspetto() const { return Aspetto; }

	/** Solo per le prove, su questo PC: cambia un cursore del volto (-1..1) senza toccare l'aspetto vero. */
	void ProvaCursore(FName Id, float Valore);

	/** Solo per le prove, su questo PC: mette un'altra testa (numero della scelta, da uomo o da donna; -1 = quella vera). */
	void ProvaTesta(int32 Numero, bool bDiDonna);

	/**
	 * (28/10) Per il palco del Registro: il MetaHuman si attacca a questo manichino (invece della Mesh del personaggio)
	 * e lo vede solo questa telecamera (il ritratto).
	 */
	void UsaSupporto(USkeletalMeshComponent* Manichino, USceneCaptureComponent2D* Ritratto);

	/** Quante teste base ci sono per un uomo o per una donna (dalle impostazioni di serie del componente). */
	static int32 TesteDisponibili(bool bPerDonna);

	/** Il volto MetaHuman (nullptr se non c'è). */
	USkeletalMeshComponent* GetVolto() const { return Volto.Get(); }

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UPROPERTY(ReplicatedUsing = OnRep_Aspetto)
	FValdorsoAspetto Aspetto;

	UPROPERTY(ReplicatedUsing = OnRep_Aspetto)
	bool bDonna = false;

	UFUNCTION()
	void OnRep_Aspetto();

	/** La testa da usare ora (prova, scelta o riserva). */
	TSoftClassPtr<AActor> TestaDaUsare() const;

	/** Rifà il corpo se la testa è cambiata, poi scrive i cursori. */
	void Aggiorna();

	/** Fa nascere il MetaHuman e lo attacca al manichino. */
	void CreaCorpo(const TSoftClassPtr<AActor>& Testa);

	/** Toglie il corpo MetaHuman (il manichino torna visibile). */
	void TogliCorpo();

	/** Il manichino su cui va il MetaHuman (quello del palco, o la Mesh del personaggio). */
	USkeletalMeshComponent* GetManichino() const;

	/** Scrive i cursori del volto nell'animazione del volto. */
	void ApplicaVolto();

	UPROPERTY(Transient)
	TObjectPtr<AActor> CorpoMetaHuman;

	/** La testa del corpo che c'è adesso. */
	TSoftClassPtr<AActor> TestaAttuale;

	TWeakObjectPtr<USkeletalMeshComponent> Volto;

	bool bIniziato = false;

	TWeakObjectPtr<USkeletalMeshComponent> Supporto;
	TWeakObjectPtr<USceneCaptureComponent2D> SoloRitratto;

	/** I cursori cambiati con ProvaCursore (vincono sull'aspetto). */
	TMap<FName, float> CursoriProva;

	/** La testa messa con ProvaTesta (vuota = quella vera). */
	TSoftClassPtr<AActor> TestaProva;
};
