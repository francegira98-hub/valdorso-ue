// Valdorso - Le campane del Tempio del Cuore (v0.1.3). Scritto da Claude il 06/11/2026.
//
// Come funziona:
// - La campana sta in cima al campanile (la mette Content/Python/nuovi/crea_suoni_valle.py) e si sente in tutta la
//   valle, più piano lontano.
// - Suona le ore canoniche, come nei paesi del Regno: alle 6 (prima), alle 9 (terza), a mezzogiorno (sesta), alle 15
//   (nona), alle 18 (vespro) e alle 21 (compieta), con tanti rintocchi quante sono le ore (a mezzogiorno 12). L'ora la
//   sa ogni PC da sé dal cielo (AValdorsoCielo), quindi non serve la rete: suonano insieme per tutti.
// - L'allarme: rintocchi fitti finché non lo si ferma. Lo decide il server e lo manda a tutti (più avanti lo suoneranno
//   la milizia e il sacerdote; per ora c'è il comando). Il server lo annota nel registro eventi.
// Comandi di console: Valdorso.Campane.Allarme 1 (0 lo ferma, solo sul server), Valdorso.Campane.Prova (tre rintocchi
// adesso, per sentire la campana).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ValdorsoCampane.generated.h"

class UAudioComponent;
class USoundBase;
class AValdorsoCielo;

namespace ValdorsoCampane
{
	/** Quanti rintocchi a quest'ora intera (0..23): solo alle ore canoniche, 0 alle altre. */
	VALDORSO_API int32 Rintocchi(int32 Ora);
}

UCLASS()
class VALDORSO_API AValdorsoCampane : public AActor
{
	GENERATED_BODY()

public:
	AValdorsoCampane();

	UPROPERTY(EditAnywhere, Category = "Campane")
	TObjectPtr<USoundBase> Suono;

	/** Fin dove si sente piena (cm) e in quanto si spegne dopo (cm): di serie tutta la valle. */
	UPROPERTY(EditAnywhere, Category = "Campane", meta = (ClampMin = "0"))
	float Vicino = 3000.f;

	UPROPERTY(EditAnywhere, Category = "Campane", meta = (ClampMin = "100"))
	float Lontano = 200000.f;

	UPROPERTY(EditAnywhere, Category = "Campane", meta = (ClampMin = "0.5"))
	float SecondiTraRintocchi = 2.6f;

	UPROPERTY(EditAnywhere, Category = "Campane", meta = (ClampMin = "0.2"))
	float SecondiAllarme = 0.7f;

	/** Solo sul server: suona o ferma l'allarme. */
	void ImpostaAllarme(bool bSuona);

	bool Allarme() const { return bAllarme; }

	/** Su questo PC: tanti rintocchi adesso (per le prove). */
	void Rintocca(int32 Quanti);

	virtual void Tick(float DeltaSeconds) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(ReplicatedUsing = OnRep_Allarme)
	bool bAllarme = false;

	UFUNCTION()
	void OnRep_Allarme();

	/** Tre voci, così un rintocco non taglia la coda di quello prima. */
	UPROPERTY(VisibleAnywhere, Category = "Campane")
	TArray<TObjectPtr<UAudioComponent>> Voci;

	void Colpo(float Tono);

	TWeakObjectPtr<AValdorsoCielo> Cielo;
	int32 UltimaOra = -1;
	int32 DaSuonare = 0;
	int32 ProssimaVoce = 0;
	double ProssimoColpo = 0.0;
	bool bTonoAlto = false;
};
