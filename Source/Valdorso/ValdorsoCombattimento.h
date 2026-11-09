// Valdorso - Il combattimento: i colpi che si sentono, l'aggancio e la pace del battito (v0.1.4 passo 4.4, 09/11/2026).
// Scritto da Claude. Dalla Roadmap precisa ("Aggancio e sensazione dei colpi") e dalla P10 approvata da Fra ("la pace
// del battito": i frammenti alzano una barriera, dentro nessuno puo' uccidere un altro giocatore).
//
// Come funziona:
// - ValdorsoCombattimento::Colpisci (solo sul server) e' l'unica strada per un colpo: pugno, armi, morsi dei lupi.
//   Toglie salute ed equilibrio (UValdorsoGE_Colpo, GAS), spinge indietro il colpito, scrive nel registro eventi e
//   manda a tutti i PC la reazione (UValdorsoReazioniComponent::MulticastColpito).
// - La pace del battito: se un giocatore colpisce un altro giocatore vicino a un frammento che batte
//   (AValdorsoPietraVillaggio::Protegge: per Val d'Orso il tempio), la salute non scende sotto 1. Chi avrebbe ucciso
//   legge "Il battito del tempio ti ferma la mano" ed e' marchiato (registro eventi: "Marchio"); il colpito legge
//   "Il battito ti protegge". Quando la pietra salta un colpo, quella frazione di secondo la pace trema (non vale).
// - La reazione su ogni PC: il suono del colpo (per tipo), un attimo di arresto delle animazioni di chi colpisce e di
//   chi e' colpito (60 ms), la telecamera che trema per chi c'e' dentro (di piu' a chi e' colpito).
// - L'aggancio (solo sul PC di chi gioca, UValdorsoAggancioComponent sul PlayerController): Tab, il tasto centrale
//   del mouse o la levetta destra premuta agganciano chi hai davanti (entro 15 m, nel campo visivo); la telecamera e il
//   colono lo seguono; di nuovo lo stesso tasto, oltre 22 m o se cade, si sgancia. Un rombo d'oro sopra il bersaglio;
//   la sua barra della salute compare solo quando lo colpisci (per 5 secondi), senza numeri (archivio Unity).
// I suoni li fanno Tools/Generatori/suoni_pericoli.py e Content/Python/nuovi/crea_pericoli.py (/Game/Valle/Pericoli/Suoni).

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Widgets/SLeafWidget.h"
#include "ValdorsoCombattimento.generated.h"

class APawn;
class USoundBase;
class UCharacterMovementComponent;
class APlayerController;
class UAbilitySystemComponent;

/** Che colpo e': decide il suono, la spinta e quanto trema la telecamera. */
UENUM()
enum class EValdorsoTipoColpo : uint8
{
	Pugno,
	Lama,
	Botta,
	Morso,
	Scudo,
};

namespace ValdorsoCombattimento
{
	/** L'arresto delle animazioni quando un colpo arriva (secondi). */
	inline constexpr float SecondiArresto = 0.06f;
	/** Entro quanto si aggancia (cm) e oltre quanto ci si sgancia. */
	inline constexpr float PortataAggancio = 1500.f;
	inline constexpr float PortataSgancio = 2200.f;
	/** Per quanto si vede la barra di chi hai colpito (secondi). */
	inline constexpr float SecondiBarra = 5.f;

	/** Vero se in quel punto, adesso, vale la pace del battito (un frammento che batte e' abbastanza vicino). */
	VALDORSO_API bool InPace(const UWorld* Mondo, const FVector& Dove);

	/** Il danno che passa dentro la pace: mai fino a zero (resta almeno 1 di salute). bOutTrattenuto: e' stato fermato. */
	VALDORSO_API float DannoNellaPace(float Danno, float Salute, bool& bOutTrattenuto);

	/** Quanto spinge indietro un colpo (cm/s) e quanto trema la telecamera di chi lo prende (0..1). */
	VALDORSO_API float Spinta(EValdorsoTipoColpo Tipo);
	VALDORSO_API float Tremore(EValdorsoTipoColpo Tipo);

	/** (09/11) Lo scudo ferma solo i colpi che arrivano davanti (entro circa 70 gradi da dove guarda chi lo porta). */
	VALDORSO_API bool DaDavanti(const FVector& AvantiDelColpito, const FVector& VersoChiColpisce);

	/** Il nome del suono del colpo ("S_Colpo_Pugno"...). */
	VALDORSO_API const TCHAR* NomeSuono(EValdorsoTipoColpo Tipo);

	/** Un suono dei pericoli (/Game/Valle/Pericoli/Suoni/<Nome>), nullptr se non c'e' ancora. */
	VALDORSO_API USoundBase* SuonoPericoli(const TCHAR* Nome);

	/** Suona un suono in un punto, che si sente fino a Lontano cm (non fa niente su un server senza finestra). */
	VALDORSO_API void Suona(UWorld* Mondo, USoundBase* Suono, const FVector& Dove, float Lontano, float Volume = 1.f, float Tono = 1.f);

	/** Il punto da guardare di un bersaglio (il petto, o la meta' della sua altezza). */
	VALDORSO_API FVector PuntoBersaglio(const AActor* Bersaglio);

	/** Salute e salute massima di chi ha le statistiche (GAS); falso se non le ha. */
	VALDORSO_API bool Salute(const AActor* Chi, float& OutSalute, float& OutMassima);

	/**
	 * Solo sul server: un colpo da Da ad A. Danno e Equilibrio sono positivi (quanto si toglie). Restituisce vero se ha
	 * toccato qualcosa che si puo' colpire (con le statistiche, o un manichino di Variant_Combat).
	 */
	VALDORSO_API bool Colpisci(AActor* Da, AActor* A, float Danno, float Equilibrio, const FVector& Punto, EValdorsoTipoColpo Tipo);
}

/** Su chi puo' essere colpito (coloni, lupi): le reazioni che si vedono e si sentono su ogni PC. */
UCLASS(ClassGroup = (Valdorso), meta = (BlueprintSpawnableComponent))
class VALDORSO_API UValdorsoReazioniComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UValdorsoReazioniComponent();

	static UValdorsoReazioniComponent* Di(const AActor* Chi);

	/** Il colpo arrivato (dal server a tutti): suono, arresto, telecamera. */
	UFUNCTION(NetMulticast, Unreliable)
	void MulticastColpito(AActor* Da, AActor* Colpito, FVector_NetQuantize Punto, uint8 Tipo);

	/** Solo su questo PC: la telecamera del proprietario trema (Forza 0..1). */
	void Scossa(float Forza);

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void Ferma();

	float ScossaRimasta = 0.f;
	float ScossaForza = 0.f;
	FVector ScossaBase = FVector::ZeroVector;
	bool bScossaBase = false;
};

/** Il rombo sopra il bersaglio agganciato e la barra di chi hai colpito: lo disegna l'aggancio, sopra il gioco. */
class VALDORSO_API SValdorsoBersaglio : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SValdorsoBersaglio) {}
		SLATE_ARGUMENT(TWeakObjectPtr<APlayerController>, Controllore)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(8.f, 8.f); }
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

private:
	TWeakObjectPtr<APlayerController> Controllore;
};

/** Sul PlayerController: l'aggancio del bersaglio e la barra di chi hai colpito (solo sul PC di chi gioca). */
UCLASS(ClassGroup = (Valdorso))
class VALDORSO_API UValdorsoAggancioComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UValdorsoAggancioComponent();

	static UValdorsoAggancioComponent* Di(const APlayerController* Controllore);

	/** Il tasto dell'aggancio: aggancia chi hai davanti, o sgancia. */
	void Alterna();

	/** Sgancia (e il colono torna a girarsi dove va). */
	void Sgancia();

	/** Chi e' agganciato adesso (nullptr se nessuno). */
	AActor* Agganciato() const { return Bersaglio.Get(); }

	/** Il colono di questo PC ha colpito Chi: la sua barra si vede per qualche secondo. */
	void HaiColpito(AActor* Chi);

	/** Di chi mostrare la barra adesso (l'ultimo colpito, se e' passato poco), e quanto e' agganciato. */
	AActor* BarraDi() const;

	/** Lato server: il colono guarda il bersaglio (gira con la telecamera) o dove va. */
	UFUNCTION(Server, Reliable)
	void ServerGuarda(bool bAgganciato);

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	AActor* Cerca() const;
	void Orienta(APawn* Corpo, bool bAgganciato);

	/** Come si girava il colono prima dell'aggancio (lo si rimette uguale quando si sgancia). */
	bool bGirataSalvata = false;
	bool bPrimaVersoMovimento = true;
	bool bPrimaVersoControllo = false;

	TWeakObjectPtr<AActor> Bersaglio;
	TWeakObjectPtr<AActor> UltimoColpito;
	double UltimoColpoAlle = -100.0;
	TSharedPtr<SValdorsoBersaglio> Schermo;
};
