// hit_record.h
#ifndef HIT_RECORD_H
#define HIT_RECORD_H

#include "vec3.h"
#include "ray.h"

// Forward declaration
class material;

struct hit_record {
    point3 p;
    vec3 normal;
    float t;
    bool front_face;
    color col;
    const material* mat_ptr;
    
    inline void set_face_normal(const ray& r, const vec3& outward_normal) {
        front_face = dot(r.direction(), outward_normal) < 0;
        normal = front_face ? outward_normal : -outward_normal;
    }
};

#endif
