// Valdorso - Il tempo della valle (v0.1.3): sereno, nuvoloso, nebbia, pioggia, temporale. Scritto da Claude il
// 06/11/2026, preparato mentre Fra dormiva.
//
// Come funziona:
// - Il tempo lo decide il server, uguale per tutti, e lo manda in rete (come l'ora del cielo): ogni tanto (di serie
//   ogni 15-40 minuti veri) sceglie il tempo dopo, con un poco di logica (dopo la pioggia viene più spesso il
//   nuvoloso, la nebbia viene soprattutto all'alba). Il cambio dura un minuto: le cose sfumano, non scattano.
// - Ogni PC (mai il server dedicato) lo fa vedere: la nebbia del livello (Exponential Height Fog) si infittisce, il sole
//   si abbassa con le nuvole (lo legge AValdorsoCielo), e con la pioggia le tende di gocce seguono la telecamera
//   (M_Pioggia, fatto da crea_tempo.py con T_Pioggia di Tools/Generatori/tempo.py) e si sente la pioggia (S_Pioggia).
//   Sotto un tetto la pioggia non si vede e si sente più piano.
// - (06/11, dopo la prima prova di Fra) Con la pioggia il cielo si fa cupo: il sole quasi sparisce (niente ombre nette),
//   l'esposizione scende e i colori si spengono (un post-process senza confini, pesato da Cupo). Le tende di gocce sono
//   sei, da 1,5 m a 26 m dalla telecamera: piove anche tra la telecamera e il colono (prima c'era una bolla di 6 m).
//   La nebbia è calcolata all'altezza della telecamera (la nebbia del livello cala con l'altezza e nella valle non si
//   vedeva): si sceglie quanto deve essere fitta dove si è, e si regola il livello perché lo sia. Di notte la nebbia fitta
//   si scurisce (non brilla). Se il livello non ha la nebbia, la mette il tempo.
// - La modalità di gioco lo fa nascere da sola se il livello non ne ha uno.
// Più avanti: il terreno bagnato (Material Parameter Collection, già deciso), i lampi e i tuoni, la neve d'inverno.
// Comando di console (server): Valdorso.Tempo Pioggia (oppure Sereno, Nuvoloso, Nebbia, Temporale, o 0-4); senza
// niente dice il tempo di adesso.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ValdorsoTempo.generated.h"

class AValdorsoCielo;
class UAudioComponent;
class UPostProcessComponent;
class UExponentialHeightFogComponent;
class UMaterialInstanceDynamic;
class UStaticMeshComponent;

UENUM()
enum class EValdorsoTempo : uint8
{
	Sereno,
	Nuvoloso,
	Nebbia,
	Pioggia,
	Temporale
};

/** Quanto c'è di ogni cosa in un tempo (0..1). */
struct FValdorsoAria
{
	float Nuvole = 0.f;
	float Nebbia = 0.f;
	float Pioggia = 0.f;
};

namespace ValdorsoTempo
{
	/** Le quantità di un tempo. */
	VALDORSO_API FValdorsoAria Aria(EValdorsoTempo Tempo);

	/** Da un tempo all'altro: Alfa 0 = il primo, 1 = il secondo. */
	VALDORSO_API FValdorsoAria Mescola(const FValdorsoAria& A, const FValdorsoAria& B, float Alfa);

	/** Il tempo dopo, da un numero a caso tra 0 e 1 (Caso) e dall'ora (la nebbia viene di più all'alba). */
	VALDORSO_API EValdorsoTempo Prossimo(EValdorsoTempo Adesso, float Caso, float Ora);

	/** Quanta luce del sole passa (1 = sereno, 0,05 = pioggia: niente ombre nette). */
	VALDORSO_API float LuceSole(const FValdorsoAria& Aria);

	/** Quanto è cupo il cielo (0..1): pesa l'esposizione più bassa e i colori spenti. */
	VALDORSO_API float Cupo(const FValdorsoAria& Aria);

	/** Quanto deve essere fitta la nebbia dove si trova la telecamera (densità di Unreal), da quella che c'è di serie. */
	VALDORSO_API float NebbiaVicina(float DiSerie, const FValdorsoAria& Aria);

	/** Il valore da dare alla nebbia del livello (FogDensity) perché all'altezza della telecamera sia fitta Vicina:
	 *  Caduta è FogHeightFalloff, SopraNebbia quanti cm la telecamera sta sopra l'attore della nebbia. */
	VALDORSO_API float DensitaPerAltezza(float Vicina, float Caduta, float SopraNebbia);

	/** Il nome del tempo ("Pioggia") e il contrario (false se il nome non c'è). */
	VALDORSO_API FString Nome(EValdorsoTempo Tempo);
	VALDORSO_API bool DaNome(const FString& Testo, EValdorsoTempo& OutTempo);
}

UCLASS()
class VALDORSO_API AValdorsoTempo : public AActor
{
	GENERATED_BODY()

public:
	AValdorsoTempo();

	/** Ogni quanto cambia (minuti veri, a caso tra questi due). */
	UPROPERTY(EditAnywhere, Category = "Tempo", meta = (ClampMin = "1"))
	float MinutiMinimi = 15.f;

	UPROPERTY(EditAnywhere, Category = "Tempo", meta = (ClampMin = "1"))
	float MinutiMassimi = 40.f;

	/** Quanto dura il passaggio da un tempo all'altro (secondi). */
	UPROPERTY(EditAnywhere, Category = "Tempo", meta = (ClampMin = "1"))
	float SecondiCambio = 60.f;

	/** Il tempo con cui parte il server. */
	UPROPERTY(EditAnywhere, Category = "Tempo")
	EValdorsoTempo TempoIniziale = EValdorsoTempo::Sereno;

	/** Il tempo di adesso (quello verso cui sta andando). */
	EValdorsoTempo GetTempo() const { return Tempo; }

	/** Le quantità di adesso, già sfumate (uguali su server e PC). */
	FValdorsoAria AriaAdesso() const;

	/** Quanta luce del sole passa (1 = sereno): la usa AValdorsoCielo. */
	float LuceSole() const;

	/** Solo sul server: cambia il tempo (sfuma in SecondiCambio). */
	void ImpostaTempo(EValdorsoTempo Nuovo);

	/** Il tempo del mondo (nullptr se non c'è). */
	static AValdorsoTempo* Di(const UWorld* Mondo);

	virtual void Tick(float DeltaSeconds) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(Replicated)
	EValdorsoTempo Tempo = EValdorsoTempo::Sereno;

	UPROPERTY(Replicated)
	EValdorsoTempo TempoPrima = EValdorsoTempo::Sereno;

	/** Quando è cominciato il cambio (orologio del server). */
	UPROPERTY(Replicated)
	double InizioCambio = -1000.0;

	double TempoServer() const;

	/** Solo sul server: quando cambiare la prossima volta. */
	double ProssimoCambio = 0.0;

	/** Su questo PC: la nebbia del livello e la sua densità di partenza. */
	TWeakObjectPtr<UExponentialHeightFogComponent> Nebbia;
	float DensitaNebbia = 0.02f;
	double ProssimaRicercaNebbia = 0.0;

	/** Le tende di gocce intorno alla telecamera, dalla più vicina alla più lontana. */
	UPROPERTY(VisibleAnywhere, Category = "Tempo")
	TArray<TObjectPtr<UStaticMeshComponent>> Tende;

	UPROPERTY(VisibleAnywhere, Category = "Tempo")
	TObjectPtr<UAudioComponent> SuonoPioggia;

	/** Il cielo cupo: esposizione più bassa e colori spenti (senza confini, pesato da Cupo). */
	UPROPERTY(VisibleAnywhere, Category = "Tempo")
	TObjectPtr<UPostProcessComponent> Cupezza;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> MaterialiTende;

	/** La nebbia del livello com'era: caduta con l'altezza e colore. */
	float CadutaNebbia = 0.2f;
	FLinearColor ColoreNebbia = FLinearColor(0.45f, 0.5f, 0.6f);
	bool bNebbiaMessa = false;

	/** Il cielo (per sapere se è giorno: di notte la nebbia fitta si scurisce). */
	TWeakObjectPtr<AValdorsoCielo> Cielo;
	double ProssimaRicercaCielo = 0.0;

	/** Sotto un tetto (0 fuori, 1 dentro), sfumato. */
	float AlRiparo = 0.f;
	double ProssimoControlloTetto = 0.0;
	bool bTettoSopra = false;
};
