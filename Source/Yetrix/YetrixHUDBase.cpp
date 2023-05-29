#include "YetrixHUDBase.h"

AYetrixHUDBase::AYetrixHUDBase() {
}

void AYetrixHUDBase::DrawHUD() {
	Super::DrawHUD();
}

void AYetrixHUDBase::BeginPlay() {
	Super::BeginPlay();

	if (ScoreWidgetClass)
	{
		scoreWidget = CreateWidget<UScoreWidget>(GetWorld(), ScoreWidgetClass);
		if (scoreWidget)
		{
			scoreWidget->AddToViewport(0);
		}
	}
}

void AYetrixHUDBase::Tick(float DeltaSeconds) {
	Super::Tick(DeltaSeconds);
}

void AYetrixHUDBase::UpdateConditionScore(int Value, int WorstValue)
{
	if (scoreWidget)
		scoreWidget->UpdateConditionScore(Value, WorstValue);
}

void AYetrixHUDBase::UpdateScore(int Value, int HiValue) {
	if (scoreWidget)
		scoreWidget->UpdateScoreCount(Value, HiValue);
}