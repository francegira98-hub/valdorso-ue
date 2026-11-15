// Valdorso - Le mura (v0.2.6 "Le mura", approvata da Fra il 15/11/2026): la grata della porta grande e la sua leva.
// Scritto da Claude il 16/11/2026, mentre il pacchetto della v0.2.5 si preparava.
//
// - AValdorsoGrata: la saracinesca di ferro della porta. Sta abbassata (sbarra il passaggio) o alzata (si passa).
//   Sale piano, come tirata da un argano (60 cm al secondo), e scende di colpo (2,5 m al secondo), con il rumore delle
//   catene. Lo stato lo decide il server e viaggia in rete; ogni PC la muove da sé. Di serie e' una griglia di sbarre
//   fatta con i cubi (9 sbarre in piedi e 6 di traverso, 4 x 4,8 m): il modello vero della chat 3D (SM_Grata) si mette
//   nei Dettagli (Modello) e prende il posto delle sbarre. Un blocco invisibile della stessa misura ferma chi passa.
//   Origine: in basso al centro, la grata nel piano Y-Z (si passa lungo la X).
// - AValdorsoLeva: la leva nella stanza dell'argano, sopra la porta. Con E alza o abbassa la grata a cui e' legata
//   (Grata, nei Dettagli). Se ha una Chiave (di serie Chiave_Porta), la usa solo chi ce l'ha nello zaino, o lo staff con
//   il permesso Mondo; gli altri leggono "La leva ha un lucchetto". Tutto finisce nel registro eventi.
// - Le porte con la chiave sono AValdorsoPorta con bChiusaAChiave e Chiave (ValdorsoInterazione.h).
// Per provare: menu Valdorso -> Villaggio -> "Mura: metti una grata di prova davanti alla telecamera".

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ValdorsoInterazione.h"
#include "ValdorsoMura.generated.h"

class UBoxComponent;
class UStaticMesh;
class UStaticMeshComponent;
class USceneComponent;

UCLASS()
class VALDORSO_API AValdorsoGrata : public AActor
{
	GENERATED_BODY()

public:
	AValdorsoGrata();

	/** Il modello vero (SM_Grata della chat 3D): se c'e', prende il posto delle sbarre fatte con i cubi. */
	UPROPERTY(EditAnywhere, Category = "Grata")
	TObjectPtr<UStaticMesh> Modello;

	/** Larghezza e altezza della grata (cm): le sbarre di serie e il blocco che ferma. */
	UPROPERTY(EditAnywhere, Category = "Grata", meta = (ClampMin = "100"))
	float Larghezza = 400.f;

	UPROPERTY(EditAnywhere, Category = "Grata", meta = (ClampMin = "100"))
	float Altezza = 480.f;

	/** Quanto sale alzandosi (cm): di serie tutta la sua altezza meno 30 cm (le punte restano in vista). */
	UPROPERTY(EditAnywhere, Category = "Grata", meta = (ClampMin = "50"))
	float Corsa = 450.f;

	/** Velocita' (cm al secondo): sale piano (l'argano), scende di colpo (cade). */
	UPROPERTY(EditAnywhere, Category = "Grata", meta = (ClampMin = "5"))
	float VelocitaSu = 60.f;

	UPROPERTY(EditAnywhere, Category = "Grata", meta = (ClampMin = "5"))
	float VelocitaGiu = 250.f;

	/** Com'e' quando parte il mondo. */
	UPROPERTY(EditAnywhere, Category = "Grata")
	bool bAlzataAllInizio = false;

	/** Vero se e' alzata (o sta salendo). */
	bool Su() const { return bSu; }

	/** Vero se si sta muovendo. */
	bool InMovimento() const { return QuotaAttuale != (bSu ? Corsa : 0.f); }

	/** Solo sul server: la alza o la abbassa (Chi per il registro; nullptr = lo staff o un evento). */
	void Muovi(bool bAlzare, AActor* Chi);

	virtual void Tick(float DeltaSeconds) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;

	/** La parte che sale e scende (le sbarre o il modello, e il blocco). */
	UPROPERTY(VisibleAnywhere, Category = "Grata")
	TObjectPtr<USceneComponent> Saracinesca;

	/** Il modello vero, quando c'e'. */
	UPROPERTY(VisibleAnywhere, Category = "Grata")
	TObjectPtr<UStaticMeshComponent> Ferro;

	/** Le sbarre di serie (cubi). */
	UPROPERTY(VisibleAnywhere, Category = "Grata")
	TArray<TObjectPtr<UStaticMeshComponent>> Sbarre;

	/** Il blocco invisibile che ferma chi passa. */
	UPROPERTY(VisibleAnywhere, Category = "Grata")
	TObjectPtr<UBoxComponent> Blocco;

private:
	UPROPERTY(ReplicatedUsing = OnRep_Su)
	bool bSu = false;

	UFUNCTION()
	void OnRep_Su();

	/** Rimette le sbarre e il blocco della misura giusta (o il modello vero). */
	void Sistema();

	/** Il rumore delle catene (o il colpo della grata che tocca terra). */
	void Suona(const TCHAR* Nome, float Volume) const;

	float QuotaAttuale = 0.f;
	double NataAlle = 0.0;
};

UCLASS()
class VALDORSO_API AValdorsoLeva : public AActor, public IValdorsoInterattivo
{
	GENERATED_BODY()

public:
	AValdorsoLeva();

	/** La grata che comanda. */
	UPROPERTY(EditAnywhere, Category = "Leva")
	TObjectPtr<AValdorsoGrata> Grata;

	/** La chiave del lucchetto (vuoto = la usa chiunque). */
	UPROPERTY(EditAnywhere, Category = "Leva")
	FName Chiave = TEXT("Chiave_Porta");

	// IValdorsoInterattivo
	virtual FText TestoInterazione(const APawn* Chi) const override;
	virtual void Interagisci(APawn* Chi) override;
	virtual FVector PuntoInterazione() const override;

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Leva")
	TObjectPtr<UStaticMeshComponent> Base;

	UPROPERTY(VisibleAnywhere, Category = "Leva")
	TObjectPtr<USceneComponent> Perno;

	UPROPERTY(VisibleAnywhere, Category = "Leva")
	TObjectPtr<UStaticMeshComponent> Asta;

	/** Chi ha provato senza chiave: tutti i vicini sentono il lucchetto. */
	UFUNCTION(NetMulticast, Unreliable)
	void MulticastLucchetto();

private:
	float AngoloAttuale = -35.f;
};
