// Copyright Epic Games, Inc. All Rights Reserved.

#include "ATLAAttributeSet.h"
#include "ATLACharacter.h"
#include "ATLAGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffectExtension.h"
#include "Net/UnrealNetwork.h"

UATLAAttributeSet::UATLAAttributeSet()
{
	InitHealth(100.f);
	InitMaxHealth(100.f);
	InitChi(100.f);
	InitMaxChi(100.f);
	InitWater(40.f);
	InitMaxWater(100.f);
	InitEarth(60.f);
	InitMaxEarth(100.f);
}

void UATLAAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(UATLAAttributeSet, Health, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UATLAAttributeSet, MaxHealth, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UATLAAttributeSet, Chi, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UATLAAttributeSet, MaxChi, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UATLAAttributeSet, Water, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UATLAAttributeSet, MaxWater, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UATLAAttributeSet, Earth, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UATLAAttributeSet, MaxEarth, COND_None, REPNOTIFY_Always);
}

void UATLAAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	if (Attribute == GetHealthAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxHealth());
	}
	else if (Attribute == GetChiAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxChi());
	}
	else if (Attribute == GetWaterAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxWater());
	}
	else if (Attribute == GetEarthAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxEarth());
	}
}

void UATLAAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	// Re-clamp after effects execute so overheals/overspends never persist
	if (Data.EvaluatedData.Attribute == GetHealthAttribute())
	{
		// Earth damage reduction: rooted 30%, armored 40% (armored wins if both).
		// Implemented as a refund of the blocked fraction of incoming damage.
		float Delta = Data.EvaluatedData.Magnitude;
		if (Delta < 0.f)
		{
			// The victim remembers the blow: cools lightning's calm gate and
			// stokes a firebender's inner drive
			if (AATLACharacter* Victim = Cast<AATLACharacter>(GetOwningActor()))
			{
				Victim->NoteDamageTaken();
			}

			// Inner drive: a stoked firebender's flames burn hotter. Only fire
			// benders ever carry these tags, so this amp is fire-only in practice.
			const FGameplayTagContainer* DriveTags = Data.EffectSpec.CapturedSourceTags.GetAggregatedTags();
			if (DriveTags)
			{
				float DriveAmp = 0.f;
				if (DriveTags->HasTag(ATLATags::State_Fire_Blazing))
				{
					DriveAmp = 0.35f;
				}
				else if (DriveTags->HasTag(ATLATags::State_Fire_Stoked))
				{
					DriveAmp = 0.20f;
				}
				if (DriveAmp > 0.f)
				{
					SetHealth(GetHealth() + Delta * DriveAmp);
					Delta *= 1.f + DriveAmp;
				}
			}

			// Empowered attacker: an active signature form amplifies ALL damage
			// the bender deals by 50% (applied as extra damage on top)
			const FGameplayTagContainer* SourceTags = Data.EffectSpec.CapturedSourceTags.GetAggregatedTags();
			if (SourceTags && SourceTags->HasTag(ATLATags::State_Bending_Empowered))
			{
				SetHealth(GetHealth() + Delta * 0.5f);
				Delta *= 1.5f;  // reductions below act on the amplified hit
			}

			if (UAbilitySystemComponent* OwnerASC = GetOwningAbilitySystemComponent())
			{
				float Reduction = 0.f;
				if (OwnerASC->HasMatchingGameplayTag(ATLATags::State_Earth_Armored))
				{
					Reduction = 0.4f;
				}
				else if (OwnerASC->HasMatchingGameplayTag(ATLATags::State_Earth_Rooted))
				{
					Reduction = 0.3f;
				}
				if (Reduction > 0.f)
				{
					SetHealth(GetHealth() - Delta * Reduction);
				}
			}
		}
		SetHealth(FMath::Clamp(GetHealth(), 0.f, GetMaxHealth()));
	}
	else if (Data.EvaluatedData.Attribute == GetChiAttribute())
	{
		SetChi(FMath::Clamp(GetChi(), 0.f, GetMaxChi()));
	}
	else if (Data.EvaluatedData.Attribute == GetWaterAttribute())
	{
		SetWater(FMath::Clamp(GetWater(), 0.f, GetMaxWater()));
	}
	else if (Data.EvaluatedData.Attribute == GetEarthAttribute())
	{
		SetEarth(FMath::Clamp(GetEarth(), 0.f, GetMaxEarth()));
	}
}

void UATLAAttributeSet::OnRep_Health(const FGameplayAttributeData& OldHealth)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UATLAAttributeSet, Health, OldHealth);
}

void UATLAAttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UATLAAttributeSet, MaxHealth, OldMaxHealth);
}

void UATLAAttributeSet::OnRep_Chi(const FGameplayAttributeData& OldChi)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UATLAAttributeSet, Chi, OldChi);
}

void UATLAAttributeSet::OnRep_MaxChi(const FGameplayAttributeData& OldMaxChi)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UATLAAttributeSet, MaxChi, OldMaxChi);
}

void UATLAAttributeSet::OnRep_Water(const FGameplayAttributeData& OldWater)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UATLAAttributeSet, Water, OldWater);
}

void UATLAAttributeSet::OnRep_MaxWater(const FGameplayAttributeData& OldMaxWater)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UATLAAttributeSet, MaxWater, OldMaxWater);
}

void UATLAAttributeSet::OnRep_Earth(const FGameplayAttributeData& OldEarth)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UATLAAttributeSet, Earth, OldEarth);
}

void UATLAAttributeSet::OnRep_MaxEarth(const FGameplayAttributeData& OldMaxEarth)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UATLAAttributeSet, MaxEarth, OldMaxEarth);
}
