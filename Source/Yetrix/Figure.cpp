#include "Figure.h"
#include <map>
#include <array>

typedef std::array<std::array<bool, 4>, 2> FigureConfigArr;

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

Figure::~Figure() {
}