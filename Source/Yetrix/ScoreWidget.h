// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Runtime/UMG/Public/UMG.h"

#include "ScoreWidget.generated.h"

/**
 * 
 */
UCLASS()
class YETRIX_API UScoreWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	UScoreWidget(const FObjectInitializer&);
	
	virtual void NativeConstruct() override;

	void UpdateScoreCount(int value);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (BindWidget))
	UTextBlock* txtScore = nullptr;
};
