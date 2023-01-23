#pragma once

#include "BlockBase.h"
#include <set>
#include <map>

typedef unsigned long long IDType;
constexpr IDType invalidID = std::numeric_limits<IDType>::max();

constexpr float finalDestroyTimeMarker = -1.f;

struct Vec2D {

	int x = 0;
	int y = 0;

	Vec2D(){}
	Vec2D(int xx, int yy) : x(xx), y(yy){}

	inline bool operator==(const Vec2D& second) const {
		return x == second.x && y == second.y;
	}

	inline bool operator!=(const Vec2D& second) const {
		return !(*this == second);
	}

	inline bool operator< (const Vec2D& second) const {
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

	static IDType NewID();

	GameBlock() {
		id = NewID();
	}

	~GameBlock();
	

	IDType GetID() const {return id;}
	IDType GetFigureID() const {return figureID;}
	const Vec2D& GetPosition() const {return position;}
	bool IsAlive() const {return finalDestroyTimer == 0.f;}
	bool IsFinallyDestroyed() const {return finalDestroyTimer == finalDestroyTimeMarker;}
	ABlockBase* GetActor() const {return actor;}

	bool TickDestroy(float dt);
	void StartDestroy();

	void SetPosition(const Vec2D& newPos) {
		position = newPos;
	}

	void SetFigure(IDType figID) {figureID = figID;}

	static FVector ToWorldPosition(const Vec2D pos);

	ABlockBase* CreateActor(UWorld* world);
	void UpdateActorPosition();

private:
	IDType id = 0;
	Vec2D position;

	float finalDestroyTimer = 0.f;
	ABlockBase* actor = nullptr;
	IDType figureID = invalidID;
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

	Figure(FigType newFigType) : type(newFigType) {
		id = NewID();
	}

	IDType GetID() const { return id; }

	std::vector<GameBlock::Ptr> CreateBlocks(const Vec2D& leftTop, UWorld* world);
	const std::vector<IDType>& GetBlockIDs() const { return blockIDs; }

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

	bool CreateRandomFigureAt(const Vec2D& pos, UWorld* world);
	bool CreateFigureAt(Figure::FigType type, const Vec2D& pos, UWorld* world);

	GameBlock::Ptr GetBlock(const Vec2D& pos, bool aliveOnly) const;
	GameBlock::Ptr GetBlock(IDType blockID) const;

	typedef std::map<IDType, std::shared_ptr<Figure> > FigureMap;
	typedef std::map<IDType,std::shared_ptr<GameBlock> > BlockMap;

	FigureMap& GetFigures() {return figures;}
	BlockMap& GetBlocks() {return blocks;}

	bool DeconstructFigures();
	IDType GetLowestFigureID();
	void CleanupBlocks(float dt);
	bool CheckFigureCanMove(Figure::Ptr figPtr, Vec2D direction, unsigned& maxDistance);
	bool MoveBlock(const Vec2D& direction);
	bool TryRotate(Figure::Ptr figPtr);

	std::map<IDType, Vec2D> GetFallingPositions(const std::set<int>& destroyedLines) const;

	static bool InitSubclasses();

protected:	
	bool CanAddBlock(GameBlock::Ptr blockPtr);
	bool AddBlock(GameBlock::Ptr blockPtr);
	bool CheckFigureBlockCanBePlaced(const Vec2D& position);
	bool CheckBlockCanMove(GameBlock::Ptr blockPtr, Vec2D direction, unsigned& maxDistance);

private:
	FigureMap figures;
	BlockMap blocks;
};