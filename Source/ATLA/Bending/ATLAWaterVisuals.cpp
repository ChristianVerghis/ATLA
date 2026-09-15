// Copyright Epic Games, Inc. All Rights Reserved.

#include "ATLAWaterVisuals.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	void ApplyMaterialOrTint(UStaticMeshComponent* Mesh, const TCHAR* MaterialPath, const FLinearColor& FallbackTint)
	{
		if (UMaterialInterface* Material = LoadObject<UMaterialInterface>(nullptr, MaterialPath))
		{
			Mesh->SetMaterial(0, Material);
		}
		else if (UMaterialInterface* Basic = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial")))
		{
			Mesh->SetMaterial(0, Basic);
			if (UMaterialInstanceDynamic* DynMat = Mesh->CreateAndSetMaterialInstanceDynamic(0))
			{
				DynMat->SetVectorParameterValue(TEXT("Color"), FallbackTint);
			}
		}
	}
}

namespace ATLAWaterVisuals
{
	void ApplyTintMaterial(UStaticMeshComponent* Mesh, const FLinearColor& Tint)
	{
		ApplyMaterialOrTint(Mesh, TEXT(""), Tint);
	}

	void SetupWaterMesh(UStaticMeshComponent* Mesh)
	{
		static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
		if (SphereMesh.Succeeded())
		{
			Mesh->SetStaticMesh(SphereMesh.Object);
		}
	}

	void SetupConeMesh(UStaticMeshComponent* Mesh)
	{
		static ConstructorHelpers::FObjectFinder<UStaticMesh> ConeMesh(TEXT("/Engine/BasicShapes/Cone.Cone"));
		if (ConeMesh.Succeeded())
		{
			Mesh->SetStaticMesh(ConeMesh.Object);
		}
	}

	void ApplyWaterMaterial(UStaticMeshComponent* Mesh)
	{
		ApplyMaterialOrTint(Mesh, TEXT("/Game/Bending/VFX/M_Water.M_Water"), FLinearColor(0.05f, 0.35f, 1.f));
	}

	void ApplyIceMaterial(UStaticMeshComponent* Mesh)
	{
		ApplyMaterialOrTint(Mesh, TEXT("/Game/Bending/VFX/M_Ice.M_Ice"), FLinearColor(0.7f, 0.85f, 1.f));
	}

	void ApplyEarthMaterial(UStaticMeshComponent* Mesh)
	{
		ApplyMaterialOrTint(Mesh, TEXT("/Game/Bending/VFX/M_Earth.M_Earth"), FLinearColor(0.22f, 0.16f, 0.1f));
	}

	void ApplyFireMaterial(UStaticMeshComponent* Mesh)
	{
		ApplyMaterialOrTint(Mesh, TEXT("/Game/Bending/VFX/M_Fire.M_Fire"), FLinearColor(1.f, 0.45f, 0.05f));
	}

	void ApplyAirMaterial(UStaticMeshComponent* Mesh)
	{
		ApplyMaterialOrTint(Mesh, TEXT("/Game/Bending/VFX/M_Air.M_Air"), FLinearColor(0.9f, 0.95f, 1.f));
	}

	void ApplyFireCoreMaterial(UStaticMeshComponent* Mesh)
	{
		ApplyMaterialOrTint(Mesh, TEXT("/Game/Bending/VFX/M_FireCore.M_FireCore"), FLinearColor(1.f, 0.92f, 0.5f));
	}

	void ApplyEmberMaterial(UStaticMeshComponent* Mesh)
	{
		ApplyMaterialOrTint(Mesh, TEXT("/Game/Bending/VFX/M_Ember.M_Ember"), FLinearColor(1.f, 0.55f, 0.1f));
	}

	void ApplyAirBandMaterial(UStaticMeshComponent* Mesh)
	{
		ApplyMaterialOrTint(Mesh, TEXT("/Game/Bending/VFX/M_AirBand.M_AirBand"), FLinearColor(0.85f, 0.93f, 1.f));
	}

	void ApplyLightningMaterial(UStaticMeshComponent* Mesh)
	{
		ApplyMaterialOrTint(Mesh, TEXT("/Game/Bending/VFX/M_Lightning.M_Lightning"), FLinearColor(0.7f, 0.8f, 1.f));
	}
}
