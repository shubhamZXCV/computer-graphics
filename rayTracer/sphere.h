#ifndef SPHERE_H
#define SPHERE_H

#include <optional>
#include "hittable.h"
#include "vec3.h"
#include "ray.h"
#include"material.h"

class sphere : public hittable {
    public:
        point3 center;
        float radius;
        color sphere_color;
        std::shared_ptr<material> mat_ptr;

        sphere() {}
        sphere(point3 cen, float r, color col)
            : center(cen), radius(r), sphere_color(col) {}

        sphere(point3 cen, float r, color col, std::shared_ptr<material> m)
            : center(cen), radius(r), sphere_color(col), mat_ptr(m) {}
    
        std::optional<hit_record> hit(const ray& r, float t_min, float t_max) const override {
            vec3 oc = r.origin() - center;
            float a = dot(r.direction(), r.direction());
            float half_b = dot(oc, r.direction());
            float c = dot(oc, oc) - radius * radius;
            float discriminant = half_b * half_b - a * c;
    
            if (discriminant < 0) return std::nullopt;
            float sqrtd = std::sqrt(discriminant);
    
            float root = (-half_b - sqrtd) / a;
            if (root < t_min || root > t_max) {
                root = (-half_b + sqrtd) / a;
                if (root < t_min || root > t_max)
                    return std::nullopt;
            }
    
            hit_record rec;
            rec.t = root;
            rec.p = r.at(rec.t);
            rec.normal = (rec.p - center) / radius;
            rec.front_face = dot(r.direction(), rec.normal) < 0;
            if (!rec.front_face)
                rec.normal = -rec.normal;

            // Update hit record to include material pointer
        if (mat_ptr) {
            rec.mat_ptr = mat_ptr.get();
        } else {
            rec.mat_ptr = nullptr;
        }
    
            rec.col = sphere_color;  
            return rec;
        }
    };
    
    

#endif
