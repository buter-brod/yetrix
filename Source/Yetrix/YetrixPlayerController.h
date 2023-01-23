// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "YetrixPlayerController.generated.h"

/**
 * 
 */
UCLASS()
class YETRIX_API AYetrixPlayerController : public APlayerController
{
	GENERATED_BODY()
	
public:
	void SetupInputComponent() override;

	void MoveLeft();
	void MoveRight();
	void Drop();
	void Down();
	void Rotate();
	void Quit();
};


