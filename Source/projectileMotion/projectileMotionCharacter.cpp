// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "projectileMotion.h"
#include "projectileMotionCharacter.h"
#include "projectileMotionProjectile.h"
#include "Animation/AnimInstance.h"
#include "GameFramework/InputSettings.h"
#include "Engine.h"
#include <string>

DEFINE_LOG_CATEGORY_STATIC(LogFPChar, Warning, All);

//////////////////////////////////////////////////////////////////////////
// AprojectileMotionCharacter

AprojectileMotionCharacter::AprojectileMotionCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// set our turn rates for input
	BaseTurnRate = 45.f;
	BaseLookUpRate = 45.f;

	// Create a CameraComponent	
	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCameraComponent->AttachParent = GetCapsuleComponent();
	FirstPersonCameraComponent->RelativeLocation = FVector(0, 0, 64.f); // Position the camera
	FirstPersonCameraComponent->bUsePawnControlRotation = true;

	// Default offset from the character location for projectiles to spawn
	GunOffset = FVector(100.0f, 0.0f, 0.0f);

	// Create a mesh component that will be used when being viewed from a '1st person' view (when controlling this pawn)
	Mesh1P = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("CharacterMesh1P"));
	Mesh1P->SetOnlyOwnerSee(true);			// only the owning player will see this mesh
	Mesh1P->AttachParent = FirstPersonCameraComponent;
	Mesh1P->RelativeLocation = FVector(0.f, 0.f, -150.f);
	Mesh1P->bCastDynamicShadow = false;
	Mesh1P->CastShadow = false;

	// Note: The ProjectileClass and the skeletal mesh/anim blueprints for Mesh1P are set in the
	// derived blueprint asset named MyCharacter (to avoid direct content references in C++)
}

//////////////////////////////////////////////////////////////////////////
// Input

void AprojectileMotionCharacter::SetupPlayerInputComponent(class UInputComponent* InputComponent)
{
	// set up gameplay key bindings
	check(InputComponent);

	InputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	InputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);
	
	//InputComponent->BindTouch(EInputEvent::IE_Pressed, this, &AprojectileMotionCharacter::TouchStarted);
	if( EnableTouchscreenMovement(InputComponent) == false )
	{
		//InputComponent->BindAction("Fire", IE_Pressed, this, &AprojectileMotionCharacter::OnFire);
	}
	
	InputComponent->BindAxis("MoveForward", this, &AprojectileMotionCharacter::MoveForward);
	InputComponent->BindAxis("MoveRight", this, &AprojectileMotionCharacter::MoveRight);
	
	// We have 2 versions of the rotation bindings to handle different kinds of devices differently
	// "turn" handles devices that provide an absolute delta, such as a mouse.
	// "turnrate" is for devices that we choose to treat as a rate of change, such as an analog joystick
	InputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
	InputComponent->BindAxis("TurnRate", this, &AprojectileMotionCharacter::TurnAtRate);
	InputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);
	InputComponent->BindAxis("LookUpRate", this, &AprojectileMotionCharacter::LookUpAtRate);
	
	if (!ConnectionSocket)
		StartTCPReceiver("RamaSocketListener", "127.0.0.1", 8081);
}

bool AprojectileMotionCharacter::StartTCPReceiver(
	const FString& socketName,
	const FString& ip, 
	const int32 port
){
	//Rama's CreateTCPConnectionListener
	TSharedRef<FInternetAddr> addr = CreateTCPConnection();
	/*ListenerSocket = CreateTCPConnectionListener(socketName, ip, port);*/

	//Not created?
	if (!ConnectionSocket)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("StartTCPReceiver>> Listen socket could not be created! ~> %s %d"), *ip, port));
		return false;
	}
	bool connected = ConnectionSocket->Connect(*addr);
	if (!connected)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Could not connect!"));
		return false;
	}
	//can thread this too
	GetWorldTimerManager().SetTimer(this,
		&AprojectileMotionCharacter::TCPSocketListener, 0.01, true);
	//GetWorldTimerManager().SetTimer(this, 
	//	&AprojectileMotionCharacter::TCPConnectionListener, 0.01, true);	
	return true;
}

bool FormatIP4ToNumber(const FString& TheIP, uint8 (&Out)[4])
{
	//IP Formatting
	TheIP.Replace( TEXT(" "), TEXT("") );
 
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	//						   IP 4 Parts
 
	//String Parts
	TArray<FString> Parts;
	TheIP.ParseIntoArray( Parts, TEXT("."), true );
	if ( Parts.Num() != 4 )
		return false;
 
	//String to Number Parts
	for ( int32 i = 0; i < 4; ++i )
	{
		Out[i] = FCString::Atoi( *Parts[i] );
	}
 
	return true;
}

TSharedRef<FInternetAddr> AprojectileMotionCharacter::CreateTCPConnection()
{
	//Create Socket
	ConnectionSocket = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateSocket(NAME_Stream, TEXT("default"), false);
	FString address = TEXT("192.168.56.101");
	int32 port = 8081;
	FIPv4Address ip;
	FIPv4Address::Parse(address, ip);
	TSharedRef<FInternetAddr> addr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
	addr->SetIp(ip.GetValue());
	addr->SetPort(port);
	return addr;
}

FSocket* AprojectileMotionCharacter::CreateTCPConnectionListener(const FString& socketName,const FString& ip, const int32 port,const int32 ReceiveBufferSize)
{
	uint8 IP4Nums[4];
	if( ! FormatIP4ToNumber(ip, IP4Nums))
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Invalid IP! Expecting 4 parts separated by ."));
		return NULL;
	}
 
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	FIPv4Endpoint Endpoint(FIPv4Address(IP4Nums[0], IP4Nums[1], IP4Nums[2], IP4Nums[3]), port);
	FSocket* ListenSocket = FTcpSocketBuilder(*socketName)
		.AsReusable()
		.BoundToEndpoint(Endpoint)
		.Listening(8);
 
	//Set Buffer Size
	int32 NewSize = 0;
	ListenSocket->SetReceiveBufferSize(ReceiveBufferSize, NewSize);
 
	//Done!
	return ListenSocket;	
}

void AprojectileMotionCharacter::TCPConnectionListener()
{
	//~~~~~~~~~~~~~
	if(!ListenerSocket) return;
	//~~~~~~~~~~~~~
 
	//Remote address
	TSharedRef<FInternetAddr> RemoteAddress = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
	bool Pending;

	//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("port ~> %s"), ListenerSocket->GetPortNo()));

	// handle incoming connections
	if (ListenerSocket->HasPendingConnection(Pending) && Pending)
	{
		//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		//Already have a Connection? destroy previous
		if(ConnectionSocket)
		{
			ConnectionSocket->Close();
			ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(ConnectionSocket);
		}
		//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 
		//New Connection receive!
		ConnectionSocket = ListenerSocket->Accept(*RemoteAddress, TEXT("RamaTCP Received Socket Connection"));
 
		if (ConnectionSocket != NULL)
		{
			//Global cache of current Remote Address
			RemoteAddressForConnection = FIPv4Endpoint(RemoteAddress);
 
			//UE_LOG "Accepted Connection! WOOOHOOOO!!!";
 
			//can thread this too
			GetWorldTimerManager().SetTimer(this, 
				&AprojectileMotionCharacter::TCPSocketListener, 0.01, true);	
		}
	}
}

FString StringFromBinaryArray(const TArray<uint8>& BinaryArray)
{
	//Create a string from a byte array!
	std::string cstr( reinterpret_cast<const char*>(BinaryArray.GetData()), BinaryArray.Num() );
	return FString(cstr.c_str());
}

void AprojectileMotionCharacter::TCPSocketListener()
{
	//~~~~~~~~~~~~~
	if(!ConnectionSocket) return;
	//~~~~~~~~~~~~~
 
 
	//Binary Array!
	TArray<uint8> ReceivedData;
 
	uint32 Size;
	while (ConnectionSocket->HasPendingData(Size))
	{
		ReceivedData.Init(FMath::Min(Size, 65507u));
 
		int32 Read = 0;
		ConnectionSocket->Recv(ReceivedData.GetData(), ReceivedData.Num(), Read);
 
		//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("Data Read! %d"), ReceivedData.Num()));
	}
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	if(ReceivedData.Num() <= 0)
	{
		//No Data Received
		return;
	}

	GEngine->AddOnScreenDebugMessage(-1, .5f, FColor::Red, FString::Printf(TEXT("Data Bytes Read ~> %d"), ReceivedData.Num()));

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	//						Rama's String From Binary Array
	const FString ReceivedUE4String = StringFromBinaryArray(ReceivedData);
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Red, FString::Printf(TEXT("As String Data ~> %s"), *ReceivedUE4String));

	float speed = FCString::Atof(*ReceivedUE4String);
	OnFire(speed);
}

void AprojectileMotionCharacter::OnFire(float speed)
{ 
	// try and fire a projectile
	if (ProjectileClass != NULL)
	{		
		const FRotator SpawnRotation = GetControlRotation();
		// MuzzleOffset is in camera space, so transform it to world space before offsetting from the character location to find the final muzzle position
		const FVector SpawnLocation = GetActorLocation() + SpawnRotation.RotateVector(GunOffset);

		UWorld* const World = GetWorld();
		if (World != NULL)
		{
			// spawn the projectile at the muzzle
			AprojectileMotionProjectile* const Projectile = World->SpawnActor<AprojectileMotionProjectile>(ProjectileClass, SpawnLocation, SpawnRotation);
			if (Projectile)
			{
				// find launch direction
				FVector const LaunchDir = SpawnRotation.Vector();
				Projectile->InitVelocity(LaunchDir, speed);
			}
		}
	}

	// try and play the sound if specified
	if (FireSound != NULL)
	{
		UGameplayStatics::PlaySoundAtLocation(this, FireSound, GetActorLocation());
	}

	// try and play a firing animation if specified
	if(FireAnimation != NULL)
	{
		// Get the animation object for the arms mesh
		UAnimInstance* AnimInstance = Mesh1P->GetAnimInstance();
		if(AnimInstance != NULL)
		{
			AnimInstance->Montage_Play(FireAnimation, 1.f);
		}
	}

}

void AprojectileMotionCharacter::BeginTouch(const ETouchIndex::Type FingerIndex, const FVector Location)
{
	if( TouchItem.bIsPressed == true )
	{
		return;
	}
	TouchItem.bIsPressed = true;
	TouchItem.FingerIndex = FingerIndex;
	TouchItem.Location = Location;
	TouchItem.bMoved = false;
}

void AprojectileMotionCharacter::EndTouch(const ETouchIndex::Type FingerIndex, const FVector Location)
{
	if (TouchItem.bIsPressed == false)
	{
		return;
	}
	if( ( FingerIndex == TouchItem.FingerIndex ) && (TouchItem.bMoved == false) )
	{
		OnFire(3000);
	}
	TouchItem.bIsPressed = false;
}

void AprojectileMotionCharacter::TouchUpdate(const ETouchIndex::Type FingerIndex, const FVector Location)
{
	if ((TouchItem.bIsPressed == true) && ( TouchItem.FingerIndex==FingerIndex))
	{
		if (TouchItem.bIsPressed)
		{
			if (GetWorld() != nullptr)
			{
				UGameViewportClient* ViewportClient = GetWorld()->GetGameViewport();
				if (ViewportClient != nullptr)
				{
					FVector MoveDelta = Location - TouchItem.Location;
					FVector2D ScreenSize;
					ViewportClient->GetViewportSize(ScreenSize);
					FVector2D ScaledDelta = FVector2D( MoveDelta.X, MoveDelta.Y) / ScreenSize;									
					if (ScaledDelta.X != 0.0f)
					{
						TouchItem.bMoved = true;
						float Value = ScaledDelta.X * BaseTurnRate;
						AddControllerYawInput(Value);
					}
					if (ScaledDelta.Y != 0.0f)
					{
						TouchItem.bMoved = true;
						float Value = ScaledDelta.Y* BaseTurnRate;
						AddControllerPitchInput(Value);
					}
					TouchItem.Location = Location;
				}
				TouchItem.Location = Location;
			}
		}
	}
}

void AprojectileMotionCharacter::MoveForward(float Value)
{
	if (Value != 0.0f)
	{
		// add movement in that direction
		AddMovementInput(GetActorForwardVector(), Value);
	}
}

void AprojectileMotionCharacter::MoveRight(float Value)
{
	if (Value != 0.0f)
	{
		// add movement in that direction
		AddMovementInput(GetActorRightVector(), Value);
	}
}

void AprojectileMotionCharacter::TurnAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void AprojectileMotionCharacter::LookUpAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}

bool AprojectileMotionCharacter::EnableTouchscreenMovement(class UInputComponent* InputComponent)
{
	bool bResult = false;
	if(FPlatformMisc::GetUseVirtualJoysticks() || GetDefault<UInputSettings>()->bUseMouseForTouch )
	{
		bResult = true;
		InputComponent->BindTouch(EInputEvent::IE_Pressed, this, &AprojectileMotionCharacter::BeginTouch);
		InputComponent->BindTouch(EInputEvent::IE_Released, this, &AprojectileMotionCharacter::EndTouch);
		InputComponent->BindTouch(EInputEvent::IE_Repeat, this, &AprojectileMotionCharacter::TouchUpdate);
	}
	return bResult;
}
