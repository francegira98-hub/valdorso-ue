// Valdorso - Il Vigore: la corsa che stanca, il fiato, la schivata e le cadute (v0.1.5 "Crescere", 11/11/2026). Scritto da
// Claude. Dalla Roadmap ("Il Vigore: la corsa consuma stamina; la schivata con mezzo secondo d'invulnerabilita' e il tempo
// di ricarica; la capriola d'atterraggio come talento a Atletica 25") e dal Design ("La capriola d'atterraggio").
// Fra il 10/11: "Medio" (si corre circa 20 secondi da riposati; a zero si e' senza fiato fino a un quarto).
//
// Come funziona (UValdorsoVigoreComponent sul colono; i conti li fa il server, il PC del colono li segue):
// - Correndo la stamina cala: il ritorno di serie (10 al secondo) si ferma e se ne perdono 5 al secondo, cioe' da pieni
//   si corre una ventina di secondi. Atletica alta allunga: un poco con i punti, un quarto con lo Scatto lungo (50), la
//   meta' con la Corsa senza fine (100). Il Secondo fiato (75) fa tornare la stamina un terzo piu' in fretta.
// - Senza fiato: a zero non si corre piu' finche' la stamina non torna a un quarto ("Sei senza fiato!").
// - In acqua Maiusc e' il nuoto veloce (piu' svelto di un terzo; il Nuoto veloce, 75, ancora di piu'): costa fiato come la
//   corsa, meno con la Bracciata leggera (25). Il fiato sott'acqua dura di piu' con il Fiato lungo (50) e l'Abisso (100).
// - La schivata: per mezzo secondo dall'inizio i colpi non arrivano ("Schivato!", ValdorsoScherma::PrimaDelColpo); poi si
//   aspetta un secondo e un quinto prima della prossima.
// - Le cadute: oltre i 6 metri circa ci si fa male (di piu' quanto piu' si cade; da una ventina di metri si muore). Con la
//   capriola d'atterraggio (Atletica 25) si rotola: un terzo del male, e si continua a correre.
// - Corsa "premi una volta" (Impostazioni, accessibilita'): Maiusc accende e spegne la corsa; fermandosi si spegne.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ValdorsoVigore.generated.h"

class ACharacter;

namespace ValdorsoVigore
{
	/** Quanta stamina si perde al secondo correndo (oltre al ritorno che si ferma), e nuotando veloci. */
	inline constexpr float CalaCorsa = 5.f;
	inline constexpr float CalaNuoto = 8.f;
	/** Il ritorno di serie della stamina (UValdorsoGE_RecuperoStamina): mentre si corre si ferma. */
	inline constexpr float RitornoDiSerie = 10.f;
	/** Senza fiato si torna a correre da questa parte della stamina. */
	inline constexpr float RipresaFiato = 0.25f;
	/** La schivata: quanto dura l'invulnerabilita' e quanto si aspetta fra due schivate (s). */
	inline constexpr float SecondiInvulnerabile = 0.5f;
	inline constexpr float SecondiTraSchivate = 1.2f;
	/** Le cadute: da che velocita' ci si fa male (cm/s, circa 6 m) e quanto (1 di salute ogni 10 cm/s in piu'). */
	inline constexpr float CadutaInnocua = 1100.f;
	inline constexpr float CadutaPerPunto = 10.f;
	/** Fin dove la capriola salva (cm/s): oltre si cade comunque (piu' in alto con lo Scatto lungo). */
	inline constexpr float CapriolaFino = 1900.f;
	inline constexpr float CapriolaFinoEvoluta = 2400.f;

	/** Quanto cala la corsa al secondo per chi ha quelle evoluzioni e quei punti di Atletica. */
	VALDORSO_API float CalaCorsaPer(float PuntiAtletica, int32 EvoluzioniAtletica);

	/** Quanto cala il nuoto veloce al secondo per chi ha quelle evoluzioni del Nuoto. */
	VALDORSO_API float CalaNuotoPer(int32 EvoluzioniNuoto);

	/** Quanto fa male una caduta a quella velocita' (verso il basso, positiva); bCapriola: con la capriola. */
	VALDORSO_API float DannoCaduta(float Velocita, bool bCapriola);

	/** Vero se a quella velocita' la capriola salva (con l'Atletica evoluta 2 volte, anche da piu' in alto). */
	VALDORSO_API bool CapriolaPossibile(float Velocita, int32 EvoluzioniAtletica);

	/** Quanto piu' a lungo dura il fiato sott'acqua per quel corpo (1, 1,5 con il Fiato lungo, 2 con l'Abisso). */
	VALDORSO_API float FattoreFiato(const AActor* Corpo);
}

UCLASS(ClassGroup = (Valdorso))
class VALDORSO_API UValdorsoVigoreComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UValdorsoVigoreComponent();

	static UValdorsoVigoreComponent* Di(const AActor* Chi);

	/** Vero se il colono e' senza fiato (non corre). Sul server e sul suo PC. */
	bool SenzaFiato() const { return bSenzaFiato; }

	/** La schivata e' partita (la chiama l'abilita', sul server e sul PC del colono). */
	void SchivataIniziata();

	/** Vero se si puo' schivare di nuovo (il tempo fra due schivate; dopo una caduta, la capriola subito). */
	bool SchivataPronta() const;

	/** Solo sul server: vero nel mezzo secondo dopo l'inizio della schivata (i colpi non arrivano). */
	bool Invulnerabile() const;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UFUNCTION()
	void QuandoAtterra(const FHitResult& Urto);

	/** Server: la stamina della corsa e del nuoto. */
	void Fatica(ACharacter* Corpo, float DeltaTime);
	/** Server e PC del colono: il movimento sa se si puo' correre e quanto si nuota veloci. */
	void Movimento(ACharacter* Corpo);
	void Avvisa(const FText& Testo, float Secondi = 2.5f) const;

	UPROPERTY(Replicated)
	bool bSenzaFiato = false;

	double InizioSchivata = -100.0;
	double CapriolaLiberaFino = 0.0;
	double NatoAlle = 0.0;
	/** La velocita' piu' forte verso il basso nella caduta di adesso (cm/s, positiva). */
	float CadutaPiuForte = 0.f;
	/** PC: da quanto si e' fermi (per spegnere la corsa "premi una volta"). */
	float Fermo = 0.f;
	bool bLegato = false;
};
