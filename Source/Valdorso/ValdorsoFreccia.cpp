// Valdorso - La freccia (vedi il .h). Scritto da Claude il 14/11/2026.

#include "ValdorsoFreccia.h"
#include "Valdorso.h"
#include "ValdorsoArmi.h"
#include "ValdorsoCombattimento.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/ProjectileMovementComponent.h"

AValdorsoFreccia::AValdorsoFreccia()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicatingMovement(true);
	InitialLifeSpan = ValdorsoFreccia::SecondiVolo;

	// la punta: una sfera piccola che si ferma su tutto (le persone, gli animali, i muri, il terreno)
	Punta = CreateDefaultSubobject<USphereComponent>(TEXT("Punta"));
	Punta->InitSphereRadius(4.f);
	Punta->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Punta->SetCollisionObjectType(ECC_WorldDynamic);
	Punta->SetCollisionResponseToAllChannels(ECR_Block);
	Punta->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	Punta->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
	Punta->SetCanEverAffectNavigation(false);
	Punta->SetGenerateOverlapEvents(false);
	RootComponent = Punta;

	// l'asta: si vede e basta
	Asta = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Asta"));
	Asta->SetupAttachment(Punta);
	Asta->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Asta->SetGenerateOverlapEvents(false);
	Asta->SetCanEverAffectNavigation(false);
	Asta->SetCastShadow(false);

	// il volo: dritta (la velocita' e' la stessa sul server e sui PC: parte dal verso in cui nasce), con un po' di gravita'
	Volo = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("Volo"));
	Volo->UpdatedComponent = Punta;
	Volo->InitialSpeed = ValdorsoArmi::VelocitaFreccia;
	Volo->MaxSpeed = ValdorsoArmi::VelocitaFreccia;
	Volo->bRotationFollowsVelocity = true;
	Volo->bShouldBounce = false;
	Volo->ProjectileGravityScale = ValdorsoFreccia::Gravita;
}

AValdorsoFreccia* AValdorsoFreccia::Scocca(APawn* Chi, const FVector& Da, const FVector& Verso, float InDanno, float InEquilibrio)
{
	UWorld* Mondo = Chi ? Chi->GetWorld() : nullptr;
	if (!Mondo || !Chi->HasAuthority() || Verso.IsNearlyZero())
	{
		return nullptr;
	}
	FActorSpawnParameters Come;
	Come.Owner = Chi;
	Come.Instigator = Chi;
	Come.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AValdorsoFreccia* Freccia = Mondo->SpawnActor<AValdorsoFreccia>(AValdorsoFreccia::StaticClass(), Da, Verso.Rotation(), Come);
	if (Freccia)
	{
		Freccia->Danno = InDanno;
		Freccia->Equilibrio = InEquilibrio;
	}
	return Freccia;
}

void AValdorsoFreccia::BeginPlay()
{
	Super::BeginPlay();
	// chi la tira non si colpisce da solo (anche sui PC: chi l'ha tirata arriva con la freccia)
	if (APawn* Chi = GetInstigator())
	{
		Punta->IgnoreActorWhenMoving(Chi, true);
	}
	Volo->OnProjectileStop.AddDynamic(this, &AValdorsoFreccia::QuandoSiFerma);
	if (GetNetMode() == NM_DedicatedServer)
	{
		return;
	}
	// il modello: quello del generatore, o un'asta grigia (il cilindro del motore, steso lungo la freccia)
	if (UStaticMesh* Modello = LoadObject<UStaticMesh>(nullptr, *ValdorsoArmi::PercorsoModello(TEXT("Freccia")), nullptr, LOAD_NoWarn | LOAD_Quiet))
	{
		Asta->SetStaticMesh(Modello);
		// l'asta del generatore va da -35 a +37 cm (la punta in +X): cosi' la punta sta sulla sfera
		Asta->SetRelativeLocation(FVector(-35.f, 0.f, 0.f));
	}
	else if (UStaticMesh* Cilindro = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"), nullptr, LOAD_NoWarn | LOAD_Quiet))
	{
		Asta->SetStaticMesh(Cilindro);
		Asta->SetRelativeRotation(FRotator(-90.f, 0.f, 0.f));
		Asta->SetRelativeLocation(FVector(-35.f, 0.f, 0.f));
		Asta->SetRelativeScale3D(FVector(0.012f, 0.012f, 0.7f));
	}
}

void AValdorsoFreccia::QuandoSiFerma(const FHitResult& Colpo)
{
	if (bArrivata)
	{
		return;
	}
	bArrivata = true;
	Punta->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	AActor* Altro = Colpo.GetActor();
	// il colpo: lo decide solo il server
	if (HasAuthority() && Altro && Altro != GetInstigator())
	{
		ValdorsoCombattimento::Colpisci(GetInstigator(), Altro, Danno, Equilibrio, Colpo.ImpactPoint, EValdorsoTipoColpo::Lama);
	}
	// resta piantata dove ha colpito (e si muove con chi la porta)
	if (USceneComponent* Dove = Colpo.GetComponent())
	{
		AttachToComponent(Dove, FAttachmentTransformRules::KeepWorldTransform, Colpo.BoneName);
	}
	SetLifeSpan(ValdorsoFreccia::SecondiPiantata);
}
