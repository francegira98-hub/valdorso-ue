// Valdorso - Lo strumento per il trailer: la matematica del percorso, il file e i comandi (vedi il .h).
// Scritto da Claude il 08/11/2026.

#include "ValdorsoTrailer.h"
#include "Valdorso.h"
#include "Algo/BinarySearch.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonWriter.h"
#include "Serialization/JsonSerializer.h"
#include "Policies/PrettyJsonPrintPolicy.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace
{
	/** Il percorso di adesso e chi l'ha cambiato da fuori (vedi ValdorsoTrailer::Corrente e Generazione). */
	FValdorsoPercorsoTrailer TrailerPercorsoAdesso;
	int32 TrailerGenerazioneAdesso = 0;
	FString TrailerNomeAdesso = TEXT("ultimo");
	/** Vero dopo la prima lettura di ultimo.json (o dopo un Carica): non si rilegge più, e chiudendo si salva. */
	bool bTrailerUltimoLetto = false;

	const TCHAR* TrailerNomeUltimo = TEXT("ultimo");

	TAutoConsoleVariable<int32> CVarTrailerFotogrammi(
		TEXT("Valdorso.Trailer.Fotogrammi"),
		0,
		TEXT("1 = mentre la telecamera della modalità foto percorre il percorso (L), il tempo va a passi fissi di 1/30 di secondo e ogni fotogramma diventa un'immagine in Saved/Trailer/<nome>/. Solo giocando senza server. 0 = niente (di serie; per il trailer basta OBS)."));

	/** La quaternione Q dalla stessa parte di Riferimento (le due metà della sfera danno la stessa rotazione). */
	FQuat TrailerStessaParte(const FQuat& Q, const FQuat& Riferimento)
	{
		const double Punto = Q.X * Riferimento.X + Q.Y * Riferimento.Y + Q.Z * Riferimento.Z + Q.W * Riferimento.W;
		return Punto < 0.0 ? FQuat(-Q.X, -Q.Y, -Q.Z, -Q.W) : Q;
	}

	/** La tangente di squad in Qui (Prima e Dopo già dalla stessa parte di Qui). */
	FQuat TrailerTangente(const FQuat& Prima, const FQuat& Qui, const FQuat& Dopo)
	{
		const FQuat Inversa = Qui.Inverse();
		const FQuat VersoDopo = (Inversa * Dopo).GetNormalized().Log();
		const FQuat VersoPrima = (Inversa * Prima).GetNormalized().Log();
		const FQuat Media(-(VersoDopo.X + VersoPrima.X) * 0.25, -(VersoDopo.Y + VersoPrima.Y) * 0.25, -(VersoDopo.Z + VersoPrima.Z) * 0.25, 0.0);
		return (Qui * Media.Exp()).GetNormalized();
	}

	/** Un numero valido letto dal file (niente infiniti o NaN), oppure Altrimenti. */
	double TrailerNumero(const TSharedPtr<FJsonObject>& Oggetto, const TCHAR* Campo, double Altrimenti)
	{
		double Valore = Altrimenti;
		if (Oggetto.IsValid() && Oggetto->TryGetNumberField(Campo, Valore) && FMath::IsFinite(Valore))
		{
			return Valore;
		}
		return Altrimenti;
	}

	FString TrailerFileDi(const FString& NomePulito)
	{
		return ValdorsoTrailer::Cartella() / (NomePulito + TEXT(".json"));
	}

	FAutoConsoleCommand ComandoTrailerSalva(
		TEXT("Valdorso.Trailer.Salva"),
		TEXT("Salva il percorso della modalità foto in Saved/Trailer/<nome>.json, per esempio Valdorso.Trailer.Salva piazza_alba."),
		FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>& Argomenti)
		{
			if (Argomenti.Num() == 0)
			{
				UE_LOG(LogValdorso, Display, TEXT("[Valdorso] Trailer: serve un nome, per esempio Valdorso.Trailer.Salva piazza_alba"));
				return;
			}
			FString File;
			if (ValdorsoTrailer::Salva(Argomenti[0], File))
			{
				UE_LOG(LogValdorso, Display, TEXT("[Valdorso] Trailer: %d tappe salvate in %s"), ValdorsoTrailer::Corrente().Tappe.Num(), *File);
			}
			else
			{
				UE_LOG(LogValdorso, Warning, TEXT("[Valdorso] Trailer: non riesco a salvare \"%s\" (nome con lettere, numeri, - e _)"), *Argomenti[0]);
			}
		}));

	FAutoConsoleCommand ComandoTrailerCarica(
		TEXT("Valdorso.Trailer.Carica"),
		TEXT("Carica un percorso salvato (Saved/Trailer/<nome>.json) nella modalità foto, per esempio Valdorso.Trailer.Carica piazza_alba."),
		FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>& Argomenti)
		{
			if (Argomenti.Num() == 0)
			{
				UE_LOG(LogValdorso, Display, TEXT("[Valdorso] Trailer: serve un nome (Valdorso.Trailer.Elenco dice quali ci sono)"));
				return;
			}
			FString Errore;
			if (ValdorsoTrailer::Carica(Argomenti[0], Errore))
			{
				UE_LOG(LogValdorso, Display, TEXT("[Valdorso] Trailer: caricato \"%s\", %d tappe"), *ValdorsoTrailer::NomeCorrente(), ValdorsoTrailer::Corrente().Tappe.Num());
			}
			else
			{
				UE_LOG(LogValdorso, Warning, TEXT("[Valdorso] Trailer: %s"), *Errore);
			}
		}));

	FAutoConsoleCommand ComandoTrailerElenco(
		TEXT("Valdorso.Trailer.Elenco"),
		TEXT("Dice i percorsi salvati in Saved/Trailer e quante tappe ha quello di adesso."),
		FConsoleCommandDelegate::CreateLambda([]()
		{
			const TArray<FString> Nomi = ValdorsoTrailer::Elenco();
			const FString Salvati = Nomi.Num() > 0 ? FString::Join(Nomi, TEXT(", ")) : FString(TEXT("nessuno"));
			const FString Nome = ValdorsoTrailer::NomeCorrente();
			const FString Dove = ValdorsoTrailer::Cartella();
			UE_LOG(LogValdorso, Display, TEXT("[Valdorso] Trailer: adesso \"%s\" con %d tappe; salvati in %s: %s"),
				*Nome, ValdorsoTrailer::Corrente().Tappe.Num(), *Dove, *Salvati);
		}));

	FAutoConsoleCommand ComandoTrailerSvuota(
		TEXT("Valdorso.Trailer.Svuota"),
		TEXT("Toglie tutte le tappe del percorso di adesso (i file salvati restano)."),
		FConsoleCommandDelegate::CreateLambda([]()
		{
			ValdorsoTrailer::Corrente().Tappe.Reset();
			++TrailerGenerazioneAdesso;
			UE_LOG(LogValdorso, Display, TEXT("[Valdorso] Trailer: percorso svuotato"));
		}));
}

// ------------------------------------------------------------------------------------------------
// La tabella delle lunghezze
// ------------------------------------------------------------------------------------------------

void FValdorsoTabellaTrailer::Prepara(const FValdorsoPercorsoTrailer& Percorso, int32 PassiPerTratto)
{
	Distanze.Reset();
	Totale = 0.f;
	Passi = FMath::Max(1, PassiPerTratto);
	Tratti = Percorso.Tratti();
	bGiro = Percorso.bGiro;
	if (Tratti <= 0)
	{
		return;
	}
	FValdorsoTappaTrailer Prima = ValdorsoTrailer::Posa(Percorso, 0.f);
	FQuat QuatPrima = Prima.Rotazione.Quaternion();
	Distanze.Reserve(Tratti * Passi + 1);
	Distanze.Add(0.f);
	for (int32 I = 1; I <= Tratti * Passi; ++I)
	{
		// l'ultimo pezzetto finisce proprio sulla tappa (anche ad anello, dove la chiave Tratti è di nuovo la prima)
		const FValdorsoTappaTrailer Qui = ValdorsoTrailer::Posa(Percorso, static_cast<float>(I) / static_cast<float>(Passi));
		const FQuat QuatQui = Qui.Rotazione.Quaternion();
		const float Strada = static_cast<float>(FVector::Dist(Prima.Posizione, Qui.Posizione));
		const float Gradi = FMath::RadiansToDegrees(static_cast<float>(QuatPrima.AngularDistance(QuatQui)));
		const float Zoom = FMath::Abs(Qui.Campo - Prima.Campo);
		Totale += FMath::Max3(Strada, Gradi * ValdorsoTrailer::CmPerGrado, Zoom * ValdorsoTrailer::CmPerGrado);
		Distanze.Add(Totale);
		Prima = Qui;
		QuatPrima = QuatQui;
	}
}

float FValdorsoTabellaTrailer::Chiave(float Distanza) const
{
	if (!Pronta() || Totale <= KINDA_SMALL_NUMBER)
	{
		return 0.f;
	}
	float D = Distanza;
	if (bGiro)
	{
		D = FMath::Fmod(D, Totale);
		if (D < 0.f)
		{
			D += Totale;
		}
	}
	else
	{
		D = FMath::Clamp(D, 0.f, Totale);
	}
	// il pezzetto in cui cade D (Distanze cresce sempre), come il giro del menu (ValdorsoMenuValle)
	const int32 Dopo = Algo::UpperBound(Distanze, D);
	const int32 J = FMath::Clamp(Dopo - 1, 0, Distanze.Num() - 2);
	const float Lungo = Distanze[J + 1] - Distanze[J];
	const float Frazione = Lungo > KINDA_SMALL_NUMBER ? FMath::Clamp((D - Distanze[J]) / Lungo, 0.f, 1.f) : 0.f;
	return (static_cast<float>(J) + Frazione) / static_cast<float>(Passi);
}

// ------------------------------------------------------------------------------------------------
// La matematica
// ------------------------------------------------------------------------------------------------

namespace ValdorsoTrailer
{
	FVector CatmullRomCentripeta(const FVector& P0, const FVector& P1, const FVector& P2, const FVector& P3, float U)
	{
		// i "tempi" dei nodi vanno con la radice della distanza (centripeta); mai zero, anche con due tappe nello stesso punto
		auto Passo = [](const FVector& A, const FVector& B)
		{
			return FMath::Max(FMath::Sqrt(FVector::Dist(A, B)), 1.0e-3);
		};
		const double T0 = 0.0;
		const double T1 = T0 + Passo(P0, P1);
		const double T2 = T1 + Passo(P1, P2);
		const double T3 = T2 + Passo(P2, P3);
		const double T = FMath::Lerp(T1, T2, static_cast<double>(FMath::Clamp(U, 0.f, 1.f)));
		// Barry e Goldman: tre interpolazioni una dentro l'altra
		const FVector A1 = P0 * ((T1 - T) / (T1 - T0)) + P1 * ((T - T0) / (T1 - T0));
		const FVector A2 = P1 * ((T2 - T) / (T2 - T1)) + P2 * ((T - T1) / (T2 - T1));
		const FVector A3 = P2 * ((T3 - T) / (T3 - T2)) + P3 * ((T - T2) / (T3 - T2));
		const FVector B1 = A1 * ((T2 - T) / (T2 - T0)) + A2 * ((T - T0) / (T2 - T0));
		const FVector B2 = A2 * ((T3 - T) / (T3 - T1)) + A3 * ((T - T1) / (T3 - T1));
		return B1 * ((T2 - T) / (T2 - T1)) + B2 * ((T - T1) / (T2 - T1));
	}

	float ValoreMorbido(float V0, float V1, float V2, float V3, float U)
	{
		// una Hermite con le tangenti "monotone" (Fritsch e Carlson): dove la curva cambia verso la tangente è zero,
		// e non è mai così forte da far uscire il valore dal tratto
		auto Tangente = [](float Prima, float Dopo)
		{
			if (Prima * Dopo <= 0.f)
			{
				return 0.f;
			}
			const float Limite = 3.f * FMath::Min(FMath::Abs(Prima), FMath::Abs(Dopo));
			return FMath::Clamp(0.5f * (Prima + Dopo), -Limite, Limite);
		};
		const float M1 = Tangente(V1 - V0, V2 - V1);
		const float M2 = Tangente(V2 - V1, V3 - V2);
		const float T = FMath::Clamp(U, 0.f, 1.f);
		const float T2 = T * T;
		const float T3 = T2 * T;
		const float Valore = (2.f * T3 - 3.f * T2 + 1.f) * V1 + (T3 - 2.f * T2 + T) * M1 + (-2.f * T3 + 3.f * T2) * V2 + (T3 - T2) * M2;
		return FMath::Clamp(Valore, FMath::Min(V1, V2), FMath::Max(V1, V2));
	}

	FQuat RotazioneMorbida(const FQuat& Q0, const FQuat& Q1, const FQuat& Q2, const FQuat& Q3, float U)
	{
		const FQuat A = Q1.GetNormalized();
		const FQuat Prima = TrailerStessaParte(Q0.GetNormalized(), A);
		const FQuat B = TrailerStessaParte(Q2.GetNormalized(), A);
		const FQuat Dopo = TrailerStessaParte(Q3.GetNormalized(), B);
		const FQuat TangenteA = TrailerTangente(Prima, A, B);
		const FQuat TangenteB = TrailerTangente(A, B, Dopo);
		// squad: tra le tappe e tra le tangenti, poi tra le due
		const double T = static_cast<double>(FMath::Clamp(U, 0.f, 1.f));
		const FQuat Tra = FQuat::Slerp(A, B, T);
		const FQuat TraTangenti = FQuat::Slerp(TangenteA, TangenteB, T);
		return FQuat::Slerp(Tra, TraTangenti, 2.0 * T * (1.0 - T)).GetNormalized();
	}

	FValdorsoTappaTrailer Posa(const FValdorsoPercorsoTrailer& Percorso, float Chiave)
	{
		const TArray<FValdorsoTappaTrailer>& Tappe = Percorso.Tappe;
		const int32 N = Tappe.Num();
		if (N == 0)
		{
			return FValdorsoTappaTrailer();
		}
		const int32 Tratti = Percorso.Tratti();
		if (Tratti <= 0)
		{
			return Tappe[0];
		}
		float K = Chiave;
		if (Percorso.bGiro)
		{
			K = FMath::Fmod(K, static_cast<float>(Tratti));
			if (K < 0.f)
			{
				K += static_cast<float>(Tratti);
			}
		}
		else
		{
			K = FMath::Clamp(K, 0.f, static_cast<float>(Tratti));
		}
		const int32 J = FMath::Clamp(FMath::FloorToInt(K), 0, Tratti - 1);
		const float U = FMath::Clamp(K - static_cast<float>(J), 0.f, 1.f);

		// le tappe vicine: ad anello si gira, aperto si ripete la prima e l'ultima
		auto Indice = [&](int32 I)
		{
			return Percorso.bGiro ? ((I % N) + N) % N : FMath::Clamp(I, 0, N - 1);
		};
		// aperto, prima della prima e dopo l'ultima si prosegue dritto (così la strada non si piega alle due estremità)
		auto Punto = [&](int32 I)
		{
			if (!Percorso.bGiro && I < 0)
			{
				return Tappe[0].Posizione * 2.0 - Tappe[1].Posizione;
			}
			if (!Percorso.bGiro && I >= N)
			{
				return Tappe[N - 1].Posizione * 2.0 - Tappe[N - 2].Posizione;
			}
			return Tappe[Indice(I)].Posizione;
		};
		// il fuoco: le tappe senza sfocatura (0) prendono quello della tappa sfocata più vicina
		auto Fuoco = [&](int32 I)
		{
			const int32 Qui = Indice(I);
			for (int32 Scarto = 0; Scarto < N; ++Scarto)
			{
				const int32 Avanti = FMath::Min(Qui + Scarto, N - 1);
				const int32 Indietro = FMath::Max(Qui - Scarto, 0);
				if (Tappe[Indietro].Fuoco > 0.f)
				{
					return Tappe[Indietro].Fuoco;
				}
				if (Tappe[Avanti].Fuoco > 0.f)
				{
					return Tappe[Avanti].Fuoco;
				}
			}
			return 0.f;
		};

		FValdorsoTappaTrailer Out;
		Out.Posizione = CatmullRomCentripeta(Punto(J - 1), Punto(J), Punto(J + 1), Punto(J + 2), U);
		Out.Rotazione = RotazioneMorbida(
			Tappe[Indice(J - 1)].Rotazione.Quaternion(), Tappe[Indice(J)].Rotazione.Quaternion(),
			Tappe[Indice(J + 1)].Rotazione.Quaternion(), Tappe[Indice(J + 2)].Rotazione.Quaternion(), U).Rotator();
		Out.Campo = ValoreMorbido(Tappe[Indice(J - 1)].Campo, Tappe[Indice(J)].Campo, Tappe[Indice(J + 1)].Campo, Tappe[Indice(J + 2)].Campo, U);
		Out.Fuoco = ValoreMorbido(Fuoco(J - 1), Fuoco(J), Fuoco(J + 1), Fuoco(J + 2), U);
		return Out;
	}

	float FattoreVelocita(float Partito, float Rimasta, float Velocita, float Rampa)
	{
		// la partenza: in Rampa secondi da ferma alla velocità piena
		const float Dentro = Rampa > 0.f ? FMath::SmoothStep(0.f, 1.f, Partito / Rampa) : 1.f;
		// l'arrivo: frena negli ultimi metri (quelli che servirebbero per fermarsi in Rampa secondi)
		float Fuori = 1.f;
		if (Rimasta >= 0.f && Rampa > 0.f && Velocita > 0.f)
		{
			const float Frenata = Velocita * Rampa * 0.5f;
			Fuori = FMath::SmoothStep(0.f, 1.f, FMath::Sqrt(FMath::Max(0.f, Rimasta) / Frenata));
		}
		// mai proprio fermi: all'inizio si parte e alla fine si arriva
		return FMath::Clamp(FMath::Min(Dentro, Fuori), 0.05f, 1.f);
	}

	float VelocitaTasto(int32 Numero)
	{
		switch (Numero)
		{
		case 1: return 200.f;
		case 2: return 400.f;
		case 3: return 800.f;
		case 4: return 1500.f;
		case 5: return 3000.f;
		default: return 0.f;
		}
	}

	float AltezzaBande(const FVector2D& Schermo, float Rapporto)
	{
		if (Schermo.X <= 0.0 || Schermo.Y <= 0.0 || Rapporto <= 0.f)
		{
			return 0.f;
		}
		const double Alta = Schermo.X / static_cast<double>(Rapporto);
		return static_cast<float>(FMath::Max(0.0, (Schermo.Y - Alta) * 0.5));
	}

	FString NomeValido(const FString& Nome)
	{
		const FString Spuntato = Nome.TrimStartAndEnd();
		FString Pulito;
		bool bQualcosa = false;
		for (const TCHAR C : Spuntato)
		{
			const bool bBuono = (C >= TEXT('a') && C <= TEXT('z')) || (C >= TEXT('A') && C <= TEXT('Z')) || (C >= TEXT('0') && C <= TEXT('9'))
				|| C == TEXT('-') || C == TEXT('_');
			Pulito.AppendChar(bBuono ? C : TEXT('_'));
			bQualcosa |= bBuono && C != TEXT('_');
		}
		Pulito = Pulito.Left(40);
		if (!bQualcosa)
		{
			return FString();
		}
		// i nomi che Windows tiene per sé (CON, NUL, COM1...)
		const FString Grande = Pulito.ToUpper();
		const bool bRiservato = Grande == TEXT("CON") || Grande == TEXT("PRN") || Grande == TEXT("AUX") || Grande == TEXT("NUL")
			|| ((Grande.StartsWith(TEXT("COM")) || Grande.StartsWith(TEXT("LPT"))) && Grande.Len() == 4 && FChar::IsDigit(Grande[3]));
		return bRiservato ? Pulito + TEXT("_") : Pulito;
	}

	FString InJson(const FValdorsoPercorsoTrailer& Percorso)
	{
		TSharedRef<FJsonObject> Oggetto = MakeShared<FJsonObject>();
		Oggetto->SetStringField(TEXT("gioco"), TEXT("Valdorso"));
		Oggetto->SetStringField(TEXT("tipo"), TEXT("trailer"));
		Oggetto->SetNumberField(TEXT("versione"), 1.0);
		Oggetto->SetNumberField(TEXT("velocita"), Percorso.Velocita);
		Oggetto->SetBoolField(TEXT("giro"), Percorso.bGiro);
		TArray<TSharedPtr<FJsonValue>> Lista;
		for (const FValdorsoTappaTrailer& Tappa : Percorso.Tappe)
		{
			TSharedRef<FJsonObject> Voce = MakeShared<FJsonObject>();
			Voce->SetNumberField(TEXT("x"), Tappa.Posizione.X);
			Voce->SetNumberField(TEXT("y"), Tappa.Posizione.Y);
			Voce->SetNumberField(TEXT("z"), Tappa.Posizione.Z);
			Voce->SetNumberField(TEXT("beccheggio"), Tappa.Rotazione.Pitch);
			Voce->SetNumberField(TEXT("imbardata"), Tappa.Rotazione.Yaw);
			Voce->SetNumberField(TEXT("rollio"), Tappa.Rotazione.Roll);
			Voce->SetNumberField(TEXT("campo"), Tappa.Campo);
			Voce->SetNumberField(TEXT("fuoco"), Tappa.Fuoco);
			Lista.Add(MakeShared<FJsonValueObject>(Voce));
		}
		Oggetto->SetArrayField(TEXT("tappe"), Lista);

		FString Testo;
		const TSharedRef<TJsonWriter<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>> Scrittore = TJsonWriterFactory<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>::Create(&Testo);
		FJsonSerializer::Serialize(Oggetto, Scrittore);
		return Testo;
	}

	bool DaJson(const FString& Testo, FValdorsoPercorsoTrailer& OutPercorso)
	{
		TSharedPtr<FJsonObject> Oggetto;
		const TSharedRef<TJsonReader<TCHAR>> Lettore = TJsonReaderFactory<TCHAR>::Create(Testo);
		if (!FJsonSerializer::Deserialize(Lettore, Oggetto) || !Oggetto.IsValid())
		{
			return false;
		}
		FString Gioco;
		FString Tipo;
		if (!Oggetto->TryGetStringField(TEXT("gioco"), Gioco) || Gioco != TEXT("Valdorso")
			|| !Oggetto->TryGetStringField(TEXT("tipo"), Tipo) || Tipo != TEXT("trailer"))
		{
			return false;
		}
		const TArray<TSharedPtr<FJsonValue>>* Lista = nullptr;
		if (!Oggetto->TryGetArrayField(TEXT("tappe"), Lista) || !Lista)
		{
			return false;
		}
		FValdorsoPercorsoTrailer Percorso;
		Percorso.Velocita = FMath::Clamp(static_cast<float>(TrailerNumero(Oggetto, TEXT("velocita"), 800.0)), 50.f, 10000.f);
		bool bGiro = false;
		Oggetto->TryGetBoolField(TEXT("giro"), bGiro);
		Percorso.bGiro = bGiro;
		for (const TSharedPtr<FJsonValue>& Valore : *Lista)
		{
			if (Percorso.Tappe.Num() >= MassimoTappe)
			{
				break;
			}
			if (!Valore.IsValid() || Valore->Type != EJson::Object)
			{
				continue;
			}
			const TSharedPtr<FJsonObject> Voce = Valore->AsObject();
			FValdorsoTappaTrailer Tappa;
			Tappa.Posizione = FVector(TrailerNumero(Voce, TEXT("x"), 0.0), TrailerNumero(Voce, TEXT("y"), 0.0), TrailerNumero(Voce, TEXT("z"), 0.0));
			Tappa.Rotazione = FRotator(TrailerNumero(Voce, TEXT("beccheggio"), 0.0), TrailerNumero(Voce, TEXT("imbardata"), 0.0), TrailerNumero(Voce, TEXT("rollio"), 0.0));
			Tappa.Campo = FMath::Clamp(static_cast<float>(TrailerNumero(Voce, TEXT("campo"), 90.0)), 15.f, 100.f);
			Tappa.Fuoco = FMath::Max(0.f, static_cast<float>(TrailerNumero(Voce, TEXT("fuoco"), 0.0)));
			Percorso.Tappe.Add(Tappa);
		}
		OutPercorso = Percorso;
		return true;
	}

	// --------------------------------------------------------------------------------------------
	// Il percorso di adesso e i file
	// --------------------------------------------------------------------------------------------

	FString Cartella()
	{
		return FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("Trailer"));
	}

	FValdorsoPercorsoTrailer& Corrente()
	{
		return TrailerPercorsoAdesso;
	}

	int32 Generazione()
	{
		return TrailerGenerazioneAdesso;
	}

	FString NomeCorrente()
	{
		return TrailerNomeAdesso;
	}

	bool Salva(const FString& Nome, FString& OutFile)
	{
		const FString Pulito = NomeValido(Nome);
		if (Pulito.IsEmpty())
		{
			return false;
		}
		IFileManager::Get().MakeDirectory(*Cartella(), true);
		OutFile = TrailerFileDi(Pulito);
		if (!FFileHelper::SaveStringToFile(InJson(TrailerPercorsoAdesso), *OutFile, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
		{
			return false;
		}
		if (Pulito != TrailerNomeUltimo)
		{
			TrailerNomeAdesso = Pulito;
		}
		return true;
	}

	bool Carica(const FString& Nome, FString& OutErrore)
	{
		const FString Pulito = NomeValido(Nome);
		if (Pulito.IsEmpty())
		{
			OutErrore = TEXT("il nome va scritto con lettere, numeri, - e _");
			return false;
		}
		const FString File = TrailerFileDi(Pulito);
		FString Testo;
		if (!FFileHelper::LoadFileToString(Testo, *File))
		{
			OutErrore = FString::Printf(TEXT("non trovo %s (Valdorso.Trailer.Elenco dice quali ci sono)"), *File);
			return false;
		}
		FValdorsoPercorsoTrailer Letto;
		if (!DaJson(Testo, Letto))
		{
			OutErrore = FString::Printf(TEXT("%s non è un percorso di Valdorso"), *File);
			return false;
		}
		TrailerPercorsoAdesso = Letto;
		++TrailerGenerazioneAdesso;
		bTrailerUltimoLetto = true;
		if (Pulito != TrailerNomeUltimo)
		{
			TrailerNomeAdesso = Pulito;
		}
		return true;
	}

	TArray<FString> Elenco()
	{
		TArray<FString> File;
		IFileManager::Get().FindFiles(File, *(Cartella() / TEXT("*.json")), true, false);
		TArray<FString> Nomi;
		for (const FString& Uno : File)
		{
			Nomi.Add(FPaths::GetBaseFilename(Uno));
		}
		Nomi.Sort();
		return Nomi;
	}

	void CaricaUltimoSeServe()
	{
		if (bTrailerUltimoLetto)
		{
			return;
		}
		bTrailerUltimoLetto = true;
		if (!FPaths::FileExists(TrailerFileDi(TrailerNomeUltimo)))
		{
			return;
		}
		// solo se adesso non c'è niente: le tappe messe prima (o caricate da console) non si perdono
		if (TrailerPercorsoAdesso.Tappe.Num() == 0)
		{
			FString Errore;
			if (Carica(TrailerNomeUltimo, Errore))
			{
				UE_LOG(LogValdorso, Log, TEXT("[Valdorso] Trailer: riletto l'ultimo percorso (%d tappe)"), TrailerPercorsoAdesso.Tappe.Num());
			}
			else
			{
				UE_LOG(LogValdorso, Warning, TEXT("[Valdorso] Trailer: %s"), *Errore);
			}
		}
	}

	void SalvaUltimo()
	{
		// mai usato il trailer: niente file (e niente cartella Saved/Trailer)
		if (TrailerPercorsoAdesso.Tappe.Num() == 0 && !FPaths::FileExists(TrailerFileDi(TrailerNomeUltimo)))
		{
			return;
		}
		FString File;
		if (!Salva(TrailerNomeUltimo, File))
		{
			UE_LOG(LogValdorso, Warning, TEXT("[Valdorso] Trailer: non riesco a salvare l'ultimo percorso in %s"), *File);
		}
	}

	bool Fotogrammi()
	{
		return CVarTrailerFotogrammi.GetValueOnGameThread() != 0;
	}
}
