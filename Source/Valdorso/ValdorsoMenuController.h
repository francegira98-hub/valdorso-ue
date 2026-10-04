// Valdorso - Il controllore del menu principale: mostra il menu "Oro e brace", guarda la scena
// dalla telecamera con l'etichetta MenuCamera (che ondeggia piano), e porta nella valle o chiude il gioco.
//
// L'ingresso nella valle (v0.1.2, passo 2.5, 04/10/2026): prima il volo, poi il collegamento.
//   - Ogni volta: il volo corto verso il frammento (2,4 s), il nero, e solo allora parte il collegamento al server.
//   - Al primo ingresso (con l'invito): il volo lungo lungo la curva AValdorsoVoloIngresso (circa 7 s),
//     con "La valle ti accoglie" e il Cuore che batte più forte; si salta con Esc.
//   Sul nero, mentre ci si collega: "In cammino verso la valle...". Se il server non risponde si torna al menu
//   con il motivo (lo fa l'istanza del gioco, ValdorsoGameInstance).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "ValdorsoGameInstance.h"
#include "ValdorsoMenuController.generated.h"

class SValdorsoMenuPrincipale;
class SValdorsoPrimaDiEntrare;
class SValdorsoVolo;
class AValdorsoVoloIngresso;
class UCameraComponent;

UCLASS()
class VALDORSO_API AValdorsoMenuController : public APlayerController
{
	GENERATED_BODY()

public:
	AValdorsoMenuController();

	/** Il livello che si apre con "Entra nella valle" (per ora la mappa di prova; dalla v0.1.3 la valle). */
	UPROPERTY(EditDefaultsOnly, Category = "Valdorso|Menu")
	FName LivelloValle = TEXT("/Game/ThirdPerson/Lvl_ThirdPerson");

	/** Quanto dura il volo verso il frammento prima di entrare nella valle, in secondi. */
	UPROPERTY(EditDefaultsOnly, Category = "Valdorso|Menu")
	float DurataTransizione = 2.4f;

	/** Al primo ingresso il Cuore batte più forte: a ogni colpo lo sguardo si stringe di questi gradi. */
	UPROPERTY(EditDefaultsOnly, Category = "Valdorso|Menu")
	float ColpoSulloSguardo = 1.6f;

	/** Quanto ondeggia la telecamera, in centimetri. */
	UPROPERTY(EditDefaultsOnly, Category = "Valdorso|Menu")
	float Ondeggio = 14.f;

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;

private:
	void EntraNellaValle();
	void Esci();

	// "Prima di entrare" (v0.1.2, passo 2.2): il pannello dell'accesso sopra il menu.
	void MostraAccesso(const FText& Messaggio);
	void ChiudiAccesso();
	void SuRichiestaAccesso(const FValdorsoRichiestaAccesso& Richiesta);
	void ProvaLocale();
	void Arriva();

	/** Se il collegamento non può partire (dopo il nero): si riapre il menu con il motivo. */
	void TornaAlMenu(const FString& Motivo);

	TSharedPtr<SValdorsoPrimaDiEntrare> Accesso;

	/** Vero: alla fine del volo ci si collega al server; falso: si apre la mappa di prova in locale. */
	bool bVersoIlServer = false;

	/** La richiesta aspetta qui la fine del volo; poi va all'istanza del gioco e qui si cancella. */
	TOptional<FValdorsoRichiestaAccesso> RichiestaDaMandare;

	TSharedPtr<SValdorsoMenuPrincipale> Menu;
	TWeakObjectPtr<AActor> Telecamera;
	FVector PosizioneBase = FVector::ZeroVector;
	FRotator RotazioneBase = FRotator::ZeroRotator;
	float Tempo = 0.f;

	/** "Entra nella valle": la telecamera vola verso il frammento, il menu sfuma, tutto diventa nero. */
	void AvanzaTransizione(float DeltaSeconds);

	/** Il volo lungo del primo ingresso, lungo la curva. */
	void AvanzaVoloLungo(float DeltaSeconds);
	void PreparaVoloLungo();
	void SaltaVolo();

	/** Dopo il nero, mentre ci si collega: la riga "In cammino verso la valle...". */
	void AvanzaCollegamento(float DeltaSeconds);

	TSharedPtr<SValdorsoVolo> Scritte;
	TWeakObjectPtr<AValdorsoVoloIngresso> Volo;
	TWeakObjectPtr<UCameraComponent> Obiettivo;
	float SguardoBase = 50.f;
	bool bVoloLungo = false;
	bool bSaltato = false;
	float AttesaSalto = 0.f;
	float TempoCollegamento = 0.f;

	bool bInTransizione = false;
	bool bBuio = false;
	bool bLivelloAperto = false;
	float TempoTransizione = 0.f;
	FVector Partenza = FVector::ZeroVector;
	FQuat GiroPartenza = FQuat::Identity;
	FVector Arrivo = FVector::ZeroVector;
	FQuat GiroArrivo = FQuat::Identity;
};
