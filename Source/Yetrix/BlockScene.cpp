#include "BlockScene.h"
#include "YetrixConfig.h"
#include <array>
#include <random>

typedef std::array<std::array<bool, 4>, 2> FigureConfigArr;

static TSubclassOf<class ABlockBase> BlockBPClass;

std::mt19937& localRnd() {

	static std::mt19937 rnd(0);
	return rnd;
}

bool BlockScene::InitSubclasses() {

	const bool wasNeedInit = !BlockBPClass.Get();
	if (wasNeedInit) {
		ConstructorHelpers::FObjectFinder<UBlueprint> blueprint_finder_BlockBP(TEXT("Blueprint'/Game/BlockBP.BlockBP'"));
		BlockBPClass = (UClass*) blueprint_finder_BlockBP.Object->GeneratedClass;
	}
	
	return wasNeedInit;
}

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

BlockScene::BlockScene(){
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
	const Figure::FigType figType = static_cast<Figure::FigType> (uni(localRnd()));
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