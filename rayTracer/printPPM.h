#ifndef PRINT_PPM_H
#define PRINT_PPM_H

#include <fstream>
#include <vector>
#include "vec3.h"

void write_color(std::ostream &out, color pixel_color, int samples_per_pixel) {
    float r = pixel_color.x();
    float g = pixel_color.y();
    float b = pixel_color.z();

    // Divide the color by the number of samples
    if (samples_per_pixel > 1) {
        float scale = 1.0 / samples_per_pixel;
        r *= scale;
        g *= scale;
        b *= scale;
    }

    // Write the translated [0,255] value of each color component
    out << static_cast<int>(255.999 * r) << ' '
        << static_cast<int>(255.999 * g) << ' '
        << static_cast<int>(255.999 * b) << '\n';
}

void write_ppm(const std::string& filename, const std::vector<color>& pixels, 
               int width, int height, int samples_per_pixel) {
    std::ofstream out(filename);
    
    // PPM Header
    out << "P3\n" << width << " " << height << "\n255\n";
    
    // Write pixel data
    for (int j = height - 1; j >= 0; --j) {
        for (int i = 0; i < width; ++i) {
            write_color(out, pixels[j * width + i], samples_per_pixel);
        }
    }
    
    out.close();
}

#endif
