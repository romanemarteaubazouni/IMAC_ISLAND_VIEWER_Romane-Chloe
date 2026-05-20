
#include "noise.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/noise.hpp>

#include <cstdint>
#include <functional>

namespace {

// Fast integer hash function
std::uint32_t hashU32(std::uint32_t v)
{
    v ^= v >> 16;
    v *= 0x7feb352du;
    v ^= v >> 15;
    v *= 0x846ca68bu;
    v ^= v >> 16;
    return v;
}

glm::vec2 seedToOffset2D(int seed)
{
    std::uint32_t const base { static_cast<std::uint32_t>(seed) };
    std::uint32_t const hx { hashU32(base ^ 0x9e3779b9u) };
    std::uint32_t const hy { hashU32(base ^ 0x85ebca6bu) };

    float const fx { static_cast<float>(hx & 0x00ffffffu) / 16777216.0f };
    float const fy { static_cast<float>(hy & 0x00ffffffu) / 16777216.0f };

    // Large translation range so seeds land on very different 2D Perlin regions.
    return {
        fx * 4096.0f - 2048.0f,
        fy * 4096.0f - 2048.0f
    };
}

} // namespace

float perlinNoise(glm::vec2 const& position) {
    return glm::perlin(position);
}

float perlinNoiseSeeded(glm::vec2 const& position, int seed) {
    // Cache computed offset because the same seed is used for many samples per frame.
    static int cachedSeed {};
    static glm::vec2 cachedOffset {};

    if (seed != cachedSeed) {
        cachedSeed = seed;
        cachedOffset = seedToOffset2D(seed);
    }

    return glm::perlin(position + cachedOffset);
}


float octaveNoise(glm::vec2 const& position, std::function<float(glm::vec2 const&)> noiseFunction, int oct, float lacu, float gain, float seed, float scale) {
    float value {0.f}; // Résultat du bruit

    float amplitude {0.5f}; // Amplitude de chaque couche du bruit
    float frequency {1.f}; // Fréquence de chaque couhce du bruit
    // Pour normaliser notre résultat, il faut sommer les amplitudes de chaque couche, puisque celle-ci n'est pas constante
    float sum_amplitudes {0.f};
    // Pour chaque couche :
    for(int i {0}; i < oct; i++) {
        // seed agit comme un offset : "valeur de départ pour la génération du bruit, permettant d'obtenir des résultats différents à chaque exécution ou de reproduire les mêmes résultats en utilisant la même seed"
        // value évolue à chaque itération grâce aux facteurs frequency et amplitude
        // position est un glm::vec2
        value += amplitude * noiseFunction(frequency * position * scale + seed);
        
        sum_amplitudes += amplitude;
        amplitude *= gain; // Gain en amplitude de chaque couche
        frequency *= lacu; // Gain en fréquence de chaque couche
    }
    // On normalise pour avoir un résultat entre [-1,1]
    // On normalise par rapport aux amplitudes (car chaque itération n'a pas le même poids)
    value /= sum_amplitudes;

    return value;
}



// J'ai modifié les paramètres d'entrée, à voir comment on comprend la consigne
// Exposition des paramètres dans l'interface pour permettre l'exploration visuelle (comment ? où ?)