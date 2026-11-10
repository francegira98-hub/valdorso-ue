// Valdorso - Il cielo e i boschi che vivono (v0.1.4, 10/11/2026). Scritto da Claude, chiesto da Fra: "rendere tutto piu'
// bello e vivo" (e nei ritocchi della v0.1.3: "piu' uccelli").
//
// Solo su ogni PC, intorno al colono di quel PC (e' aspetto: non cambia il mondo, il server dedicato non fa niente). Lo fa
// nascere AValdorsoValleViva su ogni PC. Usa l'uccello della valle viva (SM_Uccello, con le ali che battono nel
// materiale) e i cubi di serie di Unreal per le foglie: niente da importare.
//   - Di giorno, senza pioggia forte, due stormi volano alti sopra la valle (12 uccelli l'uno, a 45-90 m), cambiano strada
//     ogni tanto e se ne vanno lontano quando cala la sera.
//   - Un rapace gira largo e lento, piu' in alto (110 m), sopra il colono.
//   - All'alba e al tramonto, nel villaggio, le rondini sfrecciano basse sopra i tetti.
//   - Di notte, vicino al cimitero, alle rovine, alla piazza, al tempio e al mulino, i pipistrelli svolazzano.
//   - D'autunno, sotto gli alberi del bosco, cadono le foglie (ocra, rosse, marroni): ondeggiano, si posano, spariscono.
//     In primavera, al loro posto, i petali (bianchi e rosa).
//   - (seconda parte) In primavera e d'estate, di giorno, le farfalle (gialle, bianche, arancio) svolazzano intorno.
//   - D'estate, di notte, fuori dal villaggio, le lucciole si accendono e si spengono vicino a terra (M_Lucciola).
//   - Dai comignoli delle case vicine sale il fumo (M_Fumo): sempre d'autunno e d'inverno, la mattina e la sera
//     tutto l'anno (si cucina), poco di notte; il vento lo piega. I comignoli li trova da solo (la cima piu' alta della casa).
//   - (Ali sulle montagne, dal Design: "pastori raccontano di un'ombra enorme passata davanti alla luna") una notte ogni
//     tanto (la stessa per tutti), al calare della sera, un'ombra enorme passa alta sopra i monti a nord e sparisce. E' il
//     primo segno del drago della v0.7. Se c'e' /Game/Valle/Viva/SM_Drago (quello di Fab, a grandezza vera) si vede
//     quello; se no l'uccello della valle, grande come un drago.
//   I materiali del fumo e delle lucciole li fa crea_vita_valle.py (menu Valdorso -> Villaggio): senza, niente fumo e lucciole.
// Comandi di console: Valdorso.VitaValle 0 la spegne (1 di serie); Valdorso.VitaValle.Stagione estate (o primavera,
// autunno, inverno) per provare un'altra stagione ("" = quella vera); Valdorso.VitaValle.Drago 1 per far passare il drago
// adesso.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ValdorsoVitaValle.generated.h"

class UInstancedStaticMeshComponent;
class UStaticMeshComponent;
class AValdorsoCielo;
class UMaterialInstanceDynamic;
class AValdorsoTempo;

namespace ValdorsoVitaValle
{
	/** Le ore degli stormi (giorno), delle rondini (alba e tramonto) e dei pipistrelli (notte), con la luce del giorno 0..1. */
	VALDORSO_API bool OraDegliStormi(float Ora);
	VALDORSO_API bool OraDelleRondini(float Ora);
	VALDORSO_API bool OraDeiPipistrelli(float Ora, float LuceDelGiorno);

	/** Vero se le foglie cadono in questa stagione ("autunno"). */
	VALDORSO_API bool StagioneDelleFoglie(const FString& Stagione);

	/** La cella (10 m) di un punto, per sapere in fretta se li' ci sono alberi. */
	VALDORSO_API FIntPoint Cella(const FVector& Punto);

	/** Vero se cadono i petali ("primavera"). */
	VALDORSO_API bool StagioneDeiPetali(const FString& Stagione);

	/** Le farfalle: primavera ed estate, di giorno. */
	VALDORSO_API bool OraDelleFarfalle(float Ora, const FString& Stagione);

	/** Le lucciole: d'estate, di notte. */
	VALDORSO_API bool OraDelleLucciole(float Ora, float LuceDelGiorno, const FString& Stagione);

	/** Quanto fumo dai camini (0..1): d'autunno e d'inverno sempre, mattina e sera tutto l'anno, poco di notte. */
	VALDORSO_API float QuantoFumo(float Ora, const FString& Stagione);

	/** La stagione della vita della valle: quella vera, o quella di Valdorso.VitaValle.Stagione per provare. */
	VALDORSO_API FString StagioneAdesso();

	/** Le notti del drago: una ogni tanto (circa una su quattro), la stessa per tutti (dal giorno vero). */
	VALDORSO_API bool NotteDelDrago(const FDateTime& Giorno);

	/** Quando parte: al calare della sera (il cielo ha ancora un po' di luce, l'ombra si vede). */
	VALDORSO_API bool OraDelDrago(float Ora);
}

UCLASS()
class VALDORSO_API AValdorsoVitaValle : public AActor
{
	GENERATED_BODY()

public:
	AValdorsoVitaValle();

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;

private:
	enum class EGruppo : uint8
	{
		Stormo0,
		Stormo1,
		Rapace,
		Rondini,
		Pipistrelli,
		Drago,
	};

	struct FVolatore
	{
		EGruppo Gruppo = EGruppo::Stormo0;
		float Scala = 1.f;
		/** Le fasi del suo giro (ognuno diverso dagli altri). */
		FVector Fase = FVector::ZeroVector;
		float Raggio = 600.f;
		FVector Prima = FVector::ZeroVector;
		bool bVisibile = false;
		/** Quanto c'e' (0..1): rapace, rondini e pipistrelli arrivano e se ne vanno piano. */
		float Presenza = 0.f;
	};

	struct FStormo
	{
		FVector Centro = FVector::ZeroVector;
		FVector Meta = FVector::ZeroVector;
		FVector Velocita = FVector::ZeroVector;
		double CambiaMeta = 0.0;
		bool bInCielo = false;
		bool bVaVia = false;
	};

	/** Una farfalla o una lucciola: vola intorno a un posto vicino al colono, poco sopra il terreno. */
	struct FPiccolo
	{
		bool bFarfalla = true;
		int32 Colore = 0;
		int32 Istanza = INDEX_NONE;
		FVector Fase = FVector::ZeroVector;
		FVector Ancora = FVector::ZeroVector;
		FVector Meta = FVector::ZeroVector;
		double CambiaAncora = 0.0;
		float Presenza = 0.f;
		bool bVisibile = false;
		bool bPosto = false;
	};

	/** Uno sbuffo di fumo: sale, si allarga, il vento lo piega, svanisce. */
	struct FSbuffo
	{
		FVector Posto = FVector::ZeroVector;
		double Nato = 0.0;
		float Durata = 7.f;
		float Grande = 1.f;
		bool bVivo = false;
		bool bNascosto = true;
	};

	struct FFoglia
	{
		FVector Posto = FVector::ZeroVector;
		float Terra = 0.f;
		float Caduta = 80.f;
		FVector Fase = FVector::ZeroVector;
		double Posata = -1.0;
		bool bViva = false;
		int32 Colore = 0;
		int32 Istanza = INDEX_NONE;
	};

	UPROPERTY(VisibleAnywhere, Category = "Vita della valle")
	TObjectPtr<UInstancedStaticMeshComponent> Volatori;

	UPROPERTY(VisibleAnywhere, Category = "Vita della valle")
	TArray<TObjectPtr<UInstancedStaticMeshComponent>> Foglie;

	UPROPERTY(VisibleAnywhere, Category = "Vita della valle")
	TArray<TObjectPtr<UInstancedStaticMeshComponent>> Farfalle;

	UPROPERTY(VisibleAnywhere, Category = "Vita della valle")
	TObjectPtr<UInstancedStaticMeshComponent> Lucciole;

	UPROPERTY(VisibleAnywhere, Category = "Vita della valle")
	TObjectPtr<UInstancedStaticMeshComponent> Fumo;

	/** Il drago di Fab (SM_Drago), se c'e'. */
	UPROPERTY(VisibleAnywhere, Category = "Vita della valle")
	TObjectPtr<UStaticMeshComponent> DragoVero;

	/** I colori delle foglie (autunno) o dei petali (primavera). */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> TinteFoglie;

	TArray<FVolatore> Uccelli;
	FStormo Stormi[2];
	FVector CentroRapace = FVector::ZeroVector;
	bool bRapace = false;
	TArray<FFoglia> Cadute;
	/** Le celle con un albero (e un albero per cella): dove cadono le foglie. */
	TMap<FIntPoint, FVector> Alberi;
	bool bAlberiCercati = false;
	double ProssimaRicercaAlberi = 0.0;
	double ProssimaFoglia = 0.0;
	double ProssimoControllo = 0.0;
	bool bVicinoNotte = false;
	bool bNelVillaggio = false;
	FString Stagione;
	FString StagioneDeiColori;
	/** Quanto grandi le foglie che cadono (i petali sono piu' piccoli). */
	float GrandezzaFoglie = 1.f;

	TArray<FPiccolo> Piccoli;
	TArray<FSbuffo> Sbuffi;
	/** Le cime dei comignoli trovati (nel mondo) e le case gia' guardate (ZeroVector = senza comignolo). */
	TArray<FVector> Comignoli;
	TMap<TWeakObjectPtr<AActor>, FVector> CaseGuardate;
	double ProssimaRicercaComignoli = 0.0;
	double ProssimoSbuffo = 0.0;
	int32 ProssimoComignolo = 0;

	/** Il drago: se vola, da quando, da dove parte e dov'e'; il giorno (vero) in cui e' gia' passato. */
	bool bDragoInVolo = false;
	double DragoInizio = 0.0;
	FVector DragoDa = FVector::ZeroVector;
	FVector DragoDove = FVector::ZeroVector;
	int32 GiornoDelDrago = -1;

	TWeakObjectPtr<AValdorsoCielo> Cielo;
	TWeakObjectPtr<AValdorsoTempo> Tempo;

	void CercaAlberi();
	void AggiornaStormo(int32 Quale, bool bVoluto, const FVector& Colono, float Delta, double Adesso);
	void AggiornaUccelli(const FVector& Colono, float Delta, double Adesso, bool bRapaceVoluto, bool bRondini, bool bPipistrelli);
	void AggiornaFoglie(const FVector& Colono, float Delta, double Adesso, bool bVoluto);
	void ColoraFoglie();
	void CercaComignoli(const FVector& Colono);
	void AggiornaPiccoli(const FVector& Colono, float Delta, double Adesso, bool bConFarfalle, bool bConLucciole);
	void AggiornaFumo(const FVector& Colono, float Delta, double Adesso, float Quanto);
	void AggiornaDrago(const FVector& Colono, double Adesso, float Ora, float Pioggia);
};
