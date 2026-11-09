// Valdorso - I lupi (v0.1.4 passo 4.6, provvisori, 09/11/2026). Scritto da Claude. Dalla Roadmap ("I pericoli della
// valle: i lupi di notte e nella nebbia") e dal Design (i lupi non sono mostri: hanno paura del fuoco e del battito).
//
// Come funziona:
// - AValdorsoTaneLupi (una sola per mondo, la fa nascere la modalita' di gioco, solo sul server) tiene due tane: il
//   Boschetto e la Collina dell'Orso. Di notte o con la nebbia, se un colono e' entro 120 m da una tana, ne esce un
//   branco di 3 o 4 lupi. Di giorno (e se non c'e' piu' nessuno vicino) i lupi che non stanno cacciando tornano alla
//   tana (spariscono). Un branco ucciso tutto torna dopo 20 minuti.
// - AValdorsoLupo: un ACharacter con le statistiche (GAS, 60 di salute), mosso dal server. Gli stati:
//   Vaga (intorno alla tana) -> Fiuta (un colono entro 30 m: si avvicina piano) -> Accerchia (gira intorno a 6-7 m,
//   ringhia) -> Morde (corre e morde: 8 di salute, 25 di equilibrio, ValdorsoCombattimento::Colpisci) -> di nuovo
//   Accerchia. Sotto il 30% di salute Fugge. Chi sta vicino a un fuoco acceso o nella pace del battito non viene morso
//   (i lupi girano al largo, a 9 m). Chi lo colpisce diventa la sua preda.
// - Morto: si accascia, guaisce; a chi l'ha ucciso vanno la pelle e le zanne (nello zaino). Dopo 30 secondi sparisce.
// - L'aspetto (provvisorio, Tools/Generatori/pericoli.py -> /Game/Valle/Pericoli/Lupo): corpo, testa, coda e quattro
//   zampe; ogni PC le muove da se' (il passo dalla velocita', la coda, la testa che si abbassa per mordere). Di notte,
//   ogni tanto, un lupo che vaga ulula (si sente fino a 150 m).
// Comandi di console (sul server, o da soli): Valdorso.Lupi.Chiama (un branco a 25 m da te, anche di giorno, per le
// prove), Valdorso.Lupi.Via (tutti i lupi tornano alla tana).

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "ValdorsoLupi.generated.h"

class UAbilitySystemComponent;
class UStaticMeshComponent;
class UValdorsoAttributeSet;
class UValdorsoReazioniComponent;

UENUM()
enum class EValdorsoStatoLupo : uint8
{
	Vaga,
	Fiuta,
	Accerchia,
	Morde,
	Fuggi,
	Morto,
};

namespace ValdorsoLupi
{
	inline constexpr float SaluteLupo = 60.f;
	inline constexpr float DannoMorso = 8.f;
	inline constexpr float EquilibrioMorso = 25.f;
	/** Tra un morso e l'altro (secondi). */
	inline constexpr float SecondiMorso = 1.5f;
	/** Da quanto vicino morde (cm, dal centro del lupo al centro del colono). */
	inline constexpr float PortataMorso = 170.f;
	/** Entro quanto sente un colono, e oltre quanto lo lascia (cm). */
	inline constexpr float Fiuto = 3000.f;
	inline constexpr float Lascia = 4500.f;
	/** Quanto lontano gira intorno: a chi caccia, e a chi e' protetto (fuoco, pace del battito). */
	inline constexpr float GiroCaccia = 650.f;
	inline constexpr float GiroAlLargo = 900.f;
	/** Sotto questa parte della salute fugge. */
	inline constexpr float PauraSotto = 0.3f;
	/** Per quanto gira intorno prima di mordere, e per quanto fugge (secondi). */
	inline constexpr float SecondiAccerchia = 5.f;
	inline constexpr float SecondiFuga = 10.f;
	/** Il branco ucciso tutto torna dopo (secondi). */
	inline constexpr float SecondiRitorno = 20.f * 60.f;
	/** Entro quanto deve esserci un colono perche' il branco esca, e oltre quanto torna alla tana (cm). */
	inline constexpr float VicinoTana = 12000.f;
	inline constexpr float LontanoTana = 20000.f;

	/** Vero se i lupi escono: di notte (poca luce del giorno) o con la nebbia. */
	VALDORSO_API bool Fuori(float LuceDelGiorno, bool bNebbia);

	/**
	 * Lo stato che viene dopo (le regole, senza il mondo). SaluteParte 0..1; Distanza dalla preda in cm (negativa: nessuna
	 * preda); bProtetta: la preda sta vicino a un fuoco o nella pace; Secondi: da quanto e' nello stato di adesso.
	 */
	VALDORSO_API EValdorsoStatoLupo Prossimo(EValdorsoStatoLupo Attuale, float SaluteParte, float Distanza, bool bProtetta, float Secondi);

	/** Quanti lupi nel branco (3 o 4), dal numero della volta. */
	VALDORSO_API int32 QuantiNelBranco(int32 Volta);

	/** Quanto e' veloce in quello stato (cm/s). */
	VALDORSO_API float Velocita(EValdorsoStatoLupo Stato);
}

UCLASS()
class VALDORSO_API AValdorsoLupo : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AValdorsoLupo();

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override { return Abilita; }

	/** Solo sul server: da dove viene (la tana: ci vaga intorno). */
	void Casa(const FVector& Dove) { Tana = Dove; }

	/** Solo sul server: qualcuno l'ha colpito (diventa la sua preda). */
	void Ferito(AActor* Da);

	EValdorsoStatoLupo GetStato() const { return Stato; }
	bool Vivo() const { return Stato != EValdorsoStatoLupo::Morto; }
	/** Sta cacciando (accerchia o morde): non torna alla tana adesso. */
	bool Caccia() const { return Stato == EValdorsoStatoLupo::Accerchia || Stato == EValdorsoStatoLupo::Morde; }

	virtual void Tick(float DeltaSeconds) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;

private:
	void Pensa(float DeltaSeconds);
	void Muoviti(float DeltaSeconds);
	void Anima(float DeltaSeconds);
	void Cambia(EValdorsoStatoLupo Nuovo);
	void Muori();
	void SuonaStato();
	APawn* CercaPreda() const;

	UFUNCTION()
	void OnRep_Stato();

	UPROPERTY(VisibleAnywhere, Category = "Lupo")
	TObjectPtr<UAbilitySystemComponent> Abilita;

	UPROPERTY()
	TObjectPtr<UValdorsoAttributeSet> Attributi;

	UPROPERTY(VisibleAnywhere, Category = "Lupo")
	TObjectPtr<UValdorsoReazioniComponent> Reazioni;

	/** Il lupo disegnato: a terra (la capsula e' sollevata), si accascia quando muore. */
	UPROPERTY(VisibleAnywhere, Category = "Lupo")
	TObjectPtr<USceneComponent> Figura;

	UPROPERTY(VisibleAnywhere, Category = "Lupo")
	TObjectPtr<UStaticMeshComponent> Corpo;

	UPROPERTY(VisibleAnywhere, Category = "Lupo")
	TObjectPtr<UStaticMeshComponent> Testa;

	UPROPERTY(VisibleAnywhere, Category = "Lupo")
	TObjectPtr<UStaticMeshComponent> Coda;

	UPROPERTY(VisibleAnywhere, Category = "Lupo")
	TArray<TObjectPtr<UStaticMeshComponent>> Zampe;

	UPROPERTY(ReplicatedUsing = OnRep_Stato)
	EValdorsoStatoLupo Stato = EValdorsoStatoLupo::Vaga;

	// --- solo sul server
	FVector Tana = FVector::ZeroVector;
	TWeakObjectPtr<APawn> Preda;
	TWeakObjectPtr<AActor> UltimoAggressore;
	bool bPredaProtetta = false;
	double InizioStato = 0.0;
	double ProssimoMorso = 0.0;
	double ProssimaRicerca = 0.0;
	FVector Meta = FVector::ZeroVector;
	double NuovaMeta = 0.0;
	float AngoloGiro = 0.f;
	float VersoGiro = 1.f;
	double Fermo = 0.0;
	double ScansaFino = 0.0;
	FVector Scansa = FVector::ZeroVector;

	// --- su ogni PC (l'aspetto)
	float Passo = 0.f;
	float Accasciato = 0.f;
	double ProssimoUlulato = 0.0;
};

/** Le tane dei lupi (una per mondo, solo sul server): fa uscire i branchi e li richiama. */
UCLASS()
class VALDORSO_API AValdorsoTaneLupi : public AActor
{
	GENERATED_BODY()

public:
	AValdorsoTaneLupi();

	static AValdorsoTaneLupi* Di(const UWorld* Mondo);

	/** Per le prove: un branco vicino a quel punto, adesso (anche di giorno). Quanti ne sono usciti. */
	int32 Chiama(const FVector& Vicino);

	/** Tutti i lupi tornano alla tana (spariscono). */
	void Via();

	virtual void Tick(float DeltaSeconds) override;

private:
	struct FTana
	{
		FString Nome;
		FVector Centro = FVector::ZeroVector;
		TArray<TWeakObjectPtr<AValdorsoLupo>> Lupi;
		bool bFuori = false;
		double Ritorno = 0.0;
	};

	void Prepara();
	int32 Branco(FTana& Tana, const FVector& Dove);
	bool Terra(const FVector& Sopra, FVector& OutTerra) const;

	TArray<FTana> Tane;
	/** I lupi chiamati con la console (non hanno tana). */
	TArray<TWeakObjectPtr<AValdorsoLupo>> Chiamati;
	bool bPronte = false;
	int32 Volte = 0;
	double ProssimoControllo = 0.0;
};
