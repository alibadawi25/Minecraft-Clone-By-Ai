#include "math_utils.h"
#include <random>
#include <cmath>
#include <algorithm>

namespace MathUtils {

    // Static random number generator
    static std::mt19937 rng(1337);

    // Vector utilities
    float length2D(const glm::vec3& v) {
        return std::sqrt(v.x * v.x + v.z * v.z);
    }

    glm::vec3 normalize2D(const glm::vec3& v) {
        float len = length2D(v);
        if (len > 0.0f) {
            return glm::vec3(v.x / len, v.y, v.z / len);
        }
        return v;
    }

    float distance2D(const glm::vec3& a, const glm::vec3& b) {
        float dx = a.x - b.x;
        float dz = a.z - b.z;
        return std::sqrt(dx * dx + dz * dz);
    }

    // Coordinate conversions
    glm::ivec3 floorToInt(const glm::vec3& v) {
        return glm::ivec3(
            static_cast<int>(std::floor(v.x)),
            static_cast<int>(std::floor(v.y)),
            static_cast<int>(std::floor(v.z))
        );
    }

    glm::vec3 intToFloat(const glm::ivec3& v) {
        return glm::vec3(
            static_cast<float>(v.x),
            static_cast<float>(v.y),
            static_cast<float>(v.z)
        );
    }

    // Interpolation
    float lerp(float a, float b, float t) {
        return a + t * (b - a);
    }

    glm::vec3 lerp(const glm::vec3& a, const glm::vec3& b, float t) {
        return a + t * (b - a);
    }

    // Clamping
    float clamp(float value, float min, float max) {
        return std::max(min, std::min(max, value));
    }

    glm::vec3 clamp(const glm::vec3& value, const glm::vec3& min, const glm::vec3& max) {
        return glm::vec3(
            clamp(value.x, min.x, max.x),
            clamp(value.y, min.y, max.y),
            clamp(value.z, min.z, max.z)
        );
    }

    // Smoothing functions
    float smoothstep(float edge0, float edge1, float x) {
        float t = clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
        return t * t * (3.0f - 2.0f * t);
    }

    float smootherstep(float edge0, float edge1, float x) {
        float t = clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
        return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
    }

    // Random utilities
    void seedRandom(unsigned int seed) {
        rng.seed(seed);
    }

    float randomFloat(float min, float max) {
        std::uniform_real_distribution<float> dist(min, max);
        return dist(rng);
    }

    int randomInt(int min, int max) {
        std::uniform_int_distribution<int> dist(min, max);
        return dist(rng);
    }

    glm::vec3 randomVec3(float min, float max) {
        return glm::vec3(
            randomFloat(min, max),
            randomFloat(min, max),
            randomFloat(min, max)
        );
    }

    // Frustum implementation
    void Frustum::updateFromMatrix(const glm::mat4& viewProjection) {
        const float* m = glm::value_ptr(viewProjection);

        // Extract frustum planes from view-projection matrix
        // Left plane
        planes[0] = glm::vec4(m[3] + m[0], m[7] + m[4], m[11] + m[8], m[15] + m[12]);
        // Right plane
        planes[1] = glm::vec4(m[3] - m[0], m[7] - m[4], m[11] - m[8], m[15] - m[12]);
        // Bottom plane
        planes[2] = glm::vec4(m[3] + m[1], m[7] + m[5], m[11] + m[9], m[15] + m[13]);
        // Top plane
        planes[3] = glm::vec4(m[3] - m[1], m[7] - m[5], m[11] - m[9], m[15] - m[13]);
        // Near plane
        planes[4] = glm::vec4(m[3] + m[2], m[7] + m[6], m[11] + m[10], m[15] + m[14]);
        // Far plane
        planes[5] = glm::vec4(m[3] - m[2], m[7] - m[6], m[11] - m[10], m[15] - m[14]);

        // Normalize planes
        for (auto& plane : planes) {
            float length = glm::length(glm::vec3(plane));
            if (length > 0.0f) {
                plane /= length;
            }
        }
    }

    bool Frustum::containsPoint(const glm::vec3& point) const {
        for (const auto& plane : planes) {
            if (glm::dot(glm::vec3(plane), point) + plane.w < 0.0f) {
                return false;
            }
        }
        return true;
    }

    bool Frustum::containsAABB(const glm::vec3& min, const glm::vec3& max) const {
        for (const auto& plane : planes) {
            glm::vec3 normal = glm::vec3(plane);

            // Get the positive vertex (farthest from plane)
            glm::vec3 positiveVertex = min;
            if (normal.x >= 0) positiveVertex.x = max.x;
            if (normal.y >= 0) positiveVertex.y = max.y;
            if (normal.z >= 0) positiveVertex.z = max.z;

            // If positive vertex is behind plane, AABB is outside frustum
            if (glm::dot(normal, positiveVertex) + plane.w < 0.0f) {
                return false;
            }
        }
        return true;
    }

    bool Frustum::containsSphere(const glm::vec3& center, float radius) const {
        for (const auto& plane : planes) {
            if (glm::dot(glm::vec3(plane), center) + plane.w < -radius) {
                return false;
            }
        }
        return true;
    }

    // AABB implementation
    glm::vec3 AABB::getCenter() const {
        return (min + max) * 0.5f;
    }

    glm::vec3 AABB::getSize() const {
        return max - min;
    }

    float AABB::getRadius() const {
        return glm::length(getSize()) * 0.5f;
    }

    bool AABB::contains(const glm::vec3& point) const {
        return point.x >= min.x && point.x <= max.x &&
               point.y >= min.y && point.y <= max.y &&
               point.z >= min.z && point.z <= max.z;
    }

    bool AABB::intersects(const AABB& other) const {
        return min.x <= other.max.x && max.x >= other.min.x &&
               min.y <= other.max.y && max.y >= other.min.y &&
               min.z <= other.max.z && max.z >= other.min.z;
    }

    void AABB::expand(const glm::vec3& point) {
        min = glm::min(min, point);
        max = glm::max(max, point);
    }

    void AABB::expand(const AABB& other) {
        min = glm::min(min, other.min);
        max = glm::max(max, other.max);
    }

    // Ray implementation
    glm::vec3 Ray::getPoint(float t) const {
        return origin + direction * t;
    }

    // Raycasting
    RayHit rayAABBIntersection(const Ray& ray, const AABB& aabb) {
        RayHit hit;

        glm::vec3 invDir = 1.0f / ray.direction;
        glm::vec3 t1 = (aabb.min - ray.origin) * invDir;
        glm::vec3 t2 = (aabb.max - ray.origin) * invDir;

        glm::vec3 tMin = glm::min(t1, t2);
        glm::vec3 tMax = glm::max(t1, t2);

        float tNear = glm::max(glm::max(tMin.x, tMin.y), tMin.z);
        float tFar = glm::min(glm::min(tMax.x, tMax.y), tMax.z);

        if (tNear <= tFar && tFar > 0.0f) {
            hit.hit = true;
            hit.distance = tNear > 0.0f ? tNear : tFar;
            hit.point = ray.getPoint(hit.distance);

            // Calculate normal
            glm::vec3 center = aabb.getCenter();
            glm::vec3 offset = hit.point - center;
            glm::vec3 absOffset = glm::abs(offset);

            if (absOffset.x > absOffset.y && absOffset.x > absOffset.z) {
                hit.normal = glm::vec3(offset.x > 0 ? 1 : -1, 0, 0);
            } else if (absOffset.y > absOffset.z) {
                hit.normal = glm::vec3(0, offset.y > 0 ? 1 : -1, 0);
            } else {
                hit.normal = glm::vec3(0, 0, offset.z > 0 ? 1 : -1);
            }
        }

        return hit;
    }

    RayHit raySphereIntersection(const Ray& ray, const glm::vec3& center, float radius) {
        RayHit hit;

        glm::vec3 oc = ray.origin - center;
        float a = glm::dot(ray.direction, ray.direction);
        float b = 2.0f * glm::dot(oc, ray.direction);
        float c = glm::dot(oc, oc) - radius * radius;

        float discriminant = b * b - 4 * a * c;

        if (discriminant >= 0) {
            float t = (-b - std::sqrt(discriminant)) / (2.0f * a);
            if (t > 0.0f) {
                hit.hit = true;
                hit.distance = t;
                hit.point = ray.getPoint(t);
                hit.normal = glm::normalize(hit.point - center);
            }
        }

        return hit;
    }

    RayHit rayPlaneIntersection(const Ray& ray, const glm::vec3& point, const glm::vec3& normal) {
        RayHit hit;

        float denom = glm::dot(normal, ray.direction);
        if (std::abs(denom) > 1e-6f) {
            float t = glm::dot(point - ray.origin, normal) / denom;
            if (t > 0.0f) {
                hit.hit = true;
                hit.distance = t;
                hit.point = ray.getPoint(t);
                hit.normal = normal;
            }
        }

        return hit;
    }

    // Hash functions
    std::size_t hashCombine(std::size_t seed, std::size_t hash) {
        return seed ^ (hash + 0x9e3779b9 + (seed << 6) + (seed >> 2));
    }

    std::size_t hashVec3(const glm::vec3& v) {
        std::size_t h1 = std::hash<float>()(v.x);
        std::size_t h2 = std::hash<float>()(v.y);
        std::size_t h3 = std::hash<float>()(v.z);
        return hashCombine(hashCombine(h1, h2), h3);
    }

    std::size_t hashIVec3(const glm::ivec3& v) {
        std::size_t h1 = std::hash<int>()(v.x);
        std::size_t h2 = std::hash<int>()(v.y);
        std::size_t h3 = std::hash<int>()(v.z);
        return hashCombine(hashCombine(h1, h2), h3);
    }

    // SimpleNoise implementation
    SimpleNoise::SimpleNoise(unsigned int seed) : seed(seed) {}

    float SimpleNoise::hash(float n) {
        return std::fmod(std::sin(n + seed) * 43758.5453f, 1.0f);
    }

    float SimpleNoise::interpolate(float a, float b, float t) {
        return lerp(a, b, smoothstep(0.0f, 1.0f, t));
    }

    float SimpleNoise::noise1D(float x) {
        float i = std::floor(x);
        float f = x - i;

        float a = hash(i);
        float b = hash(i + 1.0f);

        return interpolate(a, b, f);
    }

    float SimpleNoise::noise2D(float x, float y) {
        float i = std::floor(x);
        float j = std::floor(y);
        float u = x - i;
        float v = y - j;

        float a = hash(i + j * 57.0f);
        float b = hash(i + 1.0f + j * 57.0f);
        float c = hash(i + (j + 1.0f) * 57.0f);
        float d = hash(i + 1.0f + (j + 1.0f) * 57.0f);

        float x1 = interpolate(a, b, u);
        float x2 = interpolate(c, d, u);

        return interpolate(x1, x2, v);
    }

    float SimpleNoise::noise3D(float x, float y, float z) {
        float i = std::floor(x);
        float j = std::floor(y);
        float k = std::floor(z);
        float u = x - i;
        float v = y - j;
        float w = z - k;

        float n000 = hash(i + j * 57.0f + k * 113.0f);
        float n001 = hash(i + j * 57.0f + (k + 1.0f) * 113.0f);
        float n010 = hash(i + (j + 1.0f) * 57.0f + k * 113.0f);
        float n011 = hash(i + (j + 1.0f) * 57.0f + (k + 1.0f) * 113.0f);
        float n100 = hash(i + 1.0f + j * 57.0f + k * 113.0f);
        float n101 = hash(i + 1.0f + j * 57.0f + (k + 1.0f) * 113.0f);
        float n110 = hash(i + 1.0f + (j + 1.0f) * 57.0f + k * 113.0f);
        float n111 = hash(i + 1.0f + (j + 1.0f) * 57.0f + (k + 1.0f) * 113.0f);

        float x1 = interpolate(n000, n100, u);
        float x2 = interpolate(n010, n110, u);
        float x3 = interpolate(n001, n101, u);
        float x4 = interpolate(n011, n111, u);

        float y1 = interpolate(x1, x2, v);
        float y2 = interpolate(x3, x4, v);

        return interpolate(y1, y2, w);
    }

    float SimpleNoise::fractalNoise2D(float x, float y, int octaves, float persistence) {
        float value = 0.0f;
        float amplitude = 1.0f;
        float frequency = 1.0f;
        float maxValue = 0.0f;

        for (int i = 0; i < octaves; i++) {
            value += noise2D(x * frequency, y * frequency) * amplitude;
            maxValue += amplitude;
            amplitude *= persistence;
            frequency *= 2.0f;
        }

        return value / maxValue;
    }

    float SimpleNoise::fractalNoise3D(float x, float y, float z, int octaves, float persistence) {
        float value = 0.0f;
        float amplitude = 1.0f;
        float frequency = 1.0f;
        float maxValue = 0.0f;

        for (int i = 0; i < octaves; i++) {
            value += noise3D(x * frequency, y * frequency, z * frequency) * amplitude;
            maxValue += amplitude;
            amplitude *= persistence;
            frequency *= 2.0f;
        }

        return value / maxValue;
    }
}
