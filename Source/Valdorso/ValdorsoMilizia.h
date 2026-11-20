// Valdorso - Le Guardie di Valdorso (in valle tutti dicono "la milizia"), il primo lavoro (17/11/2026, v0.2.7 "Il tempio";
// approvata da Fra il 17/11, rifatta il 18/11 con le sue correzioni).
//
// - DUE CORPI DIVERSI (Fra, 18/11). La Guardia Reale risponde solo alla Corona: al passo c'e' un suo distaccamento,
//   comandato dalla capitana Livia Serra. Le Guardie di Valdorso sono del posto: le paga e le governa il Balivo (e il
//   signore della valle, quando ci sara'), per mezzo del Comandante delle Guardie di Valdorso. Ogni citta' avra' il
//   suo Comandante; la Guardia Reale tiene la capitale. Per importanza le guardie del posto rispondono a Livia, ma
//   caserma, torri e porte sono del Balivo.
// - Per ora il Comandante non c'e': il ruolo delle guardie lo tiene Livia per conto del Balivo. Ci si arruola da lei
//   (il suo dialogo, ValdorsoDialoghiTesti.inl): serve che Livia si fidi (Cordiale o piu') e nessuna colpa aperta.
// - IL CONTRATTO (Fra, 18/11): non basta dire "firmo". Si apre una pergamena con le clausole e si firma davvero
//   (SValdorsoContratto.h). Il contratto vincola: una ferma di GiorniDiFerma giorni della valle prima di poter
//   lasciare, e resta fra i documenti del Balivo (ValdorsoContratto.h): piu' avanti conta per la fama.
// - I TURNI (19/11, secondo pezzo): si prendono da chi tiene il ruolo, uno al giorno della valle. LA RONDA: un giro del
//   villaggio facendosi vedere da Bruno, Marta, Padre Anselmo e Teodoro (chi quel giorno non c'e' non conta). LA GUARDIA
//   AL PASSO: cinque minuti al portale accanto a Tebaldo. A turno fatto si torna dalla capitana, che lo segna.
// - IL SOLDO: per ogni giorno in cui si e' fatto il turno, secondo il grado (SoldoAlGiorno). Si ritira da chi tiene il
//   ruolo; se ne tengono da parte al massimo GiorniArretrati.
// - Cinque gradi: recluta, guardia, sergente, vice comandante, comandante. Fino a sergente si sale con i turni
//   (TurniPerGuardia, TurniPerSergente) parlando con Livia; vice comandante e comandante si nominano (per ora lo
//   staff, piu' avanti il Balivo): ce n'e' uno solo per citta'.
// - Le chiavi sono oggetti veri nello zaino: recluta Chiave_Caserma; guardia anche Chiave_Prigione; le chiavi delle
//   torri delle mura e delle porte (Chiave_Torri, Chiave_Porta) solo a vice comandante e comandante (Fra, 18/11:
//   "e' una cosa importante"). Per la ronda sulle mura la chiave si prestera' per il turno (secondo pezzo).
//   Chi lascia le restituisce; a chi commette una colpa (AValdorsoPlayerState::Incolpa) vengono ritirate, e' cacciato
//   e il suo contratto resta scritto come rotto.
// - La tessera del colono mostra la qualifica del grado (Qualifica).
// - Tutto sul server. Per le prove: Valdorso.Milizia.Grado <0-5> [nome], Valdorso.Milizia.Turni <n> [nome],
//   Valdorso.Milizia.Giorni <n> [nome] (finge n giorni di servizio gia' fatti), Valdorso.Milizia.Turno [nome] (da' per
//   fatto il turno in corso, o una ronda: poi si parla con Livia), Valdorso.Milizia.Domani [nome] (toglie "per oggi hai
//   gia' fatto il turno").

#pragma once

#include "CoreMinimal.h"

class APlayerController;
class AActor;
struct FValdorsoContrattoVista;

namespace ValdorsoMilizia
{
	inline constexpr uint8 Nessuno = 0;
	inline constexpr uint8 Recluta = 1;
	inline constexpr uint8 Guardia = 2;
	inline constexpr uint8 Sergente = 3;
	inline constexpr uint8 ViceComandante = 4;
	inline constexpr uint8 Comandante = 5;

	/** Quanti turni servono per salire (oltre sergente non si sale con i turni: si viene nominati). */
	inline constexpr int32 TurniPerGuardia = 5;
	inline constexpr int32 TurniPerSergente = 20;

	/** Un giorno della valle in secondi veri (come AValdorsoCielo::MinutiPerGiorno di serie: due ore). */
	inline constexpr int64 SecondiPerGiorno = 7200;
	/** La ferma del contratto: prima di questi giorni della valle non si puo' lasciare. */
	inline constexpr int32 GiorniDiFerma = 30;
	/** Quanti giorni di soldo non ritirato si tengono da parte. */
	inline constexpr int32 GiorniArretrati = 3;

	/** (19/11) I turni. La ronda: firmare i registri della ronda (ValdorsoRegistroRonda.h) ai quattro angoli del
	 *  villaggio; se nel livello non ce ne sono almeno due, farsi vedere dagli abitanti (entro RaggioRonda cm).
	 *  La guardia: SecondiDiGuardia al portale del passo, accanto a Tebaldo (entro RaggioGuardia cm). Un turno preso
	 *  e non finito entro SecondiPerUnTurno e' perso. Si segna tornando dalla capitana, uno al giorno della valle. */
	inline constexpr uint8 TurnoRonda = 1;
	inline constexpr uint8 TurnoGuardia = 2;
	/** (20/11, v0.2.7.1) La guardia in garitta: SecondiDiGuardia nella garitta della caserma (entro RaggioGaritta cm dal
	 *  punto AValdorsoCampanella::Garitta). La capitana la da' al posto di quella al passo un giorno si' e uno no, quando
	 *  la caserma c'e' (fase 3 del villaggio). */
	inline constexpr uint8 TurnoGaritta = 3;
	inline constexpr float RaggioGaritta = 115.f;
	inline constexpr float RaggioRonda = 700.f;
	inline constexpr float RaggioGuardia = 900.f;
	inline constexpr float SecondiDiGuardia = 300.f;
	inline constexpr double SecondiPerUnTurno = 2400.0;

	/** Il tipo del contratto fra i documenti (ValdorsoContratto.h). */
	inline const FName TipoContratto(TEXT("ContrattoGuardie"));

	/** Il nome del grado ("recluta"); vuoto per chi non e' delle guardie. */
	VALDORSO_API const TCHAR* NomeGrado(uint8 Grado);

	/** La qualifica da scrivere sulla tessera ("Recluta delle Guardie di Valdorso"); vuota per chi non lo e'. */
	VALDORSO_API FString Qualifica(uint8 Grado);

	/** Gli Aureus al giorno di quel grado. */
	VALDORSO_API int32 SoldoAlGiorno(uint8 Grado);

	/** Le chiavi che spettano a quel grado (tutte quelle dei gradi sotto, piu' le sue). */
	VALDORSO_API TArray<FName> ChiaviDelGrado(uint8 Grado);

	/** Il grado che spetta con quei turni, partendo da quello di adesso (non scende mai; 0 resta 0; al massimo sergente). */
	VALDORSO_API uint8 GradoDopoITurni(uint8 Grado, int32 Turni);

	/** Quanti giorni della valle mancano alla fine della ferma (0 = finita). */
	VALDORSO_API int32 GiorniAllaFineDellaFerma(int64 DalUtc, int64 AdessoUtc);

	/** Quanti giorni di soldo si possono ritirare adesso (al massimo GiorniArretrati). */
	VALDORSO_API int32 GiorniDaPagare(int64 PagatoUtc, int64 AdessoUtc);

	/** Il grado del giocatore di quel PlayerController (0 se non e' delle guardie o non ha lo zaino). */
	VALDORSO_API uint8 GradoDi(const APlayerController* Chi);

	/** Il contratto di arruolamento da far leggere (senza il nome e la firma: li mette lo zaino). */
	VALDORSO_API FValdorsoContrattoVista Contratto();

	/** Sul server: il dialogo di Livia. Vero se l'evento era della milizia (allora InOutProssimo dice dove va il
	 *  dialogo; NAME_None con MiliziaContratto = il dialogo si chiude e si apre la pergamena). Eventi: MiliziaChiedi,
	 *  MiliziaContratto, MiliziaChiavi, MiliziaPaga, MiliziaLasciaChiedi, MiliziaLascia, MiliziaRonda, MiliziaGuardia. */
	VALDORSO_API bool EventoDialogo(FName Evento, APlayerController* Chi, AActor* Png, FName& InOutProssimo);

	/** (19/11) Sul server: il giocatore usa un registro della ronda (AValdorsoRegistroRonda): se e' di ronda e li' non
	 *  ha ancora firmato, la tappa e' fatta. Altrimenti gli dice perche' no. */
	VALDORSO_API void FirmaRegistro(APlayerController* Chi, AActor* Registro);

	/** Sul server: il giocatore ha firmato il contratto (UValdorsoZainoComponent::ServerFirmaContratto). Controlla di
	 *  nuovo le condizioni, lo arruola recluta, registra il documento. Vero se e' arruolato. */
	VALDORSO_API bool Firmato(APlayerController* Chi, AActor* Png, const FString& Firma);

	/** Sul server: mette quel grado (0 = fuori) e sistema le chiavi nello zaino. Vero se e' cambiato qualcosa. */
	VALDORSO_API bool MettiGrado(APlayerController* Chi, uint8 Grado);

	/** Sul server: chi commette una colpa ed e' delle guardie viene cacciato, perde le chiavi e il contratto e' rotto. */
	VALDORSO_API void Espelli(APlayerController* Chi);
}
