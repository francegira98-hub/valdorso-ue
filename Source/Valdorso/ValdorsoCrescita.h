// Valdorso - Crescere nel mondo: le abilita' di un colono (scheletro della v0.1.5, 10/11/2026). Scritto da Claude. Le regole
// sono in ValdorsoAbilitaDati.h, la scheda del personaggio in SValdorsoScheda.h (tasto K).
//
// UValdorsoCrescitaComponent sta sul PlayerController: le abilita' le tiene il server e arrivano solo al colono. Si salvano
// con il personaggio (FValdorsoPersonaggio::Abilita), un minuto dopo l'ultimo cambiamento (e uscendo).
// Cosa le fa salire, per ora:
//   - Atletica: correre (ogni 10 m di corsa); Nuoto: nuotare (ogni 8 m, di piu' sott'acqua) - li guarda il tick;
//   - le armi (Lotta a mani nude, Pugnali, Spade, Asce, Mazze, Aste): colpire qualcosa (ValdorsoCombattimento::Colpisci);
//     contro un manichino rende poco, e lo stesso bersaglio di fila sempre meno;
//   - Tempra: prendere colpi; Scudo: pararli; Caccia: scuoiare.
// Il momento critico (per evolversi alla soglia): la salute sotto il 40%.
// Comandi di console (sul server, o da soli): Valdorso.Crescere.Imposta <Abilita> <punti> [nome],
// Valdorso.Crescere.Elenco [nome], Valdorso.Crescere.Critico 1 (per le prove: ogni momento e' critico).

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ValdorsoAbilitaDati.h"
#include "ValdorsoCrescita.generated.h"

class APlayerController;
class SValdorsoScheda;
struct FValdorsoRegistro;

UCLASS(ClassGroup = (Valdorso))
class VALDORSO_API UValdorsoCrescitaComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UValdorsoCrescitaComponent();

	static UValdorsoCrescitaComponent* Di(const APlayerController* Controllore);

	/** Quello di chi comanda quel corpo (nullptr per i lupi, i PNG, i manichini). */
	static UValdorsoCrescitaComponent* DelCorpo(const AActor* Corpo);

	/** Le abilita' (sul server quelle vere, sul PC quelle arrivate). */
	const FValdorsoAbilitaColono& Contenuto() const { return Abilita; }

	/** Cresce a ogni cambiamento (sul PC): la scheda la guarda. */
	int32 Cambiamenti() const { return NumeroCambiamenti; }

	// --- Sul server ---------------------------------------------------------------------------------

	/** Il personaggio entra: le abilita' salvate, e (la prima volta) i vantaggi del Registro. */
	void Carica(const FValdorsoAbilitaColono& Salvato, const FValdorsoRegistro* Registro);
	FValdorsoAbilitaColono Dati() const { return Abilita; }

	/**
	 * Un uso di un'abilita' (Difficolta' 0..2, 1 = un rischio vero). Bersaglio: con che cosa (lo stesso di fila rende meno).
	 * Solo sul server; fuori dal personaggio (nel menu) non fa niente.
	 */
	void Usa(FName Id, float Difficolta, const UObject* Bersaglio = nullptr, bool bPericolo = false);

	/** Comodo: un uso da parte di chi comanda quel corpo (se e' un colono). */
	static void UsaDa(const AActor* Corpo, FName Id, float Difficolta, const UObject* Bersaglio = nullptr, bool bPericolo = false);

	/**
	 * (10/11) Quanto e' forte Chi (0..100): una creatura (per specie), un abitante (per dialogo), un colono (le sue
	 * abilita': sul server di tutti, sul PC solo il proprio). Falso se non si sa (un altro colono, visto dal PC).
	 */
	static bool ForzaDi(const AActor* Chi, float& OutForza);

	/** Per lo staff e le prove. */
	bool ImpostaPunti(FName Id, float Punti, const FString& Da);

	// --- Dal PC del giocatore (il server controlla) ------------------------------------------------------

	/** Le frecce della scheda: 0 sale, 1 ferma, 2 cala. */
	UFUNCTION(Server, Reliable)
	void ServerVerso(FName Id, uint8 Verso);

	/** La scheda aperta (solo sul PC del giocatore, ValdorsoSchedaSchermo). */
	TSharedPtr<SValdorsoScheda> Schermata;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	/** Server: e' cambiato qualcosa; si salva fra poco. */
	void Cambiato();
	/** Server: la salute del colono e' bassa (o il comando delle prove)? */
	bool MomentoCritico() const;
	void Avvisa(const FText& Testo, float Secondi) const;
	/** Server: corsa e nuoto. */
	void Movimento();

	UFUNCTION()
	void OnRep_Abilita();

	UPROPERTY(ReplicatedUsing = OnRep_Abilita)
	FValdorsoAbilitaColono Abilita;

	bool bPersonaggio = false;
	bool bDaSalvare = false;
	double SalvaDopo = 0.0;
	double UltimaRichiesta = 0.0;
	double UltimoAvvisoTetto = -1000.0;
	/** Server: quante volte di fila lo stesso bersaglio (per abilita' e per bersaglio), e fino a quando conta. */
	struct FRipetizione
	{
		int32 Volte = 0;
		double Fino = 0.0;
	};
	TMap<FName, TMap<TWeakObjectPtr<const UObject>, FRipetizione>> Ripetizioni;
	/** Server: i metri corsi e nuotati non ancora contati, e dove era il corpo. */
	float Corsi = 0.f;
	float Nuotati = 0.f;
	FVector Prima = FVector::ZeroVector;
	bool bPrima = false;
	int32 NumeroCambiamenti = 0;
};
