// Valdorso - Il nuoto (v0.1.3): dove c'è l'acqua e quanto è alta. Scritto da Claude il 06/11/2026, preparato mentre
// Fra dormiva.
//
// Perché serve: il movimento di Unreal fa nuotare solo dentro un "Physics Volume" con "Water Volume" acceso, e l'acqua
// del plugin Water (lago, fiumi) non lo è. Invece di mettere a mano centinaia di volumi lungo i fiumi, il livello ha
// un solo attore, AValdorsoAcque, con la forma dell'acqua presa dagli stessi dati che hanno fatto lago e fiumi
// (Acqua_3000.json, scritto da Tools/Generatori/acqua.py): le rive del lago e della pozza della cascata con il loro
// pelo, e i corsi d'acqua (fiume, emissario, torrente) punto per punto, con la quota del pelo e la larghezza.
// Lo mette nel livello Content/Python/nuovi/crea_acque_nuoto.py.
//
// UValdorsoMovementComponent gli chiede "a che quota è il pelo dell'acqua qui?" e decide da sé, sul server e sul PC
// allo stesso modo (così il nuoto si prevede come il camminare): con l'acqua al petto si nuota, quando si torna dove
// si tocca si cammina. Il nuoto della v0.1.3 è in superficie; sott'acqua (fiato, corrente, fondale) è della v0.1.4.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ValdorsoAcque.generated.h"

/** Uno specchio d'acqua fermo (il lago, la pozza della cascata): la riva e il pelo, nel mondo. */
USTRUCT(BlueprintType)
struct FValdorsoSpecchioAcqua
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acqua")
	FString Nome;

	/** La riva (punti nel mondo, solo X e Y), in giro. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acqua")
	TArray<FVector2D> Riva;

	/** Il pelo dell'acqua (Z nel mondo, cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acqua")
	float Pelo = 0.f;
};

/** Un corso d'acqua (fiume, emissario, torrente): i punti della linea di mezzo con il pelo (Z) e la larghezza. */
USTRUCT(BlueprintType)
struct FValdorsoCorsoAcqua
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acqua")
	FString Nome;

	/** I punti nel mondo: X e Y sulla linea di mezzo, Z = il pelo dell'acqua lì. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acqua")
	TArray<FVector> Punti;

	/** La larghezza dell'acqua in ogni punto (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acqua")
	TArray<float> Larghezze;
};

namespace ValdorsoAcqua
{
	/** Vero se il punto è dentro il poligono (pari e dispari). */
	VALDORSO_API bool DentroRiva(const TArray<FVector2D>& Riva, const FVector2D& Punto);

	/** Il pelo di un corso d'acqua in quel punto, se il punto è sull'acqua (entro mezza larghezza + Margine). */
	VALDORSO_API bool PeloSulCorso(const FValdorsoCorsoAcqua& Corso, const FVector2D& Punto, float Margine, float& OutPelo);

	/** (10/11, v0.1.4) Dove va la corrente di un corso d'acqua in quel punto (verso il pelo piu' basso del tratto piu'
	 *  vicino; a pari pelo, nell'ordine dei punti): vero se il punto e' sull'acqua del corso. OutVerso e' lungo 1. */
	VALDORSO_API bool CorrenteSulCorso(const FValdorsoCorsoAcqua& Corso, const FVector2D& Punto, float Margine, FVector2D& OutVerso);

	/** Quanto del corpo è sott'acqua (0 = niente, 1 = tutto): dal fondo della capsula al pelo, diviso l'altezza. */
	VALDORSO_API float Immersione(float Pelo, float ZCentro, float MezzaAltezza);
}

UCLASS()
class VALDORSO_API AValdorsoAcque : public AActor
{
	GENERATED_BODY()

public:
	AValdorsoAcque();

	UPROPERTY(EditAnywhere, Category = "Acqua")
	TArray<FValdorsoSpecchioAcqua> Specchi;

	UPROPERTY(EditAnywhere, Category = "Acqua")
	TArray<FValdorsoCorsoAcqua> Corsi;

	/** Quanto oltre la riva vale ancora l'acqua (cm): le rive dei dati sono a punti, l'acqua vera arriva un po' più in là
	 *  (dove il fondo è sopra il pelo, comunque, non si nuota). */
	UPROPERTY(EditAnywhere, Category = "Acqua", meta = (ClampMin = "0"))
	float Margine = 300.f;

	/** Il pelo dell'acqua in quel punto (X, Y nel mondo): vero se lì c'è acqua. Se ce n'è più d'una, la più alta. */
	bool PeloQui(const FVector2D& Punto, float& OutPelo) const;

	/** (10/11, v0.1.4) Quanto veloce va la corrente dei corsi d'acqua (cm/s): chi nuota ci viene portato. Nel lago e
	 *  nella pozza niente corrente. */
	UPROPERTY(EditAnywhere, Category = "Acqua", meta = (ClampMin = "0"))
	float VelocitaCorrente = 90.f;

	/** (10/11) La corrente in quel punto (cm/s, in piano): zero fuori dai corsi e negli specchi d'acqua. */
	FVector2D CorrenteQui(const FVector2D& Punto) const;

	/** L'attore delle acque del mondo (nullptr se il livello non ne ha). */
	static AValdorsoAcque* Di(const UWorld* Mondo);

protected:
	virtual void BeginPlay() override;

private:
	/** Le scatole intorno a ogni specchio e corso, per scartare in fretta (fatte al primo uso). */
	void PreparaScatole() const;
	mutable TArray<FBox2D> ScatoleSpecchi;
	mutable TArray<FBox2D> ScatoleCorsi;
	mutable bool bScatolePronte = false;
};
