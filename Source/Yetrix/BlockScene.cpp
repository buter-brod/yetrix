#include "BlockScene.h"
#include "YetrixConfig.h"
#include <random>
#include "Figure.h"

#include "3rdparty/nlohmann/json.hpp"

BlockScene::BlockScene(){
}

BlockScene::~BlockScene()
{	
}

Figure::Ptr BlockScene::CreateFigureAt(Figure::FigType type, const Vec2D& pos, UWorld* world) {

	Figure::Ptr newFigurePtr = std::make_shared<Figure>(type);

	const auto newBlocks = newFigurePtr->CreateBlocks(pos, world);

	for (const auto& blockPtr : newBlocks) {
		const bool canAddBlock = CanAddBlock(blockPtr);
		if (!canAddBlock)
			return nullptr;
	}

	for (const auto& blockPtr : newBlocks) {
		const bool addedOk = AddBlock(blockPtr);
		if (!addedOk) 
			return nullptr;
	}

	figures.emplace(newFigurePtr->GetID(), newFigurePtr);
	return newFigurePtr;
}

Figure::Ptr BlockScene::CreateRandomFigureAt(const Vec2D& pos, UWorld* world) {

	std::uniform_int_distribution<int> uni(0, static_cast<int>(Figure::FigType::UNDEFINED) - 1);
	const Figure::FigType figType = static_cast<Figure::FigType> (uni(Utils::localRnd()));
	const auto figAdded = CreateFigureAt(figType, pos, world);
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

	const auto newBlockID = blockPtr->GetID();

	if (blocks.count(newBlockID) > 0)
	{
		checkf(false, TEXT("BlockScene::AddBlock error, duplicate ID %s"), TCHARIFYSTDSTRING(blockPtr->GetID()));
		return false;
	}

	blocks.emplace(newBlockID, blockPtr);
	return true;
}

Vec2D GetRotated(Figure::AngleCW angle, const Vec2D coord) {

	Vec2D rotated = coord;
	int rotateIterations = static_cast<int>(angle);

	while (rotateIterations) {
		rotated = {rotated.y, -rotated.x};
		rotateIterations--;
	}

	return rotated;
}

std::map<IDType, Vec2D> BlockScene::GetRotatedPositions(const Figure::Ptr figPtr) const
{
	std::map<IDType, Vec2D> positions;

	static const std::vector<Vec2D> validOffsets = {{0, 0}, {1, 0}, {-1, 0}, {0, -1}, {0, 1}};
	const auto& blockIDs = figPtr->GetBlockIDs();

	std::vector<GameBlock::Ptr> figBlocks;
	figBlocks.reserve(blockIDs.size());
	for (auto blockID : blockIDs) {
		figBlocks.push_back(blocks.at(blockID));
	}


	static const std::vector rotations = {Figure::AngleCW::R90, Figure::AngleCW::R180, Figure::AngleCW::R270};

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
				// found valid angle and offset, let's calculate new positoins, finally

				for (const auto& block : figBlocks) {
					const auto rotatedPos = getBlockRotatedPos(block->GetPosition());
					positions[block->GetID()] = rotatedPos;
				}

				return positions;
			}
		}
	}

	// failed to rotate
	return positions;	
}

BlockScene::ConditionInfo BlockScene::CalculateSceneConditionInfo() const
{
	ConditionInfo info;

	std::map<int, int> heightMap;
		
	for (const auto& [id, blockInfo]: blocks)
	{
		if (!blockInfo->IsAlive())
			continue;

		if (blockInfo->GetBlockInfo().figureID != Utils::emptyID)
			continue;
			// block is not frozen yet

		const auto& pos = blockInfo->GetPosition();
		const auto x = pos.x;
		const auto y = pos.y;

		if (info.maxHeight < y)
			info.maxHeight = y;

		if (heightMap.count(x) == 0 || heightMap.at(x) < y)
		{
			heightMap[x] = y;			
		}

	}

	for (const auto [x, y] : heightMap)
	{
		if (info.minHeight < 0 || info.minHeight > y)
				info.minHeight = y;
	}
	
	for (int x = 1; x < rightBorderX; ++x)
	{
		if (heightMap.count(x) == 0)
			continue;
			// no blocks on this column

		int y = heightMap.at(x) - 1;
		for (; y > 0; --y)
		{
			auto block = GetBlock({x, y}, true);
			if (!block)
			{
				info.holes.emplace(x, y);
			}
		}
	}

	if (info.minHeight < 0)
		info.minHeight = 0;

	return info;
}

int BlockScene::CalculateSceneConditionScore() const
{
	const auto conditionInfo = CalculateSceneConditionInfo();

	constexpr float maxHeightCoeff = 1.f;
	constexpr float minHeightCoeff = 0.5f;
	constexpr float holesCoeff = 5.f;
	constexpr float blocksCoeff = 0.2f;


	const int resultScore = 
		maxHeightCoeff * conditionInfo.maxHeight + 
		minHeightCoeff * conditionInfo.minHeight + 
		holesCoeff * conditionInfo.holes.size() + 
		blocksCoeff * blocks.size();

	return resultScore;
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
			block->SetFigure(Utils::emptyID);
		}

		figures.erase(figID);
	}

	const bool deconstructed = !figuresToDeconstruct.empty();
	return deconstructed;
}

bool BlockScene::TryMoveBlock(const Vec2D& direction) {

	const auto lowestFigID = GetLowestFigureID();

	if (lowestFigID == Utils::emptyID)
		return false;

	const auto& figIt = figures.find(lowestFigID);
	if (figIt == figures.end())
	{
		checkf(false, TEXT("BlockScene::TryMoveBlock error, no lowest figure"));
		return false;
	}

	const auto& figure = figIt->second;
	unsigned distance = 0;
	const bool canMove = CheckFigureCanMove(figure, direction, distance);

	if (!canMove)
		return false;

	const auto& blockIDs = figure->GetBlockIDs();
	for (const auto blockID : blockIDs) {
		const auto block = GetBlock(blockID);
		const auto& prevPos = block->GetPosition();
		const auto newPos = prevPos + direction;
		block->SetPositionAndUpdateActor(newPos, moveLeftRightAnimDuration);
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
			if (block && block->IsAlive() && block->GetFigureID() == Utils::emptyID)
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

	const bool belongsToExistingFigure = blockAtPosition->GetFigureID() != Utils::emptyID;
	const bool free = belongsToExistingFigure;
	return free;
}

bool BlockScene::CheckBlockCanMove(GameBlock::Ptr blockPtr, Vec2D direction, unsigned& maxDistance) const
{
	const Vec2D blockPos = blockPtr->GetPosition();
	Vec2D farest = {blockPos.x, blockPos.y};

	bool currOk;
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
	IDType lowestFigID = Utils::emptyID;
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

void BlockScene::Tick(const float dt)
{
	CleanupBlocks(dt);

	for (const auto& blockPair : blocks)
	{
		blockPair.second->Tick(dt);
	}
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

		json& blockObject = doc["blocks"][id];
		const auto& blockInfo = blockPtr->GetBlockInfo();

		json& posObject = blockObject["pos"];
		posObject["x"] = blockInfo.position.x;
		posObject["y"] = blockInfo.position.y;

		if (blockInfo.figureID != Utils::emptyID)
			blockObject["figure"] = blockInfo.figureID;
	}
	
	doc["figures"] = json::object();
	for (const auto [id, figurePtr] : figures)
	{
		json& figureObject = doc["figures"][id];
		figureObject["type"] = figurePtr->GetType();

		figureObject["blocks"] = json::array();
		json& blocksObj = figureObject["blocks"];

		for (const auto blockID : figurePtr->GetBlockIDs())
		{
			if (blocks.count(blockID) == 0)
			{
				checkf(false, TEXT("BlockScene::Save error, figure %s refers to block %s, which doesn't exist. Cannot save game properly"), TCHARIFYSTDSTRING(figurePtr->GetID()), TCHARIFYSTDSTRING(blockID));
				continue;
			}

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
			blockInfo.figureID = blockObj["figure"].get<IDType>();

		blockInfo.id = blockIt.key().c_str();
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
		const IDType figID = figureIt.key();
		const json& figObj = figureIt.value();
		const auto figType = figObj["type"].get<int>();

		auto newFigurePtr = std::make_shared<Figure>(static_cast<Figure::FigType>(figType), figID);

		std::vector<IDType> blockIds;
		const json& blockIDsObj = figObj["blocks"];
		for (json::const_iterator blockIDsIt = blockIDsObj.begin(); blockIDsIt != blockIDsObj.end(); ++blockIDsIt) {

			const auto blockID = blockIDsIt.value().get<IDType>();
			if (blocks.count(blockID) == 0)
			{
				checkf(false, TEXT("BlockScene::Load figure %s refers to block %s, which doesn't exist. Cannot load game properly"), TCHARIFYSTDSTRING(newFigurePtr->GetID()), TCHARIFYSTDSTRING(blockID));
				continue;
			}

			blockIds.push_back(blockID);
		}

		newFigurePtr->SetBlockIDs(blockIds);

		figures.emplace(newFigurePtr->GetID(), newFigurePtr);
	}

	return true;
}