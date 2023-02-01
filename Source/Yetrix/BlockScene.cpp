#include "BlockScene.h"
#include "YetrixConfig.h"
#include <array>
#include <random>

#include "3rdparty/nlohmann/json.hpp"

typedef std::array<std::array<bool, 4>, 2> FigureConfigArr;

static TSubclassOf<class ABlockBase> BlockBPClass;

std::mt19937& localRnd() {

	static std::mt19937 rnd(0);
	return rnd;
}

bool BlockScene::InitSubclasses() {

	const bool wasNeedInit = !BlockBPClass.Get();
	if (wasNeedInit) {
		const ConstructorHelpers::FObjectFinder<UClass> blueprint_finder_BlockBP(TEXT("Blueprint'/Game/BlockBP.BlockBP_C'"));
		BlockBPClass = (UClass*) blueprint_finder_BlockBP.Object;
	}
	
	return wasNeedInit;
}

const FigureConfigArr& GetFigureConfig(const Figure::FigType type) {

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

IDType GameBlock::BlockInfo::NewID() {
	static IDType id = 0;
	return id++;
}

IDType Figure::NewID() {
	static IDType id = 0;
	return id++;
}

BlockScene::BlockScene(){
}

BlockScene::~BlockScene()
{
	
}

bool BlockScene::CreateFigureAt(Figure::FigType type, const Vec2D& pos, UWorld* world) {

	Figure::Ptr newFigurePtr = std::make_shared<Figure>(type);

	const auto newBlocks = newFigurePtr->CreateBlocks(pos, world);

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

	std::uniform_int_distribution<int> uni(0, static_cast<int>(Figure::FigType::UNDEFINED) - 1);
	const Figure::FigType figType = static_cast<Figure::FigType> (uni(localRnd()));
	const bool figAdded = CreateFigureAt(figType, pos, world);
	return figAdded;
}

GameBlock::Ptr BlockScene::GetBlock(const IDType blockID) const {
	const auto blockIt = blocks.find(blockID);
	if (blockIt == blocks.end())
		return nullptr;

	return blockIt->second;
}

GameBlock::Ptr BlockScene::GetBlock(const Vec2D& pos, bool aliveOnly) const {

	for (const auto& [id, blockPtr] : blocks)
		if (blockPtr->GetPosition() == pos && (!aliveOnly || blockPtr->IsAlive()))
			return blockPtr;

	return nullptr;
}

bool BlockScene::CanAddBlock(const GameBlock::Ptr blockPtr) const
{
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
	int rotateIterations = static_cast<int>(angle);

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

			auto getBlockRotatedPos = [offset, figOrigin, rotation](const Vec2D& blockPos)
			{
				const auto blockPosRelative = blockPos + offset - figOrigin;
				Vec2D rotated = GetRotated(rotation, blockPosRelative);
				rotated = rotated + figOrigin;
				return rotated;
			};

			for (const auto& block : figBlocks) {
				
				const auto rotated = getBlockRotatedPos(block->GetPosition());
				const bool ok = CheckFigureBlockCanBePlaced(rotated);
				if (!ok) {
					blocked = true;
					break;
				}
			}

			if (!blocked) {
				// let's rotate

				for (const auto& block : figBlocks) {
					const auto rotated = getBlockRotatedPos(block->GetPosition());
					block->SetPosition(rotated);
					block->UpdateActorPosition();
				}

				return true;
			}
		}
	}

	return false;
}

void GameBlock::Init(const BlockInfo& givenInfo)
{
	info = givenInfo;
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

			
			GameBlock::BlockInfo newBlockInfo;
			newBlockInfo.position = {leftTop.x + x, leftTop.y - y};
			newBlockInfo.figureID = GetID();

			GameBlock::Ptr newBlock = std::make_shared<GameBlock>();
			newBlock->Init(newBlockInfo);
			newBlock->CreateActor(world);

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

bool GameBlock::TickDestroy(const float dt) {

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

bool BlockScene::DeconstructFigures() {

	std::set<IDType> figuresToDeconstruct;

	for (auto& [id, figPtr] : figures) {
		
		unsigned maxHeight = 0;
		const bool canDrop = CheckFigureCanMove(figPtr, {0, -1}, maxHeight);
		if (!canDrop)
			figuresToDeconstruct.insert(id);
	}

	for (auto figID : figuresToDeconstruct) {
		const auto& fig = figures.at(figID);

		const auto& blockIDs = fig->GetBlockIDs();
		for (const auto blockID : blockIDs) {
			const auto block = GetBlock(blockID);
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

	const auto& figure = GetFigures().at(lowestFigID);

	unsigned distance = 0;
	const bool canMove = CheckFigureCanMove(figure, direction, distance);

	if (!canMove)
		return false;

	const auto& blockIDs = figure->GetBlockIDs();
	for (const auto blockID : blockIDs) {
		const auto block = GetBlock(blockID);
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

		if (fallAccum == 0)
			continue;

		for (int x = 1; x < rightBorderX; ++x) {
			const auto block = GetBlock({x, y}, true);
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

bool BlockScene::CheckFigureBlockCanBePlaced(const Vec2D& position) const
{

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

bool BlockScene::CheckBlockCanMove(GameBlock::Ptr blockPtr, Vec2D direction, unsigned& maxDistance) const
{
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

bool BlockScene::CheckFigureCanMove(const Figure::Ptr figPtr, const Vec2D direction, unsigned& maxDistance) const
{
	const auto& figBlockIDs = figPtr->GetBlockIDs();
	maxDistance = std::numeric_limits<int>::max();
	for (auto& blockID : figBlockIDs) {
		const auto blockPtr = GetBlock(blockID);
		
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

IDType BlockScene::GetLowestFigureID() const
{	
	IDType lowestFigID = invalidID;
	int lowestBlockY = std::numeric_limits<int>::max();

	for (const auto& [id, figPtr] : figures) {

		const auto& blockIDs = figPtr->GetBlockIDs();
		for (const auto blockID : blockIDs) {

			const auto& block = GetBlock(blockID);
			const auto& blockPos = block->GetPosition();

			if (lowestBlockY > blockPos.y) {
				lowestBlockY = blockPos.y;
				lowestFigID = id;
			}
		}
	}

	return lowestFigID;
}

void BlockScene::CleanupBlocks(const float dt) {

	std::set<IDType> blocksToDestroy;

	for (const auto& [id, block] : blocks) {
		const bool needFinalDestroy = block->TickDestroy(dt);
		if (needFinalDestroy)
			blocksToDestroy.insert(id);
	}

	for (auto& blockIDToDestroy : blocksToDestroy)
		blocks.erase(blockIDToDestroy);
}

json BlockScene::Save() const
{
	json doc;
	doc["blocks"] = json::object();

	for (const auto [id, blockPtr] : blocks)
	{
		if (!blockPtr->IsAlive())
			continue;

		json& blockObject = doc["blocks"][std::to_string(id)];
		const auto& blockInfo = blockPtr->GetBlockInfo();

		json& posObject = blockObject["pos"];
		posObject["x"] = blockInfo.position.x;
		posObject["y"] = blockInfo.position.y;

		if (blockInfo.figureID != invalidID)
			blockObject["figure"] = blockInfo.figureID;
	}
	
	doc["figures"] = json::object();
	json& figuresObj = doc["figures"];
	for (const auto [id, figurePtr] : figures)
	{
		json& figureObject = doc["figures"][std::to_string(id)];
		figureObject["type"] = figurePtr->GetType();

		figureObject["blocks"] = json::array();
		json& blocksObj = figureObject["blocks"];

		for (const auto blockID : figurePtr->GetBlockIDs())
		{
			blocksObj.push_back(blockID);
		}
	}

	return doc;
}

bool BlockScene::Load(const json& data, UWorld* world)
{
	figures.clear();
	blocks.clear();

	const json& doc = data;

	const json& blocksObj = doc["blocks"];

	for (json::const_iterator blockIt = blocksObj.begin(); blockIt != blocksObj.end(); ++blockIt)
	{
		const json& blockObj = blockIt.value();
		GameBlock::BlockInfo blockInfo;

		if (blockObj.contains("figure"))
			blockInfo.figureID = blockObj["figure"].get<int>();

		blockInfo.id = std::atoi(blockIt.key().c_str());
		blockInfo.position.x = blockObj["pos"]["x"].get<int>();
		blockInfo.position.y = blockObj["pos"]["y"].get<int>();

		const GameBlock::Ptr newBlock = std::make_shared<GameBlock>();
		newBlock->Init(blockInfo);
		newBlock->CreateActor(world);

		blocks[blockInfo.id] = newBlock;
	}

	const json& figuresObj = doc["figures"];
	for (json::const_iterator figureIt = figuresObj.begin(); figureIt != figuresObj.end(); ++figureIt)
	{
		const auto figID = std::atoi(figureIt.key().c_str());
		const json& figObj = figureIt.value();
		const auto figType = figObj["type"].get<int>();

		auto newFigurePtr = std::make_shared<Figure>(static_cast<Figure::FigType>(figType), figID);

		std::vector<IDType> blockIds;
		const json& blockIDsObj = figObj["blocks"];
		for (json::const_iterator blockIDsIt = blockIDsObj.begin(); blockIDsIt != blockIDsObj.end(); ++blockIDsIt)
			blockIds.push_back(blockIDsIt.value().get<int>());

		newFigurePtr->SetBlockIDs(blockIds);

		figures.emplace(newFigurePtr->GetID(), newFigurePtr);
	}

	return true;
}