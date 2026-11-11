// Valdorso - I riti della valle (v0.1.7, parte B "riti", 11/11/2026). Scritto da Claude, dal Design MMO e dalla Lore
// (Proposta P16, da confermare: la stella di nascita e la pietra dei segni sono idee nuove).
//
// - LA PRIMA ORA NELLA VALLE: la prima volta che un personaggio entra, dopo qualche secondo, la valle lo accoglie: un
//   avviso con la sua stella di nascita (sotto quale stella ha firmato il Registro) e il suo motto. Una volta sola
//   (il segno "PrimaOra" nel file del personaggio). Nel registro eventi: "PrimaOra".
// - LA STELLA DI NASCITA: dodici stelle della valle, una per ogni dodicesimo dell'anno; la decide il giorno in cui il
//   colono ha firmato il Registro (FValdorsoRegistro::FirmatoIl). Sempre la stessa per lui.
// - LA CERIMONIA DELL'ANNO E UN GIORNO: quando sono passati un anno e un giorno (366 giorni veri) dalla firma, entrando
//   nella valle il colono riceve l'avviso "Padre Anselmo ti aspetta al tempio" (il segno "AnnoCompiuto"). Padre
//   Anselmo, al ritorno, ha una risposta in piu': il nome si legge davanti al Cuore e il colono diventa cittadino di
//   Valdorso (il segno "Cittadino"). Tutti i coloni nella valle lo sanno (un avviso), e resta nel registro eventi.
// - LA PIETRA DEI SEGNI (AValdorsoPietraSegni, accanto al tempio): ogni colono, una volta, ci incide il suo segno (il
//   nome, il giorno, la sua stella). Poi la E la legge: gli ultimi segni incisi. Resta nel file Saved/<..>/Segni.json
//   del server.
// Console (prove): Valdorso.Riti.PrimaOra (la prima ora di nuovo, a chi la lancia), Valdorso.Riti.Anno (a chi la lancia:
// come se fosse passato un anno e un giorno).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ValdorsoInterazione.h"
#include "ValdorsoRiti.generated.h"

class APlayerController;
class USphereComponent;
class UStaticMeshComponent;

/** Una stella di nascita. */
struct FValdorsoStella
{
	const TCHAR* Nome = TEXT("");
	const TCHAR* Motto = TEXT("");
};

/** Un segno inciso sulla pietra. */
struct FValdorsoSegnoInciso
{
	FString Nome;
	int64 Quando = 0;
	FString Stella;
};

namespace ValdorsoRiti
{
	/** I giorni (veri) della cerimonia: un anno e un giorno. */
	inline constexpr int32 GiorniCerimonia = 366;

	/** I nomi dei segni (dialogo "Riti" e dialogo di Padre Anselmo) e l'evento della risposta. */
	inline const TCHAR* IdRiti = TEXT("Riti");
	inline const TCHAR* IdSacerdote = TEXT("PadreAnselmo");
	inline const TCHAR* SegnoPrimaOra = TEXT("PrimaOra");
	inline const TCHAR* SegnoAnno = TEXT("AnnoCompiuto");
	inline const TCHAR* SegnoCittadino = TEXT("Cittadino");
	inline const TCHAR* EventoCerimonia = TEXT("Cerimonia");

	/** Le dodici stelle, in ordine (dal primo giorno dell'anno). */
	VALDORSO_API const TArray<FValdorsoStella>& Stelle();

	/** La stella di chi ha firmato in quel momento (secondi dal 1970, UTC); 0 = oggi. */
	VALDORSO_API const FValdorsoStella& StellaDi(int64 FirmatoIl);

	/** Vero se da FirmatoIl ad Adesso (secondi, UTC) sono passati un anno e un giorno. */
	VALDORSO_API bool AnnoEUnGiorno(int64 FirmatoIl, int64 Adesso);

	/** Solo sul server, quando un personaggio entra nella valle (AValdorsoPlayerController::FaiNascere). */
	VALDORSO_API void Entra(APlayerController* Chi, int64 FirmatoIl);

	/** Solo sul server: le risposte dei dialoghi con un evento dei riti (la cerimonia); puo' cambiare il nodo dopo. */
	VALDORSO_API bool EventoDialogo(FName Evento, APlayerController* Chi, AActor* Sacerdote, FName& InOutProssimo);

	/** Aggiunge un segno all'elenco se quel nome non c'e' ancora (vero se l'ha aggiunto). */
	VALDORSO_API bool Incidi(TArray<FValdorsoSegnoInciso>& Segni, const FString& Nome, int64 Quando, const FString& Stella);

	/** Le righe da leggere sulla pietra: le ultime Quante, dalla piu' nuova. */
	VALDORSO_API TArray<FString> Righe(const TArray<FValdorsoSegnoInciso>& Segni, int32 Quante);
}

UCLASS()
class VALDORSO_API AValdorsoPietraSegni : public AActor, public IValdorsoInterattivo
{
	GENERATED_BODY()

public:
	AValdorsoPietraSegni();

	virtual FText TestoInterazione(const APawn* Chi) const override;
	virtual void Interagisci(APawn* Chi) override;
	virtual FVector PuntoInterazione() const override { return GetActorLocation() + FVector(0.f, 0.f, 90.f); }

protected:
	virtual void BeginPlay() override;

private:
	void Leggi();
	void Scrivi() const;
	FString File() const;

	UPROPERTY(VisibleAnywhere, Category = "Pietra dei segni")
	TObjectPtr<UStaticMeshComponent> Pietra;

	UPROPERTY(VisibleAnywhere, Category = "Pietra dei segni")
	TObjectPtr<USphereComponent> Presa;

	/** Solo sul server. */
	TArray<FValdorsoSegnoInciso> Segni;
	bool bLetti = false;
	/** Il file c'e' ma non si legge: non lo si riscrive sopra. */
	bool bIlleggibile = false;
};
