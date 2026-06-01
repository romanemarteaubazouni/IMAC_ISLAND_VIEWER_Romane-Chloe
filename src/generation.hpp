#pragma once

#include "app.hpp"
bool IsValid(glm::vec2 candidate, glm::vec2 sampleRegionSize, float cellSize, float radius, const std::vector<glm::vec2>& points, const std::vector<std::vector<int>>& grid);

std::vector<glm::vec2> generate2DPositions(PointsGenerationParameters const& params);

void generateObjectsPositions(AppContext& context);

float sampleHeightmap(AppContext const& context, float u, float v);

void generateHeightmap(AppContext& context);