#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"

#include "BlockScene.h"
#include "YetrixConfig.h"

#include "YetrixGameModeBase.generated.h"

/**
 * 
 */
UCLASS()
class YETRIX_API AYetrixGameModeBase : public AGameModeBase
{
	GENERATED_BODY()

	AYetrixGameModeBase();
	
	enum class DropState {
		STILL,
		DROPPING,
		DESTROYING,
		ROTATING
	};

	enum class RotateSubState
	{
		BREAK_1,
		MOVE_2,
		ASSEMBLE_3,
		STATIC
	};

	bool CheckChangeDropState();

	void ResetGame();

	void Save();
	bool Load();

	bool HandleDestruction();
	std::map<int, std::vector<IDType>> GetBlocksSortedFromLower(const Figure::Ptr figurePtr) const;
	void OnStartDropping();
	void OnStopDropping();
	void OnStopDestroying();
	
	void FinalizeLogicalDestroy();
	std::set<int> CheckDestruction();
	static std::set<int> CheckDestruction(BlockScene& theBlockScene);

	void InitSounds();

	bool PlaySoundWithRandomIndex(const std::string& prefix, const int count);
	bool PlaySound(const std::string& what);

	std::map<std::string, USoundWave*> soundsMap;

	struct State {

		State() {
			blockScenePtr = std::make_unique<BlockScene>();
		}

		DropState currDropState = DropState::STILL;
		RotateSubState currRotateState = RotateSubState::STATIC;

		float dropStateTimer = 0.f;
		float stillStateDuration = stillStateInitialDuration;
		float dropStateDuration = dropStateInitialDuration;
		float destroyStateDuration = destroyingStateInitialDuration;

		bool quickDropRequested = false;

		int leftPending = 0;
		int rightPending = 0;
		int rotatePending = 0;

		int score = 0;
		int conditionScore = 0;

		float lightAngleCurrent = 0.f;

		float lightAngleStart = 0.f;
		float lightAngleEnd = 0.f;
		float sunMoveFinishTimer = 0.f;

		std::map<IDType, Vec2D> fallingPositions;
		std::unique_ptr<BlockScene> blockScenePtr;
	};

	float dtAccum = 0.f;

	int needUpdateScoreUI = 0;
	int needUpdateConditionScoreUI = 0;

	virtual void BeginPlay() override;
	virtual void Tick(float dt) override;

	void SimulationTick(float dt);
	bool CheckAddFigures();
	bool TryRotate();
	void HandleRotateAnimation();
	void HandlePlayerPendingInput();
	void UpdateVisualDestroy(float progress);
	
	void AddScore(const int score);
	void UpdateScoreUI();
	void UpdateConditionScoreUI();
	void GameOver();
	void RequestUpdateScoreUI();
	void RequestUpdateConditionScoreUI();
	void UpdateSunlight(const float angle);
	void UpdateSunMove(const float dt);

	void UpdateSpeed();

	bool CheckConditionChange();
	
	std::unique_ptr<State> statePtr;
	int hiScore = 0;
	int worstConditionScore = 0;

public:
	void Left();
	void Right();
	void Drop();
	void Down();
	void Rotate();

	const BlockScene* GetBlockScene() const {return statePtr->blockScenePtr.get();}

private:
	std::map<IDType, Vec2D> rotatedPositions;
};
