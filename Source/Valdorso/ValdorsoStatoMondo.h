// Valdorso - Lo stato del mondo che il server racconta (07/11/2026, v0.1.3, scritto da Claude mentre Fra dormiva).
//
// Dal Design ("Il menu vivo"): il menu mostra lo stato del mondo dal server (ora, tempo, stagione, ultima notizia).
// Come funziona:
//   - Sul server (AValdorsoStatoMondo, lo fa nascere la modalità di gioco) ogni 10 secondi si mette insieme lo stato:
//     versione, ora della valle, tempo, stagione, quanti coloni ci sono, l'ultima notizia dello staff.
//   - Lo scrive in Saved/Server/StatoMondo.json e lo dà a chi lo chiede all'indirizzo http://<server>:7780/stato
//     (una pagina di sola lettura, senza nomi né dati delle persone: solo numeri e la notizia).
//   - Il menu del gioco e il launcher lo chiedono e lo scrivono in basso ("Nella valle sono le 18:40 · sereno...").
//     Se il server non risponde: "La valle dorme: il server non risponde."
//   - La notizia la scrive lo staff nel file Saved/Server/Notizie.txt sul PC del server: vale l'ultima riga scritta
//     (le righe che iniziano con # non contano). Si cambia mentre il server gira.
// Sul server, la porta 7780 (TCP) va aperta nel firewall, come la 7777 (UDP) del gioco.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HttpRouteHandle.h"
#include "ValdorsoStatoMondo.generated.h"

class IHttpRouter;

/** Lo stato del mondo, come viaggia (JSON) tra server, menu e launcher. */
struct VALDORSO_API FValdorsoStatoMondo
{
	FString Versione;
	FString NomeVersione;
	/** L'ora della valle (0..24); -1 se non si sa. */
	float Ora = -1.f;
	/** "Sereno", "Nuvoloso", "Nebbia", "Pioggia", "Temporale" (vuoto se non si sa). */
	FString Tempo;
	/** "primavera", "estate", "autunno", "inverno". */
	FString Stagione;
	int32 Coloni = 0;
	int32 Posti = 0;
	FString Notizia;
	/** Quando il server l'ha scritto (ISO 8601, UTC). */
	FString Aggiornato;
	/** Falso quando lo staff chiude la valle per manutenzione (più avanti dal pannello staff). */
	bool bAperto = true;
};

namespace ValdorsoStatoMondo
{
	inline constexpr int32 PortaDiSerie = 7780;

	VALDORSO_API FString InJson(const FValdorsoStatoMondo& Stato);
	VALDORSO_API bool DaJson(const FString& Testo, FValdorsoStatoMondo& OutStato);

	/** "18:40" (con lo zero davanti: "07:05"). */
	VALDORSO_API FString OraLeggibile(float Ora);

	/** La stagione dal mese del calendario vero (per ora la valle segue l'anno vero: novembre = autunno). */
	VALDORSO_API FString StagioneDi(const FDateTime& Quando);

	/** La riga per il menu: "Nella valle sono le 18:40 · sereno · autunno · 3 coloni". */
	VALDORSO_API FText Riga(const FValdorsoStatoMondo& Stato);

	/** "http://host:7780/stato" dall'indirizzo del gioco ("host:7777"). Vuoto se l'indirizzo è vuoto. */
	VALDORSO_API FString IndirizzoStato(const FString& IndirizzoServer, int32 Porta = PortaDiSerie);

	/** L'ultima notizia del file dello staff: l'ultima riga non vuota che non inizia con #, al massimo 200 lettere. */
	VALDORSO_API FString NotiziaDaFile(const FString& Testo);
}

UCLASS(Config = Game)
class VALDORSO_API AValdorsoStatoMondo : public AActor
{
	GENERATED_BODY()

public:
	AValdorsoStatoMondo();

	/** La porta della pagina dello stato (TCP). */
	UPROPERTY(Config, EditAnywhere, Category = "Stato del mondo")
	int32 PortaStato = ValdorsoStatoMondo::PortaDiSerie;

	/** Di serie la pagina si apre solo sul server dedicato; vero = anche in Play nell'editor (per le prove). */
	UPROPERTY(Config, EditAnywhere, Category = "Stato del mondo")
	bool bServiAncheInProva = false;

	/** Ogni quanti secondi si rifà lo stato. */
	UPROPERTY(EditAnywhere, Category = "Stato del mondo", meta = (ClampMin = "2"))
	float Intervallo = 10.f;

	/** Lo stato di adesso (solo sul server). */
	const FValdorsoStatoMondo& Stato() const { return Ultimo; }

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void Componi();
	void ApriPagina();
	void ChiudiPagina();

	FValdorsoStatoMondo Ultimo;
	/** Il JSON pronto da dare (condiviso con la pagina, che può rispondere anche dopo che l'attore è sparito). */
	TSharedPtr<FString, ESPMode::ThreadSafe> JsonPronto;
	TSharedPtr<IHttpRouter> Router;
	FHttpRouteHandle Maniglia;
	float Attesa = 0.f;
};
