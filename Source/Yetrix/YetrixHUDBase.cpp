#include "YetrixHUDBase.h"

AYetrixHUDBase::AYetrixHUDBase() {
	printf("");
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

void AYetrixHUDBase::UpdateScore(int Value) {
	if (scoreWidget)
		scoreWidget->UpdateScoreCount(Value);
}