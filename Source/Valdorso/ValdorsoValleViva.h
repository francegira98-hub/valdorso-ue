// Valdorso - La valle che reagisce (v0.1.3, dal Design: "uccelli che si alzano quando si corre vicino, rane che tacciono
// al passaggio, pesci che saltano al tramonto"). Scritto da Claude il 06/11/2026.
//
// Come funziona (solo su ogni PC, per il colono di quel PC: è aspetto, non cambia il mondo; il server dedicato non
// fa niente):
// - Gli uccellini: di giorno, senza pioggia, ogni tanto uno stormo (6-11 passeri) si posa sul prato a 18-32 m dal
//   colono, davanti a dove guarda, e becchetta. Se il colono si avvicina (da 11 m se corre, da 6 se cammina, da 3 se
//   è fermo) si alzano tutti insieme con un frullo d'ali, scappano dall'altra parte e spariscono in alto. Anche al
//   tramonto se ne vanno. Uno stormo alla volta, con tanti uccelli in una sola mesh (Instanced Static Mesh): le ali le
//   batte il materiale M_Uccello.
// - I pesci: all'alba (5:30-7:30) e al tramonto (17:30-20:30), se non piove forte, ogni 4-12 secondi un pesce salta
//   fuori dall'acqua (lago, pozza, fiume) a 8-35 m dal colono, dove l'acqua è profonda, e ricade con uno schiaffo.
// - Le rane che tacciono: le fa AValdorsoAmbiente (la riva) e AValdorsoSorgenteSuono (la palude).
// Le cose (SM_Uccello, SM_Pesce, M_Uccello, M_Pesce, S_Ali, S_Tuffo) le fanno Tools/Generatori/valle_viva.py e
// Content/Python/nuovi/crea_valle_viva.py in /Game/Valle/Viva.
// Comando di console: Valdorso.ValleViva 0 la spegne (1 di serie).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ValdorsoValleViva.generated.h"

class UAudioComponent;
class UInstancedStaticMeshComponent;
class UStaticMeshComponent;
class AValdorsoAcque;
class AValdorsoCielo;
class AValdorsoTempo;

namespace ValdorsoValleViva
{
	/** Da quanto lontano (cm) uno stormo si spaventa, secondo quanto va veloce il colono (cm/s). */
	VALDORSO_API float DistanzaSpavento(float Velocita);

	/** Vero all'alba e al tramonto, quando i pesci saltano. */
	VALDORSO_API bool OraDeiPesci(float Ora);

	/** Dove sta il pesce a metà salto: S da 0 (esce) a 1 (rientra), un arco alto Altezza e lungo Distanza. */
	VALDORSO_API FVector Salto(const FVector& Inizio, const FVector& Direzione, float Distanza, float Altezza, float S);
}

UCLASS()
class VALDORSO_API AValdorsoValleViva : public AActor
{
	GENERATED_BODY()

public:
	AValdorsoValleViva();

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;

private:
	struct FUccello
	{
		FVector Posto = FVector::ZeroVector;
		FVector Velocita = FVector::ZeroVector;
		float Yaw = 0.f;
		float Fase = 0.f;
		double ProssimoGesto = 0.0;
	};

	struct FPesce
	{
		FVector Inizio = FVector::ZeroVector;
		FVector Direzione = FVector::ForwardVector;
		float Distanza = 120.f;
		float Altezza = 70.f;
		double Partito = -1.0;
		float Durata = 0.8f;
	};

	UPROPERTY(VisibleAnywhere, Category = "Valle viva")
	TObjectPtr<UInstancedStaticMeshComponent> Stormo;

	UPROPERTY(VisibleAnywhere, Category = "Valle viva")
	TArray<TObjectPtr<UStaticMeshComponent>> Pesci;

	UPROPERTY(VisibleAnywhere, Category = "Valle viva")
	TObjectPtr<UAudioComponent> VoceAli;

	UPROPERTY(VisibleAnywhere, Category = "Valle viva")
	TArray<TObjectPtr<UAudioComponent>> VociTuffo;

	TArray<FUccello> Uccelli;
	TArray<FPesce> Salti;
	FVector CentroStormo = FVector::ZeroVector;
	bool bStormoVola = false;
	double StormoPartito = 0.0;
	double ProssimoStormo = 5.0;
	double ProssimoPesce = 0.0;
	double ProssimaRicerca = 0.0;
	int32 ProssimaVoceTuffo = 0;

	TWeakObjectPtr<AValdorsoCielo> Cielo;
	TWeakObjectPtr<AValdorsoTempo> Tempo;
	TWeakObjectPtr<AValdorsoAcque> Acque;

	bool PosaStormo(const FVector& Colono, const FRotator& Sguardo);
	void TogliStormo(double Adesso);
	void AlzaStormo(const FVector& DaDove, double Adesso);
	void AggiornaStormo(float DeltaSeconds, double Adesso);
	void FaiSaltarePesce(const FVector& Colono, double Adesso);
	void AggiornaPesci(double Adesso);
	void Tuffo(const FVector& Dove, float Volume);
	/** Il suolo sotto un punto (falso se non c'è, se è acqua o se è troppo ripido). */
	bool Suolo(const FVector& Punto, FVector& OutSuolo) const;
};
