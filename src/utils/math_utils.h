#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <array>
#include <vector>

namespace MathUtils {

    // Constants
    constexpr float PI = 3.14159265358979323846f;
    constexpr float DEG_TO_RAD = PI / 180.0f;
    constexpr float RAD_TO_DEG = 180.0f / PI;

    // Vector utilities
    float length2D(const glm::vec3& v);
    glm::vec3 normalize2D(const glm::vec3& v);
    float distance2D(const glm::vec3& a, const glm::vec3& b);

    // Coordinate conversions
    glm::ivec3 floorToInt(const glm::vec3& v);
    glm::vec3 intToFloat(const glm::ivec3& v);

    // Interpolation
    float lerp(float a, float b, float t);
    glm::vec3 lerp(const glm::vec3& a, const glm::vec3& b, float t);

    // Clamping
    float clamp(float value, float min, float max);
    glm::vec3 clamp(const glm::vec3& value, const glm::vec3& min, const glm::vec3& max);

    // Smoothing functions
    float smoothstep(float edge0, float edge1, float x);
    float smootherstep(float edge0, float edge1, float x);

    // Random utilities
    void seedRandom(unsigned int seed);
    float randomFloat(float min = 0.0f, float max = 1.0f);
    int randomInt(int min, int max);
    glm::vec3 randomVec3(float min = -1.0f, float max = 1.0f);

    // Frustum utilities
    struct Frustum {
        std::array<glm::vec4, 6> planes; // Left, Right, Bottom, Top, Near, Far

        void updateFromMatrix(const glm::mat4& viewProjection);
        bool containsPoint(const glm::vec3& point) const;
        bool containsAABB(const glm::vec3& min, const glm::vec3& max) const;
        bool containsSphere(const glm::vec3& center, float radius) const;
    };

    // AABB (Axis-Aligned Bounding Box) utilities
    struct AABB {
        glm::vec3 min, max;

        AABB() : min(0.0f), max(0.0f) {}
        AABB(const glm::vec3& min, const glm::vec3& max) : min(min), max(max) {}

        glm::vec3 getCenter() const;
        glm::vec3 getSize() const;
        float getRadius() const;
        bool contains(const glm::vec3& point) const;
        bool intersects(const AABB& other) const;
        void expand(const glm::vec3& point);
        void expand(const AABB& other);
    };

    // Ray utilities
    struct Ray {
        glm::vec3 origin;
        glm::vec3 direction;

        Ray(const glm::vec3& origin, const glm::vec3& direction)
            : origin(origin), direction(glm::normalize(direction)) {}

        glm::vec3 getPoint(float t) const;
    };

    struct RayHit {
        bool hit = false;
        float distance = 0.0f;
        glm::vec3 point = glm::vec3(0.0f);
        glm::vec3 normal = glm::vec3(0.0f);
    };

    // Raycasting
    RayHit rayAABBIntersection(const Ray& ray, const AABB& aabb);
    RayHit raySphereIntersection(const Ray& ray, const glm::vec3& center, float radius);
    RayHit rayPlaneIntersection(const Ray& ray, const glm::vec3& point, const glm::vec3& normal);

    // Hash functions
    std::size_t hashCombine(std::size_t seed, std::size_t hash);
    std::size_t hashVec3(const glm::vec3& v);
    std::size_t hashIVec3(const glm::ivec3& v);

    // Noise utilities (placeholder for FastNoise integration)
    class SimpleNoise {
    public:
        SimpleNoise(unsigned int seed = 1337);

        float noise1D(float x);
        float noise2D(float x, float y);
        float noise3D(float x, float y, float z);

        // Fractal noise
        float fractalNoise2D(float x, float y, int octaves = 4, float persistence = 0.5f);
        float fractalNoise3D(float x, float y, float z, int octaves = 4, float persistence = 0.5f);

    private:
        unsigned int seed;

        float hash(float n);
        float interpolate(float a, float b, float t);
    };
}
