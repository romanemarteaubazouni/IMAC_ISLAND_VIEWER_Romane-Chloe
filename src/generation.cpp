#include "generation.hpp"

#include "noise.hpp"
#include "raylib.h"

#include "utils/raylibUtils.hpp"
#include <algorithm> // for std::clamp

#include <cstdlib>
#include <ctime>

bool IsValid(glm::vec2 candidate, glm::vec2 sampleRegionSize, float cellSize, float radius, const std::vector<glm::vec2>& points, const std::vector<std::vector<int>>& grid) {
    if (candidate.x >= 0 && candidate.x < sampleRegionSize.x
        && candidate.y >=0 && candidate.y < sampleRegionSize.y) {
            int cellX = static_cast<int>(candidate.x/cellSize);
            int cellY = static_cast<int>(candidate.y/cellSize);
            int searchStartX = std::max(0, cellX - 2); 
            int searchEndX = std::min(cellX + 2, static_cast<int>(grid.size()) - 1);
            int searchStartY = std::max(0, cellY - 2); 
            int searchEndY = std::min(cellY + 2, static_cast<int>(grid[0].size()) - 1);

            for (int x {searchStartX}; x <= searchEndX; x++) {
                for (int y {searchStartY}; y <= searchEndY; y++) {
                    int pointIndex = grid[x][y] - 1;
                    if (pointIndex != -1) {
                        float dist = glm::length(candidate - points[pointIndex]);
                        if (dist < radius) {
                            return false;
                        }
                    }
                }
            }
            return true;
        }
    return false;
}

std::vector<glm::vec2> generate2DPositions([[maybe_unused]] PointsGenerationParameters const& params) {
    std::srand(std::time(nullptr));
    
    float cellSize = params.radius / std::sqrt(2.f);
    // The std::ceil function in C++ is used to compute the smallest integer value that is greater than or equal to a given floating-point number
    // View code Unity
    int gridWidth = static_cast<int>(std::ceil(params.sample_region_size.x / cellSize));
    int gridHeight = static_cast<int>(std::ceil(params.sample_region_size.y / cellSize));
    // Grid 2D initialized with 0s
    std::vector<std::vector<int>> grid(gridWidth, std::vector<int>(gridHeight, 0));

    std::vector<glm::vec2> points {};
    std::vector<glm::vec2> spawnPoints {};

    spawnPoints.push_back({params.sample_region_size.x / 2, params.sample_region_size.y / 2});

    while (!spawnPoints.empty() && points.size() < params.nb_of_points_max) {
        int spawnIndex = std::rand() % spawnPoints.size();
        glm::vec2 spawnCentre = spawnPoints[spawnIndex];
        bool candidateAccepted = false;
        for (int i {0}; i < params.samples_before_rejection; i++) {
            float angle = (static_cast<float>(std::rand()) / RAND_MAX) * 2.0f * M_PI;
            glm::vec2 dir(std::cos(angle), std::sin(angle));
            glm::vec2 candidate(spawnCentre + dir * (params.radius + (static_cast<float>(std::rand()) / RAND_MAX) * params.radius));

            if (IsValid(candidate, params.sample_region_size, cellSize, params.radius, points, grid)) {
                points.push_back(candidate);
                spawnPoints.push_back(candidate);
                grid[candidate.x/cellSize][candidate.y/cellSize] = points.size();
                candidateAccepted = true;
                break;
            }
        }
        if (!candidateAccepted) {
            spawnPoints.erase(spawnPoints.begin() + spawnIndex);
        }
    }
    return points;
}

void generateObjectsPositions(AppContext& context) {
    std::vector<glm::vec2> const positions {generate2DPositions(context.pointsGenerationParameters)};

    context.objectPositions.clear();
    context.objectPositions.reserve(positions.size());
    for (glm::vec2 const& p : positions)
    {
        context.objectPositions.emplace_back(
            p.x, // x
            p.y, // y
            // sample height from heightmap for each point (asuming positions are normalized in [0..1] range)
            sampleHeightmap(context, p.x, p.y)
        );
    }
    // TODO(student): extension - filter positions by sampled height range.
}

float sampleHeightmap(AppContext const& context, float u, float v)
{
    if (!context.heightmapImage.data || context.heightmapImage.width <= 0 || context.heightmapImage.height <= 0) return 0.0f;

    int const px = std::clamp(static_cast<int>(u * static_cast<float>(context.heightmapImage.width - 1)), 0, context.heightmapImage.width - 1);
    int const py = std::clamp(static_cast<int>(v * static_cast<float>(context.heightmapImage.height - 1)), 0, context.heightmapImage.height - 1);

    // If the heightmap is in R32 format, we can directly read the height value as a float. 
    if (context.heightmapImage.format == PIXELFORMAT_UNCOMPRESSED_R32)
    {
        float const* heightData = static_cast<float const*>(context.heightmapImage.data);
        int const idx = py * context.heightmapImage.width + px;
        return std::clamp(heightData[idx], 0.0f, 1.0f);
    }

    // Otherwise, we assume it's in a color format and we read the red channel as height (with normalization from [0..255] to [0..1]).
    Color const c = GetImageColor(context.heightmapImage, px, py);
    return static_cast<float>(c.r)/255.0f;
}

void generateHeightmap(AppContext& context) {

    if (context.texture.id > 0) {
        UnloadTexture(context.texture);
        context.texture = {};
    }

    if(context.image.data) {
        UnloadImage(context.image);
        context.image = {};
    }

    if (context.heightmapImage.data) {
        UnloadImage(context.heightmapImage);
        context.heightmapImage = {};
    }

    int const resolution = std::max(1, context.imageGenerationParameters.resolution);

    context.heightmapImage = GenImageFromNoiseFunction<float>(resolution, resolution, PIXELFORMAT_UNCOMPRESSED_R32,
        [&](glm::vec2 const& p)->float {
            // TODO(student): implement stack based noise and island mask

            return (perlinNoiseSeeded(p * context.imageGenerationParameters.noiseScale, context.imageGenerationParameters.noiseSeed) * 0.5f + 0.5f);
        });

    // exemple conversion from heightmap to color image
    context.image = TransformImage<float, Color>(context.heightmapImage, [&](float const& v, int const, int const) {
        if (v < 0.3f)
        {
            return color_from({ 70, 130, 180 }); // water
        }
        else if (v < 0.5f)
        {
            return color_from({ 238, 214, 175 }); // sand
        }
        else
        {
            return color_from({ 34, 139, 34 }); // grass
        }
        
    }, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);

    context.texture = LoadTextureFromImage(context.image);
    if (context.model.meshCount > 0) {
        context.model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = context.texture;
    }
}