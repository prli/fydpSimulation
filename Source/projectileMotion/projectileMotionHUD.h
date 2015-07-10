// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.
#pragma once 
#include "GameFramework/HUD.h"
#include "projectileMotionHUD.generated.h"

UCLASS()
class AprojectileMotionHUD : public AHUD
{
	GENERATED_BODY()

public:
	AprojectileMotionHUD();

	/** Primary draw call for the HUD */
	virtual void DrawHUD() override;

private:
	/** Crosshair asset pointer */
	class UTexture2D* CrosshairTex;

};

