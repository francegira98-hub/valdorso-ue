// Valdorso - Il villaggio che cresce a fasi (v0.1.3). Scritto da Claude il 06/11/2026.
// Decisa da Fra il 05/11 ("ogni cosa con una fase"), approvata il 06/11. Il Design ("Insediamenti che crescono")
// chiede che spazi e stadi si preparino già nella v0.1.3; la crescita vera (materiali, Aureus, cantieri) arriva con
// l'economia (v0.2), e allora sarà il server a far avanzare le fasi da solo.
//
// Come funziona:
// - crea_villaggio.py mette nel livello le cose di tutte le fasi. Quelle che non ci sono sempre hanno l'etichetta
//   "ValdorsoFase da-fino" (per esempio "ValdorsoFase 2-0" = dalla fase 2 per sempre, "ValdorsoFase 1-1" = solo
//   nella fase 1, come la palizzata stretta di oggi). Senza etichetta = c'è sempre.
// - Questo attore tiene la fase del villaggio: la decide il server, la salva su disco (Saved/Server/Villaggio.json,
//   così resta uguale dopo un riavvio) e la manda a tutti. Ogni PC e il server mostrano o nascondono da sé le cose
//   del livello: nascoste vuol dire anche senza collisione (non si urtano, le porte non si aprono) e luci spente.
// - Con World Partition i pezzi della valle arrivano mentre si cammina: quando ne arriva uno, si sistema subito.
// - La modalità di gioco lo fa nascere da sola se il livello non ne ha uno.
// Comando di console (per le prove e per lo staff): Valdorso.Villaggio.Fase 2 (senza numero dice la fase di adesso).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ValdorsoFasiVillaggio.generated.h"

class ULevel;

namespace ValdorsoFasi
{
	/** Il prefisso dell'etichetta delle cose a fasi ("ValdorsoFase 2-0"). */
	inline const TCHAR* const Prefisso = TEXT("ValdorsoFase ");

	/** Legge un'etichetta "ValdorsoFase da-fino": vero se è una di queste (Da >= 1, Fino 0 = per sempre). */
	VALDORSO_API bool LeggiEtichetta(const FName& Etichetta, int32& Da, int32& Fino);

	/** Vero se una cosa che c'è dalla fase Da alla fase Fino (0 = per sempre) c'è nella fase Fase. */
	VALDORSO_API bool CeNellaFase(int32 Fase, int32 Da, int32 Fino);

	/** Vero se l'attore ha un'etichetta di fase; dice anche da quale a quale fase c'è. */
	VALDORSO_API bool FaseDiAttore(const AActor* Attore, int32& Da, int32& Fino);
}

UCLASS()
class VALDORSO_API AValdorsoFasiVillaggio : public AActor
{
	GENERATED_BODY()

public:
	AValdorsoFasiVillaggio();

	/** La fase con cui parte un server nuovo (se non ce n'è una salvata). */
	UPROPERTY(EditAnywhere, Category = "Fasi", meta = (ClampMin = "1"))
	int32 FaseIniziale = 1;

	/** L'ultima fase preparata nel livello (oggi la 3, il muro di pietra). */
	UPROPERTY(EditAnywhere, Category = "Fasi", meta = (ClampMin = "1"))
	int32 FaseMassima = 3;

	/** La fase di adesso (uguale su server e PC). */
	int32 GetFase() const { return Fase; }

	/** Solo sul server: cambia la fase, la salva, la annota nel registro eventi e la manda a tutti. */
	void ImpostaFase(int32 NuovaFase);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UPROPERTY(ReplicatedUsing = OnRep_Fase)
	int32 Fase = 1;

	UFUNCTION()
	void OnRep_Fase();

	/** Mostra o nasconde le cose a fasi di tutto il mondo (o di un solo livello appena arrivato). */
	void Applica(ULevel* SoloLivello = nullptr);

	/** Un pezzo della valle è arrivato (World Partition): si sistemano le sue cose. */
	void QuandoArrivaLivello(ULevel* Livello, UWorld* Mondo);

	/** Il file dove il server tiene la fase. */
	static FString FileFase();
	int32 LeggiFaseSalvata() const;
	void SalvaFase() const;

	FDelegateHandle ManiglioLivelli;
	FTimerHandle TimerRipasso;
};
