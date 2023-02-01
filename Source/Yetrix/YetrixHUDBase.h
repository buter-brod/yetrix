// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "Components/WidgetComponent.h"
#include "ScoreWidget.h"
#include "YetrixHUDBase.generated.h"

/**
 * 
 */
UCLASS()
class YETRIX_API AYetrixHUDBase : public AHUD
{
	GENERATED_BODY()
	
	AYetrixHUDBase();

	void DrawHUD() override;
	void BeginPlay() override;
	void Tick(float DeltaSeconds) override;

	UPROPERTY(EditDefaultsOnly, Category = "Widgets")
	TSubclassOf<UUserWidget> ScoreWidgetClass;

public:
	void UpdateScore(int Value, int HiValue);

private:
	UScoreWidget* scoreWidget = nullptr;
};
