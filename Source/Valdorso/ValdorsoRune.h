// Valdorso - La lingua delle rune di Val d'Orso (v0.1.3). Scritto da Claude il 07/11/2026.
//
// Dalle Cronache (la tavola mandata da Fra il 07/11, decisa da Fra lo stesso giorno: "Lingua delle rune"):
//   - le rune degli elementi, sulla linea di base: Aeth il fuoco, Nir l'acqua, Dorn la terra, Sil il vento, Lume la luce;
//   - le rune verbo, un gesto: Var andare/scagliare, Ka proteggere, Ren guarire, Vel vedere/guidare, Tor fermare;
//   - Ur, la runa del custode: si scrive da sola, sopra, e non entra nelle parole degli incantesimi.
// Un incantesimo è elemento + verbo (Aeth-Var, il Dardo infuocato); una terza runa lo piega (Nir-Ren-Ka, Acqua che
// veglia: le quindici parole a tre rune, grado III). Nel MMO la lingua serve per rituali, iscrizioni, tomi e firme dei
// maghi; i nomi degli incantesimi restano in italiano.
//
// La forma delle rune viene da Tools/Generatori/alfabeto_rune.py (ValdorsoRuneTratti.inl, generato): la stessa che i
// generatori incidono nella pietra delle Rovine e della grotta est. SValdorsoRuna le disegna sullo schermo.
// Nelle iscrizioni e nelle bacheche, una riga che comincia con "rune:" si mostra con le rune disegnate
// (per esempio "rune: Aeth Nir Dorn Sil Lume" = cinque rune sole; "rune: Nir-Ren-Ka" = una parola; dopo "|" una
// didascalia scritta piccola sotto le rune: "rune: Aeth-Var | Dardo infuocato").
// Il quaderno delle rune (tasto R in gioco) mostra l'alfabeto e le parole delle Cronache.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SLeafWidget.h"

enum class EValdorsoRuna : uint8
{
	Aeth,
	Nir,
	Dorn,
	Sil,
	Lume,
	Var,
	Ka,
	Ren,
	Vel,
	Tor,
	Ur,
	Num
};

namespace ValdorsoRune
{
	/** Il prefisso delle righe di iscrizioni e bacheche da mostrare con le rune disegnate. */
	inline const TCHAR* const PrefissoRiga = TEXT("rune:");

	/** Le linee spezzate di una runa (coordinate da -1 a 1, y in su). */
	VALDORSO_API const TArray<TArray<FVector2D>>& Linee(EValdorsoRuna Runa);

	/** Il nome ("Aeth") e il significato ("il fuoco"). */
	VALDORSO_API FString Nome(EValdorsoRuna Runa);
	VALDORSO_API FText Significato(EValdorsoRuna Runa);

	VALDORSO_API bool Elemento(EValdorsoRuna Runa);
	VALDORSO_API bool Verbo(EValdorsoRuna Runa);

	/** "aeth" -> Aeth (maiuscole e minuscole uguali); falso se non è una runa. */
	VALDORSO_API bool DaNome(const FString& Testo, EValdorsoRuna& Fuori);

	/** "Aeth-Var-Tor" -> [Aeth, Var, Tor]; falso (e Fuori vuoto) se c'è qualcosa che non è una runa. */
	VALDORSO_API bool LeggiParola(const FString& Testo, TArray<EValdorsoRuna>& Fuori);

	/** [Aeth, Var] -> "Aeth-Var". */
	VALDORSO_API FString ScriviParola(const TArray<EValdorsoRuna>& Parola);

	/** Un incantesimo: un elemento, poi uno o due verbi diversi; Ur mai. */
	VALDORSO_API bool IncantesimoValido(const TArray<EValdorsoRuna>& Parola);

	/** Il nome della parola nelle Cronache ("Dardo infuocato"), vuoto se non ne ha uno. */
	VALDORSO_API FText NomeIncantesimo(const TArray<EValdorsoRuna>& Parola);

	/** Il grado nelle Cronache: II per elemento + verbo, III con la terza runa; 0 se non è un incantesimo. */
	VALDORSO_API int32 Grado(const TArray<EValdorsoRuna>& Parola);

	/** Le parole con un nome nelle Cronache, in ordine: ("Aeth-Var", "Dardo infuocato"), ... */
	VALDORSO_API TArray<TPair<FString, FText>> ParoleConNome();

	/** Le righe del quaderno delle rune (si mostrano nella bacheca: le righe "rune: ... | didascalia" con le rune). */
	VALDORSO_API TArray<FString> RigheQuaderno();

	/** Una riga "rune: Aeth-Var Nir": le parole che contiene (separate da spazi). Falso se la riga non è di rune.
	 *  Dopo una barra "|" può venire una didascalia, che qui si ignora (la bacheca la scrive sotto le rune). */
	VALDORSO_API bool LeggiRiga(const FString& Riga, TArray<TArray<EValdorsoRuna>>& Parole);
}

/** Disegna una parola di rune (o una runa sola), come sono incise: in fila, unite da un trattino. */
class VALDORSO_API SValdorsoRuna : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SValdorsoRuna)
		: _Altezza(48.f)
		, _Spessore(3.f)
		, _Colore(FLinearColor(0.48f, 0.23f, 0.07f))
	{}
		SLATE_ARGUMENT(TArray<EValdorsoRuna>, Parola)
		/** Quanto è alta una runa (pixel). */
		SLATE_ARGUMENT(float, Altezza)
		SLATE_ARGUMENT(float, Spessore)
		SLATE_ATTRIBUTE(FLinearColor, Colore)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	void ImpostaParola(const TArray<EValdorsoRuna>& Nuova);

	virtual FVector2D ComputeDesiredSize(float LayoutScaleMultiplier) const override;
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

private:
	TArray<EValdorsoRuna> Parola;
	float Altezza = 48.f;
	float Spessore = 3.f;
	TAttribute<FLinearColor> Colore;
};
