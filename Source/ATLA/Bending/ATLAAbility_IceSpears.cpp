// Copyright Epic Games, Inc. All Rights Reserved.

#include "ATLAAbility_IceSpears.h"
#include "ATLAGameplayTags.h"
#include "ATLAGameplayEffects.h"
#include "ATLAIceSpear.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"

UATLAAbility_IceSpears::UATLAAbility_IceSpears()
{
	ElementTag = ATLATags::Element_Water;
	SpearClass = AATLAIceSpear::StaticClass();

	SetAssetTags(FGameplayTagContainer(ATLATags::Ability_Water_IceSpears));

	CostGameplayEffectClass = UGE_Cost_IceSpears::StaticClass();
	CooldownGameplayEffectClass = UGE_Cooldown_IceSpears::StaticClass();

	// Sharp, fast throwing form
	MontageRate = 1.5f;
	CastDelay = 0.4f;
	CastDuration = 1.0f;
	MovementScaleDuringCast = 0.0f;
	UpperBodyCastMontage = TSoftObjectPtr<UAnimMontage>(FSoftObjectPath(TEXT("/Game/Bending/Anims/AM_WaterStrikes_Upper.AM_WaterStrikes_Upper")));
	// Tai chi pushes at the form's measured strongest moments (re-scanned on
	// the pinned-pelvis retarget), sped up so the flowing shape reads as a
	// sharp volley cast
	UpperBodyCastSequence = TSoftObjectPtr<UAnimSequenceBase>(FSoftObjectPath(TEXT("/Game/Anims/Mixamo/MXQ_MX_Standing_2H_Magic_Attack_02.MXQ_MX_Standing_2H_Magic_Attack_02")));
	SequenceStartTimes = { 0.78f };
	MontageRate = 1.30f;
}

void UATLAAbility_IceSpears::OnCast()
{
	ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (!Character || !Character->HasAuthority() || !SpearClass || SpearCount < 1)
	{
		return;
	}

	const FVector SpawnLocation = Character->GetActorLocation() + Character->GetControlRotation().Vector() * 80.f + FVector(0.f, 0.f, 40.f);
	const FRotator AimRotation = GetCrosshairAimRotation(SpawnLocation);

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = Character;
	SpawnParams.Instigator = Character;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	const float Step = SpearCount > 1 ? FanSpread / (SpearCount - 1) : 0.f;
	TArray<AATLAIceSpear*> Volley;
	for (int32 i = 0; i < SpearCount; ++i)
	{
		const float YawOffset = -FanSpread * 0.5f + Step * i;
		const FRotator SpearRotation = AimRotation + FRotator(0.f, YawOffset, 0.f);

		// Stagger spawn points slightly along the fan so the volley never overlaps
		const FVector Offset = SpearRotation.Vector() * 30.f;
		AATLAIceSpear* Spear = Character->GetWorld()->SpawnActor<AATLAIceSpear>(SpearClass, SpawnLocation + Offset, SpearRotation, SpawnParams);
		if (Spear)
		{
			Volley.Add(Spear);
		}
	}

	// Sibling spears pass through each other
	for (AATLAIceSpear* A : Volley)
	{
		for (AATLAIceSpear* B : Volley)
		{
			if (A != B)
			{
				A->IgnoreActorForMovement(B);
			}
		}
	}
}
