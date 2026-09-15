// Copyright Epic Games, Inc. All Rights Reserved.

#include "ATLAHUD.h"
#include "ATLAAttributeSet.h"
#include "ATLAGameplayTags.h"
#include "ATLACharacter.h"
#include "ATLAPlayerController.h"
#include "AbilitySystemComponent.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"

void AATLAHUD::DrawHUD()
{
	Super::DrawHUD();

	const AATLACharacter* Character = Cast<AATLACharacter>(GetOwningPawn());
	if (!Character || !Character->GetAbilitySystemComponent())
	{
		return;
	}

	const UATLAAttributeSet* Attributes = Character->GetAbilitySystemComponent()->GetSet<UATLAAttributeSet>();
	if (!Attributes)
	{
		return;
	}

	// Duel meters: both benders' health top-center while an opponent stands
	if (const AATLAPlayerController* PC = Cast<AATLAPlayerController>(GetOwningPlayerController()))
	{
		if (const AATLACharacter* Foe = Cast<AATLACharacter>(PC->GetSparringPartner()))
		{
			if (const UAbilitySystemComponent* FoeASC = Foe->GetAbilitySystemComponent())
			{
				if (const UATLAAttributeSet* FoeAttr = FoeASC->GetSet<UATLAAttributeSet>())
				{
					const float W = 340.f;
					const float H = 20.f;
					const float Mid = Canvas->SizeX * 0.5f;
					const float Y = 36.f;
					DrawBar(Mid - W - 30.f, Y, W, H,
						Attributes->GetMaxHealth() > 0.f ? Attributes->GetHealth() / Attributes->GetMaxHealth() : 0.f,
						FLinearColor(0.15f, 0.75f, 0.25f),
						FString::Printf(TEXT("YOU  %.0f"), Attributes->GetHealth()));
					DrawBar(Mid + 30.f, Y, W, H,
						FoeAttr->GetMaxHealth() > 0.f ? FoeAttr->GetHealth() / FoeAttr->GetMaxHealth() : 0.f,
						FLinearColor(0.8f, 0.2f, 0.15f),
						FString::Printf(TEXT("OPPONENT  %.0f"), FoeAttr->GetHealth()));
					DrawText(TEXT("DUEL"), FLinearColor(1.f, 0.85f, 0.3f), Mid - 18.f, Y - 22.f, nullptr, 1.2f);
				}
			}
		}
	}

	// Aim dot: dark outline, white center
	const float CX = Canvas->SizeX * 0.5f;
	const float CY = Canvas->SizeY * 0.5f;
	DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.6f), CX - 3.f, CY - 3.f, 6.f, 6.f);
	DrawRect(FLinearColor(1.f, 1.f, 1.f, 0.9f), CX - 1.5f, CY - 1.5f, 3.f, 3.f);

	const float BarWidth = 260.f;
	const float BarHeight = 16.f;
	const float X = 40.f;
	const float YBase = Canvas->SizeY - 124.f;

	// Signature form active: everything hits harder and bigger
	if (Character->GetAbilitySystemComponent()->HasMatchingGameplayTag(ATLATags::State_Bending_Empowered))
	{
		DrawText(TEXT("~ EMPOWERED ~  attacks amplified"), FLinearColor(1.f, 0.85f, 0.25f), X, YBase - 24.f, GEngine->GetSmallFont());
	}

	DrawBar(X, YBase, BarWidth, BarHeight,
		Attributes->GetMaxHealth() > 0.f ? Attributes->GetHealth() / Attributes->GetMaxHealth() : 0.f,
		FLinearColor(0.15f, 0.8f, 0.25f),
		FString::Printf(TEXT("Health  %.0f / %.0f"), Attributes->GetHealth(), Attributes->GetMaxHealth()));

	DrawBar(X, YBase + 34.f, BarWidth, BarHeight,
		Attributes->GetMaxChi() > 0.f ? Attributes->GetChi() / Attributes->GetMaxChi() : 0.f,
		FLinearColor(0.95f, 0.8f, 0.2f),
		FString::Printf(TEXT("Chi     %.0f / %.0f"), Attributes->GetChi(), Attributes->GetMaxChi()));

	// The element resource bar follows the loadout (fire and air run on chi)
	switch (Character->GetElementLoadout())
	{
	case EATLAElement::Water:
		if (Character->IsNearWaterSource())
		{
			// At the source the meter is meaningless — the pool is the reserve
			DrawBar(X, YBase + 68.f, BarWidth, BarHeight, 1.f,
				FLinearColor(0.3f, 0.75f, 1.f),
				TEXT("Water   ~ bending the source ~"));
		}
		else
		{
			DrawBar(X, YBase + 68.f, BarWidth, BarHeight,
				Attributes->GetMaxWater() > 0.f ? Attributes->GetWater() / Attributes->GetMaxWater() : 0.f,
				FLinearColor(0.15f, 0.55f, 1.f),
				FString::Printf(TEXT("Water   %.0f / %.0f (reserve)"), Attributes->GetWater(), Attributes->GetMaxWater()));
		}
		break;
	case EATLAElement::Earth:
		DrawBar(X, YBase + 68.f, BarWidth, BarHeight,
			Attributes->GetMaxEarth() > 0.f ? Attributes->GetEarth() / Attributes->GetMaxEarth() : 0.f,
			FLinearColor(0.55f, 0.38f, 0.18f),
			FString::Printf(TEXT("Earth   %.0f / %.0f"), Attributes->GetEarth(), Attributes->GetMaxEarth()));
		// Mastery track: land earth hits to unlock metalbending, then keep it
		DrawBar(X, YBase + 102.f, BarWidth, BarHeight,
			Character->HasMetalbending() ? 1.f : Character->GetEarthMastery(),
			FLinearColor(0.62f, 0.66f, 0.74f),
			Character->HasMetalbending() ? TEXT("Metal   ~ METALBENDING ~") : TEXT("Mastery (land hits to learn metal)"));
		break;
	case EATLAElement::Fire:
		DrawBar(X, YBase + 68.f, BarWidth, BarHeight,
			Attributes->GetMaxChi() > 0.f ? Attributes->GetChi() / Attributes->GetMaxChi() : 0.f,
			FLinearColor(1.f, 0.4f, 0.08f),
			TEXT("Fire    (burns chi)"));
		break;
	case EATLAElement::Air:
		DrawBar(X, YBase + 68.f, BarWidth, BarHeight,
			Attributes->GetMaxChi() > 0.f ? Attributes->GetChi() / Attributes->GetMaxChi() : 0.f,
			FLinearColor(0.85f, 0.92f, 1.f),
			TEXT("Air     (rides chi)"));
		break;
	}
}

void AATLAHUD::DrawBar(float X, float Y, float Width, float Height, float Fraction, const FLinearColor& FillColor, const FString& Label)
{
	DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.55f), X - 2.f, Y - 2.f, Width + 4.f, Height + 4.f);
	DrawRect(FillColor, X, Y, Width * FMath::Clamp(Fraction, 0.f, 1.f), Height);
	DrawText(Label, FLinearColor::White, X, Y - 18.f, GEngine->GetSmallFont());
}
