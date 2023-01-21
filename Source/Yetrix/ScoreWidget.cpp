#include "ScoreWidget.h"

UScoreWidget::UScoreWidget(const FObjectInitializer& initializer) : Super(initializer) {

}

void UScoreWidget::UpdateScoreCount(int value) {

	txtScore->SetText(FText::FromString(FString::FromInt(value)));
}

void UScoreWidget::NativeConstruct() {
	Super::NativeConstruct();

}