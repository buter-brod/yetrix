#include "YetrixPlayerController.h"
#include "YetrixPawn.h"

void AYetrixPlayerController::SetupInputComponent() 
{
    // Always call this.
    Super::SetupInputComponent();

    // This is initialized on startup, you can go straight to binding
    InputComponent->BindAction("Left", IE_Pressed, this, &AYetrixPlayerController::MoveLeft);
    InputComponent->BindAction("Right", IE_Pressed, this, &AYetrixPlayerController::MoveRight);

    InputComponent->BindAction("Left", IE_Repeat, this, &AYetrixPlayerController::MoveLeft);
    InputComponent->BindAction("Right", IE_Repeat, this, &AYetrixPlayerController::MoveRight);

    InputComponent->BindAction("Drop", IE_Pressed, this, &AYetrixPlayerController::Drop); 
    InputComponent->BindAction("Down", IE_Pressed, this, &AYetrixPlayerController::Down); 
    InputComponent->BindAction("Rotate", IE_Pressed, this, &AYetrixPlayerController::Rotate); 
}

void AYetrixPlayerController::Rotate() {
    
    auto* yetrixPawn = dynamic_cast<AYetrixPawn*>(GetPawn());
    yetrixPawn->Rotate();
}

void AYetrixPlayerController::MoveLeft() {
    
    auto* yetrixPawn = dynamic_cast<AYetrixPawn*>(GetPawn());
    yetrixPawn->Left();
}

void AYetrixPlayerController::MoveRight() {

    auto* yetrixPawn = dynamic_cast<AYetrixPawn*>(GetPawn());
    yetrixPawn->Right();
}

void AYetrixPlayerController::Down() {

    auto* yetrixPawn = dynamic_cast<AYetrixPawn*>(GetPawn());
    yetrixPawn->Down();
}

void AYetrixPlayerController::Drop() {

    auto* yetrixPawn = dynamic_cast<AYetrixPawn*>(GetPawn());
    yetrixPawn->Drop();
}