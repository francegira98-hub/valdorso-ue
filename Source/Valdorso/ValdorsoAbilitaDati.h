// Valdorso - Crescere: le abilita' che salgono usandole (scheletro della v0.1.5, 10/11/2026). Scritto da Claude, chiesto da
// Fra: "inizia a mettere scheletri". Le regole sono quelle del Design completo:
//   - non esistono classi: ogni abilita' sale da 0 a 100 usandola nel mondo, e il personaggio diventa cio' che fa;
//   - ogni uso utile da' esperienza; gli usi ripetuti senza rischio (sempre lo stesso manichino) rendono sempre meno;
//   - le soglie 25, 50, 75 e 100: arrivata li' l'abilita' si ferma e "si carica"; carica, si evolve solo in un momento
//     critico (la salute bassa, un nemico che puo' ucciderti), e la voce del Cuore annuncia il nuovo nome;
//   - il tetto: la somma di tutte le abilita' arriva al massimo a 700. Ognuna si puo' far salire, fermare o calare (come
//     le frecce della scheda): oltre il tetto, per salire ne cala una messa a calare; se nessuna cala, non si sale;
//   - i vantaggi del Registro: "Cosa facevi prima?" da' 5 punti nell'abilita' del vecchio mestiere, e chi ha sentito il
//     richiamo della magia 5 punti in Concentrazione;
//   - le abilita' sono dati (la tabella in ValdorsoAbilitaDati.cpp): aggiungerne una non tocca il sistema.
// Qui ci sono solo dati e regole (si provano nei test, Valdorso.Crescere.*). Chi le usa nel mondo e' ValdorsoCrescita.h; la
// scheda del personaggio e' SValdorsoScheda.h (tasto K).

#pragma once

#include "CoreMinimal.h"
#include "ValdorsoAbilitaDati.generated.h"

struct FValdorsoRegistro;

/** Che fa un'abilita' usandola: sale, resta ferma, o cala per fare spazio alle altre. */
UENUM()
enum class EValdorsoVerso : uint8
{
	Sale,
	Ferma,
	Scende,
};

/** Un'abilita' di un colono. */
USTRUCT()
struct FValdorsoAbilitaValore
{
	GENERATED_BODY()

	UPROPERTY()
	FName Id;

	/** 0..100. */
	UPROPERTY()
	float Punti = 0.f;

	UPROPERTY()
	EValdorsoVerso Verso = EValdorsoVerso::Sale;

	/** Quante soglie ha passato (0..4: 25, 50, 75, 100). Il nome dell'abilita' cambia con le evoluzioni. */
	UPROPERTY()
	int32 Evoluzioni = 0;

	/** Ferma a una soglia: quanto e' carica (0..1). Carica, aspetta il momento critico. */
	UPROPERTY()
	float Carica = 0.f;
};

/** Le abilita' di un colono (si salva con il personaggio: FValdorsoPersonaggio::Abilita). */
USTRUCT()
struct FValdorsoAbilitaColono
{
	GENERATED_BODY()

	UPROPERTY()
	int32 Versione = 1;

	UPROPERTY()
	TArray<FValdorsoAbilitaValore> Valori;

	/** Vero quando i vantaggi del Registro sono gia' stati dati (una volta sola). */
	UPROPERTY()
	bool bVantaggiDati = false;
};

/** Le famiglie, per mettere in ordine la scheda. */
enum class EValdorsoFamigliaAbilita : uint8
{
	Corpo,
	Armi,
	Mondo,
	Mestieri,
	Spirito,
};

/** Un'abilita' del catalogo. */
struct FValdorsoAbilitaTipo
{
	FName Id;
	EValdorsoFamigliaAbilita Famiglia = EValdorsoFamigliaAbilita::Corpo;
	/** Il nome a ogni evoluzione: [0] da 0 a 24, [1] dopo la soglia 25, ... [4] dopo la soglia 100. */
	const TCHAR* Nomi[5] = { TEXT(""), TEXT(""), TEXT(""), TEXT(""), TEXT("") };
	const TCHAR* Descrizione = TEXT("");
	/** Cosa si sblocca a ogni soglia (25, 50, 75, 100): tecniche attive e talenti (arrivano con la v0.1.5). */
	const TCHAR* Sblocchi[4] = { TEXT(""), TEXT(""), TEXT(""), TEXT("") };
	/** Vero se nel mondo non c'e' ancora niente che la fa salire (i mestieri, la magia): si vede nella scheda, ma ferma. */
	bool bInArrivo = false;
};

/** Cosa e' successo usando un'abilita' (il componente lo dice al colono). */
struct FValdorsoEsitoUso
{
	/** Quanto e' salita (0 se ferma, al tetto, o senza spazio). */
	float Guadagno = 0.f;
	/** Vero se ha passato un numero intero (37.9 -> 38.0): la scheda si aggiorna, nessun avviso. */
	bool bPuntoIntero = false;
	/** Vero se alla soglia si e' caricata un poco (la scheda si aggiorna). */
	bool bCaricata = false;
	/** Vero se si e' appena caricata del tutto a una soglia (si avvisa una volta). */
	bool bPronta = false;
	/** Vero se si e' evoluta (la voce del Cuore): Evoluzione e' il nuovo grado (1..4). */
	bool bEvoluta = false;
	int32 Evoluzione = 0;
	/** L'abilita' calata per fare spazio (oltre il tetto), se c'e'. */
	FName Calata;
	/** Vero se non e' salita perche' il tetto e' pieno e nessuna e' messa a calare (si avvisa ogni tanto). */
	bool bTettoPieno = false;
};

namespace ValdorsoCrescere
{
	/** Il massimo di un'abilita' e il tetto della somma di tutte. */
	inline constexpr float Massimo = 100.f;
	inline constexpr float Tetto = 700.f;
	/** Ogni quanto c'e' una soglia. */
	inline constexpr float Passo = 25.f;
	/** Quanto rende chi ha rischiato davvero (Difficolta' 1); un uso facile ne rende meno, uno difficile di piu' (fino a 2). */
	inline constexpr float GuadagnoBase = 0.35f;
	/** Quanto si carica a ogni uso alla soglia (in punti "spesi": servono circa 3 punti d'uso per caricarsi). */
	inline constexpr float CaricaPerPunto = 1.f / 3.f;
	/** I punti del vecchio mestiere e del richiamo della magia. */
	inline constexpr float PuntiVantaggio = 5.f;

	/** Il catalogo, nell'ordine della scheda. */
	VALDORSO_API const TArray<FValdorsoAbilitaTipo>& Tutte();
	VALDORSO_API const FValdorsoAbilitaTipo* Trova(FName Id);

	/** Il nome di un'abilita' con quante evoluzioni ha (vuoto se non c'e'). */
	VALDORSO_API FString Nome(FName Id, int32 Evoluzioni);

	/** Il nome della famiglia (per la scheda). */
	VALDORSO_API FText NomeFamiglia(EValdorsoFamigliaAbilita Famiglia);

	/** L'abilita' di un'arma della mano destra (NAME_None, o un oggetto che non e' un'arma: la Lotta, a mani nude). */
	VALDORSO_API FName AbilitaDellArma(FName IdOggetto);

	/** L'abilita' del vecchio mestiere del Registro (NAME_None se nessuna). */
	VALDORSO_API FName AbilitaDelMestiere(const FString& Mestiere);

	/** Quanto vale (0 se il colono non l'ha mai usata). */
	VALDORSO_API float Punti(const FValdorsoAbilitaColono& Colono, FName Id);
	VALDORSO_API const FValdorsoAbilitaValore* Valore(const FValdorsoAbilitaColono& Colono, FName Id);
	/** Per scriverla: se il colono non l'ha ancora, la aggiunge (a 0). */
	VALDORSO_API FValdorsoAbilitaValore& Prendi(FValdorsoAbilitaColono& Colono, FName Id);

	/** La somma di tutte. */
	VALDORSO_API float Totale(const FValdorsoAbilitaColono& Colono);

	/** La prossima soglia (25, 50, 75, 100) di chi ha quelle evoluzioni; 0 se le ha passate tutte. */
	VALDORSO_API float Soglia(int32 Evoluzioni);

	/**
	 * Quanto rende un uso: Difficolta' 0..2 (1 = un rischio vero), Ripetizioni = quante volte di fila la stessa cosa
	 * nell'ultimo minuto (lo stesso manichino). Piu' l'abilita' e' alta, piu' sale piano.
	 */
	VALDORSO_API float Guadagno(float PuntiAdesso, float Difficolta, int32 Ripetizioni);

	/** Usa un'abilita': sale (se puo'), si carica alla soglia, si evolve se il momento e' critico. */
	VALDORSO_API FValdorsoEsitoUso Usa(FValdorsoAbilitaColono& Colono, FName Id, float Difficolta, int32 Ripetizioni, bool bMomentoCritico);

	/** Sale, ferma o cala. Falso se l'abilita' non c'e'. */
	VALDORSO_API bool ImpostaVerso(FValdorsoAbilitaColono& Colono, FName Id, EValdorsoVerso Verso);

	/** Per lo staff e le prove: i punti (0..100), con le evoluzioni che ne seguono. */
	VALDORSO_API bool ImpostaPunti(FValdorsoAbilitaColono& Colono, FName Id, float Nuovi);

	/** I vantaggi del Registro (una volta sola): vero se ha dato qualcosa. */
	VALDORSO_API bool DaiVantaggi(FValdorsoAbilitaColono& Colono, const FValdorsoRegistro& Registro);

	/** Mette in ordine dopo la lettura dal file: via quelle che non esistono piu', i valori nei limiti, le doppie. */
	VALDORSO_API void Sistema(FValdorsoAbilitaColono& Colono);

	/** La frase della voce del Cuore per un'evoluzione. */
	VALDORSO_API FText Annuncio(FName Id, int32 Evoluzione);

	/** Cosa si sblocca alla prossima soglia (per la scheda), vuoto se niente. */
	VALDORSO_API FString ProssimoSblocco(FName Id, int32 Evoluzioni);

	// --- (10/11) Quanto e' forte qualcuno: in questo mondo si capisce a occhio chi e' forte e chi no ------------------

	/** La forza di un colono (0..100): la sua arma migliore (70%) e la tempra (30%). */
	VALDORSO_API float Forza(const FValdorsoAbilitaColono& Colono);

	/** La forza di una creatura (per specie: la lepre 0, il lupo 25, l'orso 60...). */
	VALDORSO_API float ForzaSpecie(FName Specie);

	/** La forza di un abitante (per dialogo: le guardie del passo 90, Lucia, ex avventuriera, 60...). */
	VALDORSO_API float ForzaAbitante(FName Dialogo);

	/** Come sembra Sua a chi ha la forza Mia: la frase e il colore (rosso piu' forte, verde piu' debole). */
	VALDORSO_API FText Confronto(float Mia, float Sua, FLinearColor& OutColore);

	/** Combattendo: quanto rende un colpo contro chi ha forza Sua (piu' forte: di piu'), e se e' un pericolo vero. */
	VALDORSO_API float DifficoltaContro(float Mia, float Sua);
	VALDORSO_API bool PericoloVero(float Mia, float Sua);
}
