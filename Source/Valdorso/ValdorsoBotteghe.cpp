// Valdorso - Le botteghe (v0.1.4, 10/11/2026). Scritto da Claude. Vedi ValdorsoBotteghe.h.

#include "ValdorsoBotteghe.h"
#include "Valdorso.h"
#include "ValdorsoFalo.h"
#include "ValdorsoOggetti.h"
#include "ValdorsoZaino.h"
#include "ValdorsoZainoDati.h"
#include "Components/SphereComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

#define LOCTEXT_NAMESPACE "ValdorsoBotteghe"

namespace
{
	FValdorsoRicetta BottegaRicetta(const TCHAR* Id, EValdorsoTipoBanco Banco, const TCHAR* Verbo, const TCHAR* Risultato, int32 Quanti = 1)
	{
		FValdorsoRicetta R;
		R.Id = Id;
		R.Banco = Banco;
		R.Verbo = Verbo;
		R.Risultato = Risultato;
		R.QuantiRisultato = Quanti;
		return R;
	}

	FValdorsoRicetta BottegaServe(FValdorsoRicetta R, const TCHAR* A, int32 QA, const TCHAR* B = nullptr, int32 QB = 0,
		const TCHAR* C = nullptr, int32 QC = 0)
	{
		R.Serve[0] = { A, QA };
		R.Serve[1] = { B, QB };
		R.Serve[2] = { C, QC };
		return R;
	}

	FValdorsoRicetta BottegaCompra(const TCHAR* Id, EValdorsoTipoBanco Banco, const TCHAR* Oggetto, int32 Prezzo)
	{
		FValdorsoRicetta R = BottegaRicetta(Id, Banco, TEXT("Compra"), Oggetto);
		R.Costo = Prezzo;
		return R;
	}

	FValdorsoRicetta BottegaVendi(const TCHAR* Id, EValdorsoTipoBanco Banco, const TCHAR* Oggetto, int32 Prezzo)
	{
		FValdorsoRicetta R = BottegaServe(BottegaRicetta(Id, Banco, TEXT("Vendi"), nullptr, 0), Oggetto, 1);
		R.Guadagno = Prezzo;
		return R;
	}

	TArray<FValdorsoRicetta> BottegaCreaRicette()
	{
		using B = EValdorsoTipoBanco;
		TArray<FValdorsoRicetta> T;
		// --- il fuoco (un falo' acceso; anche la locanda ha il suo)
		T.Add(BottegaServe(BottegaRicetta(TEXT("Cuoci_Carne"), B::Fuoco, TEXT("Cuoci"), TEXT("Carne_Cotta")), TEXT("Carne_Cruda"), 1));
		T.Add(BottegaServe(BottegaRicetta(TEXT("Cuoci_Funghi"), B::Fuoco, TEXT("Cuoci"), TEXT("Funghi_Arrosto")), TEXT("Funghi"), 2));
		T.Add(BottegaServe(BottegaRicetta(TEXT("Cuoci_Stufato"), B::Fuoco, TEXT("Cuoci"), TEXT("Stufato")),
			TEXT("Carne_Cruda"), 2, TEXT("Funghi"), 1));
		// --- la conceria: le pelli diventano cuoio e vestiti caldi; le pelli si vendono
		T.Add(BottegaServe(BottegaRicetta(TEXT("Concia_Cinghiale"), B::Conceria, TEXT("Concia"), TEXT("Cuoio"), 2), TEXT("Pelle_Cinghiale"), 1));
		T.Add(BottegaServe(BottegaRicetta(TEXT("Concia_Cervo"), B::Conceria, TEXT("Concia"), TEXT("Cuoio"), 2), TEXT("Pelle_Cervo"), 1));
		{
			FValdorsoRicetta R = BottegaServe(BottegaRicetta(TEXT("Mantello_Lupo"), B::Conceria, TEXT("Cuci"), TEXT("Mantello_Lupo")),
				TEXT("Pelle_Lupo"), 2, TEXT("Cuoio"), 1);
			R.Costo = 5;
			T.Add(R);
		}
		{
			FValdorsoRicetta R = BottegaServe(BottegaRicetta(TEXT("Mantello_Orso"), B::Conceria, TEXT("Cuci"), TEXT("Mantello_Orso")),
				TEXT("Pelle_Orso"), 1, TEXT("Cuoio"), 2);
			R.Costo = 10;
			T.Add(R);
		}
		T.Add(BottegaServe(BottegaRicetta(TEXT("Cappuccio_Volpe"), B::Conceria, TEXT("Cuci"), TEXT("Cappuccio_Volpe")),
			TEXT("Pelle_Volpe"), 2, TEXT("Cuoio"), 1));
		T.Add(BottegaServe(BottegaRicetta(TEXT("Guanti_Lepre"), B::Conceria, TEXT("Cuci"), TEXT("Guanti_Lepre")),
			TEXT("Pelle_Lepre"), 3, TEXT("Cuoio"), 1));
		T.Add(BottegaVendi(TEXT("Vendi_Pelle_Lupo"), B::Conceria, TEXT("Pelle_Lupo"), 8));
		T.Add(BottegaVendi(TEXT("Vendi_Pelle_Cinghiale"), B::Conceria, TEXT("Pelle_Cinghiale"), 7));
		T.Add(BottegaVendi(TEXT("Vendi_Pelle_Orso"), B::Conceria, TEXT("Pelle_Orso"), 40));
		T.Add(BottegaVendi(TEXT("Vendi_Pelle_Cervo"), B::Conceria, TEXT("Pelle_Cervo"), 10));
		T.Add(BottegaVendi(TEXT("Vendi_Pelle_Volpe"), B::Conceria, TEXT("Pelle_Volpe"), 12));
		T.Add(BottegaVendi(TEXT("Vendi_Pelle_Lepre"), B::Conceria, TEXT("Pelle_Lepre"), 2));
		// --- la fucina di Marta: armi e scudi di ferro; compra zanne, artigli e palchi (per i manici e i ciondoli)
		T.Add(BottegaCompra(TEXT("Compra_Coltello"), B::Fucina, TEXT("Coltello"), 5));
		T.Add(BottegaCompra(TEXT("Compra_Pugnale"), B::Fucina, TEXT("Pugnale"), 15));
		T.Add(BottegaCompra(TEXT("Compra_Accetta"), B::Fucina, TEXT("Accetta"), 18));
		T.Add(BottegaCompra(TEXT("Compra_Martello"), B::Fucina, TEXT("Martello"), 12));
		T.Add(BottegaCompra(TEXT("Compra_Falcetto"), B::Fucina, TEXT("Falcetto"), 10));
		T.Add(BottegaCompra(TEXT("Compra_Lancia"), B::Fucina, TEXT("Lancia"), 25));
		T.Add(BottegaCompra(TEXT("Compra_Spada_Corta"), B::Fucina, TEXT("Spada_Corta"), 35));
		T.Add(BottegaCompra(TEXT("Compra_Mazza"), B::Fucina, TEXT("Mazza"), 40));
		T.Add(BottegaCompra(TEXT("Compra_Gladio"), B::Fucina, TEXT("Gladio"), 60));
		T.Add(BottegaCompra(TEXT("Compra_Ascia_Guerra"), B::Fucina, TEXT("Ascia_Guerra"), 70));
		T.Add(BottegaCompra(TEXT("Compra_Spada_Lunga"), B::Fucina, TEXT("Spada_Lunga"), 85));
		T.Add(BottegaCompra(TEXT("Compra_Brocchiere"), B::Fucina, TEXT("Brocchiere"), 20));
		T.Add(BottegaCompra(TEXT("Compra_Scudo_Aquilone"), B::Fucina, TEXT("Scudo_Aquilone"), 55));
		T.Add(BottegaCompra(TEXT("Compra_Scudo_Legione"), B::Fucina, TEXT("Scudo_Legione"), 90));
		T.Add(BottegaVendi(TEXT("Vendi_Zanne_Lupo"), B::Fucina, TEXT("Zanne_Lupo"), 3));
		T.Add(BottegaVendi(TEXT("Vendi_Zanne_Cinghiale"), B::Fucina, TEXT("Zanne_Cinghiale"), 4));
		T.Add(BottegaVendi(TEXT("Vendi_Artigli_Orso"), B::Fucina, TEXT("Artigli_Orso"), 15));
		// --- il falegname: legno
		T.Add(BottegaCompra(TEXT("Compra_Clava"), B::Falegname, TEXT("Clava"), 6));
		T.Add(BottegaCompra(TEXT("Compra_Bastone_Pastore"), B::Falegname, TEXT("Bastone_Pastore"), 6));
		T.Add(BottegaCompra(TEXT("Compra_Bastone_Ferrato"), B::Falegname, TEXT("Bastone_Ferrato"), 12));
		T.Add(BottegaCompra(TEXT("Compra_Forcone"), B::Falegname, TEXT("Forcone"), 10));
		T.Add(BottegaCompra(TEXT("Compra_Scudo_Legno"), B::Falegname, TEXT("Scudo_Legno"), 15));
		T.Add(BottegaCompra(TEXT("Compra_Scudo_Rotondo"), B::Falegname, TEXT("Scudo_Rotondo"), 30));
		T.Add(BottegaVendi(TEXT("Vendi_Palchi_Cervo"), B::Falegname, TEXT("Palchi_Cervo"), 8));
		T.Add(BottegaVendi(TEXT("Vendi_Rami"), B::Falegname, TEXT("Rami_Secchi"), 1));
		// --- la locanda di Bruno: da mangiare (e il suo fuoco); compra la carne e i funghi
		T.Add(BottegaCompra(TEXT("Compra_Zuppa"), B::Locanda, TEXT("Zuppa_Bruno"), 3));
		T.Add(BottegaCompra(TEXT("Compra_Pane"), B::Locanda, TEXT("Pane_Nero"), 2));
		T.Add(BottegaCompra(TEXT("Compra_Mela"), B::Locanda, TEXT("Mela"), 1));
		T.Add(BottegaCompra(TEXT("Compra_Formaggio"), B::Locanda, TEXT("Formaggio"), 3));
		T.Add(BottegaServe(BottegaRicetta(TEXT("Locanda_Stufato"), B::Locanda, TEXT("Cuoci"), TEXT("Stufato")),
			TEXT("Carne_Cruda"), 2, TEXT("Funghi"), 1));
		T.Add(BottegaServe(BottegaRicetta(TEXT("Locanda_Carne"), B::Locanda, TEXT("Cuoci"), TEXT("Carne_Cotta")), TEXT("Carne_Cruda"), 1));
		T.Add(BottegaVendi(TEXT("Vendi_Carne"), B::Locanda, TEXT("Carne_Cruda"), 2));
		T.Add(BottegaVendi(TEXT("Vendi_Funghi"), B::Locanda, TEXT("Funghi"), 1));
		T.Add(BottegaVendi(TEXT("Vendi_Cozze"), B::Locanda, TEXT("Cozze"), 1));
		T.Add(BottegaVendi(TEXT("Vendi_Perla"), B::Locanda, TEXT("Perla"), 20));
		return T;
	}

	FText BottegaNome(const TCHAR* Id)
	{
		const FValdorsoTipoOggetto* Tipo = Id ? ValdorsoOggetti::Trova(Id) : nullptr;
		return FText::FromString(Tipo ? FString(Tipo->Nome) : FString(Id ? Id : TEXT("?")));
	}
}

namespace ValdorsoBotteghe
{
	TArrayView<const FValdorsoRicetta> Ricette()
	{
		static const TArray<FValdorsoRicetta> Tabella = BottegaCreaRicette();
		return Tabella;
	}

	const FValdorsoRicetta* Trova(FName Id)
	{
		for (const FValdorsoRicetta& R : Ricette())
		{
			if (Id == FName(R.Id))
			{
				return &R;
			}
		}
		return nullptr;
	}

	TArray<const FValdorsoRicetta*> Del(EValdorsoTipoBanco Banco)
	{
		TArray<const FValdorsoRicetta*> Lista;
		for (const FValdorsoRicetta& R : Ricette())
		{
			if (R.Banco == Banco)
			{
				Lista.Add(&R);
			}
		}
		return Lista;
	}

	FText Manca(const FValdorsoZaino& Zaino, const FValdorsoRicetta& Ricetta)
	{
		for (const FValdorsoIngrediente& Ingrediente : Ricetta.Serve)
		{
			if (Ingrediente.Id && Ingrediente.Quanti > 0 && ValdorsoZaino::Conta(Zaino, Ingrediente.Id) < Ingrediente.Quanti)
			{
				return FText::Format(LOCTEXT("ServeOggetto", "Ti serve: {0} × {1}"), FText::AsNumber(Ingrediente.Quanti), BottegaNome(Ingrediente.Id));
			}
		}
		if (Zaino.Monete < Ricetta.Costo)
		{
			return FText::Format(LOCTEXT("ServeMonete", "Ti servono {0} Aureus"), FText::AsNumber(Ricetta.Costo));
		}
		if (Ricetta.Risultato)
		{
			// c'e' posto? si prova su una copia
			FValdorsoZaino Prova = Zaino;
			for (const FValdorsoIngrediente& Ingrediente : Ricetta.Serve)
			{
				if (Ingrediente.Id && Ingrediente.Quanti > 0)
				{
					ValdorsoZaino::TogliOggetto(Prova, Ingrediente.Id, Ingrediente.Quanti);
				}
			}
			if (ValdorsoZaino::Aggiungi(Prova, Ricetta.Risultato, Ricetta.QuantiRisultato) < Ricetta.QuantiRisultato)
			{
				return LOCTEXT("ZainoPieno", "Lo zaino è pieno");
			}
		}
		return FText::GetEmpty();
	}

	bool Fai(FValdorsoZaino& Zaino, const FValdorsoRicetta& Ricetta)
	{
		if (!Manca(Zaino, Ricetta).IsEmpty())
		{
			return false;
		}
		for (const FValdorsoIngrediente& Ingrediente : Ricetta.Serve)
		{
			if (Ingrediente.Id && Ingrediente.Quanti > 0)
			{
				ValdorsoZaino::TogliOggetto(Zaino, Ingrediente.Id, Ingrediente.Quanti);
			}
		}
		Zaino.Monete = FMath::Max(0, Zaino.Monete - Ricetta.Costo) + FMath::Max(0, Ricetta.Guadagno);
		if (Ricetta.Risultato)
		{
			ValdorsoZaino::Aggiungi(Zaino, Ricetta.Risultato, Ricetta.QuantiRisultato);
		}
		return true;
	}

	FText Titolo(EValdorsoTipoBanco Banco)
	{
		switch (Banco)
		{
		case EValdorsoTipoBanco::Fuoco: return LOCTEXT("TitoloFuoco", "Al fuoco");
		case EValdorsoTipoBanco::Conceria: return LOCTEXT("TitoloConceria", "La conceria");
		case EValdorsoTipoBanco::Fucina: return LOCTEXT("TitoloFucina", "La fucina di Marta");
		case EValdorsoTipoBanco::Falegname: return LOCTEXT("TitoloFalegname", "Il falegname");
		default: return LOCTEXT("TitoloLocanda", "La locanda di Bruno");
		}
	}

	FText Invito(EValdorsoTipoBanco Banco)
	{
		switch (Banco)
		{
		case EValdorsoTipoBanco::Fuoco: return LOCTEXT("InvitoFuoco", "Cucina al fuoco");
		case EValdorsoTipoBanco::Conceria: return LOCTEXT("InvitoConceria", "Lavora alla conceria");
		case EValdorsoTipoBanco::Fucina: return LOCTEXT("InvitoFucina", "Compra da Marta");
		case EValdorsoTipoBanco::Falegname: return LOCTEXT("InvitoFalegname", "Compra dal falegname");
		default: return LOCTEXT("InvitoLocanda", "Ordina da Bruno");
		}
	}

	FText TestoServe(const FValdorsoRicetta& Ricetta)
	{
		TArray<FString> Parti;
		for (const FValdorsoIngrediente& Ingrediente : Ricetta.Serve)
		{
			if (Ingrediente.Id && Ingrediente.Quanti > 0)
			{
				Parti.Add(Ingrediente.Quanti > 1 ? FString::Printf(TEXT("%d × %s"), Ingrediente.Quanti, *BottegaNome(Ingrediente.Id).ToString())
					: BottegaNome(Ingrediente.Id).ToString());
			}
		}
		if (Ricetta.Costo > 0)
		{
			Parti.Add(FString::Printf(TEXT("%d Aureus"), Ricetta.Costo));
		}
		return FText::FromString(FString::Join(Parti, TEXT(", ")));
	}

	FText TestoDa(const FValdorsoRicetta& Ricetta)
	{
		if (Ricetta.Risultato)
		{
			return Ricetta.QuantiRisultato > 1
				? FText::FromString(FString::Printf(TEXT("%d × %s"), Ricetta.QuantiRisultato, *BottegaNome(Ricetta.Risultato).ToString()))
				: BottegaNome(Ricetta.Risultato);
		}
		return FText::FromString(FString::Printf(TEXT("%d Aureus"), Ricetta.Guadagno));
	}

	bool BancoVicino(const APawn* Chi, EValdorsoTipoBanco Banco)
	{
		UWorld* Mondo = Chi ? Chi->GetWorld() : nullptr;
		if (!Mondo)
		{
			return false;
		}
		if (Banco == EValdorsoTipoBanco::Fuoco && AValdorsoFalo::CaldoVicino(Mondo, Chi->GetActorLocation()))
		{
			return true;
		}
		for (TActorIterator<AValdorsoBanco> It(Mondo); It; ++It)
		{
			if (It->Banco == Banco && FVector::Dist(It->GetActorLocation(), Chi->GetActorLocation()) <= Raggio)
			{
				return true;
			}
		}
		return false;
	}
}

AValdorsoBanco::AValdorsoBanco()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = false;
	Presa = CreateDefaultSubobject<USphereComponent>(TEXT("Presa"));
	Presa->InitSphereRadius(60.f);
	Presa->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Presa->SetCollisionResponseToAllChannels(ECR_Overlap);
	Presa->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	Presa->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
	Presa->SetCanEverAffectNavigation(false);
	Presa->SetGenerateOverlapEvents(false);
	RootComponent = Presa;
}

FText AValdorsoBanco::TestoInterazione(const APawn* Chi) const
{
	return ValdorsoBotteghe::Invito(Banco);
}

void AValdorsoBanco::Interagisci(APawn* Chi)
{
	const APlayerController* Controllore = Chi ? Cast<APlayerController>(Chi->GetController()) : nullptr;
	if (UValdorsoZainoComponent* Zaino = UValdorsoZainoComponent::Di(Controllore))
	{
		Zaino->ClientApriBottega(static_cast<uint8>(Banco));
	}
}

#undef LOCTEXT_NAMESPACE
