// Fill out your copyright notice in the Description page of Project Settings.


#include "YetrixPawn.h"

// Sets default values
AYetrixPawn::AYetrixPawn()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));

}

//#include "EnhancedPlayerInput.h"


void AYetrixPawn::Rotate() {

	yetrixGameMode->Rotate();
}

void AYetrixPawn::Down(){

	yetrixGameMode->Down();
}

void AYetrixPawn::Left() {

	yetrixGameMode->Left();
}

void AYetrixPawn::Right() {

	yetrixGameMode->Right();
}

void AYetrixPawn::Drop(){

	yetrixGameMode->Drop();
}

// Called when the game starts or when spawned
void AYetrixPawn::BeginPlay()
{
	Super::BeginPlay();
}

void AYetrixPawn::SetGameMode(AYetrixGameModeBase* gmPtr) {
	yetrixGameMode = gmPtr;
}

// Called every frame
void AYetrixPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void AYetrixPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

