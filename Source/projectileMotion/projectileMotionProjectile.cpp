// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "projectileMotion.h"
#include "projectileMotionProjectile.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Engine.h"
#include <fstream>
#include <sstream>
#include <string>

AprojectileMotionProjectile::AprojectileMotionProjectile() 
{
	// Use a sphere as a simple collision representation
	CollisionComp = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComp"));
	CollisionComp->InitSphereRadius(5.0f);
	CollisionComp->BodyInstance.SetCollisionProfileName("Projectile");
	CollisionComp->OnComponentHit.AddDynamic(this, &AprojectileMotionProjectile::OnHit);		// set up a notification for when this component hits something blocking

	// Players can't walk on it
	CollisionComp->SetWalkableSlopeOverride(FWalkableSlopeOverride(WalkableSlope_Unwalkable, 0.f));
	CollisionComp->CanCharacterStepUpOn = ECB_No;

	// Set as root component
	RootComponent = CollisionComp;

	// Use a ProjectileMovementComponent to govern this projectile's movement
	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileComp"));
	ProjectileMovement->UpdatedComponent = CollisionComp;
	//ProjectileMovement->InitialSpeed = 3000.f;
	ProjectileMovement->MaxSpeed = 10000.f;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->bShouldBounce = true;

	PrimaryActorTick.bCanEverTick = true;

	// Die after 3 seconds by default
	InitialLifeSpan = 3.0f;
}

void AprojectileMotionProjectile::OnHit(AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	// Only add impulse and destroy projectile if we hit a physics
	if ((OtherActor != NULL) && (OtherActor != this) && (OtherComp != NULL) && OtherComp->IsSimulatingPhysics())
	{
		OtherComp->AddImpulseAtLocation(GetVelocity() * 100.0f, GetActorLocation());

		Destroy();
	}
}

void AprojectileMotionProjectile::InitVelocity(const FVector& ShootDirection)
{
	if (ProjectileMovement)
	{
		std::ifstream infile;
		TArray<FString> StringArrat;
		FString projectDir = FPaths::GameDir();
		projectDir += "Data/test.txt";
		if (!FPlatformFileManager::Get().GetPlatformFile().FileExists(*projectDir))
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("** Could not Find File **"));
			return;
		}
		FFileHelper::LoadANSITextFileToStrings(*(projectDir), NULL, StringArrat);
		float Ax = 0;
		//float Gx, Gy, Gz = 0.f;
		for (int i = 0; i < StringArrat.Num(); i++)
		{
			FString str = StringArrat[i];
			TArray<FString> parsed;
			int count = str.ParseIntoArray(parsed, TEXT(","), false);
			if (count > 1)
				Ax = FCString::Atof(*parsed[0]);
		}

		ProjectileMovement->InitialSpeed = Ax;
		// set the projectile's velocity to the desired direction
		ProjectileMovement->Velocity = ShootDirection * ProjectileMovement->InitialSpeed;
	}
}

void AprojectileMotionProjectile::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	FColor LineColor = FColor::Green;
	DrawDebugLine(GetWorld(), GetActorLocation(), GetActorLocation() + GetVelocity() * DeltaSeconds, LineColor, false, 2.f, 0, 1.f);

	SetActorLocation(GetActorLocation() + GetVelocity() * DeltaSeconds, true);
}