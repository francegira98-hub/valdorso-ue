// Valdorso - Gli oggetti vivi della scena dietro il menu (poi riusati nel Registro e nel tempio):
// il frammento del Cuore che batte (due colpi e una pausa), il braciere che arde con il suo fuoco,
// il cerchio di rune che pulsa col battito, il pulviscolo e le lucciole, l'ombra dell'Orso nella nebbia
// e i monti all'orizzonte. Solo luce e materiali che cambiano: niente rete, niente logica di gioco.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ValdorsoScenaMenu.generated.h"

class UStaticMesh;
class UStaticMeshComponent;
class UInstancedStaticMeshComponent;
class USkeletalMeshComponent;
class USkeletalMesh;
class UAnimationAsset;
class UPointLightComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;

/** Il frammento del Cuore del Mondo: un cristallo che gira piano e pulsa di luce con il battito. */
UCLASS()
class VALDORSO_API AValdorsoFrammentoCuore : public AActor
{
	GENERATED_BODY()

public:
	AValdorsoFrammentoCuore();

	UPROPERTY(VisibleAnywhere, Category = "Valdorso|Frammento")
	USceneComponent* Radice;

	UPROPERTY(VisibleAnywhere, Category = "Valdorso|Frammento")
	UStaticMeshComponent* Cristallo;

	UPROPERTY(VisibleAnywhere, Category = "Valdorso|Frammento")
	UPointLightComponent* Luce;

	/** Il materiale luminoso (con i parametri Colore e Intensita), creato dallo script della scena. */
	UPROPERTY(EditAnywhere, Category = "Valdorso|Frammento")
	UMaterialInterface* Materiale = nullptr;

	UPROPERTY(EditAnywhere, Category = "Valdorso|Frammento")
	float LuceQuiete = 15.f;

	UPROPERTY(EditAnywhere, Category = "Valdorso|Frammento")
	float LuceColpo = 90.f;

	UPROPERTY(EditAnywhere, Category = "Valdorso|Frammento")
	float BagliorQuiete = 3.f;

	UPROPERTY(EditAnywhere, Category = "Valdorso|Frammento")
	float BagliorColpo = 30.f;

	/** Gradi al secondo. */
	UPROPERTY(EditAnywhere, Category = "Valdorso|Frammento")
	float Rotazione = 8.f;

	/** Il frammento si accende tutto (quando si entra nella valle). */
	void Risveglia();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	UPROPERTY()
	UMaterialInstanceDynamic* MaterialeVivo = nullptr;

	float Tempo = 0.f;
	bool bRisveglio = false;
	float Risveglio = 0.f;
};

/** Il braciere: una base di pietra, le braci, tre lingue di fuoco incrociate e una luce calda che tremola. */
UCLASS()
class VALDORSO_API AValdorsoBraciere : public AActor
{
	GENERATED_BODY()

public:
	AValdorsoBraciere();

	UPROPERTY(VisibleAnywhere, Category = "Valdorso|Braciere")
	USceneComponent* Radice;

	UPROPERTY(VisibleAnywhere, Category = "Valdorso|Braciere")
	UStaticMeshComponent* Base;

	UPROPERTY(VisibleAnywhere, Category = "Valdorso|Braciere")
	UStaticMeshComponent* Braci;

	UPROPERTY(VisibleAnywhere, Category = "Valdorso|Braciere")
	UStaticMeshComponent* Fiamma1;

	UPROPERTY(VisibleAnywhere, Category = "Valdorso|Braciere")
	UStaticMeshComponent* Fiamma2;

	UPROPERTY(VisibleAnywhere, Category = "Valdorso|Braciere")
	UStaticMeshComponent* Fiamma3;

	UPROPERTY(VisibleAnywhere, Category = "Valdorso|Braciere")
	UPointLightComponent* Luce;

	UPROPERTY(EditAnywhere, Category = "Valdorso|Braciere")
	UMaterialInterface* MaterialePietra = nullptr;

	/** Lo stesso materiale luminoso del frammento, con un colore di brace. */
	UPROPERTY(EditAnywhere, Category = "Valdorso|Braciere")
	UMaterialInterface* MaterialeBraci = nullptr;

	/** Il materiale del fuoco (M_Fuoco, con i parametri Intensita e Seme). */
	UPROPERTY(EditAnywhere, Category = "Valdorso|Braciere")
	UMaterialInterface* MaterialeFuoco = nullptr;

	UPROPERTY(EditAnywhere, Category = "Valdorso|Braciere")
	float LuceMedia = 35.f;

	UPROPERTY(EditAnywhere, Category = "Valdorso|Braciere")
	float IntensitaFuoco = 2.5f;

	/** Centimetri. */
	UPROPERTY(EditAnywhere, Category = "Valdorso|Braciere")
	float AltezzaFiamma = 80.f;

	UPROPERTY(EditAnywhere, Category = "Valdorso|Braciere")
	float LarghezzaFiamma = 60.f;

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	UStaticMeshComponent* CreaFiamma(const TCHAR* Nome, UStaticMesh* Piano);
	void SistemaFiamme();

	UPROPERTY()
	UMaterialInstanceDynamic* BraciVive = nullptr;

	UPROPERTY()
	TArray<UMaterialInstanceDynamic*> FiammeVive;

	float Tempo = 0.f;
	float Seme = 0.f;
};

/** Il cerchio di rune sul pavimento, con l'iscrizione "IL CUORE BATTE ANCORA": si accende a ogni battito. */
UCLASS()
class VALDORSO_API AValdorsoCerchioRune : public AActor
{
	GENERATED_BODY()

public:
	AValdorsoCerchioRune();

	UPROPERTY(VisibleAnywhere, Category = "Valdorso|Rune")
	UStaticMeshComponent* Cerchio;

	/** Il materiale delle rune (M_Rune, con i parametri Colore e Intensita). */
	UPROPERTY(EditAnywhere, Category = "Valdorso|Rune")
	UMaterialInterface* Materiale = nullptr;

	/** Centimetri. */
	UPROPERTY(EditAnywhere, Category = "Valdorso|Rune")
	float Diametro = 560.f;

	UPROPERTY(EditAnywhere, Category = "Valdorso|Rune")
	FLinearColor Colore = FLinearColor(1.f, 0.55f, 0.2f);

	UPROPERTY(EditAnywhere, Category = "Valdorso|Rune")
	float RuneQuiete = 0.5f;

	UPROPERTY(EditAnywhere, Category = "Valdorso|Rune")
	float RuneColpo = 4.f;

	/** Il cerchio risponde al Cuore poco dopo il colpo, in secondi. */
	UPROPERTY(EditAnywhere, Category = "Valdorso|Rune")
	float Ritardo = 0.08f;

	/** Al primo ingresso il Cuore batte più forte e le rune rispondono più accese (v0.1.2, passo 2.5). */
	void Risveglia();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	UPROPERTY()
	UMaterialInstanceDynamic* RuneVive = nullptr;

	bool bRisveglio = false;
	float Risveglio = 0.f;
};

/** Il pulviscolo che galleggia nella luce e qualche lucciola che si accende e si spegne. */
UCLASS()
class VALDORSO_API AValdorsoPulviscolo : public AActor
{
	GENERATED_BODY()

public:
	AValdorsoPulviscolo();

	UPROPERTY(VisibleAnywhere, Category = "Valdorso|Pulviscolo")
	USceneComponent* Radice;

	UPROPERTY(VisibleAnywhere, Category = "Valdorso|Pulviscolo")
	UInstancedStaticMeshComponent* Polvere;

	UPROPERTY(VisibleAnywhere, Category = "Valdorso|Pulviscolo")
	UInstancedStaticMeshComponent* Lucciole;

	/** Lo stesso materiale luminoso del frammento (M_Luminoso). */
	UPROPERTY(EditAnywhere, Category = "Valdorso|Pulviscolo")
	UMaterialInterface* Materiale = nullptr;

	UPROPERTY(EditAnywhere, Category = "Valdorso|Pulviscolo")
	int32 NumeroPolvere = 160;

	UPROPERTY(EditAnywhere, Category = "Valdorso|Pulviscolo")
	int32 NumeroLucciole = 12;

	/** Metà della scatola in cui galleggiano (X, Y) e altezza (Z), in centimetri. */
	UPROPERTY(EditAnywhere, Category = "Valdorso|Pulviscolo")
	FVector Spazio = FVector(700.f, 700.f, 450.f);

	UPROPERTY(EditAnywhere, Category = "Valdorso|Pulviscolo")
	float BagliorPolvere = 3.f;

	UPROPERTY(EditAnywhere, Category = "Valdorso|Pulviscolo")
	float BagliorLucciole = 40.f;

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	struct FGranello
	{
		FVector Posizione = FVector::ZeroVector;
		FVector Deriva = FVector::ZeroVector;
		float Fase = 0.f;
		float Dimensione = 1.f;
	};

	void Rimetti(FGranello& G, bool bOvunque);
	void Aggiorna(UInstancedStaticMeshComponent* Componente, const TArray<FGranello>& Elenco, bool bLampeggia);

	TArray<FGranello> Granelli;
	TArray<FGranello> Luci;
	TArray<FTransform> Trasformazioni;
	FRandomStream Caso;
	float Tempo = 0.f;
};

/**
 * L'Orso che ogni tanto attraversa la nebbia, lontano, e sparisce.
 * Con un modello animato (Modello e Camminata) cammina davvero; senza, resta la sagoma piatta.
 */
UCLASS()
class VALDORSO_API AValdorsoOmbraOrso : public AActor
{
	GENERATED_BODY()

public:
	AValdorsoOmbraOrso();

	UPROPERTY(VisibleAnywhere, Category = "Valdorso|Orso")
	USceneComponent* Radice;

	UPROPERTY(VisibleAnywhere, Category = "Valdorso|Orso")
	UStaticMeshComponent* Sagoma;

	UPROPERTY(VisibleAnywhere, Category = "Valdorso|Orso")
	USkeletalMeshComponent* Corpo;

	/** Il materiale della sagoma (M_OmbraOrso, con i parametri Opacita e Verso). */
	UPROPERTY(EditAnywhere, Category = "Valdorso|Orso")
	UMaterialInterface* Materiale = nullptr;

	/** Il modello animato dell'orso (se c'e', prende il posto della sagoma). */
	UPROPERTY(EditAnywhere, Category = "Valdorso|Orso")
	USkeletalMesh* Modello = nullptr;

	/** L'animazione della camminata, ripetuta. */
	UPROPERTY(EditAnywhere, Category = "Valdorso|Orso")
	UAnimationAsset* Camminata = nullptr;

	/** Quante volte piu' grande di un orso vero (e' l'Orso della leggenda). */
	UPROPERTY(EditAnywhere, Category = "Valdorso|Orso")
	float ScalaModello = 3.f;

	/** Gradi da aggiungere perche' il modello guardi dove cammina (dipende da come e' stato fatto). */
	UPROPERTY(EditAnywhere, Category = "Valdorso|Orso")
	float GiroModello = -90.f;

	/** Velocita' dell'animazione: si regola finche' le zampe non scivolano sul terreno. */
	UPROPERTY(EditAnywhere, Category = "Valdorso|Orso")
	float VelocitaAnimazione = 1.f;

	/** Altezza della sagoma, in centimetri (è un orso leggendario). */
	UPROPERTY(EditAnywhere, Category = "Valdorso|Orso")
	float Altezza = 900.f;

	/** Quanto cammina, da un lato all'altro, in centimetri. */
	UPROPERTY(EditAnywhere, Category = "Valdorso|Orso")
	float Percorso = 9000.f;

	/** Centimetri al secondo. */
	UPROPERTY(EditAnywhere, Category = "Valdorso|Orso")
	float Velocita = 160.f;

	/** La prima passata, in secondi dall'avvio. */
	UPROPERTY(EditAnywhere, Category = "Valdorso|Orso")
	float PrimaPassata = 6.f;

	UPROPERTY(EditAnywhere, Category = "Valdorso|Orso")
	float PausaMinima = 25.f;

	UPROPERTY(EditAnywhere, Category = "Valdorso|Orso")
	float PausaMassima = 50.f;

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	void Posiziona(float Y, float Opacita);

	UPROPERTY()
	UMaterialInstanceDynamic* SagomaViva = nullptr;

	bool bInCammino = false;
	float Attesa = 0.f;
	float Avanzamento = 0.f;   // da 0 a 1 lungo il percorso
	float Verso = 1.f;
	float Tempo = 0.f;
};

/** I monti all'orizzonte: un anello di sagome scure, con le creste disegnate dal materiale. */
UCLASS()
class VALDORSO_API AValdorsoMonti : public AActor
{
	GENERATED_BODY()

public:
	AValdorsoMonti();

	UPROPERTY(VisibleAnywhere, Category = "Valdorso|Monti")
	UInstancedStaticMeshComponent* Anello;

	/** Il materiale dei monti (M_Monti, con i parametri Pezzi, Seme e Colore). */
	UPROPERTY(EditAnywhere, Category = "Valdorso|Monti")
	UMaterialInterface* Materiale = nullptr;

	/** Centimetri dal centro della scena. */
	UPROPERTY(EditAnywhere, Category = "Valdorso|Monti")
	float Raggio = 14000.f;

	/** Altezza massima delle creste, in centimetri. */
	UPROPERTY(EditAnywhere, Category = "Valdorso|Monti")
	float Altezza = 3500.f;

	/** Quanto sotto terra parte l'anello, in centimetri. */
	UPROPERTY(EditAnywhere, Category = "Valdorso|Monti")
	float Sotto = 300.f;

	UPROPERTY(EditAnywhere, Category = "Valdorso|Monti", meta = (ClampMin = "6", ClampMax = "96"))
	int32 Pezzi = 24;

	/** Cambia la forma delle creste. */
	UPROPERTY(EditAnywhere, Category = "Valdorso|Monti")
	float Seme = 1.f;

	UPROPERTY(EditAnywhere, Category = "Valdorso|Monti")
	FLinearColor Colore = FLinearColor(0.004f, 0.006f, 0.012f);

	virtual void OnConstruction(const FTransform& Transform) override;

private:
	UPROPERTY()
	UMaterialInstanceDynamic* MontiVivi = nullptr;
};
