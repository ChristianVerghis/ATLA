// Copyright Epic Games, Inc. All Rights Reserved.

#include "ATLAAbility_DrawWater.h"
#include "ATLAGameplayTags.h"
#include "ATLAGameplayEffects.h"
#include "ATLAAttributeSet.h"
#include "ATLAWaterSource.h"
#include "ATLAWaterSplash.h"
#include "AbilitySystemComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "TimerManager.h"

UATLAAbility_DrawWater::UATLAAbility_DrawWater()
{
	ElementTag = ATLATags::Element_Water;
	SetAssetTags(FGameplayTagContainer(ATLATags::Ability_Water_Pull));
}

void UATLAAbility_DrawWater::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	AActor* Avatar = GetAvatarActorFromActorInfo();
	if (!Avatar)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	CurrentSource = AATLAWaterSource::FindSourceInRange(Avatar->GetWorld(), Avatar->GetActorLocation());
	if (!CurrentSource)
	{
		if (GEngine && ActorInfo->IsLocallyControlled())
		{
			GEngine->AddOnScreenDebugMessage(1, 1.5f, FColor::Orange, TEXT("No water within reach"));
		}
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// The periodic intake runs server-side; clients see the attribute replicate
	if (Avatar->HasAuthority())
	{
		UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
		const FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(UGE_DrawWater::StaticClass(), 1.f, ASC->MakeEffectContext());
		if (Spec.IsValid())
		{
			DrawEffectHandle = ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
		}
	}

	VisualAccumulator = 0.f;
	Avatar->GetWorldTimerManager().SetTimer(CheckTimer, FTimerDelegate::CreateUObject(this, &UATLAAbility_DrawWater::TickDraw), CheckInterval, true);
}

void UATLAAbility_DrawWater::TickDraw()
{
	AActor* Avatar = GetAvatarActorFromActorInfo();
	const UATLAAttributeSet* Attributes = GetAbilitySystemComponentFromActorInfo() ? GetAbilitySystemComponentFromActorInfo()->GetSet<UATLAAttributeSet>() : nullptr;

	const bool bOutOfRange = !Avatar || !CurrentSource ||
		FVector::DistSquared(Avatar->GetActorLocation(), CurrentSource->GetActorLocation()) > FMath::Square(CurrentSource->DrawRange);
	const bool bFull = Attributes && Attributes->GetWater() >= Attributes->GetMaxWater();

	if (bOutOfRange || bFull)
	{
		EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
		return;
	}

	// Splash on the pool surface while channeling, roughly twice a second
	VisualAccumulator += CheckInterval;
	if (Avatar->HasAuthority() && VisualAccumulator >= 0.5f)
	{
		VisualAccumulator = 0.f;
		const FVector Toward = (Avatar->GetActorLocation() - CurrentSource->GetActorLocation()).GetSafeNormal2D();
		const FVector SpawnAt = CurrentSource->GetActorLocation() + Toward * 120.f + FVector(0.f, 0.f, 30.f);
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Avatar->GetWorld()->SpawnActor<AATLAWaterSplash>(AATLAWaterSplash::StaticClass(), SpawnAt, FRotator::ZeroRotator, SpawnParams);
	}
}

void UATLAAbility_DrawWater::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (const AActor* Avatar = GetAvatarActorFromActorInfo())
	{
		Avatar->GetWorldTimerManager().ClearTimer(CheckTimer);
	}

	if (DrawEffectHandle.IsValid())
	{
		if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
		{
			ASC->RemoveActiveGameplayEffect(DrawEffectHandle);
		}
		DrawEffectHandle.Invalidate();
	}

	CurrentSource = nullptr;
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
