// Valdorso - La freccia (v0.2.3 "Armi vere", 14/11/2026). Scritto da Claude.
//
// Con l'arco in mano (ValdorsoOggetti.h, "Arco") il tasto del colpo tende l'arco e tira una freccia dallo zaino
// (UValdorsoAbilitaPugno: senza frecce non si tira). La freccia la fa nascere il server: vola dritta dove guarda la
// telecamera del colono, scende un poco con la distanza (la gravita'), e quello che tocca riceve il colpo come gli altri
// (ValdorsoCombattimento::Colpisci: la pace del battito, la scherma, lo scudo, il crescere dell'abilita' "Arco").
// Il danno e' quello dell'arco per il suo grado (ValdorsoQualita.h). Poi resta piantata dove ha colpito, per un po'.
// Il modello e' /Game/Valle/Pericoli/Armi/SM_Arma_Freccia (Tools/Generatori/pericoli.py); se manca, un'asta grigia.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ValdorsoFreccia.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UProjectileMovementComponent;
class APawn;
struct FHitResult;

namespace ValdorsoFreccia
{
	/** Per quanto resta piantata dopo aver colpito (secondi). */
	inline constexpr float SecondiPiantata = 20.f;
	/** Quanto vola al massimo prima di sparire (secondi). */
	inline constexpr float SecondiVolo = 6.f;
	/** Quanto la tira giu' la gravita' (1 = come un sasso). */
	inline constexpr float Gravita = 0.35f;
}

UCLASS()
class VALDORSO_API AValdorsoFreccia : public AActor
{
	GENERATED_BODY()

public:
	AValdorsoFreccia();

	/** Solo sul server: tira una freccia da Da verso Verso (alla velocita' ValdorsoArmi::VelocitaFreccia), per conto di
	 *  Chi. nullptr se non nasce. */
	static AValdorsoFreccia* Scocca(APawn* Chi, const FVector& Da, const FVector& Verso, float InDanno, float InEquilibrio);

protected:
	virtual void BeginPlay() override;

private:
	/** La freccia si ferma: ha toccato qualcosa (su ogni PC; il colpo lo da' solo il server). */
	UFUNCTION()
	void QuandoSiFerma(const FHitResult& Colpo);

	UPROPERTY(VisibleAnywhere, Category = "Valdorso")
	TObjectPtr<USphereComponent> Punta;

	UPROPERTY(VisibleAnywhere, Category = "Valdorso")
	TObjectPtr<UStaticMeshComponent> Asta;

	UPROPERTY(VisibleAnywhere, Category = "Valdorso")
	TObjectPtr<UProjectileMovementComponent> Volo;

	/** Sul server: quanto fa male e quanto equilibrio toglie. */
	float Danno = 0.f;
	float Equilibrio = 0.f;
	bool bArrivata = false;
};
