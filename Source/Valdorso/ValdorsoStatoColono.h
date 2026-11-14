// Valdorso - Lo stato del colono che vedono tutti (v0.1.5 "Crescere", 11/11/2026). Scritto da Claude. Dal Punto di
// ripresa del 10/11: "Gli altri coloni per ora non si valutano dal PC (serve un PlayerState con la forza replicata)".
//
// AValdorsoPlayerState: il PlayerState di Valdorso (lo mette AValdorsoGameMode). Porta a tutti i PC la forza del colono
// (0..100: l'arma migliore e la tempra, ValdorsoCrescere::Forza), cosi' agganciando un altro colono sotto il rombo si legge
// "Alla tua pari", "Piu' forte di te"... come per gli animali e i PNG. La aggiorna il server quando le abilita' cambiano
// (UValdorsoCrescitaComponent). Le abilita' vere restano segrete: arriva solo il numero, arrotondato.
//
// (14/11, v0.2.4 "PNG veri") La colpa, come il cursore arancione di SAO (ValdorsoFiducia.h): chi colpisce un PNG e'
// colpevole per dieci minuti, e tutti i PC lo sanno (il suo nome sopra la testa diventa rosso). Il server, ogni secondo:
// dopo un poco (o subito, se ha colpito una guardia) le guardie lo prendono, lo portano al posto di guardia e gli chiedono
// la multa o la cella (il dialogo "Arresto"); chi scappa dal dialogo viene ripreso dopo trenta secondi; la cella tiene
// fermo il colono per una "notte" (novanta secondi veri), poi e' libero. Senza guardie nella valle la colpa passa da sola.

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

	// --- (14/11, v0.2.4) La colpa ------------------------------------------------------------------------------------

	/** Vero se adesso e' colpevole (su ogni PC: il nome rosso). */
	bool Colpevole() const { return bColpevole; }

	/** Vero se il colono di quel corpo e' colpevole. */
	static bool ColpevoleDi(const AActor* Corpo);

	/** Solo sul server: ha colpito un PNG (bGuardia: una guardia, e allora lo prendono subito). Vero se conta come una
	 *  colpa nuova (la prima, o dopo ValdorsoFiducia::SecondiTraColpe): allora scendono la fiducia e si annota. */
	bool Incolpa(bool bGuardia);

	/** Solo sul server: la colpa e' pagata (o perdonata dallo staff). */
	void Assolvi();

	/** Solo sul server: una notte in cella (fermo, poi libero e assolto). */
	void InCella();

	/** Solo sul server: il dialogo dell'arresto si e' chiuso (se la colpa c'e' ancora, lo riprendono fra poco). */
	void ArrestoChiuso();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	/** Solo sul server, ogni secondo: la colpa che passa, le guardie, la cella. */
	void GiroColpa();
	/** Le guardie lo prendono: al posto di guardia, e il dialogo dell'arresto. */
	void Arresta();

	UPROPERTY(Replicated)
	bool bColpevole = false;

	/** Solo sul server (secondi del mondo). */
	double ColpaFinoA = 0.0;
	double ArrestoAlle = 0.0;
	double UltimaColpa = -1000.0;
	double CellaFinoA = 0.0;
	bool bInArresto = false;
	bool bInCellaOra = false;
	FTimerHandle TimerColpa;

	UPROPERTY(Replicated)
	uint8 Forza = 0;

	UPROPERTY(Replicated)
	bool bForzaNota = false;
};
