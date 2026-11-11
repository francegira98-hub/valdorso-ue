// Valdorso - Le scorte del villaggio (v0.2.0 "Mani e materiali", 11/11/2026). Scritto da Claude.
//
// Il villaggio ha tre scorte: GRANO, LEGNA, PIETRA (da 0 a 200). Calano con il tempo vero (il grano e la legna si
// consumano: un po' ogni dieci minuti, la legna il doppio d'inverno), la pietra solo quando si costruisce (piu' avanti).
// I coloni le riempiono al MAGAZZINO del villaggio (AValdorsoMagazzino, accanto alla Gilda): E porta dentro tutta la
// legna e la pietra dello zaino; la cassa del villaggio paga (2 Aureus la legna, 3 la pietra grezza, 6 la squadrata)
// e ogni 5 pezzi la Gilda da' un punto. Senza niente da portare, la E mostra le scorte.
// Quando il grano scende sotto un terzo, la valle ha fame: Teodoro, al mulino, puo' confessare il grano nascosto
// (ValdorsoStorie: evento StoriaGrano), e il granaio si riempie un poco (una volta per carestia).
// Il file: Saved/<Prova|Solo|Server>/Scorte.json. Console (server): Valdorso.Scorte.Mostra, Valdorso.Scorte.Imposta
// <grano> <legna> <pietra>.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ValdorsoInterazione.h"
#include "ValdorsoScorte.generated.h"

class USphereComponent;

/** Le scorte del villaggio. */
struct FValdorsoScorteValle
{
	float Grano = 120.f;
	float Legna = 80.f;
	float Pietra = 40.f;
	/** Il grano nascosto dei mugnai e' gia' uscito in questa carestia (si riarma quando il grano torna sopra meta'). */
	bool bGranoNascostoUsato = false;
	/** L'ultima volta che si sono consumate (secondi veri dal 1970). */
	int64 UltimoConsumo = 0;
};

namespace ValdorsoScorte
{
	inline constexpr float Massimo = 200.f;
	/** Quanto si consuma ogni dieci minuti veri: il grano, la legna (d'inverno il doppio). */
	inline constexpr float GranoOgniDieci = 1.f;
	inline constexpr float LegnaOgniDieci = 1.f;
	/** Sotto questo grano c'e' la fame (un terzo). */
	inline constexpr float SogliaFame = Massimo / 3.f;

	/** Puro (per i test): consuma da Prima fino ad Adesso (secondi veri). */
	VALDORSO_API void Consuma(FValdorsoScorteValle& Scorte, int64 Adesso, bool bInverno);

	/** Puro: quanto paga la cassa per quel materiale (0 = il magazzino non lo prende), e in che scorta va. */
	VALDORSO_API int32 Prezzo(FName Oggetto);
	VALDORSO_API float* Scorta(FValdorsoScorteValle& Scorte, FName Oggetto, float& OutQuanto);

	/** Puro: le righe da leggere al magazzino. */
	VALDORSO_API TArray<FString> Righe(const FValdorsoScorteValle& Scorte);

	/** Solo sul server: le scorte di adesso (lette dal file la prima volta, consumate fino ad ora). */
	VALDORSO_API FValdorsoScorteValle& Adesso(const UObject* Contesto);

	/** Solo sul server: salva il file. */
	VALDORSO_API void Salva(const UObject* Contesto);

	/** Solo sul server: c'e' fame (il grano sotto un terzo). */
	VALDORSO_API bool Fame(const UObject* Contesto);
}

UCLASS()
class VALDORSO_API AValdorsoMagazzino : public AActor, public IValdorsoInterattivo
{
	GENERATED_BODY()

public:
	AValdorsoMagazzino();

	virtual FText TestoInterazione(const APawn* Chi) const override;
	virtual void Interagisci(APawn* Chi) override;
	virtual FVector PuntoInterazione() const override { return GetActorLocation() + FVector(0.f, 0.f, 80.f); }

private:
	UPROPERTY(VisibleAnywhere, Category = "Magazzino")
	TObjectPtr<USphereComponent> Presa;
};
