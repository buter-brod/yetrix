#include "Block.h"

#include "YetrixConfig.h"
#include "BlockBase.h"
#include "GeometryCollection/GeometryCollectionComponent.h"

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
	worldPos.Y = 0.0;
	worldPos.Z = pos.y * blockSize;

	return worldPos;
}

GameBlock::~GameBlock() {
	if(IsValid(actor))
		actor->Destroy();
}

FVector GameBlock::GetActorLocation() const
{
	if (!actor)
	{
		checkf(false, TEXT("GameBlock::GetActorLocation error, no actor for block %s"), TCHARIFYSTDSTRING(GetID()));
		return {0.f, 0.f, 0.f};
	}

	const auto& location = actor->GetActorLocation();
	return location;
}

void GameBlock::SetActorLocation(const FVector location)
{
	actor->SetActorLocation(location);
}

void GameBlock::Explode()
{
	TArray<UActorComponent*> components;
	actor->GetComponents(components);
	constexpr unsigned geometryComponentInd = 2;
	auto* geometryComponent = Cast<UGeometryCollectionComponent>(components[geometryComponentInd]);
	if (geometryComponent)
		geometryComponent->SetSimulatePhysics(true);
}

void GameBlock::StartAnimatedMove(const float theAnimDuration, const FVector destination)
{
	fromPosition = actor->GetActorLocation();
	toPosition = destination;
	animDuration = theAnimDuration;
	moveTimer = theAnimDuration;
}

bool GameBlock::SetPosition(const Vec2D& newPos)
{
	if (info.position == newPos)
		return false;

	info.position = newPos;
	return true;
}

void GameBlock::SetPositionAndUpdateActor(const Vec2D& newPos, const float theAnimDuration) {

	const bool positionUpdated = SetPosition(newPos);

	if (!positionUpdated)
		return;

	if (theAnimDuration > 0.f)
	{
		const auto destination = ToWorldPosition(info.position);
		StartAnimatedMove(theAnimDuration, destination);
	}
	else
	{
		UpdateActorFromLogicalPosition();
	}		
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
	Explode();
}

ABlockBase* GameBlock::CreateActor(UWorld* world) {

	const FRotator rotator = FRotator::ZeroRotator;
	const FActorSpawnParameters spawnParams;
	const FVector spawnLocation = ToWorldPosition(info.position);

	const auto newBlockActor = world->SpawnActor<ABlockBase>(BlockBPClass, spawnLocation, rotator, spawnParams);
	actor = newBlockActor;
	return actor;
}

void GameBlock::Tick(const float dt)
{
	if (moveTimer > 0.f)
	{
		moveTimer -= dt;
		if (moveTimer < 0.f)
			moveTimer = 0.f;

		UpdateActorMovePosition();
	}
}

void GameBlock::UpdateActorFromLogicalPosition() const
{
	if (!actor)
		return;

	const auto resultPosition = ToWorldPosition(info.position);
	actor->SetActorLocation(resultPosition);
}

void GameBlock::UpdateActorMovePosition() const
{
	if (!actor)
		return;

	const auto elapsed = animDuration - moveTimer;
	const auto progress = elapsed / animDuration;

	const auto fullDeltsPos = toPosition - fromPosition;
	const auto resultPosition = fromPosition + fullDeltsPos * progress;

	actor->SetActorLocation(resultPosition);
}
