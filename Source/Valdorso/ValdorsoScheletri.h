// Valdorso - Gli scheletri delle gallerie dei nani (v0.1.7 "La grotta del nord", 11/11/2026). Scritto da Claude. Dalla
// Roadmap ("Gli scheletri. Solo nella cripta delle Rovine e nella grotta nord... Lenti, duri, temono la luce") e dal
// Punto di ripresa del 10/11 ("scheletri: nuova specie con forza 40-70"). Fra il 10/11: "generati ora, Fab dopo".
//
// Come funziona:
// - Tre tipi (ValdorsoScheletri::Tipi): lo scheletro (forza 40), lo scheletro con lo scudo (55: para da davanti) e il
//   capitano (70, uno solo, nella sala delle ossa). Lenti e duri: le botte (mazze, martelli, clave) fanno di piu', le lame
//   e i pugni di meno, i morsi quasi niente (Debolezza, la usa ValdorsoScherma::PrimaDelColpo).
// - AValdorsoScheletro: un ACharacter con le statistiche (GAS), mosso dal server. Dorme come un mucchio d'ossa finche' un
//   colono non gli arriva a otto metri (o lo colpisce): allora si alza (due secondi), insegue, colpisce con la sua spada
//   arrugginita (ValdorsoCombattimento::Colpisci). Teme il fuoco: chi sta vicino a un falo' acceso non lo attacca (resta a
//   quattro metri). Se il colono scappa lontano torna al suo posto e si riaddormenta.
// - Morto: crolla in un mucchio. E: "Rovista tra le ossa" (monete dei nani, a volte un pugnale; il capitano porta l'anello
//   della storia di Lucia, ValdorsoGrottaNord.h). Le ossa restano due minuti.
// - AValdorsoOssario (lo mette crea_grotta_nord.py nelle sale): fa nascere i suoi scheletri addormentati e, quando sono
//   tutti morti, li rifa' dopo i suoi minuti. Solo sul server.
// - L'aspetto (provvisorio, Tools/Generatori/grotta_nord.py -> /Game/Valle/Grotta/Scheletri): teschio, torace, bacino, due
//   braccia, due gambe, la spada e lo scudo; ogni PC li muove da se'. Quando arriva uno scheletro di Fab si cambia solo
//   il modello.
// Comandi di console: Valdorso.Scheletri.Chiama [tipo] (uno a cinque metri, sveglio), Valdorso.Scheletri.Via.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "ValdorsoCombattimento.h"
#include "ValdorsoInterazione.h"
#include "ValdorsoScheletri.generated.h"

class UAbilitySystemComponent;
class UStaticMeshComponent;
class UValdorsoAttributeSet;
class UValdorsoReazioniComponent;

UENUM()
enum class EValdorsoStatoScheletro : uint8
{
	Dorme,
	SiAlza,
	Caccia,
	Colpisce,
	Torna,
	Morto,
};

/** Un tipo di scheletro. */
struct FValdorsoTipoScheletro
{
	const TCHAR* Id = TEXT("");
	const TCHAR* ConArticolo = TEXT("");
	float Forza = 40.f;
	float Salute = 80.f;
	float Danno = 9.f;
	float Equilibrio = 20.f;
	float SecondiColpo = 1.8f;
	float Portata = 170.f;
	float Passo = 110.f;
	float Corsa = 280.f;
	/** Para da davanti (lo scudo): quanto del colpo si ferma (0..1). */
	float Parata = 0.f;
	/** Grande (il capitano): x. */
	float Grandezza = 1.f;
	/** (11/11) I modelli: SM_Scheletro_Teschio... ("SM_Guardiano_" per il guardiano della forgia). */
	const TCHAR* Modello = TEXT("SM_Scheletro_");
	/** (11/11) Di pietra (0..1): le lame e i pugni ci rimbalzano (il guardiano della forgia). */
	float Pietra = 0.f;
	/** (11/11) Teme il fuoco e il battito del tempio (il guardiano no). */
	bool bTemeFuoco = true;
};

namespace ValdorsoScheletri
{
	/** Da quanto vicino si sveglia (cm) e oltre quanto lascia andare chi insegue. */
	inline constexpr float Sveglia = 800.f;
	inline constexpr float Lascia = 2600.f;
	/** Quanto lontano dal fuoco resta (cm). */
	inline constexpr float LontanoDalFuoco = 400.f;
	/** Quanti secondi per alzarsi, e quanto restano le ossa dopo la morte. */
	inline constexpr float SecondiPerAlzarsi = 2.f;
	inline constexpr float SecondiOssa = 120.f;

	VALDORSO_API TArrayView<const FValdorsoTipoScheletro> Tipi();
	VALDORSO_API const FValdorsoTipoScheletro* Tipo(FName Id);

	/** Quanto fa un colpo di quel tipo a uno scheletro (x): botte 1,5, lame 0,7, pugni 0,6, morsi 0,4. */
	VALDORSO_API float Debolezza(EValdorsoTipoColpo Colpo);

	/** Lo stato dopo (le regole senza il mondo, per i test). Distanza < 0 = nessun colono vicino. */
	VALDORSO_API EValdorsoStatoScheletro Prossimo(EValdorsoStatoScheletro Attuale, float Distanza, bool bFerito, bool bProtetto,
		float SecondiNelloStato, float DistanzaDaCasa);
}

UCLASS()
class VALDORSO_API AValdorsoScheletro : public ACharacter, public IAbilitySystemInterface, public IValdorsoInterattivo
{
	GENERATED_BODY()

public:
	AValdorsoScheletro();

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override { return Abilita; }

	/** Solo sul server, prima di FinishSpawning: che tipo e', e dove dorme. bSveglio: si alza subito (le prove). */
	void Prepara(FName QualeTipo, const FVector& DoveDorme, bool bSveglio);

	/** Solo sul server: qualcuno l'ha colpito (si sveglia, e lo insegue). */
	void Ferito(AActor* Da);

	/** Solo sul server: resta fermo per qualche secondo (una tecnica che stordisce). */
	void Stordisci(float Secondi);

	/** Quanto fa un colpo di quel tipo (x), con lo scudo per chi lo porta (lo usa ValdorsoScherma::PrimaDelColpo). */
	float Debolezza(EValdorsoTipoColpo Colpo) const;

	/** La sua forza (40..70), per "a colpo d'occhio". */
	float Forza() const;

	const FValdorsoTipoScheletro* GetTipo() const;
	bool Vivo() const { return Stato != EValdorsoStatoScheletro::Morto; }
	EValdorsoStatoScheletro GetStato() const { return Stato; }

	// IValdorsoInterattivo: rovistare tra le ossa
	virtual FText TestoInterazione(const APawn* Chi) const override;
	virtual void Interagisci(APawn* Chi) override;
	virtual FVector PuntoInterazione() const override { return GetActorLocation(); }

	virtual void Tick(float DeltaSeconds) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;

private:
	void Vesti();
	void Pensa(float DeltaSeconds);
	void Muoviti(float DeltaSeconds);
	void Anima(float DeltaSeconds);
	void Cambia(EValdorsoStatoScheletro Nuovo);
	void Muori();
	APawn* CercaPreda() const;
	void Suona(const TCHAR* Nome) const;

	UFUNCTION()
	void OnRep_Stato();

	UFUNCTION()
	void OnRep_Tipo();

	UPROPERTY(VisibleAnywhere, Category = "Scheletro")
	TObjectPtr<UAbilitySystemComponent> Abilita;

	UPROPERTY()
	TObjectPtr<UValdorsoAttributeSet> Attributi;

	UPROPERTY(VisibleAnywhere, Category = "Scheletro")
	TObjectPtr<UValdorsoReazioniComponent> Reazioni;

	UPROPERTY(VisibleAnywhere, Category = "Scheletro")
	TObjectPtr<USceneComponent> Figura;

	/** I pezzi: teschio, torace, bacino, braccio sinistro, braccio destro, gamba sinistra, gamba destra, spada, scudo. */
	UPROPERTY(VisibleAnywhere, Category = "Scheletro")
	TArray<TObjectPtr<UStaticMeshComponent>> Pezzi;

	UPROPERTY(ReplicatedUsing = OnRep_Tipo)
	FName Tipo = TEXT("Scheletro");

	UPROPERTY(ReplicatedUsing = OnRep_Stato)
	EValdorsoStatoScheletro Stato = EValdorsoStatoScheletro::Dorme;

	UPROPERTY(Replicated)
	bool bRovistato = false;

	// --- solo sul server
	FVector Casa = FVector::ZeroVector;
	TWeakObjectPtr<APawn> Preda;
	TWeakObjectPtr<AActor> UltimoAggressore;
	bool bFerito = false;
	bool bProtetto = false;
	bool bColpoDato = false;
	double InizioStato = 0.0;
	double ProssimoColpo = 0.0;
	double ProssimaRicerca = 0.0;
	double StorditoFino = 0.0;
	double Fermo = 0.0;
	double ScansaFino = 0.0;
	FVector Scansa = FVector::ZeroVector;

	// --- su ogni PC (l'aspetto)
	double InizioStatoQui = 0.0;
	float Passo = 0.f;
	float Mucchio = 1.f;
	FName Vestito;
	TArray<FTransform> PosaInPiedi;
	TArray<FTransform> PosaMucchio;
};

/** Un ossario: dove dormono degli scheletri (lo mette crea_grotta_nord.py nelle sale). Solo sul server. */
UCLASS()
class VALDORSO_API AValdorsoOssario : public AActor
{
	GENERATED_BODY()

public:
	AValdorsoOssario();

	/** Che tipo (Scheletro, ScheletroScudo, Capitano), quanti, entro che raggio, e dopo quanti minuti tornano. */
	UPROPERTY(EditAnywhere, Category = "Ossario")
	FName TipoScheletro = TEXT("Scheletro");

	UPROPERTY(EditAnywhere, Category = "Ossario", meta = (ClampMin = "1", ClampMax = "8"))
	int32 Quanti = 3;

	UPROPERTY(EditAnywhere, Category = "Ossario", meta = (ClampMin = "0"))
	float Raggio = 500.f;

	UPROPERTY(EditAnywhere, Category = "Ossario", meta = (ClampMin = "1"))
	float MinutiRitorno = 30.f;

	/** Per le prove (console): tutti gli scheletri via. */
	static void ViaTutti(UWorld* Mondo);

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;

private:
	void FaiNascere();

	TArray<TWeakObjectPtr<AValdorsoScheletro>> Vivi;
	double Ritorno = 0.0;
	bool bNati = false;
};
