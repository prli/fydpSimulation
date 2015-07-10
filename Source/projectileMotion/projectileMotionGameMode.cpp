// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "projectileMotion.h"
#include "projectileMotionGameMode.h"
#include "projectileMotionHUD.h"
#include "projectileMotionCharacter.h"

AprojectileMotionGameMode::AprojectileMotionGameMode()
	: Super()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnClassFinder(TEXT("/Game/FirstPerson/Blueprints/FirstPersonCharacter"));
	DefaultPawnClass = PlayerPawnClassFinder.Class;

	// use our custom HUD class
	HUDClass = AprojectileMotionHUD::StaticClass();
}
