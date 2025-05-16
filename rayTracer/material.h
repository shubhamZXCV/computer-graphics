#ifndef MATERIAL_H
#define MATERIAL_H

#include "ray.h"
#include "vec3.h"

// Forward declarations to break circular dependency
struct hit_record;

// Utility function
inline vec3 reflect(const vec3& v, const vec3& n) {
    return v - 2 * dot(v, n) * n;
}

class material {
public:
    virtual bool scatter(
        const ray& r_in, const hit_record& rec,
        color& attenuation, ray& scattered
    ) const = 0;

    virtual ~material() = default;
};

class metal : public material {
public:
    color albedo;

    metal(const color& a) : albedo(a) {}

    bool scatter(
        const ray& r_in, const hit_record& rec,
        color& attenuation, ray& scattered
    ) const override {
        vec3 reflected = reflect(unit_vector(r_in.direction()), rec.normal);
        scattered = ray(rec.p, reflected);
        attenuation = albedo;
        return (dot(scattered.direction(), rec.normal) > 0);
    }
};

class noMaterial : public material {
public:
    noMaterial() {}

    bool scatter(
        const ray& r_in, const hit_record& rec,
        color& attenuation, ray& scattered
    ) const override {
        // This material doesn't reflect anything
        // We're not modifying 'scattered' because it doesn't matter
        // Setting attenuation to black (0,0,0) for completeness
        attenuation = color(0, 0, 0);
        return false; // Always return false to indicate no reflection
    }
};

#endif
