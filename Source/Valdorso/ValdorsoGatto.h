// Valdorso - Il gatto della locanda, quello che sa le cose (v0.1.6 "Vita nella valle", La locanda viva; 11/11/2026).
// Scritto da Claude mentre Fra era a cena. Dalla roadmap ("La locanda viva: le voci vere che cambiano, l'ordine
// abituale del locandiere, il gatto che sa le cose").
//
// Come funziona:
// - AValdorsoGatto (lo mette Content/Python/nuovi/crea_gatto.py vicino a Bruno, nella locanda): un gatto grigio
//   tigrato fatto di tre pezzi (corpo, testa, coda; Tools/Generatori/gatto.py), mosso da ogni PC da solo. Non serve la
//   rete per vederlo camminare: ogni ventiquattro secondi sceglie un posto vicino a casa con lo stesso caso su tutti i
//   PC (il seme e l'orologio del server), ci va in cinque secondi e poi si siede, si sdraia su un fianco o si lava.
//   Nessuna collisione: i coloni ci passano attraverso, il gatto non se ne offende.
// - E: "Accarezza il gatto". Fa le fusa (tutti lo sentono e lo vedono girarsi verso chi lo accarezza). Una volta ogni
//   venti minuti per colono il gatto "sa una cosa": un indizio vero su un posto della valle (il muschio che fa luce
//   della grotta est, la polvere delle rovine, il fiore del belvedere, la piuma di Gaspare, le pagine di Lucia, i colpi
//   sotto la montagna del nord; con l'Atto I, il pelo dritto verso il tempio). Ogni indizio una volta sola per colono,
//   scritto nel taccuino (J, Iscrizioni: "Il gatto della locanda").
// - Suoni: S_Gatto_Fusa e S_Gatto_Miao (Tools/Generatori/gatto.py -> /Game/Valle/Pericoli/Suoni), con i sottotitoli.
// Console (prove): Valdorso.Gatto.Qui (porta il gatto davanti a te, solo su questo PC fino al prossimo giro).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ValdorsoInterazione.h"
#include "ValdorsoGatto.generated.h"

class USphereComponent;
class UStaticMeshComponent;

/** Un indizio del gatto: l'Id (segno nel taccuino) e quello che fa. */
struct FValdorsoIndizioGatto
{
	const TCHAR* Id = TEXT("");
	const TCHAR* Testo = TEXT("");
	/** Solo con l'Atto I cominciato. */
	bool bAttoI = false;
};

namespace ValdorsoGatto
{
	/** Il dialogo (finto) nel taccuino che tiene gli indizi gia' dati. */
	inline constexpr const TCHAR* MemoriaGatto = TEXT("Gatto");
	/** Ogni quanto un indizio per colono (s), quanto dura un giro e quanto il cammino dentro un giro. */
	inline constexpr double SecondiTraIndizi = 1200.0;
	inline constexpr double SecondiGiro = 24.0;
	inline constexpr double SecondiCammino = 5.0;
	/** Fin dove va da casa (cm). */
	inline constexpr float Raggio = 260.f;

	VALDORSO_API const TArray<FValdorsoIndizioGatto>& Indizi();

	/** Il prossimo indizio per chi conosce gia' quelli in Saputi (senza mondo, per i test). nullptr = li sa tutti. */
	VALDORSO_API const FValdorsoIndizioGatto* Prossimo(const TSet<FName>& Saputi, int32 Atto);

	/** Che cosa fa nel giro numero Giro: 0 seduto, 1 sdraiato, 2 si lava. */
	VALDORSO_API int32 PosaDelGiro(int64 Giro, int32 Seme);
}

UCLASS()
class VALDORSO_API AValdorsoGatto : public AActor, public IValdorsoInterattivo
{
	GENERATED_BODY()

public:
	AValdorsoGatto();

	/** Il seme del suo girovagare (due gatti nello stesso posto non vanno insieme). */
	UPROPERTY(EditAnywhere, Category = "Gatto")
	int32 Seme = 7;

	// IValdorsoInterattivo
	virtual FText TestoInterazione(const APawn* Chi) const override;
	virtual void Interagisci(APawn* Chi) override;
	virtual FVector PuntoInterazione() const override { return GetActorLocation() + FVector(0.f, 0.f, 20.f); }

	virtual void Tick(float DeltaSeconds) override;

	/** Per la console: vieni qui (solo su questo PC, fino al prossimo giro). */
	void VieniQui(const FVector& Dove);

protected:
	virtual void BeginPlay() override;

private:
	/** Su ogni PC: fa le fusa girato verso Verso (yaw) fino a Fino (orologio del server). */
	UFUNCTION(NetMulticast, Reliable)
	void MulticastFusa(float Verso, double Fino);

	double Orologio() const;
	FVector Posto(int64 Giro) const;
	float TerraSotto(const FVector& Dove) const;
	void Posa(int32 Quale, float Passo, float Tempo, float DeltaSeconds);
	void Miagola() const;

	UPROPERTY(VisibleAnywhere, Category = "Gatto")
	TObjectPtr<USphereComponent> Presa;

	UPROPERTY(VisibleAnywhere, Category = "Gatto")
	TObjectPtr<UStaticMeshComponent> Corpo;

	UPROPERTY(VisibleAnywhere, Category = "Gatto")
	TObjectPtr<UStaticMeshComponent> Testa;

	UPROPERTY(VisibleAnywhere, Category = "Gatto")
	TObjectPtr<UStaticMeshComponent> Coda;

	FVector Casa = FVector::ZeroVector;
	int64 GiroVisto = MIN_int64;
	FVector Da = FVector::ZeroVector;
	FVector A = FVector::ZeroVector;
	float ZDa = 0.f;
	float ZA = 0.f;
	double FusaFino = -1.0;
	float FusaVerso = 0.f;
	float Yaw = 0.f;
	/** La posa di adesso, che va verso quella giusta (cosi' non scatta). */
	FVector CorpoPos = FVector::ZeroVector;
	FRotator CorpoRot = FRotator::ZeroRotator;

	/** Solo sul server: l'ultimo indizio dato a ogni colono (per nome), in secondi del mondo. */
	TMap<FString, double> UltimoIndizio;
};
