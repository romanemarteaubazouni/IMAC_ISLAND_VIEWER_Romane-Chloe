#include "draw.hpp"

#include "app.hpp"

#include "generation.hpp"

#include "imgui.h"
#include "raylib.h"
#include "raymath.h"

void draw3DScene(AppContext& context) {
    ClearBackground(RAYWHITE);
    
    BeginMode3D(context.camera);

    Matrix const terrainCentering { getTerrainCenteringMatrix(context) };
    Vector3 const terrainCenterOffset { terrainCentering.m12, terrainCentering.m13, terrainCentering.m14 };

    DrawModel(context.model, terrainCenterOffset, 1.0f, WHITE);
    // drawCubes(context, terrainCentering);
    drawTrees(context, terrainCentering);
    DrawGrid(20, 1.0f);

    EndMode3D();
}

void drawTrees(AppContext const& context, Matrix const& terrainCentering)
{
    if (context.objectPositions.empty()) {
        return;
    }

    float angle {0.f};

    for (glm::vec3 const& pos : context.objectPositions) {
        Vector3 modelPos = {
            pos.x * context.terrainSize.x,
            pos.z * context.terrainSize.y,
            pos.y * context.terrainSize.z
        };
        Vector3 position = Vector3Transform(modelPos, terrainCentering);
        // Régions des sapins
        if (position.y > context.pointsGenerationParameters.separation_of_trees) {
            if (!context.pointsGenerationParameters.noChristmasTree) {
                DrawModelEx(context.christmasTree, position, {0, 1, 0}, 0.0f, {0.04f, context.pointsGenerationParameters.christmasTreeHeight, 0.04f}, WHITE);
            }
        }
        // Régions des arbres normaux
        else {
            if (!context.pointsGenerationParameters.noNormalTree) {
                DrawModelEx(context.normalTree, position, {0, 1, 0}, angle, {0.04f, context.pointsGenerationParameters.normalTreeHeight, 0.04f},  WHITE);
                angle += 20.f;
            }
        }
    }
}

void drawCubes(AppContext const& context, Matrix const& terrainCentering)
{
    if (context.objectPositions.empty()) {
        return;
    }

    float const cubeHalfHeight { 0.5f * context.cubeScale };

    for (glm::vec3 const& pos : context.objectPositions) {
        Matrix const objectTranslation { MatrixTranslate(
            pos.x * context.terrainSize.x,
            pos.z * context.terrainSize.y + cubeHalfHeight,
            pos.y * context.terrainSize.z
        )};
        Matrix const centeredTranslation { MatrixMultiply(objectTranslation, terrainCentering) };
        Matrix const scale { MatrixScale(context.cubeScale, context.cubeScale, context.cubeScale) };
        Matrix const transform { MatrixMultiply(scale, centeredTranslation) };
        DrawMesh(context.cube, context.cubeMaterial, transform);
    }
}

void drawImGui(AppContext& context) {
    if(ImGui::Button("Generate random positions")) {
        generateObjectsPositions(context);
    }

    if(ImGui::Button("Remove Christmas Trees")) {
        context.pointsGenerationParameters.noChristmasTree = !context.pointsGenerationParameters.noChristmasTree;
    }

    if(ImGui::Button("Remove Normal Trees")) {
        context.pointsGenerationParameters.noNormalTree = !context.pointsGenerationParameters.noNormalTree;
    }

    if (ImGui::CollapsingHeader("objects", ImGuiTreeNodeFlags_DefaultOpen)) {
        // ImGui::SliderFloat("Cube Scale", &context.cubeScale, 0.01f, 1.0f);
        ImGui::SliderFloat("Christmas Trees Scale", &context.pointsGenerationParameters.christmasTreeHeight, 0.03f, 0.1f);
        ImGui::SliderFloat("Normal Trees Scale", &context.pointsGenerationParameters.normalTreeHeight, 0.03f, 0.06f);
        ImGui::SliderFloat("Separation of trees", &context.pointsGenerationParameters.separation_of_trees, 0.01f, 1.0f);
    }

    if (ImGui::CollapsingHeader("placement", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::SliderFloat("Radius", &context.pointsGenerationParameters.radius, 0.01f, 0.5f);
        ImGui::SliderInt("Samples before rejection", &context.pointsGenerationParameters.samples_before_rejection, 10, 100);
        ImGui::SliderInt("Max nb of objects", &context.pointsGenerationParameters.nb_of_points_max, 10, 1500);
        ImGui::SliderFloat("Minimum z", &context.pointsGenerationParameters.minimum_z, -0.1f, 1.f);
        ImGui::SliderFloat("Maximum z", &context.pointsGenerationParameters.maximum_z, 0.f, 1.f);
    }
}

void drawRaylibUI(AppContext& context) {
    int screenWidth { GetScreenWidth() };
    
    float wanted_size { 400.f };
    float scale_factor { wanted_size / std::max(context.texture.width, context.texture.height) };
    float const preview_x { screenWidth - wanted_size - 20.f };
    float const preview_y { 20.f };
    float const preview_w { context.texture.width * scale_factor };
    float const preview_h { context.texture.height * scale_factor };
    // DrawTexture(context.texture, screenWidth - context.texture.width - 20, 20, WHITE);
    DrawTextureEx(context.texture, { preview_x, preview_y }, 0.0f, scale_factor, WHITE);
    DrawRectangleLines(screenWidth - wanted_size - 20, 20, wanted_size, wanted_size, GREEN);

    //draw positions on top of the heightmap
    for (auto const& pos : context.objectPositions)
    {
        // Remap normalized coordinates [0..1] to the preview image in screen space.
        float const px { preview_x + Clamp(pos.x, 0.0f, 1.0f) * preview_w };
        float const py { preview_y + Clamp(pos.y, 0.0f, 1.0f) * preview_h };

        DrawCircleV({ px, py }, 2.0f, RED);
    }

    DrawFPS(10, 10);
}

