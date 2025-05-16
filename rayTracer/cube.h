#ifndef CUBE_H
#define CUBE_H

#include <optional>
#include <algorithm> // for std::min
#include "hittable.h"
#include "vec3.h"
#include "ray.h"
#include "material.h"


class cube : public hittable {
public:
    point3 center;         // Center of the cube
    float width, height, depth; // Dimensions of the cube
    color cube_color;
    std::shared_ptr<material> mat_ptr;

    cube() {}
    
    // Constructor for a cube with no material
    cube(point3 cen, float w, float h, float d, color col)
        : center(cen), width(w), height(h), depth(d), cube_color(col) {}
    
    // Constructor for a cube with material
    cube(point3 cen, float w, float h, float d, color col, std::shared_ptr<material> m)
        : center(cen), width(w), height(h), depth(d), cube_color(col), mat_ptr(m) {}
    
    // Constructor for a perfect cube (all sides equal)
    cube(point3 cen, float size, color col)
        : center(cen), width(size), height(size), depth(size), cube_color(col) {}
    
    // Constructor for a perfect cube with material
    cube(point3 cen, float size, color col, std::shared_ptr<material> m)
        : center(cen), width(size), height(size), depth(size), cube_color(col), mat_ptr(m) {}

    std::optional<hit_record> hit(const ray& r, float t_min, float t_max) const override {
        // Calculate the half extents of the cube
        float half_w = width / 2.0f;
        float half_h = height / 2.0f;
        float half_d = depth / 2.0f;
        
        // Calculate the min and max points of the cube
        point3 min_point = center - vec3(half_w, half_h, half_d);
        point3 max_point = center + vec3(half_w, half_h, half_d);
        
        // Calculate intersection with each slab
        float tx1 = (min_point.x() - r.origin().x()) / r.direction().x();
        float tx2 = (max_point.x() - r.origin().x()) / r.direction().x();
        
        float ty1 = (min_point.y() - r.origin().y()) / r.direction().y();
        float ty2 = (max_point.y() - r.origin().y()) / r.direction().y();
        
        float tz1 = (min_point.z() - r.origin().z()) / r.direction().z();
        float tz2 = (max_point.z() - r.origin().z()) / r.direction().z();
        
        // Handle division by zero (parallel to an axis)
        if (r.direction().x() == 0.0) {
            tx1 = -INFINITY;
            tx2 = INFINITY;
        }
        if (r.direction().y() == 0.0) {
            ty1 = -INFINITY;
            ty2 = INFINITY;
        }
        if (r.direction().z() == 0.0) {
            tz1 = -INFINITY;
            tz2 = INFINITY;
        }
        
        // Swap if needed to ensure t1 < t2
        if (tx1 > tx2) std::swap(tx1, tx2);
        if (ty1 > ty2) std::swap(ty1, ty2);
        if (tz1 > tz2) std::swap(tz1, tz2);
        
        // Find the latest entry and earliest exit
        float tmin = std::max(std::max(tx1, ty1), tz1);
        float tmax = std::min(std::min(tx2, ty2), tz2);
        
        // No intersection if exit before entry or beyond t range
        if (tmax < tmin || tmax < t_min || tmin > t_max) {
            return std::nullopt;
        }
        
        // Use the entry point for the hit record
        float t = (tmin > t_min) ? tmin : tmax;
        if (t > t_max) return std::nullopt;
        
        hit_record rec;
        rec.t = t;
        rec.p = r.at(t);
        
        // Determine the normal based on which face was hit
        vec3 normal;
        point3 hit_point = rec.p;
        
        // Calculate distances to each face
        float dx1 = std::abs(hit_point.x() - min_point.x());
        float dx2 = std::abs(hit_point.x() - max_point.x());
        float dy1 = std::abs(hit_point.y() - min_point.y());
        float dy2 = std::abs(hit_point.y() - max_point.y());
        float dz1 = std::abs(hit_point.z() - min_point.z());
        float dz2 = std::abs(hit_point.z() - max_point.z());
        
        // Find the minimum distance to determine which face was hit
        // Fixed: using multiple std::min calls instead of initializer list
        float min_dist = std::min(dx1, std::min(dx2, std::min(dy1, std::min(dy2, std::min(dz1, dz2)))));
        
        if (min_dist == dx1) normal = vec3(-1, 0, 0);
        else if (min_dist == dx2) normal = vec3(1, 0, 0);
        else if (min_dist == dy1) normal = vec3(0, -1, 0);
        else if (min_dist == dy2) normal = vec3(0, 1, 0);
        else if (min_dist == dz1) normal = vec3(0, 0, -1);
        else normal = vec3(0, 0, 1);
        
        rec.normal = normal;
        rec.front_face = dot(r.direction(), normal) < 0;
        if (!rec.front_face)
            rec.normal = -normal;
        
        // Set material information
        if (mat_ptr) {
            rec.mat_ptr = mat_ptr.get();
        } else {
            rec.mat_ptr = nullptr;
        }
        
        rec.col = cube_color;
        return rec;
    }
};

#endif
