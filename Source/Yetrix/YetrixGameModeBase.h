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
		DESTROYING
	};

	bool CheckChangeDropState();

	void Reset();

	void OnStartDestroying();
	void OnStartDropping();
	void OnStopDropping();
	void OnStopDestroying();
	
	void FinalizeLogicalDrop();
	void FinalizeLogicalDestroy();
	std::set<int> CheckDestruction();

	void InitSounds();

	bool PlaySoundWithRandomIndex(const std::string& prefix, const int count);
	bool PlaySound(const std::string& what);

	std::map<std::string, USoundWave*> soundsMap;

	struct State {

		State() {
			blockScenePtr = std::make_unique<BlockScene>();
		}

		DropState currDropState = DropState::STILL;

		float dropStateTimer = 0.f;
		float stillStateDuration = stillStateInitialDuration;
		float dropStateDuration = dropStateInitialDuration;
		float destroyStateDuration = destroyingStateInitialDuration;

		bool quickDropRequested = false;

		int leftPending = 0;
		int rightPending = 0;
		int rotatePending = 0;

		int score = 0;

		float lightAngleCurrent = 0.f;

		float lightAngleStart = 0.f;
		float lightAngleEnd = 0.f;
		float sunMoveFinishTimer = 0.f;

		std::map<IDType, Vec2D> fallingPositions;
		std::unique_ptr<BlockScene> blockScenePtr;
	};

	float dtAccum = 0.f;

	virtual void BeginPlay() override;
	virtual void Tick(float dt) override;

	void SimulationTick(float dt);
	void CheckAddFigures();
	void UpdateVisualDrop(float progress);
	void UpdateVisualDestroy(float progress);
	
	void AddScore(const int score);
	void UpdateScoreUI();
	void UpdateSunlight(const float angle);
	void UpdateSunMove(const float dt);
	
	std::unique_ptr<State> statePtr;

public:
	void Left();
	void Right();
	void Drop();
	void Down();
	void Rotate();
};
