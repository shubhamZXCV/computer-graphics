// triangle.h
#ifndef TRIANGLE_H
#define TRIANGLE_H

#include "hittable.h"
#include "vec3.h"

class triangle : public hittable {
public:
    point3 v0, v1, v2;  
    color col;
    std::shared_ptr<material> mat_ptr;

    triangle(point3 _v0, point3 _v1, point3 _v2, color col)
        : v0(_v0), v1(_v1), v2(_v2), col(col) {}
    triangle(point3 _v0, point3 _v1, point3 _v2, color col, std::shared_ptr<material> m)
        : v0(_v0), v1(_v1), v2(_v2), col(col), mat_ptr(m) {}
        

    std::optional<hit_record> hit(const ray& r, float t_min, float t_max) const override {
        vec3 edge1 = v1 - v0;
        vec3 edge2 = v2 - v0;
        vec3 h = cross(r.direction(), edge2);
        float a = dot(edge1, h);

        if (fabs(a) < 1e-6) return std::nullopt;  // Ray parallel to triangle

        float f = 1.0 / a;
        vec3 s = r.origin() - v0;
        float u = f * dot(s, h);
        if (u < 0.0 || u > 1.0) return std::nullopt;

        vec3 q = cross(s, edge1);
        float v = f * dot(r.direction(), q);
        if (v < 0.0 || u + v > 1.0) return std::nullopt;

        float t = f * dot(edge2, q);
        if (t < t_min || t > t_max) return std::nullopt;

        hit_record rec;
        rec.t = t;
        rec.p = r.at(t);
        rec.set_face_normal(r, unit_vector(cross(edge1, edge2)));
        rec.col = col;
        if (mat_ptr) {
            rec.mat_ptr = mat_ptr.get();
        } else {
            rec.mat_ptr = nullptr;
        }
        
        return rec;
    }
};

#endif
