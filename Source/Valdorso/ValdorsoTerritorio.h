// Valdorso - La valle come territorio e la pietra del villaggio nel tempio (07/11/2026, v0.1.3, scritto da Claude
// mentre Fra dormiva).
//
// Dal Design ("Territori, pietre e conquista", "Da predisporre subito"): la valle della v0.1.3 viene registrata come
// primo territorio, con nome, signore la Corona e la pietra del villaggio nel tempio; il registro eventi annota in
// quale territorio avviene ogni azione.
//
// - FValdorsoTerritorio: i dati di un territorio (nome, signore, chi governa, futuro, custode della pietra, centro e
//   raggio). Per ora ce n'è uno solo, Val d'Orso, scritto qui; quando arriveranno i feudi si leggeranno da un file.
// - ValdorsoTerritori::Di(Dove): in quale territorio sta un punto (il registro eventi lo scrive in ogni riga).
// - AValdorsoPietraVillaggio: il frammento del Cuore sull'altare del Tempio del Cuore, cioè la pietra di Val d'Orso.
//   Batte come il frammento del menu (due colpi e una pausa), con la luce che pulsa. Con E: "Guarda la pietra" apre
//   la pergamena con il territorio, il custode e la forza della pietra.
//   La forza (0..1) la decide il server e la manda a tutti; più avanti scenderà con i giorni e salirà con i rituali di
//   ricarica (v0.6). Già adesso, sotto metà, il battito ogni tanto salta un colpo e la luce si fa più fioca (le voci
//   della locanda: "il frammento del tempio ha saltato un battito"). Comando: Valdorso.Pietra.Forza 0.4 (prove).
// La mette sull'altare crea_villaggio.py (o, senza rifare il villaggio, Content/Python/nuovi/metti_pietra_tempio.py).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ValdorsoInterazione.h"
#include "ValdorsoTerritorio.generated.h"

class UStaticMeshComponent;
class UPointLightComponent;
class UBoxComponent;
class UPostProcessComponent;
class USphereComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;

struct VALDORSO_API FValdorsoTerritorio
{
	/** Il codice che va nel registro eventi ("valdorso"). */
	FString Id;
	FText Nome;
	FText Signore;
	FText Governo;
	FText Futuro;
	FText Custode;
	FText Fede;
	/** Il centro e il raggio (cm) dentro cui vale (per ora tutta la valle di 3 × 3 km e i monti intorno). */
	FVector2D Centro = FVector2D::ZeroVector;
	float Raggio = 0.f;
};

namespace ValdorsoTerritori
{
	/** Val d'Orso, il primo territorio. */
	VALDORSO_API const FValdorsoTerritorio& ValDOrso();

	/** Il territorio in cui sta un punto (nullptr fuori da ogni territorio). */
	VALDORSO_API const FValdorsoTerritorio* Di(const FVector& Dove);

	/** Il codice per il registro eventi ("valdorso"; "nessuno" fuori da ogni territorio). */
	VALDORSO_API FString IdDi(const FVector& Dove);

	/** Come sta la pietra a parole, dalla forza (0..1): "piena", "buona", "debole", "allo stremo". */
	VALDORSO_API FText ParolaForza(float Forza);

	/**
	 * Vero se in quel giro di battito (uno ogni 1,6 s) il colpo salta. Mai sopra metà forza; sotto, sempre più spesso
	 * (a forza 0 un giro su due). Uguale su tutti i PC per lo stesso giro.
	 */
	VALDORSO_API bool SaltaIlBattito(int64 Giro, float Forza);

	/** Il battito della pietra (0..1) a quel secondo, con i colpi saltati. */
	VALDORSO_API float Battito(double Secondi, float Forza);
}

UCLASS()
class VALDORSO_API AValdorsoPietraVillaggio : public AActor, public IValdorsoInterattivo
{
	GENERATED_BODY()

public:
	AValdorsoPietraVillaggio();

	/** Il materiale luminoso (con i parametri Colore e Intensita): di serie /Game/Menu/M_Luminoso, lo stesso del menu. */
	UPROPERTY(EditAnywhere, Category = "Pietra")
	TObjectPtr<UMaterialInterface> Materiale;

	/** La luce (candele) in quiete e al colpo, a forza piena. */
	UPROPERTY(EditAnywhere, Category = "Pietra")
	float LuceQuiete = 1.2f;

	UPROPERTY(EditAnywhere, Category = "Pietra")
	float LuceColpo = 4.f;

	/** Quanto brilla il cristallo in quiete e al colpo. */
	UPROPERTY(EditAnywhere, Category = "Pietra")
	float BagliorQuiete = 0.15f;

	UPROPERTY(EditAnywhere, Category = "Pietra")
	float BagliorColpo = 0.9f;

	/** La forza di adesso (0..1). La decide il server. */
	float GetForza() const { return Forza; }

	/** Solo sul server: cambia la forza (per le prove; più avanti i giorni e i rituali). */
	void ImpostaForza(float Nuova);

	// IValdorsoInterattivo
	virtual FText TestoInterazione(const APawn* Chi) const override;
	virtual void Interagisci(APawn* Chi) override;
	virtual FVector PuntoInterazione() const override;

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Pietra")
	TObjectPtr<USceneComponent> Radice;

	/** Il cuscino di lino sotto il frammento. */
	UPROPERTY(VisibleAnywhere, Category = "Pietra")
	TObjectPtr<UStaticMeshComponent> Cuscino;

	/** Il punto che gira e galleggia: porta i due coni del frammento. */
	UPROPERTY(VisibleAnywhere, Category = "Pietra")
	TObjectPtr<USceneComponent> Gemma;

	/** Il frammento (due coni uniti alla base, finché non arriva il modello vero): la punta in su... */
	UPROPERTY(VisibleAnywhere, Category = "Pietra")
	TObjectPtr<UStaticMeshComponent> Cristallo;

	/** ...e la punta in giù. */
	UPROPERTY(VisibleAnywhere, Category = "Pietra")
	TObjectPtr<UStaticMeshComponent> CristalloSotto;

	UPROPERTY(VisibleAnywhere, Category = "Pietra")
	TObjectPtr<UPointLightComponent> Luce;

	/** (07/11 sera) Intorno all'altare: dentro, l'occhio si abitua un po' meno al buio (il tempio era bianco e il
	 *  frammento sembrava una fiamma bianca). Solo una scatola invisibile per il post process qui sotto. */
	UPROPERTY(VisibleAnywhere, Category = "Pietra")
	TObjectPtr<UBoxComponent> ZonaTempio;

	UPROPERTY(VisibleAnywhere, Category = "Pietra")
	TObjectPtr<UPostProcessComponent> VistaTempio;

	/** Quanto più scuro si vede dentro la zona del tempio (stop di esposizione; 0 = come fuori). */
	UPROPERTY(EditAnywhere, Category = "Pietra")
	float EsposizioneTempio = -1.f;

	/** La zona invisibile che la E trova (il cristallo non ha collisione). */
	UPROPERTY(VisibleAnywhere, Category = "Pietra")
	TObjectPtr<USphereComponent> Zona;

private:
	UPROPERTY(Replicated)
	float Forza = 1.f;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> MaterialeVivo;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> MaterialeLino;

	/** Dove galleggia il frammento, sopra la base (cm). */
	static constexpr float AltezzaGemma = 30.f;

	float Tempo = 0.f;
};
