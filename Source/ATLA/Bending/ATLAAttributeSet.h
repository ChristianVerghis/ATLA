// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "ATLAAttributeSet.generated.h"

// Standard accessor boilerplate for gameplay attributes
#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

/**
 * Core attributes for a bender: health and chi.
 * Chi is the resource spent by bending abilities and regenerated over time.
 */
UCLASS()
class ATLA_API UATLAAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	UATLAAttributeSet();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Clamp attribute changes before they are applied */
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

	UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_Health)
	FGameplayAttributeData Health;
	ATTRIBUTE_ACCESSORS(UATLAAttributeSet, Health)

	UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_MaxHealth)
	FGameplayAttributeData MaxHealth;
	ATTRIBUTE_ACCESSORS(UATLAAttributeSet, MaxHealth)

	UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_Chi)
	FGameplayAttributeData Chi;
	ATTRIBUTE_ACCESSORS(UATLAAttributeSet, Chi)

	UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_MaxChi)
	FGameplayAttributeData MaxChi;
	ATTRIBUTE_ACCESSORS(UATLAAttributeSet, MaxChi)

	/** Carried water — the bender's inventory. No passive regen; refill at water sources. */
	UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_Water)
	FGameplayAttributeData Water;
	ATTRIBUTE_ACCESSORS(UATLAAttributeSet, Water)

	UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_MaxWater)
	FGameplayAttributeData MaxWater;
	ATTRIBUTE_ACCESSORS(UATLAAttributeSet, MaxWater)

	/** Positional earth ammo — regenerates only while standing on bendable ground */
	UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_Earth)
	FGameplayAttributeData Earth;
	ATTRIBUTE_ACCESSORS(UATLAAttributeSet, Earth)

	UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_MaxEarth)
	FGameplayAttributeData MaxEarth;
	ATTRIBUTE_ACCESSORS(UATLAAttributeSet, MaxEarth)

protected:
	UFUNCTION()
	void OnRep_Health(const FGameplayAttributeData& OldHealth);

	UFUNCTION()
	void OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth);

	UFUNCTION()
	void OnRep_Chi(const FGameplayAttributeData& OldChi);

	UFUNCTION()
	void OnRep_MaxChi(const FGameplayAttributeData& OldMaxChi);

	UFUNCTION()
	void OnRep_Water(const FGameplayAttributeData& OldWater);

	UFUNCTION()
	void OnRep_MaxWater(const FGameplayAttributeData& OldMaxWater);

	UFUNCTION()
	void OnRep_Earth(const FGameplayAttributeData& OldEarth);

	UFUNCTION()
	void OnRep_MaxEarth(const FGameplayAttributeData& OldMaxEarth);
};
