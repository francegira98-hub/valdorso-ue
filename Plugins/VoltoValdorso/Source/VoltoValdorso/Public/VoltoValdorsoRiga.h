// Valdorso - Una riga della tabella dei cursori del volto. Scritto da Claude il 17/10/2026.
//
// Ogni riga dice: quando il cursore "Cursore" vale 1, l'osso "Osso" si sposta di SpostamentoPerUnita (cm, nello
// spazio del componente: X = di lato, sinistra positiva; Y = in avanti; Z = in alto) e la sua scala cresce di
// ScalaPerUnita (0,2 = 20% più grande a +1, 20% più piccolo a -1). A valore 0 non cambia niente.
// Più righe possono toccare lo stesso osso: gli effetti si sommano.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "VoltoValdorsoRiga.generated.h"

USTRUCT(BlueprintType)
struct VOLTOVALDORSO_API FVoltoValdorsoRiga : public FTableRowBase
{
	GENERATED_BODY()

	// Il nome del cursore: lo stesso Id del catalogo (ValdorsoAspetto) e lo stesso nome della variabile Float
	// nell'Anim Blueprint del volto (es. NasoLunghezza).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Volto")
	FName Cursore;

	// L'osso da muovere (es. FACIAL_C_NoseTip).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Volto")
	FName Osso;

	// Spostamento in cm quando il cursore vale 1 (a -1 è l'opposto).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Volto")
	FVector SpostamentoPerUnita = FVector::ZeroVector;

	// Crescita della scala quando il cursore vale 1 (0 = la scala non cambia). Gli assi sono quelli dell'osso:
	// per ora usiamo sempre lo stesso numero su X, Y e Z.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Volto")
	FVector ScalaPerUnita = FVector::ZeroVector;
};
