// Valdorso - La ruota dei gesti (v0.1.7, parte C, 11/11/2026). Scritto da Claude; i gesti sono quelli di Mixamo che
// Fra ha scaricato, piu' Martellare e Spazzare provvisori (Content/Python/nuovi/importa_gesti.py).
//
// Tieni premuto B: in mezzo allo schermo si apre una ruota con i gesti (salutare, inchino, pregare, applaudire,
// indicare, spallucce, esultare, sedersi per terra). Muovi il mouse verso quello che vuoi e lascia B: il colono lo fa,
// e lo vedono tutti quelli vicini. Lasciando B al centro non fa niente. Muoversi interrompe il gesto (sedersi per terra
// resta finche' non ti muovi).
// Il server controlla: non da caduti, non da seduti o a letto, al massimo uno ogni secondo e mezzo.
// Le animazioni stanno in /Game/Mixamo/Gesti/<Nome> (Content/Mixamo non va su GitHub): se una manca, quel gesto non si vede.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ValdorsoGesti.generated.h"

class APlayerController;
class UAnimSequence;

/** Un gesto della ruota. */
struct FValdorsoGesto
{
	/** Il nome dell'animazione in /Game/Mixamo/Gesti. */
	const TCHAR* Animazione;
	/** Cosa si legge sulla ruota. */
	const TCHAR* Nome;
	/** Si ripete finche' il colono non si muove (sedersi). */
	bool bResta;
};

namespace ValdorsoGesti
{
	/** I gesti della ruota, in giro dall'alto in senso orario. */
	VALDORSO_API const TArray<FValdorsoGesto>& Tutti();

	/** Quale spicchio della ruota (0..Quanti-1) in quella direzione dal centro (x a destra, y in giu', in pixel), o -1
	 *  se e' troppo vicino al centro. */
	VALDORSO_API int32 Spicchio(const FVector2D& DalCentro, int32 Quanti, float RaggioMorto);

	/** Sul PC: il tasto B premuto (apre la ruota) e lasciato (sceglie). */
	VALDORSO_API void Apri(APlayerController* Controllore);
	VALDORSO_API void Lascia(APlayerController* Controllore, bool bRimettiComandi = true);

	/** Sul PC: la ruota e' aperta. */
	VALDORSO_API bool Aperta();
}

/** Sul corpo del colono: chi fa un gesto lo fa vedere a tutti. */
UCLASS(ClassGroup = (Valdorso))
class VALDORSO_API UValdorsoGestiComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UValdorsoGestiComponent();

	/** Sul PC del colono: chiede al server quel gesto. */
	void Chiedi(int32 Indice);

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	UFUNCTION(Server, Reliable)
	void ServerGesto(uint8 Indice);

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastGesto(uint8 Indice);

	void Fai(int32 Indice);
	void Ferma();

	/** Solo sul server: l'ultimo gesto (per non farne troppi). */
	double UltimoGesto = -1000.0;

	/** Su ogni PC: c'e' un gesto in corso, e dove era il corpo quando e' cominciato. */
	bool bInCorso = false;
	FVector Partenza = FVector::ZeroVector;
	double Inizio = 0.0;
	float Durata = 0.f;
	bool bResta = false;
};
