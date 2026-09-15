// Copyright Epic Games, Inc. All Rights Reserved.

#include "ATLAAbility_IceWall.h"
#include "ATLAGameplayTags.h"
#include "ATLAGameplayEffects.h"
#include "ATLAIceWall.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"

UATLAAbility_IceWall::UATLAAbility_IceWall()
{
	ElementTag = ATLATags::Element_Water;
	WallClass = AATLAIceWall::StaticClass();

	SetAssetTags(FGameplayTagContainer(ATLATags::Ability_Water_IceWall));

	CostGameplayEffectClass = UGE_Cost_IceWall::StaticClass();
	CooldownGameplayEffectClass = UGE_Cooldown_IceWall::StaticClass();

	// Heavier technique: slower wind-up, longer recovery
	CastMontage = TSoftObjectPtr<UAnimMontage>(FSoftObjectPath(TEXT("/Game/Variant_Combat/Anims/AM_ChargedAttack.AM_ChargedAttack")));
	MontageRate = 1.4f;
	CastDelay = 0.7f;
	CastDuration = 1.5f;
	MovementScaleDuringCast = 0.0f;
	UpperBodyCastMontage = TSoftObjectPtr<UAnimMontage>(FSoftObjectPath(TEXT("/Game/Bending/Anims/AM_HeavyCast_Upper.AM_HeavyCast_Upper")));
	// Tai chi form — rising wall gesture
	UpperBodyCastSequence = TSoftObjectPtr<UAnimSequenceBase>(FSoftObjectPath(TEXT("/Game/Anims/CMU_Manny/MNY_taichi_full.MNY_taichi_full")));
	SequenceStartTimes = { 62.0f };
	MontageRate = 0.9f;
}

void UATLAAbility_IceWall::OnCast()
{
	ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (!Character || !Character->HasAuthority() || !WallClass)
	{
		return;
	}

	// The wall rises perpendicular to the camera's yaw, planted at foot level
	const float Yaw = Character->GetControlRotation().Yaw;
	const FRotator WallRotation(0.f, Yaw, 0.f);
	const float FootZ = Character->GetActorLocation().Z - Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	FVector SpawnLocation = Character->GetActorLocation() + WallRotation.Vector() * WallDistance;
	SpawnLocation.Z = FootZ;

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = Character;
	SpawnParams.Instigator = Character;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	Character->GetWorld()->SpawnActor<AATLAIceWall>(WallClass, SpawnLocation, WallRotation, SpawnParams);
}
