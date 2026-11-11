// Valdorso - Lo stato del colono che vedono tutti (v0.1.5 "Crescere", 11/11/2026). Scritto da Claude. Dal Punto di
// ripresa del 10/11: "Gli altri coloni per ora non si valutano dal PC (serve un PlayerState con la forza replicata)".
//
// AValdorsoPlayerState: il PlayerState di Valdorso (lo mette AValdorsoGameMode). Porta a tutti i PC la forza del colono
// (0..100: l'arma migliore e la tempra, ValdorsoCrescere::Forza), cosi' agganciando un altro colono sotto il rombo si legge
// "Alla tua pari", "Piu' forte di te"... come per gli animali e i PNG. La aggiorna il server quando le abilita' cambiano
// (UValdorsoCrescitaComponent). Le abilita' vere restano segrete: arriva solo il numero, arrotondato.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "ValdorsoStatoColono.generated.h"

UCLASS()
class VALDORSO_API AValdorsoPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	AValdorsoPlayerState();

	/** Vero quando il server ha gia' dato la forza (un colono in gioco). */
	bool ForzaNota() const { return bForzaNota; }

	/** La forza (0..100), arrotondata a 1. */
	float GetForza() const { return static_cast<float>(Forza); }

	/** Solo sul server. */
	void ImpostaForza(float Nuova);

	/** La forza del colono di quel corpo, se si sa (sul server e su ogni PC). */
	static bool ForzaDi(const AActor* Corpo, float& OutForza);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	UPROPERTY(Replicated)
	uint8 Forza = 0;

	UPROPERTY(Replicated)
	bool bForzaNota = false;
};
