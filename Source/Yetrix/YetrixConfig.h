#pragma once

#include <array>

constexpr float blockSize = 100.f;
constexpr float destroyActorAfter = 3.f;
constexpr float simulationUpdateInterval = 0.01f;

constexpr float speedUpCoeff = 0.95f;

constexpr float destroyYShift = 300.f;

constexpr int checkHeight = 20;

constexpr int newFigureX = 5;
constexpr int newFigureY = 25;

constexpr int rightBorderX = 11;

constexpr float lightZRotationInit = -10.f;
constexpr float lightZRotationAddPerExplosion = 15.f;

constexpr float sunMoveDuration = 1.f;

constexpr unsigned minFigures = 1;

static const std::array<int, 4> scorePerCombo = {10, 25, 40, 60};

constexpr float stillStateInitialDuration = 0.5f;
constexpr float dropStateInitialDuration = 0.1f;
constexpr float destroyingStateInitialDuration = 0.5f;

constexpr int scoreYeahInterval = 100;