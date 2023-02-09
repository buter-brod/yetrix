#pragma once

#include <set>
#include <map>
#include "Utils.h"
#include "Figure.h"

#include "3rdparty/nlohmann/json_fwd.hpp"

using json = nlohmann::json;

class BlockScene {

public:
	BlockScene();
	~BlockScene();

	bool CreateRandomFigureAt(const Vec2D& pos, UWorld* world);
	bool CreateFigureAt(Figure::FigType type, const Vec2D& pos, UWorld* world);

	GameBlock::Ptr GetBlock(const Vec2D& pos, bool aliveOnly) const;
	GameBlock::Ptr GetBlock(IDType blockID) const;

	typedef std::map<IDType, std::shared_ptr<Figure> > FigureMap;
	typedef std::map<IDType, std::shared_ptr<GameBlock> > BlockMap;

	FigureMap& GetFigures() {return figures;}
	BlockMap& GetBlocks() {return blocks;}

	json Save() const;
	bool Load(const json& data, UWorld* world);

	bool DeconstructFigures();
	IDType GetLowestFigureID() const;

	void Tick(float dt);
	void CleanupBlocks(float dt);

	bool CheckFigureCanMove(Figure::Ptr figPtr, Vec2D direction, unsigned& maxDistance) const;
	bool TryMoveBlock(const Vec2D& direction);
	bool TryRotate(Figure::Ptr figPtr);

	std::map<IDType, Vec2D> GetRotatedPositions(Figure::Ptr figPtr) const;
	std::map<IDType, Vec2D> GetFallingPositions(const std::set<int>& destroyedLines) const;

protected:	
	bool CanAddBlock(GameBlock::Ptr blockPtr) const;
	bool AddBlock(GameBlock::Ptr blockPtr);
	bool CheckFigureBlockCanBePlaced(const Vec2D& position) const;
	bool CheckBlockCanMove(GameBlock::Ptr blockPtr, Vec2D direction, unsigned& maxDistance) const;

private:
	FigureMap figures;
	BlockMap blocks;
};