// Valdorso - I suoni della valle, l'ambiente (v0.1.3). Scritto da Claude il 06/11/2026.
//
// Come funziona (tutto solo su ogni PC: il server dedicato non suona):
// - AValdorsoAmbiente (uno per mondo, lo fa nascere la modalità di gioco come il tempo) suona intorno al colono:
//   il vento (più forte con le nuvole e la pioggia), gli uccelli di giorno, i grilli di notte (zitti con la pioggia);
//   e tre suoni che si spostano da soli nel punto d'acqua più vicino: il fiume (sulla linea del corso più vicino),
//   la riva del lago e le rane (sulla riva, di sera e di notte). La forma dell'acqua la prende da AValdorsoAcque.
// - AValdorsoSorgenteSuono è un suono fermo in un posto (la cascata, la fucina di Marta, la locanda di Bruno, il
//   mulino, le rane della palude): quando si sente (sempre, di giorno, di notte, in orario di lavoro, di sera) e
//   fin dove. Le mette nella valle Content/Python/nuovi/crea_suoni_valle.py.
// - I suoni li fa Tools/Generatori/suoni_valle.py (da zero, senza licenze); crea_suoni_valle.py li importa in /Game/Valle/Suoni.
// - Le campane del tempio sono in ValdorsoCampane.h.
// Comando di console: Valdorso.Ambiente.Volume 1.0 (tutti i suoni della valle insieme; 0 = spenti).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ValdorsoAmbiente.generated.h"

class UAudioComponent;
class USoundBase;
class AValdorsoCielo;
class AValdorsoTempo;
class AValdorsoAcque;

/** Quando si sente un suono fermo. */
UENUM(BlueprintType)
enum class EValdorsoQuando : uint8
{
	Sempre,
	Giorno,
	Notte,
	/** Dalle 7 alle 19 (la fucina, il mulino che macina). */
	Lavoro,
	/** Sempre, ma di più dalle 18 a mezzanotte (la locanda). */
	Sera
};

namespace ValdorsoAmbiente
{
	/** Il volume (0..1) di un suono fermo: Ora 0..24, Giorno 0..1 (quanto è giorno), Pioggia 0..1. */
	VALDORSO_API float Volume(EValdorsoQuando Quando, float Ora, float Giorno, float Pioggia);

	/** I volumi dei suoni intorno al colono. */
	VALDORSO_API float Vento(float Nuvole, float Pioggia);
	VALDORSO_API float Uccelli(float Ora, float Giorno, float Pioggia);
	VALDORSO_API float Grilli(float Giorno, float Pioggia);
	VALDORSO_API float Rane(float Ora, float Giorno, float Pioggia);

	/** (06/11) Le rane tacciono se il colono passa vicino (Distanza cm) muovendosi (Velocita cm/s). */
	VALDORSO_API bool RaneSpaventate(float Distanza, float Velocita, float Raggio = 1500.f);

	/** Il punto più vicino a P su una linea spezzata (anche la Z viene dalla linea). Falso se la linea è vuota. */
	VALDORSO_API bool PiuVicinoSuLinea(const TArray<FVector>& Linea, const FVector& P, FVector& OutPunto);

	/** Il punto più vicino a P sul giro di una riva (chiusa), a quota Pelo. Falso se la riva è vuota. */
	VALDORSO_API bool PiuVicinoSuRiva(const TArray<FVector2D>& Riva, float Pelo, const FVector& P, FVector& OutPunto);
}

/** Un suono fermo in un posto della valle. */
UCLASS()
class VALDORSO_API AValdorsoSorgenteSuono : public AActor
{
	GENERATED_BODY()

public:
	AValdorsoSorgenteSuono();

	UPROPERTY(EditAnywhere, Category = "Suono")
	TObjectPtr<USoundBase> Suono;

	UPROPERTY(EditAnywhere, Category = "Suono")
	EValdorsoQuando Quando = EValdorsoQuando::Sempre;

	/** Fin dove si sente pieno (cm). */
	UPROPERTY(EditAnywhere, Category = "Suono", meta = (ClampMin = "0"))
	float Vicino = 500.f;

	/** Da Vicino a Vicino + Lontano si spegne (cm). */
	UPROPERTY(EditAnywhere, Category = "Suono", meta = (ClampMin = "100"))
	float Lontano = 3000.f;

	UPROPERTY(EditAnywhere, Category = "Suono", meta = (ClampMin = "0"))
	float Volume = 1.f;

	/** (06/11) Tace quando il colono di questo PC passa vicino (le rane della palude): entro Vicino + 5 m, in movimento,
	 *  e riprende piano dopo una decina di secondi. */
	UPROPERTY(EditAnywhere, Category = "Suono")
	bool bTaceVicino = false;

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(VisibleAnywhere, Category = "Suono")
	TObjectPtr<UAudioComponent> Audio;

	TWeakObjectPtr<AValdorsoCielo> Cielo;
	TWeakObjectPtr<AValdorsoTempo> Tempo;
	float VolumeAdesso = 0.f;
	double TaceFino = 0.0;
};

/** I suoni intorno al colono (vento, uccelli, grilli) e quelli dell'acqua che lo seguono. */
UCLASS()
class VALDORSO_API AValdorsoAmbiente : public AActor
{
	GENERATED_BODY()

public:
	AValdorsoAmbiente();

	virtual void Tick(float DeltaSeconds) override;

	/** Fa un componente audio con il suono e fin dove si sente (Lontano 0 = intorno, senza posto). */
	static UAudioComponent* PreparaAudio(UAudioComponent* Audio, USoundBase* Suono, float Vicino, float Lontano);

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(VisibleAnywhere, Category = "Suoni")
	TObjectPtr<UAudioComponent> Vento;

	UPROPERTY(VisibleAnywhere, Category = "Suoni")
	TObjectPtr<UAudioComponent> Uccelli;

	UPROPERTY(VisibleAnywhere, Category = "Suoni")
	TObjectPtr<UAudioComponent> Grilli;

	UPROPERTY(VisibleAnywhere, Category = "Suoni")
	TObjectPtr<UAudioComponent> Fiume;

	UPROPERTY(VisibleAnywhere, Category = "Suoni")
	TObjectPtr<UAudioComponent> Riva;

	UPROPERTY(VisibleAnywhere, Category = "Suoni")
	TObjectPtr<UAudioComponent> RaneRiva;

	TWeakObjectPtr<AValdorsoCielo> Cielo;
	TWeakObjectPtr<AValdorsoTempo> Tempo;
	TWeakObjectPtr<AValdorsoAcque> Acque;
	double ProssimaRicerca = 0.0;
	double ProssimoPunto = 0.0;
	FVector PuntoFiume = FVector::ZeroVector;
	FVector PuntoRiva = FVector::ZeroVector;
	bool bFiume = false;
	bool bRiva = false;
	/** (06/11) Le rane della riva tacciono quando il colono passa vicino. */
	double RaneTaccionoFino = 0.0;
	float RaneAdesso = 1.f;
};
