#pragma once
#include "Block.h"
#include "Utils.h"

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

	~Figure();

	explicit Figure(const FigType newFigType) : type(newFigType) {
		id = Utils::NewID();
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
	IDType id = Utils::emptyID;
	FigType type = FigType::UNDEFINED;
};
