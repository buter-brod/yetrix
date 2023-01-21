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
	AYetrixPawn();

	void Left();
	void Right();
	void Drop();
	void Down();
	void Rotate();

	void SetGameMode(AYetrixGameModeBase* bsPtr);

protected:
	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

private:

	AYetrixGameModeBase* yetrixGameMode = nullptr;
};
