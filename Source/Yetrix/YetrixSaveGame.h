// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "YetrixSaveGame.generated.h"

/**
 * 
 */
UCLASS()
class YETRIX_API UYetrixSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY()
	FString jsonDump;
};
