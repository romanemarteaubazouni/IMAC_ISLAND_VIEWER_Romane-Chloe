#include "generation.hpp"

#include "noise.hpp"
#include "raylib.h"

#include "utils/raylibUtils.hpp"
#include <algorithm> // for std::clamp

#include <cstdlib>
#include <ctime>

#include "generation.hpp"

#include "noise.hpp"
#include "raylib.h"

#include "utils/raylibUtils.hpp"
#include <algorithm> // for std::clamp

#include <cstdlib>
#include <ctime>

std::vector<glm::vec2> generate2DPositions([[maybe_unused]] PointsGenerationParameters const& params) {
    std::srand(std::time(nullptr));

    float width = 1;
    float height = 1;
    float r = params.radius;
    int k = params.samples_before_rejection;

    // Step 0 : initializing
    float w = r / sqrt(2);
    int columns = width / w;
    int rows = height / w;

    std::vector<glm::vec2> grid(rows * columns, glm::vec2(-1.0f, -1.0f));

    // Step 1 : choosing random starting point between 0.0 and 1.0
    float x = (static_cast<float>(std::rand()) / RAND_MAX) * width;
    float y = (static_cast<float>(std::rand()) / RAND_MAX) * height;
    int startCol = static_cast<int>(x / w);
    int startRow = static_cast<int>(y / w);
    glm::vec2 startPos = {x, y};
    grid[startCol + startRow * columns] = startPos;

    // Step 2 : creating active list
    std::vector<glm::vec2> active {};
    active.push_back(startPos);

    while (!active.empty()) {
        int randIndex = std::rand() % active.size();
        glm::vec2 position = active[randIndex];
        bool found = false;
        
        for (int n {}; n < k ; n++) {
            float angle = (static_cast<float>(std::rand()) / RAND_MAX) * 2.0f * M_PI;
            float m = (static_cast<float>(std::rand()) / RAND_MAX) * r + r;
            float offsetX = cos(angle);
            float offsetY = sin(angle);
            glm::vec2 offset = position + glm::vec2(offsetX, offsetY) * m;
            if (offset.x >= 0 && offset.x < width && offset.y >= 0 && offset.y < height) {
                int col = offset.x / w;
                int row = offset.y / w;

                bool ok = true;

                for (int i {-1}; i <= 1; i++)  {
                    for (int j {-1}; j <= 1; j++) {
                        int index = (col + i) + (row + j) * columns;
                        glm::vec2 neighbor = grid[index];
                        if (neighbor != glm::vec2(-1, -1)) {
                            float d = glm::distance(offset, neighbor);

                            if (d < r) {
                                ok = false;
                            }
                        }
                    }
                }

                if (ok) {
                    found = true;
                    grid[col + row*columns] = offset;
                    active.push_back(offset);
                    break;
                }
            }
        }
        
        if (!found) {
            active.erase(active.begin() + randIndex);
        }
    }
    return grid;

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