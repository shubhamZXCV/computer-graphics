// hittable.h
#ifndef HITTABLE_H
#define HITTABLE_H

#include "ray.h"
#include "hit_record.h"
#include <optional>

class hittable {
public:
    virtual std::optional<hit_record> hit(const ray& r, float t_min, float t_max) const = 0;
    virtual ~hittable() = default;
};

#endif
