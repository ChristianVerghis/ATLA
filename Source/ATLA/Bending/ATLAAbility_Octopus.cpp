// Copyright Epic Games, Inc. All Rights Reserved.

#include "ATLAAbility_Octopus.h"
#include "ATLAGameplayTags.h"
#include "ATLAGameplayEffects.h"
#include "ATLAOctopusForm.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"

UATLAAbility_Octopus::UATLAAbility_Octopus()
{
	ElementTag = ATLATags::Element_Water;
	FormClass = AATLAOctopusForm::StaticClass();

	SetAssetTags(FGameplayTagContainer(ATLATags::Ability_Water_Octopus));

	CostGameplayEffectClass = UGE_Cost_Octopus::StaticClass();
	CooldownGameplayEffectClass = UGE_Cooldown_Octopus::StaticClass();

	// The grandest technique gets the biggest wind-up
	CastMontage = TSoftObjectPtr<UAnimMontage>(FSoftObjectPath(TEXT("/Game/Variant_Combat/Anims/AM_ChargedAttack.AM_ChargedAttack")));
	MontageRate = 1.3f;
	CastDelay = 0.7f;
	CastDuration = 1.6f;
	MovementScaleDuringCast = 0.0f;
	UpperBodyCastMontage = TSoftObjectPtr<UAnimMontage>(FSoftObjectPath(TEXT("/Game/Bending/Anims/AM_HeavyCast_Upper.AM_HeavyCast_Upper")));
	// Tai chi form — the octopus opening stance
	UpperBodyCastSequence = TSoftObjectPtr<UAnimSequenceBase>(FSoftObjectPath(TEXT("/Game/Anims/CMU_Manny/MNY_taichi_full.MNY_taichi_full")));
	SequenceStartTimes = { 75.0f };
	MontageRate = 1.20f;
}

void UATLAAbility_Octopus::OnCast()
{
	ApplyEmpowered();  // the signature form supercharges regular attacks for 10s
	ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (!Character || !Character->HasAuthority() || !FormClass)
	{
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = Character;
	SpawnParams.Instigator = Character;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	Character->GetWorld()->SpawnActor<AATLAOctopusForm>(FormClass, Character->GetActorLocation(), FRotator::ZeroRotator, SpawnParams);
}
