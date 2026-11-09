// Valdorso - Gli oggetti della valle: il catalogo (v0.1.4 "Pericoli", seconda parte: lo zaino). Scritto da Claude
// il 08/11/2026.
//
// Qui c'è tutto quello che si può portare nello zaino, in un posto solo (lo usano il server, che decide, e la
// schermata, che mostra): il nome, due righe di descrizione, il peso in kg, quanti ne stanno in un posto (Pila),
// cosa succede usandolo (mangiare, bere, curarsi, leggere) e il disegno (poche linee a penna, come le rune: niente
// texture da importare). Le monete sono gli Aureus della Corona: non stanno nei posti, si contano a parte.
//
// Gli oggetti di partenza vengono dal Registro di Val d'Orso (ValdorsoRegistro.h, "I vantaggi delle risposte"):
// il ricordo che il colono porta con sé, l'attrezzo del vecchio mestiere, più Aureus per chi è venuto per un debito.
// I ricordi non si gettano e non si perdono morendo.
//
// Per aggiungere un oggetto: una riga in Catalogo() (ValdorsoOggetti.cpp) e il suo disegno in Disegno().
// I test (Valdorso.Zaino.*) controllano che ogni oggetto abbia nome, peso e disegno.

#pragma once

#include "CoreMinimal.h"

struct FValdorsoRegistro;

/** Cosa succede usando un oggetto (clic destro nello zaino, o il suo tasto nella barra delle azioni). */
enum class EValdorsoUso : uint8
{
	Niente,
	/** Si mangia: uno in meno, salute e stamina. */
	Mangia,
	/** Si beve un sorso: una carica in meno (la borraccia si riempie entrando nell'acqua). */
	Bevi,
	/** Si mette sulle ferite: uno in meno, salute. */
	Cura,
	/** Si legge: una riga in alto sullo schermo (non si consuma). */
	Leggi,
	/** (08/11) Si accende un falò davanti al colono (l'acciarino; servono i rami secchi: ValdorsoFalo.h). */
	Accendi,
	/** (09/11) Si apre come un foglio vero (la tessera del colono, ValdorsoTessera.h) e si può mostrare. */
	Documento,
	/** (09/11, v0.1.4) Si impugna (un'arma nella mano destra, lo scudo nella sinistra); di nuovo, si rimette via. */
	Impugna,
};

/** (09/11, v0.1.4) Dove si porta un oggetto addosso. */
enum class EValdorsoPosto : uint8
{
	Nessuno,
	ManoDestra,
	ManoSinistra,
};

/** Un tipo di oggetto del catalogo. */
struct FValdorsoTipoOggetto
{
	FName Id;
	const TCHAR* Nome = TEXT("");
	const TCHAR* Descrizione = TEXT("");
	/** Quanto pesa uno (kg). */
	float Peso = 0.f;
	/** Quanti ne stanno in un posto dello zaino. */
	int32 Pila = 1;
	EValdorsoUso Uso = EValdorsoUso::Niente;
	/** Usandolo: quanta salute, stamina ed equilibrio ridà. */
	float Salute = 0.f;
	float Stamina = 0.f;
	float Equilibrio = 0.f;
	/** Quante cariche ha pieno (la borraccia: i sorsi); 0 = niente cariche. */
	int32 Cariche = 0;
	/** Un ricordo (o la tessera): non si getta, non si perde morendo. */
	bool bRicordo = false;
	/** Cosa si legge usandolo (Uso Leggi). */
	const TCHAR* Lettura = TEXT("");

	/** (09/11, v0.1.4) Le armi: dove si impugnano, il danno, quanto equilibrio toglie, la stamina in piu' a colpo
	 *  (oltre ai 10 del pugno), la portata (cm) e che colpo e' (EValdorsoTipoColpo: 0 pugno, 1 lama, 2 botta, 4 scudo). */
	EValdorsoPosto Posto = EValdorsoPosto::Nessuno;
	float DannoArma = 0.f;
	float EquilibrioArma = 0.f;
	float StaminaArma = 0.f;
	float PortataArma = 0.f;
	uint8 TipoColpo = 0;
	/** Per lo scudo: quanto del colpo che arriva da davanti si ferma (0..1). */
	float Parata = 0.f;
	/** (09/11) Il mantello di pelle di lupo: quanti gradi in piu' si sentono finche' lo si porta nello zaino. */
	float Calore = 0.f;
};

namespace ValdorsoOggetti
{
	/** Quanti kg si portano senza fatica; oltre non si corre. */
	inline constexpr float PortataKg = 30.f;
	/** Oltre questi kg si cammina piano (FattoreVelocita). */
	inline constexpr float SovraccaricoKg = 40.f;
	/** Quanto pesa un Aureus (kg): cento monete fanno un chilo. */
	inline constexpr float PesoMoneta = 0.01f;
	/** Quanti Aureus ha un colono all'arrivo, e quanti chi è venuto per un debito. */
	inline constexpr int32 MoneteIniziali = 12;
	inline constexpr int32 MoneteDebito = 40;
	/** (08/11) Quanti rami secchi brucia un falò. */
	inline constexpr int32 RamiPerFalo = 2;

	/** Tutti gli oggetti, nell'ordine del catalogo. */
	VALDORSO_API TArrayView<const FValdorsoTipoOggetto> Catalogo();

	/** L'oggetto con quell'Id (nullptr se non c'è). */
	VALDORSO_API const FValdorsoTipoOggetto* Trova(FName Id);

	/** Il disegno dell'oggetto: linee spezzate con le coordinate da -1 a 1 (y in su), come le rune. Vuoto se non c'è. */
	VALDORSO_API const TArray<TArray<FVector2D>>& Disegno(FName Id);

	/** Il disegno della moneta (l'Aureus), per lo zaino. */
	VALDORSO_API const TArray<TArray<FVector2D>>& DisegnoMoneta();

	/** L'attrezzo del vecchio mestiere (chiave del Registro: "contadino"...); NAME_None se non ne ha uno. */
	VALDORSO_API FName AttrezzoDelMestiere(const FString& Mestiere);

	/** Il ricordo (chiave del Registro: "anello"...): l'Id dell'oggetto (Ricordo_Anello...); NAME_None se non c'è. */
	VALDORSO_API FName OggettoDelRicordo(const FString& Ricordo);

	/** Quello che un colono ha con sé all'arrivo (Id e quantità), dal suo Registro (nullptr = un colono qualunque). */
	VALDORSO_API TArray<TPair<FName, int32>> Partenza(const FValdorsoRegistro* Registro);

	/** Gli Aureus all'arrivo (più per chi è venuto per un debito). */
	VALDORSO_API int32 MonetePartenza(const FValdorsoRegistro* Registro);

	/**
	 * Il dono di un PNG quando dà un segno (ValdorsoDialoghi.h, "I segni"): Bruno e la prima scodella (il pane nero),
	 * Nonna Edda e il rispetto (le erbe). Falso se quel segno non porta niente.
	 */
	VALDORSO_API bool DonoDelSegno(FName IdDialogo, FName Segno, FName& OutOggetto, int32& OutQuantita);

	/** Quanto si va piano con questo peso (1 = normale; oltre SovraccaricoKg 0,6). */
	VALDORSO_API float FattoreVelocita(float PesoKg);

	/** Vero se con questo peso si può ancora correre (fino a PortataKg). */
	VALDORSO_API bool PuoCorrere(float PesoKg);

	/** "2,4 kg" (una cifra dopo la virgola, alla maniera italiana). */
	VALDORSO_API FString TestoPeso(float PesoKg);
}
