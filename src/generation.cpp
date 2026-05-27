#include "generation.hpp"

#include "noise.hpp"
#include "raylib.h"

#include "utils/raylibUtils.hpp"
#include <algorithm> // for std::clamp


std::vector<glm::vec2> generate2DPositions([[maybe_unused]] PointsGenerationParameters const& params) {
    std::vector<glm::vec2> positions {};

    positions.reserve(1000);
    // Naive random generation
    for (int i {0}; i < 1000; ++i)
    {
        positions.emplace_back(
            static_cast<float>(GetRandomValue(0, INT_MAX)) / static_cast<float>(INT_MAX),
            static_cast<float>(GetRandomValue(0, INT_MAX)) / static_cast<float>(INT_MAX)
        );
    }

    // TODO(student): implement Poisson disk sampling to replace the above naive random generation
    // points output should be in [0..1] range, where (0,0) is one corner of the terrain and (1,1) is the opposite corner, so they can be easily scaled to terrain size and sampled from heightmap.
    return positions;
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
///////////////////////////////////////////////////////////////////////////////
struct Lab{ float L; float a; float b;};
Lab linear_srgb_to_oklab(glm::vec3 c){
            //transfo oklab
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


//on revient en RGB
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
            l[i]=(pow(l[i],(1/2.4)))*1.055-0.055;
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

    context.heightmapImage = GenImageFromNoiseFunction<float>(resolution, resolution, PIXELFORMAT_UNCOMPRESSED_R32,
        [&](glm::vec2 const& p)->float { //c'est des coordonnées
            // int oct {10};
            // float lacu {0.6f};
            // float gain {0.2f};
            // TODO(student): implement stack based noise and island mask
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



        
    // exemple conversion from heightmap to color image
    //VERIFIER LES INTERVALLES DES COULEURS CAR RENDU PAS OK
    context.image = TransformImage<float, Color>(context.heightmapImage, [&](float const& v, int const, int const) {

              
    glm::vec3 eau = glm::vec3(70, 130, 200) ;glm::vec3 sable  = glm::vec3(238, 214, 175) ;
    glm::vec3 herbe = glm::vec3(34, 139, 34) ;
    glm::vec3 couleur ; 


    

        if (v <= 0.01f)
        {
            float pourcentage=0.1f*v; //pourcentage probleme
            //on transforme en lineare puis OKlab
            Lab a = linear_srgb_to_oklab(sRGB_to_Linear(eau));
            Lab b =linear_srgb_to_oklab(sRGB_to_Linear(sable));

            //transfo en OKLab //problème ici
             Lab mixLab;
            mixLab.L = (a.L + pourcentage * (b.L - a.L));
            mixLab.a = (a.a + pourcentage * (b.a - a.a));
            mixLab.b = (a.b + pourcentage * (b.b - a.b));

             //interpolation linéaire en OK lab
            couleur= Linear_to_sRGB(oklab_to_linear_srgb(mixLab));
        
        }
        else if (v < 0.4f)
        {
            float pourcentage= v*10;
            //on transforme en lineare puis OKlab
            Lab a = linear_srgb_to_oklab(sRGB_to_Linear(sable));
            Lab b =linear_srgb_to_oklab(sRGB_to_Linear(herbe));

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
           couleur = herbe;
        }

        //uitlisation de clamp pr rester dans le bon intervalle
        //unsigned char car signature Color (cf cette struct)
    //     return color_from({
    //     (unsigned char)glm::clamp(couleur.r, 0.f, 255.f),
    //     (unsigned char)glm::clamp(couleur.g, 0.f, 255.f),
    //     (unsigned char)glm::clamp(couleur.b, 0.f, 255.f)
    return color_from({
    (unsigned char)(v * 255),
    (unsigned char)(v * 255),
    (unsigned char)(v * 255)
});
    // });

        
    }, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);

    context.texture = LoadTextureFromImage(context.image);
    if (context.model.meshCount > 0) {
        context.model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = context.texture;
    }
}


