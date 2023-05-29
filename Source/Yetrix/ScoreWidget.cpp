#include "ScoreWidget.h"

UScoreWidget::UScoreWidget(const FObjectInitializer& initializer) : Super(initializer) {

}

void UScoreWidget::UpdateScoreCount(int Value, int HiValue) const
{
	FString scoreString = FString::FromInt(Value);

	if (HiValue > 0)
		scoreString = scoreString + "/" + FString::FromInt(HiValue);

	txtScore->SetText(FText::FromString(scoreString));
}

void UScoreWidget::UpdateConditionScore(int Value, int WorstValue) const
{
	FString scoreString = FString::FromInt(Value);
	scoreString = scoreString + "/" + FString::FromInt(WorstValue);

	TxtCondition->SetText(FText::FromString(scoreString));
}

void UScoreWidget::NativeConstruct() {
	Super::NativeConstruct();

}