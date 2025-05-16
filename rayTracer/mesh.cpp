#include <iostream>
#include <vector>
#include <memory>

#include "vec3.h"
#include "ray.h"
#include "sphere.h"
#include "cube.h"
#include "hittable_list.h"
#include "hit_record.h"
#include "printPPM.h"
#include "triangle.h"
#include "material.h"
#include "../include/OFFReader.h"






// Light
point3 light_pos(-10,5, -5);     // A light source in the scene
color light_color(1.0, 1.0, 1.0); // White light

color ray_color(const ray& r, const hittable& world, int depth = 50) {
    // If we've exceeded the ray bounce limit, no more light is gathered
    if (depth <= 0)
        return color(0, 0, 0);
        
    auto rec = world.hit(r, 0.001, INFINITY);
    if (rec) {
        point3 hit_point = rec->p;
        vec3 normal = rec->normal;
        
        // Use the material system if a material is assigned
        if (rec->mat_ptr) {
            ray scattered;
            color attenuation;
            if (rec->mat_ptr->scatter(r, *rec, attenuation, scattered)) {
                return attenuation * ray_color(scattered, world, depth-1);
            }
            return color(0, 0, 0);
        }
        
        // Fallback to the original lighting if no material is present
        vec3 light_dir = unit_vector(light_pos - hit_point);
        ray shadow_ray(hit_point + 0.001 * normal, light_dir);
        auto shadow_hit = world.hit(shadow_ray, 0.001, (light_pos - hit_point).length());

        if (shadow_hit.has_value()) {
            return 0.2 * rec->col;  // ambient in shadow
        } else {
            float brightness = std::max(dot(normal, light_dir), 0.0f);
            color ambient = 0.2 * rec->col;
            color diffuse = brightness * rec->col;
            return ambient + diffuse;
        }
    }

    // Background - dark color to simulate room environment
    return color(0.05, 0.05, 0.05);
}



int main() {
    // Image
    const int image_width = 800;
    const int image_height = 600;
    const int samples_per_pixel = 1; // can increase later for anti-aliasing

    // Camera - positioned to view the room from the front
    point3 origin(0, 0, 2);  // Camera pulled back to see the room
    float viewport_height = 2.0;
    float viewport_width = viewport_height * image_width / image_height;
    float focal_length = 1.0;

    vec3 horizontal(viewport_width, 0, 0);
    vec3 vertical(0, viewport_height, 0);
    point3 lower_left_corner = origin - horizontal/2 - vertical/2 - vec3(0, 0, focal_length);

    // Light - positioned at the ceiling
    light_pos = point3(-0.5, 0.9, -1);  // Light at the ceiling

    hittable_list mesh;

    offmodel* model =  readOffFile("../models/house.off");  
    auto metal_material = std::make_shared<metal>(color(1.0, 0.0, 0.0)); 

 for (int i = 0; i < model->numberOfPolygons; ++i) {
    const Polygon& poly = model->polygons[i];
    if (poly.noSides < 3) continue;  // Skip degenerate polygons

    for (int j = 1; j < poly.noSides - 1; ++j) {
        point3 v0 = point3(model->vertices[poly.v[0]].x, model->vertices[poly.v[0]].y, model->vertices[poly.v[0]].z);
        point3 v1 = point3(model->vertices[poly.v[j]].x, model->vertices[poly.v[j]].y, model->vertices[poly.v[j]].z);
        point3 v2 = point3(model->vertices[poly.v[j + 1]].x, model->vertices[poly.v[j + 1]].y, model->vertices[poly.v[j + 1]].z);
        mesh.add(std::make_shared<triangle>(v0, v1, v2, color(1.0,0.0,0.0)));
    }
}
    
   
    // Render
    std::vector<color> pixels(image_width * image_height);

    for (int j = image_height - 1; j >= 0; --j) {
        std::cerr << "\rScanlines remaining: " << j << std::flush;
        for (int i = 0; i < image_width; ++i) {
            float u = float(i) / (image_width - 1);
            float v = float(j) / (image_height - 1);

            ray r(origin, lower_left_corner + u * horizontal + v * vertical - origin);
            color pixel_color = ray_color(r, mesh); // Or whatever depth value is appropriate

            pixels[j * image_width + i] = pixel_color;
        }
    }

    std::cerr << "\nWriting image...\n";
    write_ppm("image.ppm", pixels, image_width, image_height, samples_per_pixel);
    std::cerr << "Done.\n";

    return 0;
}
