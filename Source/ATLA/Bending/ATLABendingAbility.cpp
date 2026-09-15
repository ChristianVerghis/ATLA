// Copyright Epic Games, Inc. All Rights Reserved.

#include "ATLABendingAbility.h"
#include "ATLAGameplayTags.h"
#include "ATLAGameplayEffects.h"
#include "AbilitySystemComponent.h"

UATLABendingAbility::UATLABendingAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

	// Every bending ability marks its owner as bending while active,
	// and dead characters can never bend
	ActivationOwnedTags.AddTag(ATLATags::State_Bending);
	ActivationBlockedTags.AddTag(ATLATags::State_Dead);
}

void UATLABendingAbility::ApplyEmpowered()
{
	const AActor* Avatar = GetAvatarActorFromActorInfo();
	if (!Avatar || !Avatar->HasAuthority())
	{
		return;
	}
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		const FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(UGE_Empowered::StaticClass(), 1.f, ASC->MakeEffectContext());
		if (Spec.IsValid())
		{
			ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
		}
	}
}
