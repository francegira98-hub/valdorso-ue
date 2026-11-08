// Valdorso - I pericoli della valle (v0.1.4 "Pericoli", prima parte). Scritto da Claude il 08/11/2026.
//
// Come funziona:
// - UValdorsoPericoliComponent sta su ogni colono (AValdorsoCharacter). Sul server ascolta la salute e l'equilibrio
//   (le statistiche del Gameplay Ability System, ValdorsoAttributeSet.h) e decide.
// - L'equilibrio: ogni pugno ne toglie 30 (UValdorsoGE_DannoPugno), ricresce da solo di 5 al secondo
//   (UValdorsoGE_RecuperoEquilibrio). A zero il colono barcolla: per un secondo e mezzo non si muove (lo stordimento
//   già esistente di ClientAvviso, con la scritta "Perdi l'equilibrio!"), poi l'equilibrio torna pieno.
// - La morte: quando la salute arriva a zero il colono cade. Il server lo segna (bCaduto, arriva a tutti) e ogni PC lo
//   fa cadere: con il montaggio della morte se nel Blueprint del personaggio è scelto (componente Pericoli, voce
//   "Montaggio Morte"; si ferma sull'ultimo fotogramma), altrimenti come una bambola di pezza (la fisica del manichino).
//   I tasti del corpo si spengono (movimento, salto, pugno, schivata); restano quelli del PlayerController (pausa, chat).
//   Sullo schermo del colono caduto lo schermo si scurisce: "Sei caduto. Il Cuore ti riporta al tempio…"
//   (SValdorsoVeloCaduta, SValdorsoBarre.h).
// - La rinascita: dopo 6 secondi il server rialza il colono nel Tempio del Cuore, davanti all'altare con la pietra di
//   Val d'Orso (AValdorsoPietraVillaggio, ValdorsoTerritorio.h), girato verso l'ingresso; se la pietra non c'è, nel
//   luogo "Tempio del Cuore" (ValdorsoLuoghiDati.inl); se manca anche quello, al punto di partenza della mappa.
//   Tutte le statistiche tornano piene e il personaggio si salva (AValdorsoPlayerController::SalvaPersonaggio).
//   La pietra sta sull'altare con la rotazione della luce (cioè nessuna): il verso dell'ingresso si ricava dal centro
//   del tempio (il luogo), che sta più avanti nella navata; se i due punti coincidono, verso sud (+Y, la piazza).
// - Nel registro eventi (ValdorsoRegistroEventi.h): "Morte" (chi è caduto, chi gli ha dato l'ultimo colpo) e
//   "Rinascita" (dove).
// Comandi di console (sul server, o giocando da soli): Valdorso.Ferisci <quantità> [nome] (un numero negativo cura),
// Valdorso.Muori [nome]. Senza nome: il proprio colono se si gioca da soli o si ospita, tutti sul server dedicato.
// Da fare (08/11): chi esce dal gioco mentre è caduto rientra dov'era con la salute di partenza (il file del
// personaggio non sa ancora della morte).

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/EngineTypes.h"
#include "ValdorsoPericoli.generated.h"

class UAnimMontage;
class ACharacter;
struct FOnAttributeChangeData;

namespace ValdorsoPericoli
{
	/** Quanto equilibrio toglie un pugno (lo usa UValdorsoGE_DannoPugno). */
	inline constexpr float EquilibrioPerPugno = 30.f;

	/** Quanto ne ridà al secondo il recupero (UValdorsoGE_RecuperoEquilibrio: 1,25 ogni 0,25 s). */
	inline constexpr float EquilibrioAlSecondo = 5.f;

	/** L'altare del tempio è alto 118 cm: la pietra ci sta sopra (crea_villaggio.py). */
	inline constexpr float AltezzaAltare = 118.f;

	/** Quanto davanti all'altare si rinasce (cm): oltre la pedana (150), prima delle panche (390), nella navata. */
	inline constexpr float DistanzaDallAltare = 250.f;

	/** Vero se con questo equilibrio il colono barcolla (a zero, o quasi). */
	VALDORSO_API bool PerdeEquilibrio(float Equilibrio);

	/** Vero se con questa salute il colono cade. */
	VALDORSO_API bool Caduta(float Salute);

	/**
	 * Il verso dell'ingresso del tempio, in piano e lungo 1: dalla pietra verso il centro del tempio. Se i due punti
	 * sono quasi uguali (meno di mezzo metro), verso sud (+Y: il tempio guarda la piazza).
	 */
	VALDORSO_API FVector VersoIngresso(const FVector& Pietra, const FVector& CentroTempio);

	/**
	 * Dove si rinasce: DistanzaDallAltare davanti alla pietra, verso l'ingresso, con i piedi sul pavimento del tempio
	 * (la pietra sta AltezzaAltare sopra) e il centro della capsula MezzaAltezza più su; girato verso l'ingresso.
	 */
	VALDORSO_API FTransform DavantiAllAltare(const FVector& Pietra, const FVector& Verso, float MezzaAltezza);
}

UCLASS(ClassGroup = (Valdorso))
class VALDORSO_API UValdorsoPericoliComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UValdorsoPericoliComponent();

	/** Il componente dei pericoli di quel corpo (nullptr se non ce l'ha). */
	static UValdorsoPericoliComponent* Di(const AActor* Corpo);

	/** Vero dalla caduta alla rinascita (su tutti i PC). */
	bool Caduto() const { return bCaduto; }

	/** Vero mentre barcolla (solo sul server). */
	bool StaBarcollando() const { return bBarcolla; }

	/** Server: toglie tanta salute (console Valdorso.Ferisci); un numero negativo cura. */
	void Ferisci(float Quantita);

	/** Server: il colono cade subito (console Valdorso.Muori). */
	void Muori();

	/** Il montaggio della morte (scelto nel Blueprint del personaggio). Se manca, il colono cade come una bambola di pezza. */
	UPROPERTY(EditDefaultsOnly, Category = "Pericoli")
	TObjectPtr<UAnimMontage> MontaggioMorte;

	/** Dopo quanti secondi dalla caduta il colono rinasce al tempio. */
	UPROPERTY(EditDefaultsOnly, Category = "Pericoli", meta = (ClampMin = "1.0"))
	float SecondiRinascita = 6.f;

	/** Quanto dura il barcollare, in secondi. */
	UPROPERTY(EditDefaultsOnly, Category = "Pericoli", meta = (ClampMin = "0.2"))
	float SecondiBarcollo = 1.5f;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	/** Server: la salute o l'equilibrio sono cambiati. */
	void QuandoCambiaSalute(const FOnAttributeChangeData& Dati);
	void QuandoCambiaEquilibrio(const FOnAttributeChangeData& Dati);

	/** Server: la caduta (Come va nel registro eventi). */
	void Cadi(const FString& Come);
	void FineBarcollo();
	void Rinasci();

	/** Server: dove rinasce questo colono. */
	FTransform PuntoDiRinascita(const ACharacter* Colono) const;

	/** Su ogni PC: fa cadere il corpo o lo rialza (montaggio o bambola di pezza, tasti, movimento). */
	void MostraCaduta();
	void MostraRialzata();
	void FermaPosaMorte();

	UFUNCTION()
	void OnRep_Caduto();

	UPROPERTY(ReplicatedUsing = OnRep_Caduto)
	bool bCaduto = false;

	/** Su questo PC il corpo è già a terra (o già rialzato): niente doppioni. */
	bool bCadutaMostrata = false;
	/** Su questo PC il corpo è caduto con la fisica (da rimettere a posto alla rinascita). */
	bool bBambola = false;
	/** Su questo PC i tasti del corpo sono stati spenti. */
	bool bTastiSpenti = false;

	/** Solo sul server. */
	bool bBarcolla = false;
	TWeakObjectPtr<AActor> UltimoColpevole;

	/** Com'era la mesh prima della bambola di pezza (posto sotto la capsula e collisioni). */
	FTransform MeshRelativa = FTransform::Identity;
	FName ProfiloMesh = NAME_None;
	bool bMeshLetta = false;

	FTimerHandle TimerRinascita;
	FTimerHandle TimerBarcollo;
	FTimerHandle TimerPosaMorte;
	FDelegateHandle ManigliaSalute;
	FDelegateHandle ManigliaEquilibrio;
};
