// Valdorso - Il giorno e la notte (v0.1.3, primo pezzo). Scritto da Claude il 03/11/2026, preparato mentre Fra dormiva.
//
// Come funziona:
// - L'ora del mondo la decide il server, uguale per tutti: un giorno della valle dura MinutiPerGiorno minuti veri
//   (di partenza 120: 2 ore). In rete viaggiano solo tre numeri (l'ora a un certo istante e quanto dura un giorno):
//   ogni PC calcola da sé l'ora di adesso, senza traffico continuo.
// - Su ogni PC (mai sul server dedicato) il sole del livello (la Directional Light con etichetta "Sole", o la prima che
//   trova) gira: sorge a est alle 6, è alto a mezzogiorno, tramonta alle 18. Di notte si accende la luna, una seconda
//   luce fredda e debole che aggiunge questo attore. La Sky Atmosphere colora il cielo da sola (alba, tramonto, notte).
// - La modalità di gioco lo fa nascere da sola se nel livello non ce n'è uno: per cambiarne i numeri (ora di partenza,
//   durata del giorno, luce della luna) si può anche metterlo a mano nel livello.
// Comandi di console (per le prove): Valdorso.Ora 21 (mette le 21), Valdorso.Giorno.Minuti 2 (un giorno in 2 minuti).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ValdorsoCielo.generated.h"

class UDirectionalLightComponent;
class USkyLightComponent;

UCLASS()
class VALDORSO_API AValdorsoCielo : public AActor
{
	GENERATED_BODY()

public:
	AValdorsoCielo();

	/** L'ora quando il server parte (0..24). */
	UPROPERTY(EditAnywhere, Category = "Cielo", meta = (ClampMin = "0", ClampMax = "24"))
	float OraIniziale = 9.f;

	/** Quanti minuti veri dura un giorno della valle. */
	UPROPERTY(EditAnywhere, Category = "Cielo", meta = (ClampMin = "1"))
	float MinutiPerGiorno = 120.f;

	/** Quanto sale il sole a mezzogiorno (gradi sopra l'orizzonte). */
	UPROPERTY(EditAnywhere, Category = "Cielo", meta = (ClampMin = "10", ClampMax = "90"))
	float AltezzaMassimaSole = 62.f;

	/** Da dove sorge il sole (gradi, come la Rotazione Z di Unreal). */
	UPROPERTY(EditAnywhere, Category = "Cielo")
	float DirezioneAlba = -90.f;

	/** La luce della luna (lux): debole, fredda. */
	UPROPERTY(EditAnywhere, Category = "Cielo", meta = (ClampMin = "0"))
	float LuceLuna = 0.3f;

	/** L'ora di adesso (0..24), uguale su server e PC. */
	float Ora() const;

	/** Solo sul server: mette un'ora (per le prove e, più avanti, per lo staff). */
	void ImpostaOra(float NuovaOra);

	/** Solo sul server: cambia la durata del giorno tenendo l'ora di adesso. */
	void ImpostaMinutiPerGiorno(float Minuti);

	virtual void Tick(float DeltaSeconds) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;

private:
	/** L'orologio del server (anche sui PC, tramite lo stato del gioco). */
	double TempoServer() const;

	/** Su questo PC: cerca il sole e la luce del cielo del livello. */
	void TrovaLuci();

	UPROPERTY(Replicated)
	float OraAllInizio = 9.f;

	UPROPERTY(Replicated)
	double SecondiAllInizio = 0.0;

	UPROPERTY(Replicated)
	float MinutiReplicati = 120.f;

	UPROPERTY(VisibleAnywhere, Category = "Cielo")
	TObjectPtr<UDirectionalLightComponent> Luna;

	TWeakObjectPtr<UDirectionalLightComponent> Sole;
	TWeakObjectPtr<USkyLightComponent> LuceCielo;
	float IntensitaSole = 10.f;
	bool bLuciCercate = false;
	double ProssimaCattura = 0.0;
};
