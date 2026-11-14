// Valdorso - Il Banditore della Corona: la Gazzetta del passo e gli annunci dello staff (13/11/2026, idea approvata da
// Fra: "la Gazzetta, con revisione dello staff fuori dal gioco; poi nel gioco la proclama un ente; e lo stesso per gli
// annunci dei gamemaster"). Scritto da Claude.
//
// Come funziona:
// 1. Fuori dal gioco: Tools\Gazzetta\gazzetta.py compone la bozza della settimana dal registro eventi
//    (Saved/Server/Eventi) in Saved/Server/Gazzetta/Bozza.txt; lo staff la legge e la corregge con il Blocco note;
//    poi "approva" la mette in Saved/Server/Gazzetta/Approvate come Gazzetta_<numero>.json.
// 2. Nel gioco: il Banditore (questo, solo sul server) guarda la cartella Approvate ogni 30 secondi. Ogni foglio nuovo
//    lo proclama: un proclama sulla bacheca della piazza ("Il Banditore della Corona: ...") e un avviso a tutti i coloni
//    nella valle. Si ricorda quelli già proclamati (Proclamati.txt); le prove dell'editor e il gioco da soli hanno una cartella loro.
// 3. Gli annunci dello staff: dalla finestra F9 (scheda Bacheche) o con Tools\Gazzetta "annuncio": scritti nella stessa
//    cartella, proclamati subito.
// 4. Il sito e il launcher: "Pubblica Valdorso" (scelta 2) prende i fogli approvati e li mette tra le notizie.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Engine/TimerHandle.h"
#include "ValdorsoBanditore.generated.h"

UCLASS()
class VALDORSO_API UValdorsoBanditore : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	/** Quello del mondo di Contesto (nullptr sul PC di chi gioca con un server, o fuori dalla valle). */
	static UValdorsoBanditore* Di(const UObject* Contesto);

	/** Saved/Server/Gazzetta (la bozza, i fogli approvati, quelli proclamati). Nelle prove dell'editor Saved/Prova/Gazzetta,
	 *  con l'archivio da soli Saved/Solo/Gazzetta (come le bacheche): le prove non toccano i fogli del server vero. */
	FString Cartella() const;

	/** Lo staff: un annuncio a tutta la valle, subito (scritto anche tra gli approvati, così va sul sito). Restituisce
	 *  cosa dire a chi l'ha scritto. */
	FString Annuncia(const FString& Testo, const FString& Autore);

	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

private:
	/** Guarda i fogli approvati e proclama quelli nuovi. */
	void Guarda();

	/** Un proclama sulla bacheca e l'avviso a tutti. */
	void Proclama(const FString& Riassunto, const FString& Avviso, const FString& Ente, const FString& Autore, const FString& Tipo);

	FString FileProclamati() const;

	TSet<FString> Proclamati;
	bool bAttivo = false;
	FTimerHandle Orologio;
};
