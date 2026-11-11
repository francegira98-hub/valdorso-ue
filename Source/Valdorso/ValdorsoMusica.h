// Valdorso - La musica della valle (v0.1.7, parte C, 11/11/2026). Scritto da Claude; i brani (provvisori) li fa Fra.
//
// Solo sul PC di chi gioca (un componente del PlayerController): sceglie il brano che va con il momento e lo cambia
// piano (dissolvenza di qualche secondo):
//   COMBATTIMENTO - c'e' un bersaglio agganciato, o hai colpito da poco, o ti hanno ferito da poco (12 s);
//   TEMPIO        - dentro il Tempio del Cuore (vicino a Padre Anselmo, sotto il tetto);
//   NOTTE         - dalle 20 alle 6 della valle;
//   GIORNO        - il resto.
// I brani stanno in /Game/Valle/Musica: M_Giorno, M_Notte, M_Tempio, M_Combattimento (li importa
// Content/Python/nuovi/importa_musica.py da V:\Texture\valdorso\musica\*.wav). Se un brano manca, quel momento resta
// in silenzio. Come nei giochi di ruolo: giorno, notte e tempio suonano e poi tacciono un poco (40-100 s) prima di
// ricominciare; il combattimento suona di fila finche' dura. Il volume e' quello della musica nelle impostazioni.
// Console (prove): Valdorso.Musica.Stato (quale brano adesso e perche').

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ValdorsoMusica.generated.h"

class UAudioComponent;
class USoundBase;

UENUM()
enum class EValdorsoMomentoMusica : uint8
{
	Silenzio,
	Giorno,
	Notte,
	Tempio,
	Combattimento,
};

namespace ValdorsoMusica
{
	/** Il momento della musica (puro, per i test): Ora 0..24 della valle. */
	VALDORSO_API EValdorsoMomentoMusica Momento(bool bCombattimento, bool bTempio, float Ora);

	/** Il nome del momento ("giorno"...). */
	VALDORSO_API const TCHAR* Nome(EValdorsoMomentoMusica Momento);
}

UCLASS(ClassGroup = (Valdorso))
class VALDORSO_API UValdorsoMusicaComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UValdorsoMusicaComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Per la console: com'e' adesso. */
	FString Racconta() const;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	bool InCombattimento(double Ora);
	bool NelTempio() const;
	void Suona(EValdorsoMomentoMusica Nuovo, double Ora);
	USoundBase* Brano(EValdorsoMomentoMusica Quale);

	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> Uno;

	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> Due;

	UPROPERTY(Transient)
	TMap<EValdorsoMomentoMusica, TObjectPtr<USoundBase>> Brani;

	TSet<EValdorsoMomentoMusica> Mancanti;
	EValdorsoMomentoMusica Adesso = EValdorsoMomentoMusica::Silenzio;
	bool bSuUno = true;
	double Controllo = 0.0;
	double ProssimoGiro = 0.0;
	double CombattimentoFino = -1.0;
	float SaluteVista = -1.f;
	float VolumeUsato = -1.f;
	TWeakObjectPtr<AActor> Tempio;
	double CercatoTempio = -1000.0;
};
