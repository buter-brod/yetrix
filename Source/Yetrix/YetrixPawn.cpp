#include "YetrixPawn.h"

AYetrixPawn::AYetrixPawn()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
}

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

void AYetrixPawn::BeginPlay()
{
	Super::BeginPlay();
}

void AYetrixPawn::SetGameMode(AYetrixGameModeBase* gmPtr) {
	yetrixGameMode = gmPtr;
}

void AYetrixPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AYetrixPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

