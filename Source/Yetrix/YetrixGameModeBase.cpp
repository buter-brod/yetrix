#include "YetrixGameModeBase.h"
#include "YetrixPlayerController.h"
#include "YetrixPawn.h"

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

	const auto soundIt = soundsMap.find(what);

	if (soundIt == soundsMap.end())
		return false;

	UGameplayStatics::PlaySound2D(this, soundIt->second);
	return true;
}

void AYetrixGameModeBase::InitSounds() {

	static const std::set<std::string> soundNames = {
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

		const ConstructorHelpers::FObjectFinder<USoundWave> waveAsset(assetPath.c_str());
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

	ResetGame();
	Load();
}

void AYetrixGameModeBase::OnStartDestroying() {
}

void AYetrixGameModeBase::HandleDestruction()
{
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
}

void AYetrixGameModeBase::OnStartDropping() {

	statePtr->blockScenePtr->DeconstructFigures();
	HandleDestruction();

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

			block->SetPositionAndUpdateActor(dropLogicalPos, statePtr->dropStateDuration);
		}
	}

	if (statePtr->quickDropRequested)
	{
		statePtr->quickDropRequested = false;
		PlaySoundWithRandomIndex("bdysh", 3);
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

void AYetrixGameModeBase::GameOver()
{
	if (statePtr->score > hiScore)
		hiScore = statePtr->score;

	ResetGame();
	Save();
	PlaySound("gameover");
}

void AYetrixGameModeBase::CheckAddFigures() {

	if (statePtr->blockScenePtr->GetFigures().size() >= minFigures)
		return;

	const bool added = statePtr->blockScenePtr->CreateRandomFigureAt({ newFigureX, newFigureY }, GetWorld());
	if (!added) {
		GameOver();
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
		}
	}

	return linesToBoom;
}

void AYetrixGameModeBase::FinalizeLogicalDestroy() {

	for (const auto& fallingBlockInfo : statePtr->fallingPositions) {

		const auto blockPtr = statePtr->blockScenePtr->GetBlock(fallingBlockInfo.first);
		blockPtr->SetPositionAndUpdateActor(fallingBlockInfo.second);
	}
	
	statePtr->fallingPositions.clear();
}

void AYetrixGameModeBase::OnStopDestroying() {
	UpdateVisualDestroy(1.f);
	FinalizeLogicalDestroy();

	Save();
}

void AYetrixGameModeBase::OnStopDropping() {
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

	if (statePtr->currDropState == DropState::ROTATING) {
		statePtr->currDropState = DropState::STILL;
		statePtr->dropStateTimer = statePtr->stillStateDuration;
	}

	if (statePtr->currDropState == DropState::STILL) {
		statePtr->currDropState = DropState::DROPPING;
		statePtr->dropStateTimer = statePtr->dropStateDuration;	
		OnStartDropping();
	}
	else if (statePtr->currDropState == DropState::DROPPING) {
		statePtr->currDropState = DropState::STILL;
		statePtr->dropStateTimer = statePtr->stillStateDuration;

		if (statePtr->quickDropRequested)
			statePtr->dropStateTimer = 0.f;
			// should drop quickly, don't wait in STILL state

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

void AYetrixGameModeBase::ResetGame() {

	statePtr = std::make_unique<State>();
	RequestUpdateScoreUI();
	UpdateSunlight(lightZRotationInit);
}

void AYetrixGameModeBase::UpdateVisualDestroy(const float progress) {

	for (const auto& [blockID, endLogicalPos] : statePtr->fallingPositions)
	{
		const auto block = statePtr->blockScenePtr->GetBlock(blockID);
		const auto startLogicalPos = block->GetPosition();

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

		block->SetActorLocation(dropIntermediateWorldPos);
	}
}

bool AYetrixGameModeBase::TryRotate()
{
	const auto lowestFigID = statePtr->blockScenePtr->GetLowestFigureID();

	if (lowestFigID == Utils::emptyID)
		return false;
		// nothing to rotate

	const auto& figure = statePtr->blockScenePtr->GetFigures().at(lowestFigID);
	rotatedPositions = statePtr->blockScenePtr->GetRotatedPositions(figure);
	const bool canRotate = !rotatedPositions.empty();
	if (!canRotate)
		return false;
	
	PlaySound("k2");

	statePtr->currDropState = DropState::ROTATING;
	statePtr->dropStateTimer = rotateStateInitialDuration;
	statePtr->currRotateState = RotateSubState::BREAK_1;

	const auto& blockIDs = figure->GetBlockIDs();

	std::vector<float> depthOffsets;

	constexpr float depthOffsetMultiplier = 2.f;

	const float depthOffsetCompensation = (blockIDs.size() / 2) * blockSize * depthOffsetMultiplier;

	// pivot block should not change its Z
	depthOffsets.push_back(0.f);

	for (int i = 1; i < blockIDs.size(); ++i)
		depthOffsets.push_back(i * blockSize * depthOffsetMultiplier - depthOffsetCompensation);

	int depthOffsetInd = 0;
	for (const auto blockID : blockIDs)
	{
		const auto blockPtr = statePtr->blockScenePtr->GetBlock(blockID);

		const auto& worldPos = blockPtr->GetActorLocation();
		auto newPos = worldPos;
		newPos.Y = depthOffsets.at(depthOffsetInd);

		blockPtr->StartAnimatedMove(rotate1StageDuration, newPos);
		blockPtr->SetPosition(rotatedPositions.at(blockID));

		statePtr->currRotateState = RotateSubState::BREAK_1;
		++depthOffsetInd;
	}

	return true;
}

void AYetrixGameModeBase::HandleRotateAnimation()
{
	const float elapsed = rotateStateInitialDuration - statePtr->dropStateTimer;
	if (elapsed < rotate1StageDuration)
	{
		// visual 'break' in progress, no actions needed
		return;
	}

	const auto lowestFigID = statePtr->blockScenePtr->GetLowestFigureID();
	const auto& figure = statePtr->blockScenePtr->GetFigures().at(lowestFigID);
	const auto& blockIDs = figure->GetBlockIDs();
	
	if (statePtr->currRotateState == RotateSubState::BREAK_1 && elapsed < rotate1StageDuration + rotate2StageDuration)
	{
		// moving to rotated positions, but still in modified 'depth'-planes
		for (const auto blockID : blockIDs)
		{
			const auto blockPtr = statePtr->blockScenePtr->GetBlock(blockID);
			const auto& worldPos = blockPtr->GetActorLocation();
			auto newPos = GameBlock::ToWorldPosition(rotatedPositions.at(blockID));

			// change only XZ plane, depth (Y) should still be alterated
			newPos.Y = worldPos.Y;	

			blockPtr->StartAnimatedMove(rotate2StageDuration, newPos);
		}

		statePtr->currRotateState = RotateSubState::MOVE_2;
	}
	else if (statePtr->currRotateState == RotateSubState::MOVE_2 && elapsed > rotate1StageDuration + rotate2StageDuration)
	{
		for (const auto blockID : blockIDs)
		{
			const auto blockPtr = statePtr->blockScenePtr->GetBlock(blockID);
			const auto& worldPos = blockPtr->GetActorLocation();
			auto newPos = worldPos;
			newPos.Y = 0.f;
			blockPtr->StartAnimatedMove(rotate3StageDuration, newPos);
		}

		statePtr->currRotateState = RotateSubState::ASSEMBLE_3;
	}
}

void AYetrixGameModeBase::HandlePlayerPendingInput()
{
	while (statePtr->leftPending)
	{
		statePtr->leftPending--;

		const bool moveOk = statePtr->blockScenePtr->TryMoveBlock({-1, 0});
		if (moveOk)
			PlaySound("k0");
		else
			break;
	}
	while (statePtr->rightPending) {

		statePtr->rightPending--;
		const bool moveOk = statePtr->blockScenePtr->TryMoveBlock({1, 0});
		if (moveOk)
			PlaySound("k1");
		else
			break;
	}

	while (statePtr->rotatePending)
	{
		statePtr->rotatePending--;

		const bool rotated = TryRotate();
		if (!rotated) 
			break;
	}
}

void AYetrixGameModeBase::SimulationTick(float dt) {

	UpdateSunMove(dt);

	statePtr->dropStateTimer -= dt;
	CheckChangeDropState();

	if (statePtr->currDropState == DropState::ROTATING) {

		// handling rotating substate
		HandleRotateAnimation();
	}

	if (statePtr->currDropState == DropState::DESTROYING) {

		const float dropProgress = 1.f - (statePtr->dropStateTimer / statePtr->destroyStateDuration);
		UpdateVisualDestroy(dropProgress);
	}
	else if (statePtr->currDropState == DropState::DROPPING) {

		// no action required

	} else {

		//STILL or ROTATING	
		HandlePlayerPendingInput();
	}

	statePtr->blockScenePtr->Tick(dt);
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