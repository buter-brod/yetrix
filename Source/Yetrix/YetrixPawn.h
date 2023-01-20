// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"

#include "YetrixGameModeBase.h"

#include "YetrixPawn.generated.h"

UCLASS()
class YETRIX_API AYetrixPawn : public APawn
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	AYetrixPawn();

	void Left();
	void Right();
	void Drop();
	void Down();
	void Rotate();

	void SetGameMode(AYetrixGameModeBase* bsPtr);

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

private:

	AYetrixGameModeBase* yetrixGameMode = nullptr;
};
