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

    for (size_t i = 0; i < context.objectPositions.size(); i++)
    {
        // If out of bounds :
        if (context.objectPositions[i].z <= context.pointsGenerationParameters.minimum_z || context.objectPositions[i].z >  context.pointsGenerationParameters.maximum_z) {
            // We erase it
            context.objectPositions.erase(context.objectPositions.begin() + i);
            // We want to stay on that spot, otherwise we skip the next point (because erasing means erasing the empty spot too)
            i--;
        }
    }
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
///////////////////////////////////////////////////////////////////////////////
struct Lab{ float L; float a; float b;};
Lab linear_srgb_to_oklab(glm::vec3 c){
 //Transfo oklab
        float l = 0.4122214708f * c.r + 0.5363325363f * c.g + 0.0514459929f * c.b;
	    float m = 0.2119034982f * c.r + 0.6806995451f * c.g + 0.1073969566f * c.b;
        float s = 0.0883024619f * c.r + 0.2817188376f * c.g + 0.6299787005f * c.b;
            //racine cubique
        float l_ = cbrtf(l);
        float m_ = cbrtf(m);
        float s_ = cbrtf(s);
            // Tranfo en OKLab
        return 
        {0.2104542553f*l_ + 0.7936177850f*m_ - 0.0040720468f*s_,
        1.9779984951f*l_ - 2.4285922050f*m_ + 0.4505937099f*s_,
        0.0259040371f*l_ + 0.7827717662f*m_ - 0.8086757660f*s_,};
}


//On revient en RGB
glm::vec3 oklab_to_linear_srgb(Lab c){
    float l_ = c.L + 0.3963377774f * c.a + 0.2158037573f * c.b;
    float m_ = c.L - 0.1055613458f * c.a - 0.0638541728f * c.b;
    float s_ = c.L - 0.0894841775f * c.a - 1.2914855480f * c.b;
    //puissance 3
    float l = pow(l_,3);
    float m =pow(m_,3);
    float s = pow(s_,3);
    //Tranfo en rgb
    return {
		+4.0767416621f * l - 3.3077115913f * m + 0.2309699292f * s,
		-1.2684380046f * l + 2.6097574011f * m - 0.3413193965f * s,
		-0.0041960863f * l - 0.7034186147f * m + 1.7076147010f * s,
    };
}

glm::vec3 sRGB_to_Linear(glm::vec3 c){
    //on parcourt les 3 composante de c
    for (int i=0;i<3;i++){
        if (c[i]<=0.04045){
            c[i]=c[i]/12.92;
        }
        else{
            c[i]=pow(((c[i]+0.055)/1.055),2.4);
        }
    }
    return c;
}

glm::vec3 Linear_to_sRGB(glm::vec3 l){
    //on parcourt les 3 composante de c
    for (int i=0;i<3;i++){
        if (l[i]<=0.0031308){
            l[i]=l[i]*12.92;
        }
        else{
            l[i]=(pow(l[i],(1.f/2.4f)))*1.055-0.055;
        }
    }
    return l;
}

///////////////////////////////////////////////////////////////////////////////:

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
    // int oct {100};
    // float lacu {0.2f};
    // float gain {0.3f};
    context.heightmapImage = GenImageFromNoiseFunction<float>(resolution, resolution, PIXELFORMAT_UNCOMPRESSED_R32,
        [&](glm::vec2 const& p)->float { //c'est des coordonnées
            //on doit calculer distance au centre

            glm::vec2 const centre = {0.5f, 0.5f};
            float d= sqrt(pow(centre.x-p.x,2) + pow(centre.y-p.y,2));   //calcul distance du centre ac pythagore 
            // + loins -effet 
            float masque;
            float r=0.5;
            //si au delà du rayon r=0.5 (eau)
            if (d>r){
                masque=0.25;
            }
            else{
                 masque= pow(1.f-d,2);
             } //au carré car sinon masque trop faible
             float n=octaveNoise(p, perlinNoise, context.imageGenerationParameters.oct, context.imageGenerationParameters.lacu,context.imageGenerationParameters.gain, context.imageGenerationParameters.noiseSeed, context.imageGenerationParameters.noiseScale);
             n = (n + 1.f) * 0.5f; // on normalise ici A VOIR PQ
            return masque*n;
        });



    
    context.image = TransformImage<float, Color>(context.heightmapImage, [&](float const& v, int const, int const) {

    glm::vec3 eau_profonde = glm::vec3(10, 40, 120) / 255.f;
    glm::vec3 eau_claire   = glm::vec3(70, 130, 200) / 255.f;

    glm::vec3 sable_fonce  = glm::vec3(194, 178, 128) / 255.f;
    glm::vec3 sable_clair  = glm::vec3(238, 214, 175) / 255.f;

    glm::vec3 herbe_claire = glm::vec3(80, 170, 70) / 255.f;
    glm::vec3 herbe        = glm::vec3(30, 120, 40) / 255.f;

    glm::vec3 foret        = glm::vec3(10, 70, 30) / 255.f;

    glm::vec3 roche        = glm::vec3(120, 120, 120) / 255.f;

    glm::vec3 couleur ; 
    

        if (v <= 0.15f)
        {
            float pourcentage=v/0.15f; //pourcentage probleme plutot mapper entre 0 et 1
            //on transforme en lineare puis OKlab
            Lab a = linear_srgb_to_oklab(sRGB_to_Linear(eau_profonde));
            Lab b =linear_srgb_to_oklab(sRGB_to_Linear(eau_claire));

            //transfo en OKLab //problème ici
             Lab mixLab;
            mixLab.L = (a.L + pourcentage * (b.L - a.L));
            mixLab.a = (a.a + pourcentage * (b.a - a.a));
            mixLab.b = (a.b + pourcentage * (b.b - a.b));

             //interpolation linéaire en OK lab
            couleur= Linear_to_sRGB(oklab_to_linear_srgb(mixLab));
        
        }
    

        else if (v <= 0.2f)
        {
            float pourcentage=(v-0.15f)/(0.2f-0.15f); //pourcentage probleme plutot mapper entre 0 et 1
            //on transforme en lineare puis OKlab
            Lab a = linear_srgb_to_oklab(sRGB_to_Linear(eau_claire));
            Lab b =linear_srgb_to_oklab(sRGB_to_Linear(sable_clair));

            //transfo en OKLab //problème ici
             Lab mixLab;
            mixLab.L = (a.L + pourcentage * (b.L - a.L));
            mixLab.a = (a.a + pourcentage * (b.a - a.a));
            mixLab.b = (a.b + pourcentage * (b.b - a.b));

             //interpolation linéaire en OK lab
            couleur= Linear_to_sRGB(oklab_to_linear_srgb(mixLab));
        
        }

          else if (v <= 0.25f)
        {
            float pourcentage=(v-0.2f)/(0.25f-0.2f); //pourcentage probleme plutot mapper entre 0 et 1
            //on transforme en lineare puis OKlab
            Lab a = linear_srgb_to_oklab(sRGB_to_Linear(sable_clair));
            Lab b =linear_srgb_to_oklab(sRGB_to_Linear(sable_fonce));

            //transfo en OKLab //problème ici
             Lab mixLab;
            mixLab.L = (a.L + pourcentage * (b.L - a.L));
            mixLab.a = (a.a + pourcentage * (b.a - a.a));
            mixLab.b = (a.b + pourcentage * (b.b - a.b));

             //interpolation linéaire en OK lab
            couleur= Linear_to_sRGB(oklab_to_linear_srgb(mixLab));
        
        }
        else if (v < 0.35f)
        {
            float pourcentage=(v - 0.25f) / (0.35f - 0.25f); // mapper (0.3 - 0.5) vers (0, 1)
            //on transforme en lineare puis OKlab
            Lab a = linear_srgb_to_oklab(sRGB_to_Linear(sable_fonce));
            Lab b =linear_srgb_to_oklab(sRGB_to_Linear(herbe_claire));

            //transfo en OKLab
            Lab mixLab;
            mixLab.L = a.L + pourcentage * (b.L - a.L);
            mixLab.a = a.a + pourcentage * (b.a - a.a);
            mixLab.b = a.b + pourcentage * (b.b - a.b);
    
             //interpolation linéaire en OK lab
            couleur= Linear_to_sRGB(oklab_to_linear_srgb(mixLab));

        }
         else if (v < 0.45f)
        {
            float pourcentage=(v - 0.35) / (0.45f - 0.35f); // mapper (0.3 - 0.5) vers (0, 1)
            //on transforme en lineare puis OKlab
            Lab a = linear_srgb_to_oklab(sRGB_to_Linear(herbe_claire));
            Lab b =linear_srgb_to_oklab(sRGB_to_Linear(herbe));

            //transfo en OKLab
            Lab mixLab;
            mixLab.L = a.L + pourcentage * (b.L - a.L);
            mixLab.a = a.a + pourcentage * (b.a - a.a);
            mixLab.b = a.b + pourcentage * (b.b - a.b);
    
             //interpolation linéaire en OK lab
            couleur= Linear_to_sRGB(oklab_to_linear_srgb(mixLab));

        }
         else if (v < 0.6f)
        {
            float pourcentage=(v - 0.45f) / (0.6f - 0.45f); // mapper (0.3 - 0.5) vers (0, 1)
            //on transforme en lineare puis OKlab
            Lab a = linear_srgb_to_oklab(sRGB_to_Linear(herbe));
            Lab b =linear_srgb_to_oklab(sRGB_to_Linear(foret));

            //transfo en OKLab
            Lab mixLab;
            mixLab.L = a.L + pourcentage * (b.L - a.L);
            mixLab.a = a.a + pourcentage * (b.a - a.a);
            mixLab.b = a.b + pourcentage * (b.b - a.b);
    
             //interpolation linéaire en OK lab
            couleur= Linear_to_sRGB(oklab_to_linear_srgb(mixLab));
        }
         else if (v < 0.8f)
        {
            float pourcentage=(v - 0.6f) / (0.8f - 0.6f); // mapper (0.3 - 0.5) vers (0, 1)
            //on transforme en lineare puis OKlab
            Lab a = linear_srgb_to_oklab(sRGB_to_Linear(foret));
            Lab b =linear_srgb_to_oklab(sRGB_to_Linear(roche));

            //transfo en OKLab
            Lab mixLab;
            mixLab.L = a.L + pourcentage * (b.L - a.L);
            mixLab.a = a.a + pourcentage * (b.a - a.a);
            mixLab.b = a.b + pourcentage * (b.b - a.b);
    
             //interpolation linéaire en OK lab
            couleur= Linear_to_sRGB(oklab_to_linear_srgb(mixLab));
        }
        else
        {
           couleur = roche;
        }

       // uitlisation de clamp pr rester dans le bon intervalle
        //unsigned char car signature Color (cf cette struct)
       return color_from({
    (unsigned char)(glm::clamp(couleur.r, 0.f, 1.f) * 255.f),
    (unsigned char)(glm::clamp(couleur.g, 0.f, 1.f) * 255.f),
    (unsigned char)(glm::clamp(couleur.b, 0.f, 1.f) * 255.f)
});

        
    }, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);

    context.texture = LoadTextureFromImage(context.image);
    if (context.model.meshCount > 0) {
        context.model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = context.texture;
    }
}


