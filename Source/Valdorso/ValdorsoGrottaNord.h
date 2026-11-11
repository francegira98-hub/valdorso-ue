// Valdorso - La grotta del nord: la frana che si apre, le gallerie dei nani, il bottino e la storia di Lucia (v0.1.7,
// 11/11/2026). Scritto da Claude. Dal Punto di ripresa del 10/11 ("v0.1.7 La grotta del nord, il primo dungeon: la frana
// si apre (evento dello staff), gallerie e sale, scheletri, bottino, la quest segreta di Lucia 'Quelli che non
// tornarono'") e dalle scelte di Fra del 10/11 (frana con l'evento e lo scavo insieme, grotta generata, storia proposta da
// Claude e segnata nella Lore come Proposta).
//
// Come funziona:
// - AValdorsoFrana (davanti al portale dei nani, la mette crea_grotta_nord.py): tre stati, salvati dal server in
//   Saved/<Server|Solo|Prova>/GrottaNord.json. CHIUSA: la frana e' roccia (E: la si guarda, e di notte si sente battere).
//   SMOSSA: una notte la terra trema (lo staff, F9 -> Mondo -> "Frana del nord: trema", o Valdorso.Grotta.Trema): da
//   allora i coloni la liberano insieme, sasso dopo sasso (E con un piccone o un martello nello zaino; costa fiato e fa
//   crescere la Tempra); i sassi davanti al portale spariscono man mano. APERTA: al cento per cento la frana cede, e chi
//   e' vicino lo vede. E: "Entra nelle gallerie".
// - Le gallerie stanno sotto la montagna (il paesaggio non si buca): si entra dal portale e si arriva giu' (lo schermo
//   si apre dal nero). Dentro: la galleria della frana, la sala delle colonne, il campo della spedizione, la scala dei
//   nani, la sala delle ossa con il capitano e la porta dei dormienti (chiusa: v0.6). AValdorsoPassaggio riporta fuori.
// - AValdorsoForziere: il forziere dei nani (una volta al giorno per colono): monete dei nani, Aureus, a volte un pugnale.
// - La storia di Lucia, "Quelli che non tornarono" (Proposta nella Lore, da confermare): nell'autunno del 309 quattro
//   della Gilda entrarono dalla fessura sopra la frana; la frana scese ancora e restarono dentro. Lucia, fuori con la
//   corda, ha tenuto due pagine della mappa. Chi le dimostra di aver visto la porta degli Antichi (grotta est) riceve le
//   pagine (Mappa_Gallerie) e la storia nel taccuino; dentro trova il taccuino di Gisella (il campo), la lanterna di Cecco
//   (la sala delle ossa) e l'anello di Ranieri (al collo del capitano). Riportati a Lucia: Aureus, punti della Gilda e la
//   fine della storia. Ranieri tra le ossa non c'e': il suo anello si'.
// Comandi di console (sul server): Valdorso.Grotta.Trema, .Apri, .Chiudi, .Dentro (porta il colono dentro), .Fuori.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ValdorsoInterazione.h"
#include "ValdorsoGrottaNord.generated.h"

class APawn;
class APlayerController;
class UStaticMeshComponent;
class USphereComponent;
struct FValdorsoZaino;

UENUM()
enum class EValdorsoStatoFrana : uint8
{
	Chiusa,
	Smossa,
	Aperta,
};

/** Quello che il server ricorda della grotta del nord. */
USTRUCT()
struct FValdorsoGrottaSalvata
{
	GENERATED_BODY()

	UPROPERTY()
	int32 Versione = 1;

	UPROPERTY()
	int32 Stato = 0;

	UPROPERTY()
	float Scavo = 0.f;

	/** Quando si e' aperta (secondi Unix, 0 = mai). */
	UPROPERTY()
	int64 Aperta = 0;
};

namespace ValdorsoGrottaNord
{
	/** Quanto libera un colpo di piccone (0..1 della frana), con la Tempra (0..100): circa duecento colpi in tutto. */
	VALDORSO_API float PassoScavo(float PuntiTempra);

	/** Quanto fiato costa un colpo di piccone. */
	inline constexpr float StaminaScavo = 12.f;

	/** Ogni quanto un colono puo' dare un colpo (s). */
	inline constexpr double SecondiTraColpi = 1.2;

	/** Vero se con quello zaino si scava (un piccone o un martello). */
	VALDORSO_API bool HaAttrezzo(const FValdorsoZaino& Zaino);

	/** Il dialogo di Lucia (ValdorsoDialoghiTesti.inl): i segni e gli eventi della storia. */
	inline constexpr const TCHAR* DialogoLucia = TEXT("Lucia");
	inline constexpr const TCHAR* SegnoMappa = TEXT("GallerieMappa");
	inline constexpr const TCHAR* SegnoFinite = TEXT("GallerieFinite");
	inline constexpr const TCHAR* EventoMappa = TEXT("LuciaMappa");
	inline constexpr const TCHAR* EventoConsegna = TEXT("LuciaConsegna");
	inline constexpr const TCHAR* NodoMancano = TEXT("mancano");
	/** L'iscrizione della porta degli Antichi (grotta est, Tools/Generatori/villaggio_grotta.py): chi l'ha letta l'ha vista. */
	inline constexpr const TCHAR* TitoloPortaAntichi = TEXT("La porta in fondo alla grotta");

	/** Le tre cose da ritrovare (l'Id nello zaino). */
	VALDORSO_API TArrayView<const FName> Tracce();

	/** La storia nel taccuino: il titolo e le righe di quando Lucia la racconta, e di quando finisce. */
	VALDORSO_API FString TitoloStoria();
	VALDORSO_API TArray<FString> RigheStoria();
	VALDORSO_API TArray<FString> RigheFine();
	/** Cosa si scrive nel taccuino trovando una traccia. */
	VALDORSO_API TArray<FString> RigheTraccia(FName Oggetto);

	/**
	 * Solo sul server, dentro il dialogo (AValdorsoPNG::Scegli): gli eventi della storia di Lucia. Puo' cambiare il nodo
	 * dopo (la consegna senza tutte le cose va a "mancano"). Vero se l'evento era della grotta.
	 */
	VALDORSO_API bool EventoDialogo(FName Evento, APlayerController* Chi, FName& InOutProssimo);

	/** Solo sul server: dai a quel colono una traccia della storia (se non ce l'ha, e se la storia non e' finita). */
	VALDORSO_API bool DaiTraccia(APawn* Chi, FName Oggetto);

	/** Solo sul server: porta un colono in un punto, con lo schermo che si riapre dal nero. */
	VALDORSO_API void Porta(APawn* Chi, const FVector& Dove, float Gradi, const FText& Avviso);
}

/** La frana davanti al portale dei nani: chiusa, smossa (si scava), aperta (si entra). */
UCLASS()
class VALDORSO_API AValdorsoFrana : public AActor, public IValdorsoInterattivo
{
	GENERATED_BODY()

public:
	AValdorsoFrana();

	/** Dove si arriva entrando (la galleria sotto la montagna), e verso dove si guarda. */
	UPROPERTY(EditAnywhere, Category = "Frana")
	FVector Dentro = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, Category = "Frana")
	float GradiDentro = -90.f;

	static AValdorsoFrana* Di(const UWorld* Mondo);

	EValdorsoStatoFrana GetStato() const { return Stato; }
	float GetScavo() const { return Scavo; }

	/** Solo sul server: la notte in cui la terra trema (da chiusa a smossa). */
	void Trema();
	/** Solo sul server: la frana cede (o lo staff la apre). */
	void Apri();
	/** Solo sul server: lo staff la richiude (per le prove). */
	void Chiudi();

	// IValdorsoInterattivo
	virtual FText TestoInterazione(const APawn* Chi) const override;
	virtual void Interagisci(APawn* Chi) override;
	virtual FVector PuntoInterazione() const override { return GetActorLocation() + FVector(0.f, 0.f, 90.f); }

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;

private:
	void Carica();
	void Salva() const;
	FString File() const;
	/** Su ogni PC: i sassi che restano, e la frana vera (gli attori con l'etichetta "ValdorsoFrana") che sparisce aperta. */
	void Mostra();
	void Avvisa(APawn* Chi, const FText& Testo, float Secondi = 3.f) const;
	/** Solo sul server: un avviso a tutti i coloni entro Raggio (cm). */
	void AvvisaVicini(const FText& Testo, float Raggio) const;

	/** Su ogni PC: la terra che trema (la telecamera, il suono, la scritta). */
	UFUNCTION(NetMulticast, Reliable)
	void MulticastTrema(bool bCede);

	UFUNCTION()
	void OnRep_Frana();

	UPROPERTY(VisibleAnywhere, Category = "Frana")
	TObjectPtr<USceneComponent> Radice;

	UPROPERTY(VisibleAnywhere, Category = "Frana")
	TArray<TObjectPtr<UStaticMeshComponent>> Sassi;

	/** Dove la E trova la frana (i sassi spariscono quando si apre, la presa resta). */
	UPROPERTY(VisibleAnywhere, Category = "Frana")
	TObjectPtr<USphereComponent> Presa;

	UPROPERTY(ReplicatedUsing = OnRep_Frana)
	EValdorsoStatoFrana Stato = EValdorsoStatoFrana::Chiusa;

	UPROPERTY(ReplicatedUsing = OnRep_Frana)
	float Scavo = 0.f;

	// --- solo sul server
	TMap<TWeakObjectPtr<APawn>, double> UltimoColpo;
	int64 ApertaAlle = 0;
	int32 UltimoDecimo = 0;
};

/** Un passaggio: E porta da un'altra parte (dentro le gallerie: "Torna alla luce"). */
UCLASS()
class VALDORSO_API AValdorsoPassaggio : public AActor, public IValdorsoInterattivo
{
	GENERATED_BODY()

public:
	AValdorsoPassaggio();

	UPROPERTY(EditAnywhere, Category = "Passaggio")
	FVector Destinazione = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, Category = "Passaggio")
	float GradiDestinazione = 90.f;

	/** Cosa si legge ("Torna alla luce"). */
	UPROPERTY(EditAnywhere, Category = "Passaggio")
	FString Testo = TEXT("Torna alla luce");

	/** (11/11) Segreto: la E lo trova solo da vicinissimo (Vicino, cm), e solo chi ha l'iscrizione richiesta, se c'e'. */
	UPROPERTY(EditAnywhere, Category = "Passaggio")
	bool bSegreto = false;

	UPROPERTY(EditAnywhere, Category = "Passaggio")
	float Vicino = 130.f;

	UPROPERTY(EditAnywhere, Category = "Passaggio")
	FString IscrizioneRichiesta;

	virtual FText TestoInterazione(const APawn* Chi) const override;
	virtual void Interagisci(APawn* Chi) override;
	virtual FVector PuntoInterazione() const override { return GetActorLocation() + FVector(0.f, 0.f, 90.f); }

private:
	/** Dove la E trova il passaggio (invisibile). */
	UPROPERTY(VisibleAnywhere, Category = "Passaggio")
	TObjectPtr<USphereComponent> Presa;
};

/** Il forziere dei nani: si apre una volta al giorno per colono. */
UCLASS()
class VALDORSO_API AValdorsoForziere : public AActor, public IValdorsoInterattivo
{
	GENERATED_BODY()

public:
	AValdorsoForziere();

	/** Quanto e' ricco (1 normale, 2 quello della sala delle ossa). */
	UPROPERTY(EditAnywhere, Category = "Forziere", meta = (ClampMin = "1", ClampMax = "3"))
	int32 Ricchezza = 1;

	/** (11/11) Una trappola: aprirlo fa scattare l'AValdorsoTesoroAllarme piu' vicino (la grata, le ossa). */
	UPROPERTY(EditAnywhere, Category = "Forziere")
	bool bAllarme = false;

	virtual FText TestoInterazione(const APawn* Chi) const override;
	virtual void Interagisci(APawn* Chi) override;
	virtual FVector PuntoInterazione() const override { return GetActorLocation() + FVector(0.f, 0.f, 50.f); }

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(VisibleAnywhere, Category = "Forziere")
	TObjectPtr<UStaticMeshComponent> Cassa;

	/** Solo sul server: chi l'ha aperto, e quando (secondi Unix). */
	TMap<FString, int64> Aperto;
};

/** Una cosa della storia di Lucia da raccogliere (il taccuino di Gisella, la lanterna di Cecco). */
UCLASS()
class VALDORSO_API AValdorsoTraccia : public AActor, public IValdorsoInterattivo
{
	GENERATED_BODY()

public:
	AValdorsoTraccia();

	/** L'oggetto che si raccoglie (Taccuino_Gisella, Lanterna_Cecco). */
	UPROPERTY(EditAnywhere, Category = "Traccia")
	FName Oggetto;

	virtual FText TestoInterazione(const APawn* Chi) const override;
	virtual void Interagisci(APawn* Chi) override;
	virtual FVector PuntoInterazione() const override { return GetActorLocation() + FVector(0.f, 0.f, 30.f); }

private:
	UPROPERTY(VisibleAnywhere, Category = "Traccia")
	TObjectPtr<UStaticMeshComponent> Figura;

	/** Dove la E trova la cosa (la figura e' piccola e non ha collisione). */
	UPROPERTY(VisibleAnywhere, Category = "Traccia")
	TObjectPtr<USphereComponent> Presa;
};
