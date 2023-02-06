#include "YetrixGameModeBase.h"
#include "YetrixPlayerController.h"
#include "BlockBase.h"
#include "YetrixPawn.h"

#include "GeometryCollection/GeometryCollectionComponent.h"
#include "Engine/DamageEvents.h"

#include "YetrixHUDBase.h"
#include "YetrixConfig.h"

#include "3rdparty/nlohmann/json.hpp"

#include "Utils.h"

AYetrixGameModeBase::AYetrixGameModeBase() {

	PrimaryActorTick.bCanEverTick = true;
	GameBlock::InitSubclasses();
	PlayerControllerClass = AYetrixPlayerController::StaticClass();

	InitSounds();
}

bool AYetrixGameModeBase::PlaySoundWithRandomIndex(const std::string& prefix, const int count) {

	const std::string finalSoundName = prefix + std::to_string(Utils::rnd0xi(count));
	return PlaySound(finalSoundName);
}

bool AYetrixGameModeBase::PlaySound(const std::string& what){

	auto soundIt = soundsMap.find(what);

	if (soundIt == soundsMap.end())
		return false;

	UGameplayStatics::PlaySound2D(this, soundIt->second);
	return true;
}

void AYetrixGameModeBase::InitSounds() {

	std::set<std::string> soundNames = {
		"bah10",
		"bah11",
		"bah12",
		"bah13",
		"bah20",
		"bah30",
		"bah40",
		"bdysh0",
		"bdysh1",
		"bdysh2",
		"gameover",
		"k0",
		"k1",
		"k2",
		"k3",
		"k4",
		"k5",
		"k6",
		"k7",
		"k8",
		"k9",
		"yeah"
	};

	for (const auto& name : soundNames){

		std::wstring wname(name.begin(), name.end());
		std::wstring assetPath = L"/Game/Sound/" + wname + L"." + wname;

		ConstructorHelpers::FObjectFinder<USoundWave> waveAsset(assetPath.c_str());
		// assert(waveAsset.Object)
		soundsMap[name] = waveAsset.Object;
	}
}

void AYetrixGameModeBase::BeginPlay() {

	const auto* world = GetWorld();
	if (!world)
		return;

	auto* pawn = world->GetFirstPlayerController()->GetPawn();
	auto* yetrixPawn = dynamic_cast<AYetrixPawn*>(pawn);
	yetrixPawn->SetGameMode(this);

	Reset();
	Load();
}

void AYetrixGameModeBase::OnStartDestroying() {
}

void AYetrixGameModeBase::OnStartDropping() {

	statePtr->blockScenePtr->DeconstructFigures();
	const auto& linesToDestruct = CheckDestruction();

	if (!linesToDestruct.empty()) {

		statePtr->fallingPositions = statePtr->blockScenePtr->GetFallingPositions(linesToDestruct);
		statePtr->currDropState = DropState::DESTROYING;
		statePtr->dropStateTimer = statePtr->destroyStateDuration;	
		OnStartDestroying();

		const auto howManyLines = linesToDestruct.size();

		const int prevHundreds = statePtr->score / 100;

		//assert(howManyLines <= 4);
		AddScore(scorePerCombo[howManyLines - 1]);

		const int nowHundreds = statePtr->score / 100;

		if (nowHundreds > prevHundreds) {

			FTimerHandle TimerHandle;
			constexpr float sayYeahAfterSeconds = 1.f;
			const auto* world = GetWorld();

			if (world)
			{
				world->GetTimerManager().SetTimer(TimerHandle, [&]()
				{
					PlaySound("yeah");
				}, sayYeahAfterSeconds, false);
			}			
		}

		statePtr->lightAngleStart = statePtr->lightAngleCurrent;
		statePtr->lightAngleEnd += lightZRotationAddPerExplosion;
		statePtr->sunMoveFinishTimer = sunMoveDuration;
	}
	
	CheckAddFigures();
}

void AYetrixGameModeBase::UpdateSpeed()
{
	const float speedMultiplier = std::powf(speedUpCoeff, statePtr->score / 10.f);

	statePtr->stillStateDuration = stillStateInitialDuration * speedMultiplier;
	statePtr->dropStateDuration = dropStateInitialDuration * speedMultiplier;
}

void AYetrixGameModeBase::AddScore(const int score) {

	statePtr->score += score;
	RequestUpdateScoreUI();
	UpdateSpeed();
}

void AYetrixGameModeBase::UpdateSunMove(const float dt) {

	if (statePtr->sunMoveFinishTimer <= 0.f) {
		statePtr->sunMoveFinishTimer = 0.f;
		return;
	}

	statePtr->sunMoveFinishTimer -= dt;

	if (statePtr->sunMoveFinishTimer <= 0.f) {
		statePtr->sunMoveFinishTimer = 0.f;
	}

	const float progress = 1.f - statePtr->sunMoveFinishTimer / sunMoveDuration;
	const float currAngle = statePtr->lightAngleStart + (statePtr->lightAngleEnd - statePtr->lightAngleStart) * progress;

	UpdateSunlight(currAngle);
}

void AYetrixGameModeBase::UpdateSunlight(const float angle) {

	statePtr->lightAngleCurrent = angle;

	TArray<AActor*> sunlightActors;
	UGameplayStatics::GetAllActorsWithTag(GetWorld(), "Sunlight", sunlightActors);

	if (sunlightActors.IsEmpty())
	{
		//error
		return;
	}

	const auto sunlightActor = sunlightActors[0];
	auto rotation = sunlightActor->GetActorRotation();
	rotation.Yaw = angle;
	sunlightActor->SetActorRotation(rotation);
}

void AYetrixGameModeBase::UpdateScoreUI()
{
	if (needUpdateScoreUI > 0)
		needUpdateScoreUI--;

	const auto* world = GetWorld();
	if (!world)
		return;

	AYetrixHUDBase* hud = Cast<AYetrixHUDBase> (world->GetFirstPlayerController()->GetHUD());
	if (!hud)
		return;

	hud->UpdateScore(statePtr->score, hiScore);
}

void AYetrixGameModeBase::CheckAddFigures() {

	if (statePtr->blockScenePtr->GetFigures().size() >= minFigures)
		return;

	const bool added = statePtr->blockScenePtr->CreateRandomFigureAt({ newFigureX, newFigureY }, GetWorld());
	if (!added) {
		// Game over

		if (statePtr->score > hiScore)
			hiScore = statePtr->score;

		Reset();
		Save();
		PlaySound("gameover");
	}	
}

void AYetrixGameModeBase::Left() {
	
	statePtr->leftPending++;
}

void AYetrixGameModeBase::Right() {

	statePtr->rightPending++;
}

void AYetrixGameModeBase::Rotate() {

	statePtr->rotatePending++;
}

void AYetrixGameModeBase::Drop() {
	statePtr->quickDropRequested = true;
	statePtr->dropStateTimer = 0.f;

	PlaySoundWithRandomIndex("bdysh", 3);
}

void AYetrixGameModeBase::Down() {
	if (statePtr->currDropState == DropState::STILL)
	{
		PlaySound("k2");
		statePtr->dropStateTimer = 0.f;
	}
}

std::set<int> AYetrixGameModeBase::CheckDestruction() {

	std::set<int> linesToBoom;

	for (int y = 1; y < checkHeight; ++y) {
		bool hasHoles = false;
		for (int x = 1; x < rightBorderX; ++x)
		{
			Vec2D checkPos(x, y);
			const auto blockPtr = statePtr->blockScenePtr->GetBlock(checkPos, true);
			if (!blockPtr || !blockPtr->IsAlive() || blockPtr->GetFigureID() != Utils::emptyID) {
				hasHoles = true;
				break;
			}				
		}

		if (!hasHoles) {
			linesToBoom.insert(y);
		}
	}

	if (!linesToBoom.empty()) {
		const auto linesCount = linesToBoom.size();
		

		if (linesCount == 1) {
			PlaySoundWithRandomIndex("bah1", 4);
		}
		else {
			const std::string sound = "bah" + std::to_string(linesCount) + "0";
			PlaySound(sound);
		}
	}

	for (int y : linesToBoom) {

		for (int x = 1; x < rightBorderX; ++x)
		{
			Vec2D blockPos(x, y);
			const auto blockPtr = statePtr->blockScenePtr->GetBlock(blockPos, true);

			blockPtr->StartDestroy();

			TArray<UActorComponent*> components;
			blockPtr->GetActor()->GetComponents(components);
			auto* geometryComponent = Cast<UGeometryCollectionComponent>(components[2]);
			if (geometryComponent)
				geometryComponent->SetSimulatePhysics(true);
		}
	}

	return linesToBoom;
}

void AYetrixGameModeBase::FinalizeLogicalDestroy() {

	for (const auto& fallingBlockInfo : statePtr->fallingPositions) {

		auto blockPtr = statePtr->blockScenePtr->GetBlock(fallingBlockInfo.first);
		blockPtr->SetPosition(fallingBlockInfo.second);
		blockPtr->UpdateActorPosition();
	}
	
	statePtr->fallingPositions.clear();
}

void AYetrixGameModeBase::FinalizeLogicalDrop() {

	const auto lowestFigID = statePtr->blockScenePtr->GetLowestFigureID();

	const auto& figures = statePtr->blockScenePtr->GetFigures();
	for (const auto& [figID, fig] : figures) {
		
		unsigned maxHeight = 0;
		const bool canDrop = statePtr->blockScenePtr->CheckFigureCanMove(fig, {0, -1}, maxHeight);
		if (!canDrop)
			continue;

		const bool isLowestOne = figID == lowestFigID;
		const bool canQuickDrop = isLowestOne && statePtr->quickDropRequested; 
		const int heightToDrop = canQuickDrop ? maxHeight : 1;
		
		const auto& blockIDs = fig->GetBlockIDs();
		
		for (const auto blockID : blockIDs) {
			const auto block = statePtr->blockScenePtr->GetBlock(blockID);
			const auto& logicalPos = block->GetPosition();
			auto dropLogicalPos = logicalPos;
			dropLogicalPos.y -= heightToDrop;

			block->SetPosition(dropLogicalPos);
		}
	}

	statePtr->quickDropRequested = false;
}

void AYetrixGameModeBase::OnStopDestroying() {
	UpdateVisualDestroy(1.f);
	FinalizeLogicalDestroy();

	Save();
}

void AYetrixGameModeBase::OnStopDropping() {

	UpdateVisualDrop(1.f);
	FinalizeLogicalDrop();
}

void AYetrixGameModeBase::Save()
{
	json doc;
	doc["blockScene"] = statePtr->blockScenePtr->Save();
	doc["score"] = statePtr->score;
	doc["hiscore"] = hiScore;

	auto* saveGame = Cast<UYetrixSaveGame>(UGameplayStatics::CreateSaveGameObject(UYetrixSaveGame::StaticClass()));

	const auto dumpStr = doc.dump();
	saveGame->jsonDump = dumpStr.c_str();

	UGameplayStatics::SaveGameToSlot(saveGame, "0", 0);
}

bool AYetrixGameModeBase::Load()
{
	const auto* saveGame = Cast<UYetrixSaveGame>(UGameplayStatics::LoadGameFromSlot("0", 0));
	if (saveGame == nullptr)
	{
		return false;
	}

	const std::string dataStr(TCHAR_TO_UTF8(*saveGame->jsonDump));
	json doc = json::parse(dataStr);

	statePtr->blockScenePtr->Load(doc["blockScene"], GetWorld());
	statePtr->score = doc["score"].get<int>();
	hiScore = doc["hiscore"].get<int>();

	RequestUpdateScoreUI();
	UpdateSpeed();

	return true;
}

bool AYetrixGameModeBase::CheckChangeDropState() {

	if (statePtr->dropStateTimer >= 0.f)
		return false;

	if (statePtr->currDropState == DropState::STILL) {
		statePtr->currDropState = DropState::DROPPING;
		statePtr->dropStateTimer = statePtr->dropStateDuration;	
		OnStartDropping();
	}
	else if (statePtr->currDropState == DropState::DROPPING) {
		statePtr->currDropState = DropState::STILL;
		statePtr->dropStateTimer = statePtr->stillStateDuration;
		OnStopDropping();
	}
	else if (statePtr->currDropState == DropState::DESTROYING) {
		statePtr->currDropState = DropState::STILL;
		statePtr->dropStateTimer = statePtr->stillStateDuration;
		OnStopDestroying();
	}

	return true;
}

void AYetrixGameModeBase::RequestUpdateScoreUI()
{
	needUpdateScoreUI++;
}

void AYetrixGameModeBase::Reset() {

	statePtr = std::make_unique<State>();
	RequestUpdateScoreUI();
	UpdateSunlight(lightZRotationInit);
}

void AYetrixGameModeBase::UpdateVisualDestroy(const float progress) {

	for (const auto& fallingPosPair : statePtr->fallingPositions)
	{
		auto block = statePtr->blockScenePtr->GetBlock(fallingPosPair.first);

		const auto startLogicalPos = block->GetPosition();
		const auto& endLogicalPos = fallingPosPair.second;

		const auto blockWorldPos = GameBlock::ToWorldPosition(startLogicalPos);
		const auto dropWorldPos = GameBlock::ToWorldPosition(endLogicalPos);

		const auto dropWorldPosNormalY = dropWorldPos.Y;
		auto dropPosIntermediateY = dropWorldPosNormalY;

		const auto dYFull = destroyYShift - dropWorldPosNormalY;

		constexpr float progressReturnStart = 0.5f;

		if (progress < progressReturnStart) {
			dropPosIntermediateY = dropWorldPosNormalY + dYFull * (progress / progressReturnStart);
		}
		else {
			const float returnProgress = (progress - progressReturnStart) / (1.f - progressReturnStart);
			dropPosIntermediateY = dropWorldPosNormalY + dYFull * (1.f - returnProgress);
		}

		const auto posDiff = dropWorldPos - blockWorldPos;
		const auto dPosCurr = posDiff * progress;

		auto dropIntermediateWorldPos = blockWorldPos + dPosCurr;
		dropIntermediateWorldPos.Y = dropPosIntermediateY;

		block->GetActor()->SetActorLocation(dropIntermediateWorldPos);
	}
}

void AYetrixGameModeBase::UpdateVisualDrop(const float progress) {

	const auto lowestFigID = statePtr->blockScenePtr->GetLowestFigureID();

	const auto& figures = statePtr->blockScenePtr->GetFigures();
	for (const auto& [figID, figPtr] : figures) {
		
		unsigned maxHeight = 0;
		const bool canDrop = statePtr->blockScenePtr->CheckFigureCanMove(figPtr, {0, -1}, maxHeight);
		if (!canDrop)
			continue;

		const bool isLowestOne = figID == lowestFigID;
		const bool canQuickDrop = isLowestOne && statePtr->quickDropRequested; 
		const int heightToDrop = canQuickDrop ? maxHeight : 1;
		
		const auto& blockIDs = figPtr->GetBlockIDs();
		
		for (const auto blockID : blockIDs) {
			const auto block = statePtr->blockScenePtr->GetBlock(blockID);
			const auto logicalPos = block->GetPosition();
			auto dropLogicalPos = logicalPos;
			dropLogicalPos.y -= heightToDrop;

			const auto blockWorldPos = GameBlock::ToWorldPosition(logicalPos);
			const auto dropWorldPos = GameBlock::ToWorldPosition(dropLogicalPos);
			const auto posDiff = dropWorldPos - blockWorldPos;
			const auto dPosCurr = posDiff * progress;

			const auto dropIntermediateWorldPos = blockWorldPos + dPosCurr;

			block->GetActor()->SetActorLocation(dropIntermediateWorldPos);		
		}
	}
}

void AYetrixGameModeBase::SimulationTick(float dt) {

	UpdateSunMove(dt);

	statePtr->dropStateTimer -= dt;
	CheckChangeDropState();

	if (statePtr->currDropState == DropState::DESTROYING) {

		const float dropProgress = 1.f - (statePtr->dropStateTimer / statePtr->destroyStateDuration);
		UpdateVisualDestroy(dropProgress);

	}
	else if (statePtr->currDropState == DropState::DROPPING) {
		const float dropProgress = 1.f - (statePtr->dropStateTimer / statePtr->dropStateDuration);
		UpdateVisualDrop(dropProgress);
	} else {
	
		while (statePtr->leftPending)
		{
			statePtr->leftPending--;

			const bool moveOk = statePtr->blockScenePtr->MoveBlock({-1, 0});
			if (moveOk)
				PlaySound("k0");
			else
				break;
		}
		while (statePtr->rightPending) {

			
			statePtr->rightPending--;
			const bool moveOk = statePtr->blockScenePtr->MoveBlock({1, 0});
			if (moveOk)
				PlaySound("k1");
			else
				break;
		}

		while (statePtr->rotatePending)
		{
			statePtr->rotatePending--;

			const auto lowestFigID = statePtr->blockScenePtr->GetLowestFigureID();
			if (lowestFigID != Utils::emptyID) {
				const auto& figure = statePtr->blockScenePtr->GetFigures().at(lowestFigID);

				if (figure->GetType() == Figure::FigType::BOX)
					continue;
					// no need to rotate box figure

				const bool rotated = statePtr->blockScenePtr->TryRotate(figure);
				if (rotated)
					PlaySound("k2");
			}
		}
	}

	statePtr->blockScenePtr->CleanupBlocks(dt);
}

void AYetrixGameModeBase::Tick(float dt) {

	dtAccum += dt;
	while (dtAccum >= simulationUpdateInterval)
	{
		dtAccum -= simulationUpdateInterval;
		SimulationTick(simulationUpdateInterval);
	}

	if (needUpdateScoreUI > 0)
		UpdateScoreUI();
}