#include "YetrixGameModeBase.h"
#include "YetrixPlayerController.h"
#include "BlockBase.h"
#include "YetrixPawn.h"
#include <array>
#include <random>

#include "Blueprint/UserWidget.h"

#include "GeometryCollection/GeometryCollectionComponent.h"
#include "Engine/DamageEvents.h"

#include "YetrixHUDBase.h"

TSubclassOf<class ABlockBase> BlockBPClass;

typedef std::array<std::array<bool, 4>, 2> FigureConfigArr;

std::mt19937 localRnd(0);

constexpr float blockSize = 100.f;
constexpr float destroyActorAfter = 3.f;
constexpr float simulationUpdateInterval = 0.01f;

constexpr float speedUpCoeff = 0.9f;

constexpr float destroyYShift = 300.f;

constexpr int checkHeight = 20;

constexpr int newFigureX = 5;
constexpr int newFigureY = 25;

constexpr int rightBorderX = 11;

constexpr unsigned minFigures = 1;

static const std::array<int, 4> scorePerCombo = {10, 25, 40, 60};

const FigureConfigArr& GetFigureConfig(Figure::FigType type) {

	static std::map<Figure::FigType, FigureConfigArr> configArrMap;

	const auto& configIt = configArrMap.find(type);
	if (configIt != configArrMap.end())
		return configIt->second;
	
	configArrMap[Figure::FigType::LONG]       = {{{1,1,1,1}, {0,0,0,0}}};
	configArrMap[Figure::FigType::LEFT_BOOT]  = {{{1,0,0,0}, {1,1,1,0}}};
	configArrMap[Figure::FigType::RIGHT_BOOT] = {{{0,0,1,0}, {1,1,1,0}}};
	configArrMap[Figure::FigType::BOX]        = {{{1,1,0,0}, {1,1,0,0}}};
	configArrMap[Figure::FigType::LEFT_GUN]   = {{{0,1,1,0}, {1,1,0,0}}};
	configArrMap[Figure::FigType::RIGHT_GUN]  = {{{1,1,0,0}, {0,1,1,0}}};
	configArrMap[Figure::FigType::HAT]        = {{{0,1,0,0}, {1,1,1,0}}};

	const auto& configArr = configArrMap.at(type);

	return configArr;
}

IDType GameBlock::NewID() {
	static IDType id = 0;
	return id++;
}

IDType Figure::NewID() {
	static IDType id = 0;
	return id++;
}

AYetrixGameModeBase::AYetrixGameModeBase() {

	PrimaryActorTick.bCanEverTick = true;
	InitSubclasses();
}

void AYetrixGameModeBase::InitSubclasses() {

	if (!BlockBPClass.Get()) {
		ConstructorHelpers::FObjectFinder<UBlueprint> blueprint_finder_BlockBP(TEXT("Blueprint'/Game/BlockBP.BlockBP'"));
		BlockBPClass = (UClass*) blueprint_finder_BlockBP.Object->GeneratedClass;
	}
	PlayerControllerClass = AYetrixPlayerController::StaticClass();
}

bool BlockScene::CreateFigureAt(Figure::FigType figType, const Vec2D& pos, UWorld* world) {

	Figure::Ptr newFigurePtr = std::make_shared<Figure>(figType);
	
	auto newBlocks = newFigurePtr->CreateBlocks(pos, world);

	for (const auto& blockPtr : newBlocks) {
		const bool canAddBlock = CanAddBlock(blockPtr);
		if (!canAddBlock)
			return false;
	}

	for (const auto& blockPtr : newBlocks) {
		const bool addedOk = AddBlock(blockPtr);
		if (!addedOk) 
			return false;
	}

	figures.emplace(newFigurePtr->GetID(), newFigurePtr);
	return true;
}

bool BlockScene::CreateRandomFigureAt(const Vec2D& pos, UWorld* world) {

	std::uniform_int_distribution<int> uni(0, (int)Figure::FigType::UNDEFINED - 1);
	const Figure::FigType figType = static_cast<Figure::FigType> (uni(localRnd));
	//const Figure::FigType figType = Figure::FigType::BOX;
	const bool figAdded = CreateFigureAt(figType, pos, world);
	return figAdded;
}

GameBlock::Ptr BlockScene::GetBlock(IDType blockID) const {

	auto blockIt = blocks.find(blockID);
	if (blockIt == blocks.end())
		return nullptr;

	return blockIt->second;
}

GameBlock::Ptr BlockScene::GetBlock(const Vec2D& pos, bool aliveOnly) const {

	for (auto& block : blocks)
		if (block.second->GetPosition() == pos && (!aliveOnly || block.second->IsAlive()))
			return block.second;

	return nullptr;
}

bool BlockScene::CanAddBlock(GameBlock::Ptr blockPtr) {

	const auto& blockPos = blockPtr->GetPosition();
	const auto& existingBlock = GetBlock(blockPos, true);
	const bool canAdd = existingBlock == nullptr;

	return canAdd;
}

bool BlockScene::AddBlock(GameBlock::Ptr blockPtr) {

	const bool canAdd = CanAddBlock(blockPtr);
	if (!canAdd)
		return false;

	blocks.emplace(blockPtr->GetID(), blockPtr);
	return true;
}

Figure::~Figure() {
}

Vec2D GetRotated(Figure::AngleCW angle, Vec2D coord) {

	Vec2D rotated = coord;
	int rotateIterations = (int) angle;

	while (rotateIterations) {
		rotated = {rotated.y, -rotated.x};
		rotateIterations--;
	}

	return rotated;
}

bool BlockScene::TryRotate(Figure::Ptr figPtr) {

	static const std::vector<Vec2D> validOffsets = {{0, 0}, {1, 0}, {-1, 0}, {0, -1}, {0, 1}};
	const auto& blockIDs = figPtr->GetBlockIDs();

	std::vector<GameBlock::Ptr> figBlocks;
	figBlocks.reserve(blockIDs.size());
	for (auto blockID : blockIDs) {
		figBlocks.push_back(blocks.at(blockID));
	}

	Vec2D selectedOffset = validOffsets.at(0);
	Figure::AngleCW selectedRotation = Figure::AngleCW::R0;

	const std::vector<Figure::AngleCW> rotations = {Figure::AngleCW::R90, Figure::AngleCW::R180, Figure::AngleCW::R270};
	for (const auto rotation : rotations) {
		for (const auto& offset : validOffsets) {

			bool blocked = false;
			const Vec2D& figOrigin = figBlocks.at(0)->GetPosition() + offset;
			for (auto& block : figBlocks) {

				Vec2D blockPos = block->GetPosition();
				blockPos = blockPos + offset;
				blockPos = blockPos - figOrigin;
				Vec2D rotated = GetRotated(rotation, blockPos);
				rotated = rotated + figOrigin;
				const bool ok = CheckFigureBlockCanBePlaced(rotated);
				if (!ok) {
					blocked = true;
					break;
				}
			}

			if (!blocked) {
				// let's rotate

				for (auto& block : figBlocks) {

					Vec2D blockPos = block->GetPosition();
					blockPos = blockPos + offset;
					blockPos = blockPos - figOrigin;
					Vec2D rotated = GetRotated(rotation, blockPos);
					rotated = rotated + figOrigin;
					block->SetPosition(rotated);
					block->UpdateActorPosition();
				}

				return true;
			}
		}
	}

	return false;
}

std::vector<GameBlock::Ptr> Figure::CreateBlocks(const Vec2D& leftTop, UWorld* world) {

	std::vector<GameBlock::Ptr> newBlocks;
	const auto& figConfig = GetFigureConfig(type);

	const int xSize = figConfig[0].size();
	const int ySize = figConfig.size();

	for (int x = 0; x < xSize; ++x) {
		for (int y = 0; y < ySize; ++y) {

			const bool hasBlock = figConfig[y][x];
			if (!hasBlock)
				continue;

			GameBlock::Ptr newBlock = std::make_shared<GameBlock>();
			newBlock->SetPosition({leftTop.x + x, leftTop.y - y});
			newBlock->CreateActor(world);

			newBlock->SetFigure(GetID());

			newBlocks.push_back(newBlock);
			blockIDs.push_back(newBlock->GetID());
		}
	}

	return newBlocks;
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

bool GameBlock::TickDestroy(float dt) {

	if (finalDestroyTimer > 0.f)
		finalDestroyTimer -= dt;

	if (finalDestroyTimer < 0.f) {
		finalDestroyTimer = finalDestroyTimeMarker;
		
		return true;
	}

	return false;
}

void GameBlock::StartDestroy() {

	finalDestroyTimer = destroyActorAfter;
}

ABlockBase* GameBlock::CreateActor(UWorld* world) {

	const FRotator rotator = FRotator::ZeroRotator;
	const FActorSpawnParameters spawnParams;
	const FVector spawnLocation = ToWorldPosition(position);
	
	auto newBlockActor = world->SpawnActor<ABlockBase>(BlockBPClass, spawnLocation, rotator, spawnParams);
	actor = newBlockActor;
	return actor;
}

void GameBlock::UpdateActorPosition() {

	if (!actor)
		return;

	actor->SetActorLocation(ToWorldPosition(position));
}

void AYetrixGameModeBase::BeginPlay() {

	Reset();
}

void AYetrixGameModeBase::OnStartDestroying() {
}

void AYetrixGameModeBase::OnStartDropping() {

	blockScenePtr->DeconstructFigures();
	const auto& linesToDestruct = CheckDestruction();

	if (!linesToDestruct.empty()) {

		fallingPositions = blockScenePtr->GetFallingPositions(linesToDestruct);
		currDropState = DropState::DESTROYING;
		dropStateTimer = destroyStateDuration;	
		OnStartDestroying();

		const auto howManyLines = linesToDestruct.size();

		//assert(howManyLines <= 4);
		UpdateScore(score + scorePerCombo[howManyLines - 1]);

		stillStateDuration *= speedUpCoeff;
		dropStateDuration *= speedUpCoeff;
	}
	
	CheckAddFigures();
}

void AYetrixGameModeBase::UpdateScore(const int newScore) {
	
	score = newScore;
	AYetrixHUDBase* hud = Cast<AYetrixHUDBase> (GetWorld()->GetFirstPlayerController()->GetHUD());
	hud->UpdateScore(newScore);
}

void AYetrixGameModeBase::CheckAddFigures() {

	if (blockScenePtr->GetFigures().size() < minFigures)
	{
		const bool added = blockScenePtr->CreateRandomFigureAt({newFigureX, newFigureY}, GetWorld());
		if (!added)
			Reset();
	}
}

bool BlockScene::DeconstructFigures() {

	std::set<IDType> figuresToDeconstruct;

	for (auto& figure : figures) {
		
		unsigned maxHeight = 0;
		const bool canDrop = CheckFigureCanMove(figure.second, {0, -1}, maxHeight);
		if (!canDrop)
			figuresToDeconstruct.insert(figure.first);
	}

	for (auto figID : figuresToDeconstruct) {
		auto& fig = figures.at(figID);

		const auto& blockIDs = fig->GetBlockIDs();
		for (auto blockID : blockIDs) {
			auto block = GetBlock(blockID);
			block->SetFigure(invalidID);
		}

		figures.erase(figID);
	}

	const bool deconstructed = !figuresToDeconstruct.empty();
	return deconstructed;
}

bool BlockScene::MoveBlock(const Vec2D& direction) {

	const auto lowestFigID = GetLowestFigureID();

	if (lowestFigID == invalidID)
		return false;

	auto& figure = GetFigures().at(lowestFigID);

	unsigned distance = 0;
	const bool canMove = CheckFigureCanMove(figure, direction, distance);

	if (!canMove)
		return false;

	const auto& blockIDs = figure->GetBlockIDs();
	for (auto blockID : blockIDs) {

		auto block = GetBlock(blockID);
		const auto& prevPos = block->GetPosition();
		const auto newPos = prevPos + direction;
		block->SetPosition(newPos);
		block->UpdateActorPosition();
	}

	return true;
}

void AYetrixGameModeBase::Left() {
	
	leftPending++;
}

void AYetrixGameModeBase::Right() {

	rightPending++;
}

void AYetrixGameModeBase::Rotate() {

	rotatePending++;
}

void AYetrixGameModeBase::Drop() {
	quickDropRequested = true;
	dropStateTimer = 0.f;
}

void AYetrixGameModeBase::Down() {
	if (currDropState == DropState::STILL)
		dropStateTimer = 0.f;
}

std::map<IDType, Vec2D> BlockScene::GetFallingPositions(const std::set<int>& destroyedLines) const {

	std::map<IDType, Vec2D> fallingPositions;
	int fallAccum = 0;

	for (int y = 1; y < checkHeight; ++y) {
		
		const bool isLineDestroyed = destroyedLines.count(y) > 0;
		if (isLineDestroyed) {
			++fallAccum;
			continue;
		}
		else if (fallAccum == 0)
			continue;

		for (int x = 1; x < rightBorderX; ++x) {
			auto block = GetBlock({x, y}, true);
			if (block && block->IsAlive() && block->GetFigureID() == invalidID)
			{
				const int newY = y - fallAccum;
				const auto id = block->GetID();
				fallingPositions.insert({id, {x, newY}});
			}
		}
	}

	return fallingPositions;
}

std::set<int> AYetrixGameModeBase::CheckDestruction() {

	std::set<int> linesToBoom;

	for (int y = 1; y < checkHeight; ++y) {
		bool hasHoles = false;
		for (int x = 1; x < rightBorderX; ++x)
		{
			Vec2D checkPos(x, y);
			auto blockPtr = blockScenePtr->GetBlock(checkPos, true);
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
			auto blockPtr = blockScenePtr->GetBlock(blockPos, true);

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

	for (const auto& fallingBlockInfo : fallingPositions) {

		auto blockPtr = blockScenePtr->GetBlock(fallingBlockInfo.first);
		blockPtr->SetPosition(fallingBlockInfo.second);
		blockPtr->UpdateActorPosition();
	}
	
	fallingPositions.clear();
}

void AYetrixGameModeBase::FinalizeLogicalDrop() {

	const auto lowestFigID = blockScenePtr->GetLowestFigureID();

	auto& figures = blockScenePtr->GetFigures();
	for (auto& figure : figures) {
		
		unsigned maxHeight = 0;
		const bool canDrop = blockScenePtr->CheckFigureCanMove(figure.second, {0, -1}, maxHeight);
		if (!canDrop)
			continue;

		const bool isLowestOne = figure.first == lowestFigID;
		const bool canQuickDrop = isLowestOne && quickDropRequested; 
		const int heightToDrop = canQuickDrop ? maxHeight : 1;
		
		const auto& blockIDs = figure.second->GetBlockIDs();
		
		for (auto blockID : blockIDs) {
			auto block = blockScenePtr->GetBlock(blockID);
			const auto& logicalPos = block->GetPosition();
			auto dropLogicalPos = logicalPos;
			dropLogicalPos.y -= heightToDrop;

			block->SetPosition(dropLogicalPos);
		}
	}

	quickDropRequested = false;
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

	if (dropStateTimer >= 0.f)
		return false;

	if (currDropState == DropState::STILL) {
		currDropState = DropState::DROPPING;
		dropStateTimer = dropStateDuration;	
		OnStartDropping();
	}
	else if (currDropState == DropState::DROPPING) {
		currDropState = DropState::STILL;
		dropStateTimer = stillStateDuration;
		OnStopDropping();
	}
	else if (currDropState == DropState::DESTROYING) {
		currDropState = DropState::STILL;
		dropStateTimer = stillStateDuration;
		OnStopDestroying();
	}

	return true;
}

bool BlockScene::CheckFigureBlockCanBePlaced(const Vec2D& position) {

	const bool positionBoundsOk = position.y > 0 && position.x > 0 && position.x < rightBorderX;
	if (!positionBoundsOk)
		return false;

	const auto& blockAtPosition = GetBlock(position, true);
	if (!blockAtPosition)
		return true;

	const bool belongsToExistingFigure = blockAtPosition->GetFigureID() != invalidID;
	const bool free = belongsToExistingFigure;
	return free;
}

bool BlockScene::CheckBlockCanMove(GameBlock::Ptr blockPtr, Vec2D direction, unsigned& maxDistance){

	const Vec2D blockPos = blockPtr->GetPosition();
	Vec2D farest = {blockPos.x, blockPos.y};

	bool currOk = true;
	do {
		farest = farest + direction;
		currOk = CheckFigureBlockCanBePlaced(farest);
		++maxDistance;
	}
	while(currOk);
	
	--maxDistance;
	farest = farest - direction;
	const bool canDrop = blockPos != farest;
	return canDrop;
}

bool BlockScene::CheckFigureCanMove(Figure::Ptr figPtr, Vec2D direction, unsigned& maxDistance) {

	const auto& figBlockIDs = figPtr->GetBlockIDs();
	maxDistance = std::numeric_limits<int>::max();
	for (auto& blockID : figBlockIDs) {
		
		auto blockPtr = GetBlock(blockID);
		
		unsigned maxDistForBlock = 0;
		const bool canMoveBlock = CheckBlockCanMove(blockPtr, direction, maxDistForBlock);
		if (!canMoveBlock) {
			maxDistance = 0;
			return false;
		}
		maxDistance = std::min(maxDistForBlock, maxDistance);
	}

	const bool canDrop = maxDistance > 0;
	return canDrop;
}

IDType BlockScene::GetLowestFigureID() {
	
	IDType lowestFigID = invalidID;
	int lowestBlockY = std::numeric_limits<int>::max();

	for (const auto& figurePair : figures) {

		const auto& blockIDs = figurePair.second->GetBlockIDs();
		for (const auto blockID : blockIDs) {

			const auto& block = GetBlock(blockID);
			const auto& blockPos = block->GetPosition();

			if (lowestBlockY > blockPos.y) {
				lowestBlockY = blockPos.y;
				lowestFigID = figurePair.first;
			}
		}
	}

	return lowestFigID;
}

void AYetrixGameModeBase::Reset() {

	blockScenePtr = std::make_shared<BlockScene>();
	dropStateTimer = stillStateInitialDuration;

	auto* pawn = GetWorld()->GetFirstPlayerController()->GetPawn();
	auto* yetrixPawn = dynamic_cast<AYetrixPawn*>(pawn);
	yetrixPawn->SetGameMode(this);

	UpdateScore(0);
}

void AYetrixGameModeBase::UpdateVisualDestroy(const float progress) {

	for (const auto& fallingPosPair : fallingPositions)
	{
		auto block = blockScenePtr->GetBlock(fallingPosPair.first);

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

	const auto lowestFigID = blockScenePtr->GetLowestFigureID();

	auto& figures = blockScenePtr->GetFigures();
	for (auto& figure : figures) {
		
		unsigned maxHeight = 0;
		const bool canDrop = blockScenePtr->CheckFigureCanMove(figure.second, {0, -1}, maxHeight);
		if (!canDrop)
			continue;

		const bool isLowestOne = figure.first == lowestFigID;
		const bool canQuickDrop = isLowestOne && quickDropRequested; 
		const int heightToDrop = canQuickDrop ? maxHeight : 1;
		
		const auto& blockIDs = figure.second->GetBlockIDs();
		
		for (auto blockID : blockIDs) {
			auto block = blockScenePtr->GetBlock(blockID);
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

void BlockScene::CleanupBlocks(const float dt) {

	std::set<IDType> blocksToDestroy;

	for (auto& block : blocks) {
		const bool needFinalDestroy = block.second->TickDestroy(dt);
		if (needFinalDestroy)
			blocksToDestroy.insert(block.second->GetID());
	}

	for (auto& blockIDToDestroy : blocksToDestroy)
		blocks.erase(blockIDToDestroy);
}

void AYetrixGameModeBase::SimulationTick(float dt) {

	dropStateTimer -= dt;
	CheckChangeDropState();

	if (currDropState == DropState::DESTROYING) {

		const float dropProgress = 1.f - (dropStateTimer / destroyStateDuration);
		UpdateVisualDestroy(dropProgress);

	}
	else if (currDropState == DropState::DROPPING) {
		const float dropProgress = 1.f - (dropStateTimer / dropStateDuration);
		UpdateVisualDrop(dropProgress);
	} else {
	
		while (leftPending)
		{
			leftPending--;

			const bool moveOk = blockScenePtr->MoveBlock({-1, 0});
			if (!moveOk)
				break;
		}
		while (rightPending) {

			rightPending--;
			const bool moveOk = blockScenePtr->MoveBlock({1, 0});
			if (!moveOk)
				break;
		}

		while (rotatePending)
		{
			rotatePending--;

			const auto lowestFigID = blockScenePtr->GetLowestFigureID();
			if (lowestFigID != invalidID) {
				auto& figure = blockScenePtr->GetFigures().at(lowestFigID);
				const bool rotated = blockScenePtr->TryRotate(figure);
			}			
		}
	}

	blockScenePtr->CleanupBlocks(dt);
}

void AYetrixGameModeBase::Tick(float dt) {

	dtAccum += dt;
	while (dtAccum >= simulationUpdateInterval)
	{
		dtAccum -= simulationUpdateInterval;
		SimulationTick(simulationUpdateInterval);
	}	
}