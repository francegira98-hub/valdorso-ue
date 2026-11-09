// Valdorso - Bagnati e asciutti (v0.1.4 "Pericoli", seconda parte). Scritto da Claude il 08/11/2026.
//
// Come funziona:
// - UValdorsoBagnatoComponent sta sul colono (AValdorsoCharacter). Quanto è bagnato (0 asciutto, 1 fradicio) lo
//   decide il server ogni mezzo secondo e arriva a tutti (servirà anche per farlo vedere addosso):
//     nell'acqua (si nuota, o l'acqua arriva alla vita) si è subito fradici;
//     sotto la pioggia, senza un tetto sopra la testa, ci si bagna (in un minuto e mezzo di pioggia fitta si è
//     fradici; con il mantello nello zaino la metà);
//     altrimenti ci si asciuga: in quattro minuti all'aperto, in due sotto un tetto.
// - (08/11) Accanto a un falò (ValdorsoFalo.h) ci si asciuga in mezzo minuto, e il freddo non si sente.
// - Da bagnati (da 0,3 in su) la stamina torna a metà velocità: il freddo si sente. Quando si è di nuovo asciutti torna
//   come prima. Accanto alle barre, una goccia dice quanto si è bagnati (SValdorsoBarre).
// - (09/11) Il caldo e il freddo (ValdorsoClima.h): lo stesso componente calcola anche la temperatura che sente il
//   colono (arriva a tutti, a mezzi gradi) e ne applica gli effetti (stamina più lenta, il gelo che indebolisce).
// - Più avanti: i vestiti bagnati che pesano, i vestiti caldi e leggeri (con l'equipaggiamento).
// Comando di console (sul server, o giocando da soli): Valdorso.Bagnato <0..1> [nome].

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ValdorsoBagnato.generated.h"

namespace ValdorsoBagnato
{
	/** Da qui in su si è bagnati: la stamina torna a metà. */
	inline constexpr float Soglia = 0.3f;
	/** Quanti secondi di pioggia fitta per essere fradici (con il mantello il doppio). */
	inline constexpr float SecondiPioggia = 90.f;
	/** In quanti secondi ci si asciuga all'aperto e sotto un tetto. */
	inline constexpr float SecondiAsciugaFuori = 240.f;
	inline constexpr float SecondiAsciugaDentro = 120.f;
	/** (08/11) Accanto a un falò (ValdorsoFalo.h) ci si asciuga in mezzo minuto, anche sotto la pioggia. */
	inline constexpr float SecondiAsciugaFuoco = 30.f;
	/** Quanto rallenta il ritorno della stamina da bagnati (0,5 = a metà). */
	inline constexpr float FrenoStamina = 0.5f;

	/**
	 * Un passo: quanto si è bagnati dopo Delta secondi. bInAcqua: nell'acqua; Pioggia 0..1 (quella che c'è); bTetto:
	 * c'è un tetto sopra; bMantello: il mantello nello zaino; bFuoco: accanto a un falò.
	 */
	VALDORSO_API float Passo(float Bagnato, float Delta, bool bInAcqua, float Pioggia, bool bTetto, bool bMantello, bool bFuoco = false);

	/** Vero se con questo bagnato la stamina torna più piano. */
	VALDORSO_API bool Infreddolito(float Bagnato);
}

UCLASS(ClassGroup = (Valdorso))
class VALDORSO_API UValdorsoBagnatoComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UValdorsoBagnatoComponent();

	/** Il componente di quel colono (nullptr se non ce l'ha). */
	static UValdorsoBagnatoComponent* Di(const AActor* Corpo);

	/** Quanto è bagnato (0..1). */
	float Bagnato() const { return Livello; }

	/** Per le prove (console) e per chi lo decide sul server: quanto è bagnato. */
	void Imposta(float Nuovo);

	/** (09/11) La temperatura che sente il colono (gradi, a mezzi gradi) e quella dell'aria dove sta. */
	float Temperatura() const { return Percepita; }
	float TemperaturaAria() const { return GradiAria; }

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	/** Server: c'è un tetto sopra la testa? */
	bool TettoSopra() const;
	/** Server: il mantello è nello zaino? */
	bool HaMantello() const;
	/** (09/11) Quanti gradi in piu' danno i vestiti caldi nello zaino (il mantello di pelle di lupo). */
	float CaloreVestiti() const;

	/** Quanto è bagnato, come arriva a tutti (a mezzi centesimi: si manda in rete solo quando cambia davvero). */
	UPROPERTY(Replicated)
	float Livello = 0.f;

	/** Solo sul server: il valore esatto, che cambia un poco a ogni controllo. */
	float LivelloEsatto = 0.f;

	double ProssimoControllo = 0.0;
	/** L'ultimo controllo (per misurare quanto tempo è passato davvero). */
	double UltimoControllo = -1.0;
	bool bTetto = false;
	double ProssimoTetto = 0.0;
	/** La stamina di prima (per frenarne il ritorno). */
	float StaminaPrima = -1.f;

	/** (09/11) La temperatura che sente (arriva a tutti, a mezzi gradi) e, solo sul server, quella dell'aria. */
	UPROPERTY(Replicated)
	float Percepita = 15.f;
	float GradiAria = 15.f;
	/** Il cielo del livello (per l'ora), cercato una volta. */
	TWeakObjectPtr<class AValdorsoCielo> Cielo;
	bool bCieloCercato = false;
};
