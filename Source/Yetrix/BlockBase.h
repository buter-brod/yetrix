#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BlockBase.generated.h"

UCLASS()
class YETRIX_API ABlockBase : public AActor
{
	GENERATED_BODY()
	
public:	
	ABlockBase();

protected:
	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;

};
