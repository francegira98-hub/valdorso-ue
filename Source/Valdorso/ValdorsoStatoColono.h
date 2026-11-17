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

	/** Solo sul server: in cella. (16/11, v0.2.6) Nella fase 3 e' una cella vera degli alloggi della milizia
	 *  (AValdorsoCella): il colono ci compare dentro, puo' muoversi ma se esce ci torna; la fine della pena sta nel
	 *  file del personaggio (FValdorsoZaino::CellaFinoUtc), quindi scorre anche a gioco chiuso e vale dopo la morte e
	 *  dopo l'uscita. Senza celle (fasi 1 e 2) resta fermo dov'e', come prima. Poi libero e assolto. */
	void InCella();

	/** Solo sul server: se e' in cella esce adesso (lo staff: Valdorso.Colpa.Assolvi). */
	void LiberaDallaCella();

	/** Vero se e' in cella (solo sul server). */
	bool InCellaOra() const { return bInCellaOra; }

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
	/** (16/11, v0.2.6) Ogni secondo: la pena scritta nel file del personaggio (chi rientra e' ancora in cella). */
	void LeggiPenaSalvata(double Adesso);
	/** (16/11, v0.2.6) Ogni secondo finche' e' in cella: dentro la sua cella (o fermo, se celle non ce ne sono). */
	void TieniInCella();
	/** (16/11, v0.2.6) Fine della pena: fuori dalla cella, assolto. */
	void EsciDallaCella(bool bAvvisa);
	/** (16/11, v0.2.6) Scrive nel file del personaggio fino a quando resta in cella (0 = libero). */
	void SalvaPena(int64 FinoUtc);
	/** (16/11, v0.2.6) Rida' il movimento tolto dalla cella senza prigione (solo se l'aveva tolto lei). */
	void RidaiMovimento();

	UPROPERTY(Replicated)
	bool bColpevole = false;

	/** Solo sul server (secondi del mondo). */
	double ColpaFinoA = 0.0;
	double ArrestoAlle = 0.0;
	double UltimaColpa = -1000.0;
	double CellaFinoA = 0.0;
	bool bInArresto = false;
	bool bInCellaOra = false;
	/** (16/11, v0.2.6) La cella dove sta (solo sul server). */
	TWeakObjectPtr<class AValdorsoCella> CellaMia;
	/** (16/11, v0.2.6) Vero se la cella (senza prigione) gli ha tolto il movimento: solo allora glielo si rida'. */
	bool bFermoPerCella = false;
	FTimerHandle TimerColpa;

	UPROPERTY(Replicated)
	uint8 Forza = 0;

	UPROPERTY(Replicated)
	bool bForzaNota = false;
};
