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

    InputComponent->BindAction("Quit", IE_Pressed, this, &AYetrixPlayerController::Quit);
}

void AYetrixPlayerController::Quit() {
    FGenericPlatformMisc::RequestExit(false);
}

void AYetrixPlayerController::Rotate() {

    AYetrixPawn* yetrixPawn = Cast<AYetrixPawn>(GetPawn());
    yetrixPawn->Rotate();
}

void AYetrixPlayerController::MoveLeft() {
    
    AYetrixPawn* yetrixPawn = Cast<AYetrixPawn>(GetPawn());
    yetrixPawn->Left();
}

void AYetrixPlayerController::MoveRight() {

    AYetrixPawn* yetrixPawn = Cast<AYetrixPawn>(GetPawn());
    yetrixPawn->Right();
}

void AYetrixPlayerController::Down() {

    AYetrixPawn* yetrixPawn = Cast<AYetrixPawn>(GetPawn());
    yetrixPawn->Down();
}

void AYetrixPlayerController::Drop() {

    AYetrixPawn* yetrixPawn = Cast<AYetrixPawn>(GetPawn());
    yetrixPawn->Drop();
}
