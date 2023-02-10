#pragma once

#include "Utils.h"

class ABlockBase;

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
	bool IsFinallyDestroyed() const {return finalDestroyRequested;}

	FVector GetActorLocation() const;
	void SetActorLocation(const FVector location);

	void SmokePuff();
	void Explode();
	void StartAnimatedMove(float theAnimDuration, FVector destination);

	static bool InitSubclasses();

	bool TickDestroy(float dt);
	void StartDestroy();

	bool SetPosition(const Vec2D& newPos);
	void SetPositionAndUpdateActor(const Vec2D& newPos, const float animDuration = 0.f);

	void SetFigure(const IDType figID) {info.figureID = figID;}
	static FVector ToWorldPosition(const Vec2D pos);

	ABlockBase* CreateActor(UWorld* world);
	void UpdateActorMovePosition() const;
	void UpdateActorFromLogicalPosition() const;


	void Tick(const float dt);

	struct BlockInfo {
		IDType id = Utils::emptyID;
		Vec2D position;
		IDType figureID;

		BlockInfo() : id(Utils::NewID()) {
		}
	};

	void Init(const BlockInfo& givenInfo);

	const BlockInfo& GetBlockInfo() const {return info;}

private:	
	BlockInfo info;
	float finalDestroyTimer = 0.f;

	FVector fromPosition;
	FVector toPosition;
	float animDuration = 0.f;
	float moveTimer = 0.f;

	bool finalDestroyRequested = false;
	
	ABlockBase* actor = nullptr;
};