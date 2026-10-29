// Valdorso - Il palco del Registro di Val d'Orso (v0.1.2, passo 4.2b, prima parte). Scritto da Claude il 05/10/2026.
//
// Mentre il giocatore compila il Registro, a destra della pergamena si vede il suo colono, in un ritratto: il
// manichino (uomo o donna, secondo la risposta) che respira sul cerchio di rune, con il frammento del Cuore alle
// spalle e due bracieri; una luce calda davanti e, dietro, una luce del colore dell'elemento della sua fede.
// Il palco esiste solo sul PC del giocatore (non va in rete), molto in alto sopra la mappa: una telecamera
// (SceneCapture) lo disegna in un'immagine che la schermata del Registro mostra. Vede solo gli attori del palco,
// quindi lo sfondo è nero e la luce del giorno della mappa non entra.
// Alla firma il frammento e le rune si risvegliano. Gli abiti del mestiere e l'aspetto vero arrivano con Mutable (4.4).
//
// Comandi della console (per ritoccare la luce senza ricompilare):
//   Valdorso.Ritratto.Esposizione <numero>   correzione dell'esposizione (0 di serie; +1 più chiaro, -1 più scuro)

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ValdorsoRegistro.h"
#include "ValdorsoPalcoRegistro.generated.h"

class USceneComponent;
class USkeletalMeshComponent;
class USkeletalMesh;
class UAnimSequence;
class USceneCaptureComponent2D;
class UPointLightComponent;
class UTextureRenderTarget2D;
class UValdorsoAspettoComponent;

UCLASS(NotPlaceable)
class VALDORSO_API AValdorsoPalcoRegistro : public AActor
{
	GENERATED_BODY()

public:
	AValdorsoPalcoRegistro();

	/** L'immagine in cui la telecamera del palco disegna il ritratto (nasce in BeginPlay). */
	UTextureRenderTarget2D* GetRitratto() const { return Ritratto; }

	/** Le risposte sono cambiate: uomo o donna, colore della fede. */
	void Aggiorna(const FValdorsoRegistro& Risposte);

	/** (05/10) Un personaggio diverso sul palco (scelta del personaggio, apertura del Registro): si gira verso di te. */
	void Presenta(const FValdorsoRegistro& Risposte);

	/** (28/10) Primo piano sul volto (pagina del volto del Registro) o di nuovo il colono intero. */
	void PrimoPiano(bool bSulVolto);

	/** (29/10) Il giocatore gira il colono trascinando il ritratto (gradi in più; bDiFronte = torna di fronte). */
	void Gira(float Gradi, bool bDiFronte);

	/** Il registro è firmato: il frammento batte più forte e le rune si accendono. */
	void Firmato();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(VisibleAnywhere, Category = "Valdorso|Palco")
	TObjectPtr<USceneComponent> Radice;

	UPROPERTY(VisibleAnywhere, Category = "Valdorso|Palco")
	TObjectPtr<USkeletalMeshComponent> Colono;

	UPROPERTY(VisibleAnywhere, Category = "Valdorso|Palco")
	TObjectPtr<USceneCaptureComponent2D> Obiettivo;

	/** Calda, davanti e un poco a sinistra (come un braciere vicino). */
	UPROPERTY(VisibleAnywhere, Category = "Valdorso|Palco")
	TObjectPtr<UPointLightComponent> LuceChiave;

	/** Dietro il colono, del colore della sua fede. */
	UPROPERTY(VisibleAnywhere, Category = "Valdorso|Palco")
	TObjectPtr<UPointLightComponent> LuceFede;

	/** (28/10) Il colono vero: il MetaHuman con la testa e i cursori del Registro, sopra il manichino nascosto. */
	UPROPERTY(VisibleAnywhere, Category = "Valdorso|Palco")
	TObjectPtr<UValdorsoAspettoComponent> Aspetto;

	/** Debole e fredda, perché le ombre non siano nere del tutto. */
	UPROPERTY(VisibleAnywhere, Category = "Valdorso|Palco")
	TObjectPtr<UPointLightComponent> LuceRiempimento;

private:
	void PreparaScena();
	void CambiaSesso(const FString& Sesso);

	UPROPERTY()
	TObjectPtr<UTextureRenderTarget2D> Ritratto;

	UPROPERTY()
	TObjectPtr<USkeletalMesh> CorpoUomo;

	UPROPERTY()
	TObjectPtr<USkeletalMesh> CorpoDonna;

	UPROPERTY()
	TObjectPtr<UAnimSequence> Riposo;

	/** Cerchio di rune, frammento e bracieri del palco (si distruggono con il palco). */
	UPROPERTY()
	TArray<TObjectPtr<AActor>> Scena;

	FString SessoAttuale;
	FLinearColor ColoreFede;
	FLinearColor ColoreFedeVoluto;
	float Tempo = 0.f;
	/** Gradi che mancano perché il colono guardi la telecamera (si gira piano fino a 0). */
	float GiroDaFare = 0.f;

	/** Quanto il giocatore ha girato il colono (gradi). */
	float GiroGiocatore = 0.f;
	/** 0 = colono intero, 1 = primo piano sul volto (si passa piano dall'uno all'altro). */
	float Vicino = 0.f;
	float VicinoVoluto = 0.f;
};
