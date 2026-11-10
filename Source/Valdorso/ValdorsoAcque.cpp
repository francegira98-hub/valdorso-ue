// Valdorso - Il nuoto: dove c'è l'acqua e quanto è alta (vedi il .h). Scritto da Claude il 06/11/2026.

#include "ValdorsoAcque.h"
#include "Valdorso.h"
#include "Engine/World.h"
#include "EngineUtils.h"

namespace ValdorsoAcqua
{
	bool DentroRiva(const TArray<FVector2D>& Riva, const FVector2D& Punto)
	{
		bool bDentro = false;
		const int32 N = Riva.Num();
		for (int32 i = 0, j = N - 1; i < N; j = i++)
		{
			const FVector2D& A = Riva[i];
			const FVector2D& B = Riva[j];
			if (((A.Y > Punto.Y) != (B.Y > Punto.Y))
				&& (Punto.X < (B.X - A.X) * (Punto.Y - A.Y) / (B.Y - A.Y + UE_SMALL_NUMBER) + A.X))
			{
				bDentro = !bDentro;
			}
		}
		return bDentro;
	}

	bool PeloSulCorso(const FValdorsoCorsoAcqua& Corso, const FVector2D& Punto, float Margine, float& OutPelo)
	{
		bool bTrovato = false;
		double Migliore = TNumericLimits<double>::Max();
		for (int32 i = 0; i + 1 < Corso.Punti.Num(); ++i)
		{
			const FVector2D A(Corso.Punti[i]);
			const FVector2D B(Corso.Punti[i + 1]);
			const FVector2D D = B - A;
			const double Lungo = D.SizeSquared();
			const double T = Lungo > UE_SMALL_NUMBER ? FMath::Clamp(FVector2D::DotProduct(Punto - A, D) / Lungo, 0.0, 1.0) : 0.0;
			const FVector2D Vicino = A + D * T;
			const double Distanza = FVector2D::Distance(Punto, Vicino);
			const float LargA = Corso.Larghezze.IsValidIndex(i) ? Corso.Larghezze[i] : 1000.f;
			const float LargB = Corso.Larghezze.IsValidIndex(i + 1) ? Corso.Larghezze[i + 1] : LargA;
			const double MezzaLarghezza = FMath::Lerp(LargA, LargB, static_cast<float>(T)) * 0.5 + Margine;
			if (Distanza <= MezzaLarghezza && Distanza < Migliore)
			{
				Migliore = Distanza;
				OutPelo = static_cast<float>(FMath::Lerp(Corso.Punti[i].Z, Corso.Punti[i + 1].Z, T));
				bTrovato = true;
			}
		}
		return bTrovato;
	}

	bool CorrenteSulCorso(const FValdorsoCorsoAcqua& Corso, const FVector2D& Punto, float Margine, FVector2D& OutVerso)
	{
		bool bTrovato = false;
		double Migliore = TNumericLimits<double>::Max();
		for (int32 i = 0; i + 1 < Corso.Punti.Num(); ++i)
		{
			const FVector2D A(Corso.Punti[i]);
			const FVector2D B(Corso.Punti[i + 1]);
			const FVector2D D = B - A;
			const double Lungo = D.SizeSquared();
			if (Lungo <= UE_SMALL_NUMBER)
			{
				continue;
			}
			const double T = FMath::Clamp(FVector2D::DotProduct(Punto - A, D) / Lungo, 0.0, 1.0);
			const double Distanza = FVector2D::Distance(Punto, A + D * T);
			const float LargA = Corso.Larghezze.IsValidIndex(i) ? Corso.Larghezze[i] : 1000.f;
			const float LargB = Corso.Larghezze.IsValidIndex(i + 1) ? Corso.Larghezze[i + 1] : LargA;
			const double MezzaLarghezza = FMath::Lerp(LargA, LargB, static_cast<float>(T)) * 0.5 + Margine;
			if (Distanza <= MezzaLarghezza && Distanza < Migliore)
			{
				Migliore = Distanza;
				// l'acqua scende: verso il punto con il pelo piu' basso
				const bool bAlContrario = Corso.Punti[i + 1].Z > Corso.Punti[i].Z + 0.5;
				OutVerso = (bAlContrario ? -D : D).GetSafeNormal();
				bTrovato = true;
			}
		}
		return bTrovato;
	}

	float Immersione(float Pelo, float ZCentro, float MezzaAltezza)
	{
		const float Fondo = ZCentro - MezzaAltezza;
		return FMath::Clamp((Pelo - Fondo) / FMath::Max(1.f, 2.f * MezzaAltezza), 0.f, 1.f);
	}
}

AValdorsoAcque::AValdorsoAcque()
{
	PrimaryActorTick.bCanEverTick = false;
	// Sta nel livello, uguale su server e PC: non serve mandarlo in rete.
	bReplicates = false;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Radice"));
	// Con World Partition deve esserci sempre, anche lontano dal colono: crea_acque_nuoto.py gli spegne "Is Spatially Loaded".
}

void AValdorsoAcque::BeginPlay()
{
	Super::BeginPlay();
	PreparaScatole();
	UE_LOG(LogValdorso, Log, TEXT("[Valdorso] Acque: %d specchi e %d corsi d'acqua per il nuoto"), Specchi.Num(), Corsi.Num());
}

void AValdorsoAcque::PreparaScatole() const
{
	ScatoleSpecchi.Reset();
	ScatoleCorsi.Reset();
	for (const FValdorsoSpecchioAcqua& Specchio : Specchi)
	{
		FBox2D Scatola(ForceInit);
		for (const FVector2D& P : Specchio.Riva)
		{
			Scatola += P;
		}
		ScatoleSpecchi.Add(Scatola.ExpandBy(Margine));
	}
	for (const FValdorsoCorsoAcqua& Corso : Corsi)
	{
		FBox2D Scatola(ForceInit);
		float Massima = 0.f;
		for (int32 i = 0; i < Corso.Punti.Num(); ++i)
		{
			Scatola += FVector2D(Corso.Punti[i]);
			Massima = FMath::Max(Massima, Corso.Larghezze.IsValidIndex(i) ? Corso.Larghezze[i] : 1000.f);
		}
		ScatoleCorsi.Add(Scatola.ExpandBy(Massima * 0.5f + Margine));
	}
	bScatolePronte = true;
}

bool AValdorsoAcque::PeloQui(const FVector2D& Punto, float& OutPelo) const
{
	if (!bScatolePronte || ScatoleSpecchi.Num() != Specchi.Num() || ScatoleCorsi.Num() != Corsi.Num())
	{
		PreparaScatole();
	}
	bool bTrovato = false;
	float Pelo = 0.f;
	for (int32 i = 0; i < Specchi.Num(); ++i)
	{
		if (!ScatoleSpecchi[i].IsInsideOrOn(Punto))
		{
			continue;
		}
		const FValdorsoSpecchioAcqua& Specchio = Specchi[i];
		bool bDentro = ValdorsoAcqua::DentroRiva(Specchio.Riva, Punto);
		// Un poco oltre la riva vale ancora (le rive sono a punti): si prova spostandosi verso il centro della scatola.
		if (!bDentro && Margine > 0.f)
		{
			const FVector2D Verso = (ScatoleSpecchi[i].GetCenter() - Punto).GetSafeNormal();
			bDentro = ValdorsoAcqua::DentroRiva(Specchio.Riva, Punto + Verso * Margine);
		}
		if (bDentro && (!bTrovato || Specchio.Pelo > Pelo))
		{
			Pelo = Specchio.Pelo;
			bTrovato = true;
		}
	}
	for (int32 i = 0; i < Corsi.Num(); ++i)
	{
		if (!ScatoleCorsi[i].IsInsideOrOn(Punto))
		{
			continue;
		}
		float PeloCorso = 0.f;
		if (ValdorsoAcqua::PeloSulCorso(Corsi[i], Punto, Margine, PeloCorso) && (!bTrovato || PeloCorso > Pelo))
		{
			Pelo = PeloCorso;
			bTrovato = true;
		}
	}
	if (bTrovato)
	{
		OutPelo = Pelo;
	}
	return bTrovato;
}

FVector2D AValdorsoAcque::CorrenteQui(const FVector2D& Punto) const
{
	if (VelocitaCorrente <= 0.f)
	{
		return FVector2D::ZeroVector;
	}
	if (!bScatolePronte || ScatoleSpecchi.Num() != Specchi.Num() || ScatoleCorsi.Num() != Corsi.Num())
	{
		PreparaScatole();
	}
	// nel lago (e nella pozza) l'acqua e' ferma, anche dove ci arriva un fiume
	for (int32 i = 0; i < Specchi.Num(); ++i)
	{
		if (ScatoleSpecchi[i].IsInsideOrOn(Punto) && ValdorsoAcqua::DentroRiva(Specchi[i].Riva, Punto))
		{
			return FVector2D::ZeroVector;
		}
	}
	for (int32 i = 0; i < Corsi.Num(); ++i)
	{
		FVector2D Verso;
		if (ScatoleCorsi[i].IsInsideOrOn(Punto) && ValdorsoAcqua::CorrenteSulCorso(Corsi[i], Punto, 0.f, Verso))
		{
			return Verso * VelocitaCorrente;
		}
	}
	return FVector2D::ZeroVector;
}

AValdorsoAcque* AValdorsoAcque::Di(const UWorld* Mondo)
{
	if (!Mondo)
	{
		return nullptr;
	}
	for (TActorIterator<AValdorsoAcque> It(const_cast<UWorld*>(Mondo)); It; ++It)
	{
		return *It;
	}
	return nullptr;
}
