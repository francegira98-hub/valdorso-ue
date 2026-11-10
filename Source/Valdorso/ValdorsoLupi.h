// Valdorso - Gli animali della valle (v0.1.4: i lupi il 09/11, tutti gli altri il 10/11, provvisori). Scritto da Claude.
// Dalla Roadmap ("I pericoli della valle") e dal Design (gli animali non sono mostri: hanno paura del fuoco e del
// battito). Il file si chiama ancora ValdorsoLupi perche' e' nato con i lupi.
//
// Come funziona:
// - Sei specie (ValdorsoAnimali::Specie): Lupo (predatore, di notte e con la nebbia, in branco), Cinghiale e Orso
//   (difendono il loro posto: se ti avvicini ringhiano, avanzano e poi caricano; l'orso e' raro e fortissimo), Cervo,
//   Lepre e Volpe (fuggono appena ti sentono). Ognuna con la sua salute, i suoi colpi, le sue velocita', i suoi suoni.
// - AValdorsoAnimale: un ACharacter con le statistiche (GAS), mosso dal server. Gli stati: Vaga (intorno alla tana),
//   Fiuta (il lupo si avvicina piano), Accerchia (il lupo gira intorno; cinghiale e orso avvertono e avanzano), Morde
//   (corre e colpisce: ValdorsoCombattimento::Colpisci), Fuggi (le prede sempre; gli altri feriti). Chi sta vicino a un
//   fuoco acceso o nella pace del battito non viene attaccato. Chi lo colpisce diventa la sua preda (o il suo nemico).
// - Morto: si accascia e resta a terra 5 minuti. Niente bottino da solo: E sull'animale a terra, "Scuoia il lupo",
//   con un coltello (o un pugnale) nello zaino; pelle, zanne, artigli, palchi e carne a chi lo scuoia.
// - AValdorsoTane (una per mondo, solo sul server, la fa nascere la modalita' di gioco): le tane nei luoghi della valle
//   (ValdorsoAnimali::Tane). Un gruppo esce quando e' la sua ora (di notte, di giorno o sempre) e un colono e' entro
//   120 m; torna alla tana (sparisce) quando non e' piu' la sua ora o non c'e' piu' nessuno, se non sta cacciando.
//   Un gruppo ucciso tutto torna dopo i suoi minuti (il lupo 20, l'orso 60).
// - L'aspetto (provvisorio, Tools/Generatori/animali.py -> /Game/Valle/Pericoli/Animali): corpo, testa, coda e quattro
//   zampe; ogni PC li muove da se'. Dove vanno i pezzi: ValdorsoAnimaliForme.inl (scritto dal generatore).
// Comandi di console (sul server, o da soli; dalla console dell'editor durante il Play vanno al server da soli):
//   Valdorso.Animali.Chiama <Specie> (un gruppo a 25 m da te, anche fuori dalla sua ora), Valdorso.Animali.Via (tutti
//   tornano alla tana), Valdorso.Lupi.Chiama (= Chiama Lupo), Valdorso.Lupi.Via.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "ValdorsoInterazione.h"
#include "ValdorsoLupi.generated.h"

class UAbilitySystemComponent;
class UStaticMeshComponent;
class UValdorsoAttributeSet;
class UValdorsoReazioniComponent;
struct FValdorsoZaino;

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

/** Come si comporta una specie con chi le si avvicina. */
enum class EValdorsoIndole : uint8
{
	/** Caccia: fiuta, accerchia e morde (il lupo). */
	Predatore,
	/** Difende il suo posto: avverte, avanza e carica chi gli si avvicina troppo (cinghiale, orso). */
	Territoriale,
	/** Fugge appena sente qualcuno (cervo, lepre, volpe). */
	Preda,
};

/** Quando esce dalla tana. */
enum class EValdorsoOraTana : uint8
{
	Notte,
	Giorno,
	Sempre,
};

/** Una specie: le regole, i suoni e quello che si ricava a scuoiarla. */
struct FValdorsoSpecie
{
	const TCHAR* Id = TEXT("");
	/** Il nome con l'articolo, per gli avvisi ("il lupo", "l'orso"). */
	const TCHAR* ConArticolo = TEXT("");
	EValdorsoIndole Indole = EValdorsoIndole::Preda;
	float Salute = 50.f;
	float Danno = 0.f;
	float Equilibrio = 0.f;
	float SecondiMorso = 1.5f;
	/** Entro quanto sente un colono e oltre quanto lo lascia (cm). */
	float Fiuto = 2000.f;
	float Lascia = 4000.f;
	/** Quanto lontano gira intorno (il lupo) o da quanto vicino avverte (cinghiale, orso), e per quanti secondi. */
	float Giro = 650.f;
	float SecondiAvviso = 5.f;
	/** Da quanto vicino colpisce (cm, da centro a centro). */
	float Portata = 170.f;
	/** Velocita': a spasso e di corsa (cm/s). */
	float Passo = 150.f;
	float Corsa = 600.f;
	/** Sotto questa parte della salute fugge (gli animali che attaccano). */
	float Paura = 0.3f;
	/** I suoni: quando avverte o accerchia, quando e' ferito o muore, il richiamo (ogni tanto, mentre vaga). */
	const TCHAR* SuonoAvviso = nullptr;
	const TCHAR* SuonoDolore = nullptr;
	const TCHAR* SuonoRichiamo = nullptr;
	bool bRichiamoDiNotte = true;
	/** Quello che si ricava a scuoiarlo (fino a tre oggetti). */
	const TCHAR* Bottino[3] = { nullptr, nullptr, nullptr };
	int32 Quanti[3] = { 0, 0, 0 };
};

/** Dove si attaccano i pezzi di una specie (cm, da terra): ValdorsoAnimaliForme.inl, scritto da animali.py. */
struct FValdorsoForma
{
	const TCHAR* Id;
	float CorpoZ;
	float TestaX;
	float TestaZ;
	float CodaX;
	float CodaZ;
	float ZampeAvantiX;
	float ZampeDietroX;
	float ZampeY;
	float ZampaLunga;
	float MezzoLargo;
	float MezzoLungo;
};

/** Una tana: dove, chi, quanti, quando e dopo quanto torna un gruppo ucciso. */
struct FValdorsoTana
{
	const TCHAR* Luogo;
	const TCHAR* Specie;
	int32 Minimo;
	int32 Massimo;
	EValdorsoOraTana Ora;
	float MinutiRitorno;
};

namespace ValdorsoAnimali
{
	VALDORSO_API TArrayView<const FValdorsoSpecie> Tutte();
	VALDORSO_API const FValdorsoSpecie* Specie(FName Id);
	VALDORSO_API const FValdorsoForma* Forma(FName Id);
	VALDORSO_API TArrayView<const FValdorsoTana> Tane();

	/** Vero se a quell'ora esce (di notte: poca luce o nebbia). */
	VALDORSO_API bool Fuori(EValdorsoOraTana Ora, float LuceDelGiorno, bool bNebbia);

	/** Lo stato che viene dopo, per quella specie (le regole, senza il mondo). Distanza negativa: nessuno vicino. */
	VALDORSO_API EValdorsoStatoLupo Prossimo(const FValdorsoSpecie& Specie, EValdorsoStatoLupo Attuale, float SaluteParte,
		float Distanza, bool bProtetta, float Secondi);

	/** Quanto e' veloce in quello stato (cm/s). */
	VALDORSO_API float Velocita(const FValdorsoSpecie& Specie, EValdorsoStatoLupo Stato);

	/** Quanti nel gruppo, dal numero della volta (tra il minimo e il massimo della tana). */
	VALDORSO_API int32 QuantiNelGruppo(const FValdorsoTana& Tana, int32 Volta);

	/** Si puo' scuoiare con quello che c'e' nello zaino? (un coltello o un pugnale) */
	VALDORSO_API bool HaLama(const FValdorsoZaino& Zaino);
}

/** I lupi del 09/11: le stesse regole, per i test e per chi le chiama ancora cosi'. */
namespace ValdorsoLupi
{
	inline constexpr float SaluteLupo = 60.f;
	inline constexpr float DannoMorso = 8.f;
	inline constexpr float EquilibrioMorso = 25.f;
	inline constexpr float SecondiMorso = 1.5f;
	inline constexpr float PortataMorso = 170.f;
	inline constexpr float Fiuto = 3000.f;
	inline constexpr float Lascia = 4500.f;
	inline constexpr float GiroCaccia = 650.f;
	inline constexpr float GiroAlLargo = 900.f;
	inline constexpr float PauraSotto = 0.3f;
	inline constexpr float SecondiAccerchia = 5.f;
	inline constexpr float SecondiFuga = 10.f;
	inline constexpr float SecondiRitorno = 20.f * 60.f;
	inline constexpr float VicinoTana = 12000.f;
	inline constexpr float LontanoTana = 20000.f;
	/** Per quanto resta a terra un animale morto (secondi). */
	inline constexpr float SecondiATerra = 300.f;

	VALDORSO_API bool Fuori(float LuceDelGiorno, bool bNebbia);
	VALDORSO_API EValdorsoStatoLupo Prossimo(EValdorsoStatoLupo Attuale, float SaluteParte, float Distanza, bool bProtetta, float Secondi);
	VALDORSO_API int32 QuantiNelBranco(int32 Volta);
	VALDORSO_API float Velocita(EValdorsoStatoLupo Stato);
}

UCLASS()
class VALDORSO_API AValdorsoAnimale : public ACharacter, public IAbilitySystemInterface, public IValdorsoInterattivo
{
	GENERATED_BODY()

public:
	AValdorsoAnimale();

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override { return Abilita; }

	/** Solo sul server, prima di FinishSpawning: che animale e', e da dove viene (la tana: ci vaga intorno). */
	void Prepara(FName QualeSpecie, const FVector& DoveTana);

	/** Solo sul server: qualcuno l'ha colpito (diventa la sua preda, o lo fa fuggire). */
	void Ferito(AActor* Da);

	const FValdorsoSpecie* GetSpecie() const;
	EValdorsoStatoLupo GetStato() const { return Stato; }
	bool Vivo() const { return Stato != EValdorsoStatoLupo::Morto; }
	/** Sta cacciando (accerchia o morde): non torna alla tana adesso. */
	bool Caccia() const { return Stato == EValdorsoStatoLupo::Accerchia || Stato == EValdorsoStatoLupo::Morde; }

	// IValdorsoInterattivo: scuoiare l'animale a terra
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
	void Cambia(EValdorsoStatoLupo Nuovo);
	void Muori();
	void SuonaStato();
	APawn* CercaPreda() const;

	UFUNCTION()
	void OnRep_Stato();

	UFUNCTION()
	void OnRep_Specie();

	UFUNCTION()
	void OnRep_Scuoiato();

	UPROPERTY(VisibleAnywhere, Category = "Animale")
	TObjectPtr<UAbilitySystemComponent> Abilita;

	UPROPERTY()
	TObjectPtr<UValdorsoAttributeSet> Attributi;

	UPROPERTY(VisibleAnywhere, Category = "Animale")
	TObjectPtr<UValdorsoReazioniComponent> Reazioni;

	/** L'animale disegnato: a terra (la capsula e' sollevata), si accascia quando muore. */
	UPROPERTY(VisibleAnywhere, Category = "Animale")
	TObjectPtr<USceneComponent> Figura;

	UPROPERTY(VisibleAnywhere, Category = "Animale")
	TObjectPtr<UStaticMeshComponent> Corpo;

	UPROPERTY(VisibleAnywhere, Category = "Animale")
	TObjectPtr<UStaticMeshComponent> Testa;

	UPROPERTY(VisibleAnywhere, Category = "Animale")
	TObjectPtr<UStaticMeshComponent> Coda;

	UPROPERTY(VisibleAnywhere, Category = "Animale")
	TArray<TObjectPtr<UStaticMeshComponent>> Zampe;

	UPROPERTY(ReplicatedUsing = OnRep_Specie)
	FName Specie = TEXT("Lupo");

	UPROPERTY(ReplicatedUsing = OnRep_Stato)
	EValdorsoStatoLupo Stato = EValdorsoStatoLupo::Vaga;

	UPROPERTY(ReplicatedUsing = OnRep_Scuoiato)
	bool bScuoiato = false;

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
	double ProssimoRichiamo = 0.0;
	FName Vestito;
};

/** Le tane degli animali (una per mondo, solo sul server): fa uscire i gruppi e li richiama. */
UCLASS()
class VALDORSO_API AValdorsoTane : public AActor
{
	GENERATED_BODY()

public:
	AValdorsoTane();

	static AValdorsoTane* Di(const UWorld* Mondo);

	/** Per le prove: un gruppo di quella specie vicino a quel punto, adesso. Quanti ne sono usciti. */
	int32 Chiama(FName QualeSpecie, const FVector& Vicino);

	/** Tutti gli animali tornano alla tana (spariscono). */
	void Via();

	virtual void Tick(float DeltaSeconds) override;

private:
	struct FTanaViva
	{
		const FValdorsoTana* Regola = nullptr;
		FVector Centro = FVector::ZeroVector;
		TArray<TWeakObjectPtr<AValdorsoAnimale>> Animali;
		bool bFuori = false;
		double Ritorno = 0.0;
	};

	void Prepara();
	int32 Gruppo(FName QualeSpecie, int32 Quanti, const FVector& Dove, TArray<TWeakObjectPtr<AValdorsoAnimale>>& Dentro);
	bool Terra(const FVector& Sopra, FVector& OutTerra) const;

	TArray<FTanaViva> Vive;
	/** Gli animali chiamati con la console (non hanno tana). */
	TArray<TWeakObjectPtr<AValdorsoAnimale>> Chiamati;
	bool bPronte = false;
	int32 Volte = 0;
	double ProssimoControllo = 0.0;
};
