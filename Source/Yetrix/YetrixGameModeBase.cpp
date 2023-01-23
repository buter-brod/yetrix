#include "YetrixGameModeBase.h"
#include "YetrixPlayerController.h"
#include "BlockBase.h"
#include "YetrixPawn.h"

#include "GeometryCollection/GeometryCollectionComponent.h"
#include "Engine/DamageEvents.h"

#include "YetrixHUDBase.h"
#include "YetrixConfig.h"

AYetrixGameModeBase::AYetrixGameModeBase() {

	PrimaryActorTick.bCanEverTick = true;
	BlockScene::InitSubclasses();
	PlayerControllerClass = AYetrixPlayerController::StaticClass();
}

void AYetrixGameModeBase::BeginPlay() {

	auto* pawn = GetWorld()->GetFirstPlayerController()->GetPawn();
	auto* yetrixPawn = dynamic_cast<AYetrixPawn*>(pawn);
	yetrixPawn->SetGameMode(this);

	Reset();
}

void AYetrixGameModeBase::OnStartDestroying() {
}

void AYetrixGameModeBase::OnStartDropping() {

	statePtr->blockScenePtr->DeconstructFigures();
	const auto& linesToDestruct = CheckDestruction();

	if (!linesToDestruct.empty()) {

		statePtr->fallingPositions = statePtr->blockScenePtr->GetFallingPositions(linesToDestruct);
		statePtr->currDropState = DropState::DESTROYING;
		statePtr->dropStateTimer = statePtr->destroyStateDuration;	
		OnStartDestroying();

		const auto howManyLines = linesToDestruct.size();

		//assert(howManyLines <= 4);
		AddScore(scorePerCombo[howManyLines - 1]);

		statePtr->lightAngleStart = statePtr->lightAngleCurrent;
		statePtr->lightAngleEnd += lightZRotationAddPerExplosion;
		statePtr->sunMoveFinishTimer = sunMoveDuration;

		statePtr->stillStateDuration *= speedUpCoeff;
		statePtr->dropStateDuration *= speedUpCoeff;
	}
	
	CheckAddFigures();
}

void AYetrixGameModeBase::AddScore(const int score) {

	statePtr->score += score;
	UpdateScoreUI();
}

void AYetrixGameModeBase::UpdateSunMove(const float dt) {

	if (statePtr->sunMoveFinishTimer <= 0.f) {
		statePtr->sunMoveFinishTimer = 0.f;
		return;
	}

	statePtr->sunMoveFinishTimer -= dt;

	if (statePtr->sunMoveFinishTimer <= 0.f) {
		statePtr->sunMoveFinishTimer = 0.f;
	}

	const float progress = 1.f - statePtr->sunMoveFinishTimer / sunMoveDuration;
	const float currAngle = statePtr->lightAngleStart + (statePtr->lightAngleEnd - statePtr->lightAngleStart) * progress;

	UpdateSunlight(currAngle);
}

void AYetrixGameModeBase::UpdateSunlight(const float angle) {

	statePtr->lightAngleCurrent = angle;

	TArray<AActor*> sunlightActors;
	UGameplayStatics::GetAllActorsWithTag(GetWorld(), "Sunlight", sunlightActors);

	if (sunlightActors.IsEmpty())
	{
		//error
		return;
	}	

	auto sunlightActor = sunlightActors[0];
	auto rotation = sunlightActor->GetActorRotation();
	rotation.Yaw = angle;
	sunlightActor->SetActorRotation(rotation);
}

void AYetrixGameModeBase::UpdateScoreUI() {
	
	AYetrixHUDBase* hud = Cast<AYetrixHUDBase> (GetWorld()->GetFirstPlayerController()->GetHUD());
	hud->UpdateScore(statePtr->score);
}

void AYetrixGameModeBase::CheckAddFigures() {

	if (statePtr->blockScenePtr->GetFigures().size() < minFigures)
	{
		const bool added = statePtr->blockScenePtr->CreateRandomFigureAt({newFigureX, newFigureY}, GetWorld());
		if (!added)
			Reset();
	}
}

void AYetrixGameModeBase::Left() {
	
	statePtr->leftPending++;
}

void AYetrixGameModeBase::Right() {

	statePtr->rightPending++;
}

void AYetrixGameModeBase::Rotate() {

	statePtr->rotatePending++;
}

void AYetrixGameModeBase::Drop() {
	statePtr->quickDropRequested = true;
	statePtr->dropStateTimer = 0.f;
}

void AYetrixGameModeBase::Down() {
	if (statePtr->currDropState == DropState::STILL)
		statePtr->dropStateTimer = 0.f;
}

std::set<int> AYetrixGameModeBase::CheckDestruction() {

	std::set<int> linesToBoom;

	for (int y = 1; y < checkHeight; ++y) {
		bool hasHoles = false;
		for (int x = 1; x < rightBorderX; ++x)
		{
			Vec2D checkPos(x, y);
			auto blockPtr = statePtr->blockScenePtr->GetBlock(checkPos, true);
			if (!blockPtr || !blockPtr->IsAlive() || blockPtr->GetFigureID() != invalidID) {
				hasHoles = true;
				break;
			}				
		}

		if (!hasHoles) {
			linesToBoom.insert(y);
		}
	}

	for (int y : linesToBoom) {

		for (int x = 1; x < rightBorderX; ++x)
		{
			Vec2D blockPos(x, y);
			auto blockPtr = statePtr->blockScenePtr->GetBlock(blockPos, true);

			blockPtr->StartDestroy();

			TArray<UActorComponent*> components;
			blockPtr->GetActor()->GetComponents(components);
			auto geometryComponent = Cast<UGeometryCollectionComponent>(components[2]);
			geometryComponent->SetSimulatePhysics(true);
		}
	}

	return linesToBoom;
}

void AYetrixGameModeBase::FinalizeLogicalDestroy() {

	for (const auto& fallingBlockInfo : statePtr->fallingPositions) {

		auto blockPtr = statePtr->blockScenePtr->GetBlock(fallingBlockInfo.first);
		blockPtr->SetPosition(fallingBlockInfo.second);
		blockPtr->UpdateActorPosition();
	}
	
	statePtr->fallingPositions.clear();
}

void AYetrixGameModeBase::FinalizeLogicalDrop() {

	const auto lowestFigID = statePtr->blockScenePtr->GetLowestFigureID();

	auto& figures = statePtr->blockScenePtr->GetFigures();
	for (auto& figure : figures) {
		
		unsigned maxHeight = 0;
		const bool canDrop = statePtr->blockScenePtr->CheckFigureCanMove(figure.second, {0, -1}, maxHeight);
		if (!canDrop)
			continue;

		const bool isLowestOne = figure.first == lowestFigID;
		const bool canQuickDrop = isLowestOne && statePtr->quickDropRequested; 
		const int heightToDrop = canQuickDrop ? maxHeight : 1;
		
		const auto& blockIDs = figure.second->GetBlockIDs();
		
		for (auto blockID : blockIDs) {
			auto block = statePtr->blockScenePtr->GetBlock(blockID);
			const auto& logicalPos = block->GetPosition();
			auto dropLogicalPos = logicalPos;
			dropLogicalPos.y -= heightToDrop;

			block->SetPosition(dropLogicalPos);
		}
	}

	statePtr->quickDropRequested = false;
}

void AYetrixGameModeBase::OnStopDestroying() {
	UpdateVisualDestroy(1.f);
	FinalizeLogicalDestroy();
}

void AYetrixGameModeBase::OnStopDropping() {

	UpdateVisualDrop(1.f);
	FinalizeLogicalDrop();
}

bool AYetrixGameModeBase::CheckChangeDropState() {

	if (statePtr->dropStateTimer >= 0.f)
		return false;

	if (statePtr->currDropState == DropState::STILL) {
		statePtr->currDropState = DropState::DROPPING;
		statePtr->dropStateTimer = statePtr->dropStateDuration;	
		OnStartDropping();
	}
	else if (statePtr->currDropState == DropState::DROPPING) {
		statePtr->currDropState = DropState::STILL;
		statePtr->dropStateTimer = statePtr->stillStateDuration;
		OnStopDropping();
	}
	else if (statePtr->currDropState == DropState::DESTROYING) {
		statePtr->currDropState = DropState::STILL;
		statePtr->dropStateTimer = statePtr->stillStateDuration;
		OnStopDestroying();
	}

	return true;
}

void AYetrixGameModeBase::Reset() {

	statePtr = std::make_unique<State>();
	UpdateScoreUI();
	UpdateSunlight(lightZRotationInit);
}

void AYetrixGameModeBase::UpdateVisualDestroy(const float progress) {

	for (const auto& fallingPosPair : statePtr->fallingPositions)
	{
		auto block = statePtr->blockScenePtr->GetBlock(fallingPosPair.first);

		const auto startLogicalPos = block->GetPosition();
		const auto& endLogicalPos = fallingPosPair.second;

		const auto blockWorldPos = GameBlock::ToWorldPosition(startLogicalPos);
		const auto dropWorldPos = GameBlock::ToWorldPosition(endLogicalPos);

		auto dropWorldPosNormalY = dropWorldPos.Y;
		auto dropPosIntermediateY = dropWorldPosNormalY;

		const auto dYFull = destroyYShift - dropWorldPosNormalY;

		constexpr float progressReturnStart = 0.5f;

		if (progress < progressReturnStart) {
			dropPosIntermediateY = dropWorldPosNormalY + dYFull * (progress / progressReturnStart);
		}
		else {
			const float returnProgress = (progress - progressReturnStart) / (1.f - progressReturnStart);
			dropPosIntermediateY = dropWorldPosNormalY + dYFull * (1.f - returnProgress);
		}

		const auto posDiff = dropWorldPos - blockWorldPos;
		const auto dPosCurr = posDiff * progress;

		auto dropIntermediateWorldPos = blockWorldPos + dPosCurr;
		dropIntermediateWorldPos.Y = dropPosIntermediateY;

		block->GetActor()->SetActorLocation(dropIntermediateWorldPos);
	}
}

void AYetrixGameModeBase::UpdateVisualDrop(const float progress) {

	const auto lowestFigID = statePtr->blockScenePtr->GetLowestFigureID();

	auto& figures = statePtr->blockScenePtr->GetFigures();
	for (auto& figure : figures) {
		
		unsigned maxHeight = 0;
		const bool canDrop = statePtr->blockScenePtr->CheckFigureCanMove(figure.second, {0, -1}, maxHeight);
		if (!canDrop)
			continue;

		const bool isLowestOne = figure.first == lowestFigID;
		const bool canQuickDrop = isLowestOne && statePtr->quickDropRequested; 
		const int heightToDrop = canQuickDrop ? maxHeight : 1;
		
		const auto& blockIDs = figure.second->GetBlockIDs();
		
		for (auto blockID : blockIDs) {
			auto block = statePtr->blockScenePtr->GetBlock(blockID);
			const auto logicalPos = block->GetPosition();
			auto dropLogicalPos = logicalPos;
			dropLogicalPos.y -= heightToDrop;

			const auto blockWorldPos = GameBlock::ToWorldPosition(logicalPos);
			const auto dropWorldPos = GameBlock::ToWorldPosition(dropLogicalPos);
			const auto posDiff = dropWorldPos - blockWorldPos;
			const auto dPosCurr = posDiff * progress;

			const auto dropIntermediateWorldPos = blockWorldPos + dPosCurr;

			block->GetActor()->SetActorLocation(dropIntermediateWorldPos);		
		}
	}
}

void AYetrixGameModeBase::SimulationTick(float dt) {

	UpdateSunMove(dt);

	statePtr->dropStateTimer -= dt;
	CheckChangeDropState();

	if (statePtr->currDropState == DropState::DESTROYING) {

		const float dropProgress = 1.f - (statePtr->dropStateTimer / statePtr->destroyStateDuration);
		UpdateVisualDestroy(dropProgress);

	}
	else if (statePtr->currDropState == DropState::DROPPING) {
		const float dropProgress = 1.f - (statePtr->dropStateTimer / statePtr->dropStateDuration);
		UpdateVisualDrop(dropProgress);
	} else {
	
		while (statePtr->leftPending)
		{
			statePtr->leftPending--;

			const bool moveOk = statePtr->blockScenePtr->MoveBlock({-1, 0});
			if (!moveOk)
				break;
		}
		while (statePtr->rightPending) {

			statePtr->rightPending--;
			const bool moveOk = statePtr->blockScenePtr->MoveBlock({1, 0});
			if (!moveOk)
				break;
		}

		while (statePtr->rotatePending)
		{
			statePtr->rotatePending--;

			const auto lowestFigID = statePtr->blockScenePtr->GetLowestFigureID();
			if (lowestFigID != invalidID) {
				auto& figure = statePtr->blockScenePtr->GetFigures().at(lowestFigID);
				const bool rotated = statePtr->blockScenePtr->TryRotate(figure);
			}			
		}
	}

	statePtr->blockScenePtr->CleanupBlocks(dt);
}

void AYetrixGameModeBase::Tick(float dt) {

	dtAccum += dt;
	while (dtAccum >= simulationUpdateInterval)
	{
		dtAccum -= simulationUpdateInterval;
		SimulationTick(simulationUpdateInterval);
	}	
}