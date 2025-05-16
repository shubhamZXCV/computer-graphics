// hittable_list.h
#ifndef HITTABLE_LIST_H
#define HITTABLE_LIST_H

#include "hittable.h"
#include <vector>
#include <memory>

class hittable_list : public hittable {
public:
    std::vector<std::shared_ptr<hittable>> objects;

    hittable_list() {}
    hittable_list(std::shared_ptr<hittable> object) { add(object); }

    void clear() { objects.clear(); }
    void add(std::shared_ptr<hittable> object) { objects.push_back(object); }

    std::optional<hit_record> hit(const ray& r, float t_min, float t_max) const override {
        std::optional<hit_record> temp_rec;
        auto closest_so_far = t_max;
        std::optional<hit_record> result = std::nullopt;

        for (const auto& object : objects) {
            temp_rec = object->hit(r, t_min, closest_so_far);
            if (temp_rec) {
                closest_so_far = temp_rec->t;
                result = temp_rec;
            }
        }

        return result;
    }
};

#endif
