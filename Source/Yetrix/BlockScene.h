#pragma once

#include "BlockBase.h"
#include <set>
#include <map>

#include "3rdparty/nlohmann/json_fwd.hpp"

typedef unsigned long long IDType;
constexpr IDType invalidID = std::numeric_limits<IDType>::max();
constexpr float finalDestroyTimeMarker = -1.f;

using json = nlohmann::json;

struct Vec2D {

	int x = 0;
	int y = 0;

	Vec2D(){}
	Vec2D(const int xx, const int yy) : x(xx), y(yy){}

	bool operator==(const Vec2D& second) const {
		return x == second.x && y == second.y;
	}

	bool operator!=(const Vec2D& second) const {
		return !(*this == second);
	}

	bool operator< (const Vec2D& second) const {
		if (y != second.y)
			return y < second.y;

		return x < second.x;
	}
};

inline Vec2D operator+(const Vec2D& first, const Vec2D& second) {

	return {first.x + second.x, first.y + second.y};
}

inline Vec2D operator-(const Vec2D& first, const Vec2D& second) {

	return {first.x - second.x, first.y - second.y};
}

class Figure;

class GameBlock : public std::enable_shared_from_this<GameBlock> {
public:
	typedef std::shared_ptr<GameBlock> Ptr;
		
	GameBlock() {
	}

	~GameBlock();
	
	IDType GetID() const {return info.id;}
	IDType GetFigureID() const {return info.figureID;}
	const Vec2D& GetPosition() const {return info.position;}
	bool IsAlive() const {return finalDestroyTimer == 0.f;} //-V550
	bool IsFinallyDestroyed() const {return finalDestroyTimer == finalDestroyTimeMarker;} //-V550
	ABlockBase* GetActor() const {return actor;}

	bool TickDestroy(float dt);
	void StartDestroy();

	void SetPosition(const Vec2D& newPos) {
		info.position = newPos;
	}

	void SetFigure(const IDType figID) {info.figureID = figID;}
	static FVector ToWorldPosition(const Vec2D pos);

	ABlockBase* CreateActor(UWorld* world);
	void UpdateActorPosition() const;

	struct BlockInfo {
		IDType id = 0;
		Vec2D position;
		IDType figureID = invalidID;

		static IDType NewID();

		BlockInfo() : id(NewID()) {
		}
	};

	void Init(const BlockInfo& givenInfo);

	const BlockInfo& GetBlockInfo() const {return info;}

private:	
	BlockInfo info;
	float finalDestroyTimer = 0.f;
	
	ABlockBase* actor = nullptr;
};

class Figure : public std::enable_shared_from_this<Figure> {
public:
	enum class FigType {
		LONG,
		LEFT_BOOT,
		RIGHT_BOOT,
		BOX,
		LEFT_GUN,
		RIGHT_GUN,
		HAT,
		UNDEFINED
	};

	typedef std::shared_ptr<Figure> Ptr;

	static IDType NewID();

	~Figure();

	explicit Figure(const FigType newFigType) : type(newFigType) {
		id = NewID();
	}

	explicit Figure(const FigType newFigType, const IDType givenId) : id(givenId), type(newFigType) {
	}

	IDType GetID() const { return id; }
	FigType GetType() const {return type; }

	std::vector<GameBlock::Ptr> CreateBlocks(const Vec2D& leftTop, UWorld* world);
	const std::vector<IDType>& GetBlockIDs() const { return blockIDs; }

	void SetBlockIDs(const std::vector<IDType>& ids) {blockIDs = ids;}

	enum class AngleCW {
		R0,
		R90,
		R180,
		R270
	};
		
private:
	std::vector<IDType> blockIDs;
	IDType id = 0;
	FigType type = FigType::UNDEFINED;
};

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
	void CleanupBlocks(float dt);
	bool CheckFigureCanMove(Figure::Ptr figPtr, Vec2D direction, unsigned& maxDistance) const;
	bool MoveBlock(const Vec2D& direction);
	bool TryRotate(Figure::Ptr figPtr);

	std::map<IDType, Vec2D> GetFallingPositions(const std::set<int>& destroyedLines) const;

	static bool InitSubclasses();

protected:	
	bool CanAddBlock(GameBlock::Ptr blockPtr) const;
	bool AddBlock(GameBlock::Ptr blockPtr);
	bool CheckFigureBlockCanBePlaced(const Vec2D& position) const;
	bool CheckBlockCanMove(GameBlock::Ptr blockPtr, Vec2D direction, unsigned& maxDistance) const;

private:
	FigureMap figures;
	BlockMap blocks;
};