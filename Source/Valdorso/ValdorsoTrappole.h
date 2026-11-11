// Valdorso - Le trappole dei nani, le porte segrete e il tesoro che chiama le ossa (v0.1.7 "La grotta del nord", parte 2,
// 11/11/2026). Scritto da Claude. Fra il 11/11: trappole "mortali, rinasci al tempio", stanze segrete che non si trovano
// facilmente, "allargala quanto vuoi". Ispirato ai dungeon di Sword Art Online (la stanza del tesoro che si chiude e
// chiama i mostri, il pavimento che cede, le lame a tempo).
//
// Come funziona:
// - AValdorsoTrappola: quattro tipi. LANCE: una piastra un po' piu' scura; chi ci passa sente uno scatto e ha mezzo
//   secondo per levarsi, poi dal pavimento escono le lance (chi e' ancora sopra muore). BOTOLA: il pavimento cede
//   dopo un attimo e si richiude dopo qualche secondo; sotto c'e' il pozzo. PENDOLO: due lame che oscillano attraverso
//   il corridoio, sempre allo stesso ritmo (si passa contando). FONDO: il fondo del pozzo, irto di punte.
//   Tutto il ritmo viene dall'orologio del server: ogni PC vede le lame nello stesso punto.
// - Le trappole uccidono davvero (UValdorsoPericoliComponent::Ferisci): si rinasce al tempio, lo zaino resta. Solo i
//   coloni: scheletri e animali ci passano sopra senza danno (i nani le hanno fatte per i ladri, non per i loro morti).
// - AValdorsoPortaSegreta: una lastra del muro che non si distingue dalle altre. La E la trova solo da molto vicino, e
//   da fuori solo chi ha letto l'iscrizione che la svela (IscrizioneRichiesta, nel taccuino): "Completa il conto dei
//   nani". La lastra scende nel pavimento; dopo i suoi minuti, se nessuno e' sulla soglia, risale. Da dentro si apre
//   sempre.
// - AValdorsoTesoroAllarme: nella stanza del tesoro. Chi apre il forziere (AValdorsoForziere con bAllarme) fa cadere la
//   grata dell'ingresso e svegliare le ossa: un'orda di scheletri, gia' in piedi. La grata si rialza quando sono tutti
//   a terra, o dopo due minuti e mezzo. Poi la stanza riposa (MinutiPausa).
// Console (prove): Valdorso.Trappole.Mostra (sul PC: le zone delle trappole in rosso per 20 secondi).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ValdorsoInterazione.h"
#include "ValdorsoTrappole.generated.h"

class AValdorsoScheletro;
class UBoxComponent;
class USceneComponent;
class USphereComponent;
class UStaticMeshComponent;

// (in Python "ValdorsoTipoTrappola": "ValdorsoTrappola" e' gia' la classe)
UENUM(meta = (ScriptName = "ValdorsoTipoTrappola"))
enum class EValdorsoTrappola : uint8
{
	Lance,
	Botola,
	Pendolo,
	Fondo,
};

namespace ValdorsoTrappole
{
	/** Quanto toglie una trappola: tutto. */
	inline constexpr float DannoMortale = 100000.f;

	/** L'angolo di una lama del pendolo (gradi) a quell'ora del server. */
	VALDORSO_API float AngoloPendolo(double Secondi, float Periodo, float Fase, float Ampiezza);

	/** Le lance: quanto sono fuori (0 giu', 1 su) a quanti secondi dallo scatto. */
	VALDORSO_API float LanceFuori(double DalloScatto, float Ritardo);

	/** La botola: vero se il pavimento e' giu' (aperto) a quanti secondi dallo scatto. */
	VALDORSO_API bool BotolaAperta(double DalloScatto, float Ritardo, float Ripristino);

	/** Solo sul server: la trappola uccide quel colono (solo i coloni; vero se l'ha colpito). */
	VALDORSO_API bool Uccidi(AActor* Trappola, APawn* Chi, const TCHAR* Come);

	/** L'orologio del server (uguale su tutti i PC). */
	VALDORSO_API double Orologio(const UObject* Contesto);
}

UCLASS()
class VALDORSO_API AValdorsoTrappola : public AActor
{
	GENERATED_BODY()

public:
	AValdorsoTrappola();

	UPROPERTY(EditAnywhere, Category = "Trappola")
	EValdorsoTrappola Tipo = EValdorsoTrappola::Lance;

	/** Meta' delle misure della zona (cm): lungo il corridoio, di traverso, in alto. */
	UPROPERTY(EditAnywhere, Category = "Trappola")
	FVector Misura = FVector(100.f, 150.f, 60.f);

	/** Lance e botola: quanto passa dallo scatto al colpo (s). */
	UPROPERTY(EditAnywhere, Category = "Trappola", meta = (ClampMin = "0"))
	float Ritardo = 0.45f;

	/** Lance e botola: dopo quanto si riarma (s). */
	UPROPERTY(EditAnywhere, Category = "Trappola", meta = (ClampMin = "1"))
	float Ripristino = 5.f;

	/** Pendolo: il periodo (s), lo sfasamento (0..1), l'ampiezza (gradi) e quanto e' lungo il braccio (cm). */
	UPROPERTY(EditAnywhere, Category = "Trappola")
	float Periodo = 3.2f;

	UPROPERTY(EditAnywhere, Category = "Trappola")
	float Fase = 0.f;

	UPROPERTY(EditAnywhere, Category = "Trappola")
	float Ampiezza = 62.f;

	UPROPERTY(EditAnywhere, Category = "Trappola")
	float Braccio = 300.f;

	virtual void Tick(float DeltaSeconds) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Sul PC (prove): mostra la zona per qualche secondo. */
	void Mostra(float Secondi);

protected:
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;

private:
	UFUNCTION()
	void Entrato(UPrimitiveComponent* Mia, AActor* Altro, UPrimitiveComponent* SuaParte, int32 Indice, bool bSpinta, const FHitResult& Urto);

	/** Su ogni PC: la trappola e' scattata a quell'ora del server. */
	UFUNCTION(NetMulticast, Reliable)
	void MulticastScatta(double Quando);

	void Colpisci();
	void PendoloColpisce();
	FVector PuntaPendolo(float Angolo, float Lungo) const;

	UPROPERTY(VisibleAnywhere, Category = "Trappola")
	TObjectPtr<USceneComponent> Radice;

	UPROPERTY(VisibleAnywhere, Category = "Trappola")
	TObjectPtr<UBoxComponent> Zona;

	/** Le lance, il pavimento della botola, o il perno del pendolo con la lama. */
	UPROPERTY(VisibleAnywhere, Category = "Trappola")
	TObjectPtr<UStaticMeshComponent> Pezzo;

	UPROPERTY(VisibleAnywhere, Category = "Trappola")
	TObjectPtr<USceneComponent> Perno;

	/** Quando e' scattata (orologio del server): in rete anche per chi arriva dopo (la botola aperta e' per tutti). */
	UPROPERTY(Replicated)
	double Scattata = -1000.0;

	bool bBotolaGiu = false;
	double MostraFino = 0.0;
	FTimerHandle TimerColpo;
};

UCLASS()
class VALDORSO_API AValdorsoPortaSegreta : public AActor, public IValdorsoInterattivo
{
	GENERATED_BODY()

public:
	AValdorsoPortaSegreta();

	/** Cosa si legge da fuori ("Completa il conto dei nani"). */
	UPROPERTY(EditAnywhere, Category = "Porta segreta")
	FString Testo = TEXT("Spingi la pietra");

	/** Da fuori la E la trova solo chi ha questa iscrizione nel taccuino (vuoto = chiunque arrivi abbastanza vicino). */
	UPROPERTY(EditAnywhere, Category = "Porta segreta")
	FString IscrizioneRichiesta;

	/** Quanto resta aperta (minuti), e quanto scende la lastra (cm). */
	UPROPERTY(EditAnywhere, Category = "Porta segreta", meta = (ClampMin = "1"))
	float MinutiAperta = 10.f;

	UPROPERTY(EditAnywhere, Category = "Porta segreta")
	float Discesa = 380.f;

	virtual FText TestoInterazione(const APawn* Chi) const override;
	virtual void Interagisci(APawn* Chi) override;
	virtual FVector PuntoInterazione() const override { return GetActorLocation() + FVector(0.f, 0.f, 110.f); }

	virtual void Tick(float DeltaSeconds) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;

private:
	bool DaFuori(const APawn* Chi) const;
	bool Aperta() const;

	UPROPERTY(VisibleAnywhere, Category = "Porta segreta")
	TObjectPtr<USceneComponent> Radice;

	UPROPERTY(VisibleAnywhere, Category = "Porta segreta")
	TObjectPtr<UStaticMeshComponent> Lastra;

	UPROPERTY(VisibleAnywhere, Category = "Porta segreta")
	TObjectPtr<USphereComponent> Presa;

	/** Quando si e' aperta e quando ha cominciato a richiudersi (orologio del server; -1 = mai). */
	UPROPERTY(Replicated)
	double ApertaAlle = -1.0;

	UPROPERTY(Replicated)
	double ChiusaAlle = -1.0;

	bool bSuonata = false;
};

UCLASS()
class VALDORSO_API AValdorsoTesoroAllarme : public AActor
{
	GENERATED_BODY()

public:
	AValdorsoTesoroAllarme();

	/** Chi si sveglia, quanti, entro che raggio (cm). */
	UPROPERTY(EditAnywhere, Category = "Allarme")
	FName TipoScheletro = TEXT("Scheletro");

	UPROPERTY(EditAnywhere, Category = "Allarme", meta = (ClampMin = "1", ClampMax = "12"))
	int32 Quanti = 6;

	UPROPERTY(EditAnywhere, Category = "Allarme")
	float Raggio = 420.f;

	/** La grata dell'ingresso: dove sta (rispetto a questo attore) e come e' girata. */
	UPROPERTY(EditAnywhere, Category = "Allarme")
	FVector GrataDove = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, Category = "Allarme")
	float GrataGradi = 0.f;

	/** Al massimo quanto resta chiusa (s), e quanto riposa dopo (minuti). */
	UPROPERTY(EditAnywhere, Category = "Allarme")
	float SecondiMassimi = 150.f;

	UPROPERTY(EditAnywhere, Category = "Allarme")
	float MinutiPausa = 10.f;

	/** Solo sul server: qualcuno ha aperto il forziere. */
	void Scatta(APawn* Chi);

	virtual void Tick(float DeltaSeconds) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void OnRep_Chiusa();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastSuona(bool bChiude);

	UPROPERTY(VisibleAnywhere, Category = "Allarme")
	TObjectPtr<USceneComponent> Radice;

	UPROPERTY(VisibleAnywhere, Category = "Allarme")
	TObjectPtr<UStaticMeshComponent> Grata;

	UPROPERTY(ReplicatedUsing = OnRep_Chiusa)
	bool bChiusa = false;

	// --- solo sul server
	TArray<TWeakObjectPtr<AValdorsoScheletro>> Orda;
	double ChiusaDa = 0.0;
	double PausaFino = 0.0;

	// --- su ogni PC: dove sta la grata adesso (0 su, 1 giu')
	float Giu = 0.f;
};
