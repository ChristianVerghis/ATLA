// Copyright Epic Games, Inc. All Rights Reserved.

#include "ATLAGameMode.h"
#include "ATLAHUD.h"
#include "ATLACharacter.h"
#include "AI/ATLABenderAI.h"
#include "World/ATLAHabitats.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerStart.h"
#include "Kismet/GameplayStatics.h"

AATLAGameMode::AATLAGameMode()
{
	HUDClass = AATLAHUD::StaticClass();
	EnemyPawnClass = TSoftClassPtr<APawn>(FSoftObjectPath(
		TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter.BP_ThirdPersonCharacter_C")));
}

void AATLAGameMode::BeginPlay()
{
	Super::BeginPlay();

	// Habitat dressing: the coliseum option plus each square's original bender
	{
		FActorSpawnParameters DressParams;
		DressParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		GetWorld()->SpawnActor<AATLAColiseum>(AATLAColiseum::StaticClass(),
			FVector(-9000.f, 3000.f, 0.f), FRotator::ZeroRotator, DressParams);

		struct FDress { EATLACreatureStyle Style; FVector Loc; };
		const FDress Creatures[] = {
			{ EATLACreatureStyle::KoiPair,    AATLACharacter::GetZoneAnchor(EATLAElement::Water) + FVector(900.f, 900.f, -280.f) },
			{ EATLACreatureStyle::Badgermole, AATLACharacter::GetZoneAnchor(EATLAElement::Earth) + FVector(1400.f, 1100.f, -240.f) },
			{ EATLACreatureStyle::Dragons,    AATLACharacter::GetZoneAnchor(EATLAElement::Fire) + FVector(0.f, 0.f, -240.f) },
			{ EATLACreatureStyle::SkyBison,   AATLACharacter::GetZoneAnchor(EATLAElement::Air) + FVector(0.f, 0.f, -240.f) },
		};
		const EATLAElement ZoneElems[] = { EATLAElement::Water, EATLAElement::Earth, EATLAElement::Fire, EATLAElement::Air };
		for (int32 z = 0; z < 4; ++z)
		{
			const FVector Loc = AATLACharacter::GetZoneAnchor(ZoneElems[z]) - FVector(0.f, 0.f, 260.f);
			if (AATLAHabitatZone* Zone = GetWorld()->SpawnActorDeferred<AATLAHabitatZone>(
				AATLAHabitatZone::StaticClass(), FTransform(Loc)))
			{
				Zone->Element = static_cast<uint8>(ZoneElems[z]);
				Zone->FinishSpawning(FTransform(Loc));
			}
		}

		for (const FDress& D : Creatures)
		{
			// Deferred so Style lands BEFORE BeginPlay builds the body
			if (AATLAHabitatCreature* Creature = GetWorld()->SpawnActorDeferred<AATLAHabitatCreature>(
				AATLAHabitatCreature::StaticClass(), FTransform(D.Loc)))
			{
				Creature->Style = D.Style;
				Creature->FinishSpawning(FTransform(D.Loc));
			}
		}
	}

	if (!bSpawnEnemyBender)
	{
		return;
	}
	UClass* PawnClass = EnemyPawnClass.LoadSynchronous();
	if (!PawnClass)
	{
		return;
	}

	// Face-off spawn: ahead of the player start, looking back at it
	FVector Base = FVector::ZeroVector;
	FRotator Facing = FRotator::ZeroRotator;
	if (TActorIterator<APlayerStart> It(GetWorld()); It)
	{
		Base = It->GetActorLocation();
		Facing = It->GetActorRotation();
	}
	const FVector SpawnLoc = Base + Facing.Vector() * 900.f + FVector(0.f, 0.f, 20.f);

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	APawn* Enemy = GetWorld()->SpawnActor<APawn>(PawnClass, SpawnLoc, (Facing + FRotator(0.f, 180.f, 0.f)).GetNormalized(), SpawnParams);
	if (!Enemy)
	{
		return;
	}

	// A random element each match keeps the sparring fresh
	if (AATLACharacter* Bender = Cast<AATLACharacter>(Enemy))
	{
		static const EATLAElement Elements[] = {
			EATLAElement::Water, EATLAElement::Earth, EATLAElement::Fire, EATLAElement::Air };
		Bender->SetElementLoadout(Elements[FMath::RandRange(0, 3)]);
	}

	if (AATLABenderAI* Brain = GetWorld()->SpawnActor<AATLABenderAI>(AATLABenderAI::StaticClass()))
	{
		Brain->Possess(Enemy);
	}
}
