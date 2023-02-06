#include "Block.h"

#include "YetrixConfig.h"
#include "BlockBase.h"

static TSubclassOf<ABlockBase> BlockBPClass;

bool GameBlock::InitSubclasses() {

	const bool wasNeedInit = !BlockBPClass.Get();
	if (wasNeedInit) {
		const ConstructorHelpers::FObjectFinder<UClass> blueprint_finder_BlockBP(TEXT("Blueprint'/Game/BlockBP.BlockBP_C'"));
		BlockBPClass = (UClass*) blueprint_finder_BlockBP.Object;
	}
	
	return wasNeedInit;
}

void GameBlock::Init(const BlockInfo& givenInfo)
{
	info = givenInfo;
}

FVector GameBlock::ToWorldPosition(const Vec2D pos){

	FVector worldPos;
	worldPos.X = pos.x * blockSize;
	worldPos.Z = pos.y * blockSize;

	return worldPos;
}

GameBlock::~GameBlock() {
	if(IsValid(actor))
		actor->Destroy();
}

bool GameBlock::TickDestroy(const float dt) {

	if (finalDestroyTimer > 0.f)
		finalDestroyTimer -= dt;

	if (finalDestroyTimer < 0.f) {
		finalDestroyRequested = true;
	}

	return finalDestroyRequested;
}

void GameBlock::StartDestroy() {

	finalDestroyTimer = destroyActorAfter;
}

ABlockBase* GameBlock::CreateActor(UWorld* world) {

	const FRotator rotator = FRotator::ZeroRotator;
	const FActorSpawnParameters spawnParams;
	const FVector spawnLocation = ToWorldPosition(info.position);

	const auto newBlockActor = world->SpawnActor<ABlockBase>(BlockBPClass, spawnLocation, rotator, spawnParams);
	actor = newBlockActor;
	return actor;
}

void GameBlock::UpdateActorPosition() const
{
	if (!actor)
		return;

	actor->SetActorLocation(ToWorldPosition(info.position));
}
