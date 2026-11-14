// Valdorso - Le botteghe: cucinare, conciare, comprare e vendere (v0.1.4, 10/11/2026). Scritto da Claude.
// Dalla Roadmap (dopo i pericoli: "il villaggio che lavora": conceria, fucina, locanda) e dal Design (le pelli si
// conciano, la carne si cuoce, le armi le fa Marta).
//
// Come funziona:
// - Le ricette (ValdorsoBotteghe::Ricette): dove si fanno (il banco), cosa serve (fino a tre oggetti e/o Aureus) e cosa
//   si ottiene (un oggetto o degli Aureus). Comprare e' una ricetta con gli Aureus come ingrediente; vendere e' una
//   ricetta che da' Aureus.
// - I banchi (AValdorsoBanco): la fucina di Marta (armi e scudi), la conceria (pelli -> cuoio e mantelli, e le pelli si
//   vendono), la locanda di Bruno (zuppa, stufato, pane; Bruno compra la carne), il falegname (bastoni, clave, scudi di
//   legno). Li mette nel villaggio Content/Python/nuovi/crea_botteghe.py (menu Valdorso -> Villaggio). E vicino al banco:
//   si apre la sua schermata (SValdorsoBottega.h). Il fuoco e' un banco anche lui: E vicino a un falo' acceso, "Cucina
//   al fuoco" (la carne, i funghi).
// - Il server controlla tutto: che il colono sia davvero vicino al banco giusto, che abbia quello che serve, che ci sia
//   posto nello zaino (UValdorsoZainoComponent::ServerRicetta).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ValdorsoInterazione.h"
#include "ValdorsoBotteghe.generated.h"

class APawn;
class USphereComponent;
struct FValdorsoZaino;

UENUM(BlueprintType)
enum class EValdorsoTipoBanco : uint8
{
	Fuoco,
	Conceria,
	Fucina,
	Falegname,
	Locanda,
	/** (12/11, v0.2.0) La carovana di Sibilla: il banco e' lei, quando c'e' (ValdorsoCarovana.h). */
	Carovana,
};

/** Un ingrediente: quale oggetto e quanti. */
struct FValdorsoIngrediente
{
	const TCHAR* Id = nullptr;
	int32 Quanti = 0;
};

/** Una ricetta di bottega. */
struct FValdorsoRicetta
{
	const TCHAR* Id = TEXT("");
	EValdorsoTipoBanco Banco = EValdorsoTipoBanco::Fuoco;
	/** Il verbo del pulsante ("Cuoci", "Concia", "Compra", "Vendi", "Fai"). */
	const TCHAR* Verbo = TEXT("Fai");
	FValdorsoIngrediente Serve[3];
	/** Gli Aureus che costa (0 = niente). */
	int32 Costo = 0;
	/** Quello che si ottiene (nullptr = solo Aureus) e quanti; gli Aureus che si guadagnano. */
	const TCHAR* Risultato = nullptr;
	int32 QuantiRisultato = 1;
	int32 Guadagno = 0;
	/** (14/11, v0.2.3) Il grado di quello che si ottiene (ValdorsoQualita.h) e da dove viene ("Fattura di Marta"). */
	uint8 Qualita = 0;
	const TCHAR* Provenienza = nullptr;
	/** (14/11, v0.2.3) Lo forgia il colono: il grado viene dalla sua abilita' "Bottega" e da un po' di fortuna, e la
	 *  provenienza e' "Forgiata da <nome>" (UValdorsoZainoComponent::ServerRicetta). */
	bool bForgia = false;
	/** (14/11, v0.2.3) Una riparazione dell'arma (o dello scudo) in mano: 1 il ferro (Marta), 2 il legno (il falegname). */
	uint8 Ripara = 0;
};

namespace ValdorsoBotteghe
{
	VALDORSO_API TArrayView<const FValdorsoRicetta> Ricette();
	VALDORSO_API const FValdorsoRicetta* Trova(FName Id);

	/** Le ricette di un banco, nell'ordine della tabella. */
	VALDORSO_API TArray<const FValdorsoRicetta*> Del(EValdorsoTipoBanco Banco);

	/** Perche' adesso non si puo' fare ("Ti servono 2 pelli di lupo"); vuoto se si puo'. Guarda anche il posto nello zaino. */
	VALDORSO_API FText Manca(const FValdorsoZaino& Zaino, const FValdorsoRicetta& Ricetta);

	/** La fa: toglie gli ingredienti e gli Aureus, da' il risultato. Falso (e non cambia niente) se non si puo'. */
	VALDORSO_API bool Fai(FValdorsoZaino& Zaino, const FValdorsoRicetta& Ricetta);

	/** (14/11, v0.2.3) Come Fai, ma il risultato ha questo grado e questa provenienza (le armi forgiate dal colono). */
	VALDORSO_API bool FaiCon(FValdorsoZaino& Zaino, const FValdorsoRicetta& Ricetta, uint8 Qualita, const FString& Provenienza);

	/** (14/11, v0.2.3) Per una riparazione: quale mano ripara (falso = la destra), o falso se non c'e' niente da riparare. */
	VALDORSO_API bool ManoDaRiparare(const FValdorsoZaino& Zaino, const FValdorsoRicetta& Ricetta, bool& bOutSinistra);

	/** Il nome del banco ("La fucina di Marta") e il testo vicino ("Compra da Marta"). */
	VALDORSO_API FText Titolo(EValdorsoTipoBanco Banco);
	VALDORSO_API FText Invito(EValdorsoTipoBanco Banco);

	/** Una riga con quello che serve ("2 × Pelle di lupo, 5 Aureus") e una con quello che si ottiene. */
	VALDORSO_API FText TestoServe(const FValdorsoRicetta& Ricetta);
	VALDORSO_API FText TestoDa(const FValdorsoRicetta& Ricetta);

	/** Solo sul server: c'e' un banco di quel tipo (o un falo' acceso, per il fuoco) vicino al colono? */
	VALDORSO_API bool BancoVicino(const APawn* Chi, EValdorsoTipoBanco Banco);

	/** Da quanto lontano si lavora a un banco (cm). */
	inline constexpr float Raggio = 400.f;
}

/** Un banco di bottega nel villaggio: E vicino apre la sua schermata. */
UCLASS()
class VALDORSO_API AValdorsoBanco : public AActor, public IValdorsoInterattivo
{
	GENERATED_BODY()

public:
	AValdorsoBanco();

	/** Che banco e'. */
	UPROPERTY(EditAnywhere, Category = "Valdorso")
	EValdorsoTipoBanco Banco = EValdorsoTipoBanco::Fucina;

	virtual FText TestoInterazione(const APawn* Chi) const override;
	virtual void Interagisci(APawn* Chi) override;
	virtual FVector PuntoInterazione() const override { return GetActorLocation(); }

private:
	UPROPERTY(VisibleAnywhere, Category = "Valdorso")
	TObjectPtr<USphereComponent> Presa;
};
