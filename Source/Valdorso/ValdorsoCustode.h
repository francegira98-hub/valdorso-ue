// Valdorso - Il custode del Battito (v0.2.4 "PNG veri", blocco 4, 14/11/2026). Scritto da Claude.
// Deciso da Fra il 13/11: il Battito diventa un modulo vero, un compito alla volta; il custode "guarda imbrogli ed
// economia e avvisa lo staff, senza punire da solo" (come il Cardinal di SAO, che pero' qui non tocca mai nessuno).
//
// Cosa guarda (solo sul server, ogni secondo, e dal registro eventi):
// - le monete: chi guadagna piu' di SogliaMonete Aureus in un'ora vera;
// - gli oggetti di valore: chi riceve piu' di SogliaRari oggetti Rari (o piu' su) in un'ora;
// - i passi: chi si sposta piu' veloce di quanto si possa correre, piu' volte in pochi minuti (ma non dopo una rinascita,
//   un arresto, un passaggio o un'azione dello staff, che spostano il colono di colpo);
// - i colpi: troppi colpi in dieci secondi (un trucco per colpire piu' in fretta);
// - le colpe: chi colpisce i PNG piu' volte in mezz'ora;
// - e ogni ora l'economia della valle: quanti Aureus ci sono in giro tra chi e' collegato.
// Cosa fa: scrive nel registro eventi ("Custode": nella finestra dello staff, F9, scheda Registro, filtro "Custode") e
// lo dice in chat allo staff collegato. Mai piu' di un avviso per colono e per tipo ogni dieci minuti. Non punisce:
// decide sempre lo staff. Lo staff stesso non si guarda (si sposta con F9).
// Comando: Valdorso.Custode (com'e' adesso la valle per il custode).

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Engine/TimerHandle.h"
#include "ValdorsoCustode.generated.h"

class AActor;
class APlayerController;

namespace ValdorsoCustode
{
	inline constexpr int32 SogliaMonete = 500;
	inline constexpr int32 SogliaRari = 4;
	/** Oltre quanti cm/s (in piano) un passo e' "troppo veloce", e quante volte in SecondiPassi prima di avvisare. */
	inline constexpr float VelocitaMassima = 2500.f;
	inline constexpr int32 VolteVeloce = 3;
	inline constexpr double SecondiPassi = 300.0;
	/** Dopo una rinascita, un arresto, un passaggio o l'aiuto dello staff, per quanto i passi non contano (s). */
	inline constexpr double SecondiTregua = 6.0;
	inline constexpr int32 SogliaColpi = 30;
	inline constexpr double SecondiColpi = 10.0;
	inline constexpr int32 SogliaColpe = 3;
	inline constexpr double SecondiColpe = 1800.0;
	inline constexpr double SecondiTraAvvisi = 600.0;

	/** Quante volte, tra i momenti segnati, sono negli ultimi Finestra secondi (e toglie quelli piu' vecchi). */
	VALDORSO_API int32 Conta(TArray<double>& Momenti, double Adesso, double Finestra);

	/** Quanto e' cresciuto un valore, sommando solo le salite, negli ultimi Finestra secondi (coppie momento, salita). */
	VALDORSO_API int32 Salite(TArray<TPair<double, int32>>& InSalite, double Adesso, double Finestra);

	/** Vero se quel passo (cm in piano in Secondi) e' piu' veloce di quanto si possa andare. */
	VALDORSO_API bool TroppoVeloce(double CentimetriInPiano, double Secondi);

	/** Vero se quel tipo di evento sposta il colono di colpo (e i passi per un po' non contano). */
	VALDORSO_API bool SpostaDiColpo(const FString& Tipo);
}

UCLASS()
class VALDORSO_API UValdorsoCustode : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

	/** Il registro eventi ha annotato qualcosa (lo chiama ValdorsoRegistroEventi::Annota, sul server). */
	static void DopoAnnota(const FString& Tipo, const AActor* Chi, const AActor* Bersaglio);

	/** Com'e' la valle adesso per il custode (console Valdorso.Custode). */
	FString Stato() const;

private:
	struct FGuardato
	{
		TWeakObjectPtr<const AActor> Corpo;
		FVector UltimoPunto = FVector::ZeroVector;
		double UltimoMomento = -1.0;
		double TreguaFinoA = 0.0;
		int32 UltimeMonete = -1;
		int32 UltimiRari = -1;
		TArray<TPair<double, int32>> MoneteSalite;
		TArray<TPair<double, int32>> RariSaliti;
		TArray<double> Veloci;
		TArray<double> Colpi;
		TArray<double> Colpe;
		TMap<FString, double> UltimoAvviso;
	};

	void Giro();
	FGuardato& GuardatoDi(const APlayerController* Chi);
	void Evento(const APlayerController* Chi, const FString& Tipo, bool bComeBersaglio);
	/** Avvisa (registro e staff in chat), al massimo una volta ogni SecondiTraAvvisi per colono e per tipo. */
	void Avvisa(const APlayerController* Chi, FGuardato& Suo, const FString& Tipo, const FString& Testo);
	void DilloAlloStaff(const FString& Testo);

	TMap<TWeakObjectPtr<const APlayerController>, FGuardato> Guardati;
	FTimerHandle TimerGiro;
	double ProssimaEconomia = 0.0;
	int32 Avvisi = 0;
};
