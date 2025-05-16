#ifndef LINE_RASTERIZER_H
#define LINE_RASTERIZER_H

#include <vector>
#include <GL/glew.h>
#include <algorithm> // For std::swap
#include <cmath>     // For std::abs

struct PixelPoint {
    int x;
    int y;
    PixelPoint(int _x, int _y) : x(_x), y(_y) {}
};

class LineRasterizer {
public:
    // Rasterize a line using Bresenham's algorithm - handling all quadrants and slopes
    static std::vector<PixelPoint> rasterizeLine(int x0, int y0, int x1, int y1) {
        std::vector<PixelPoint> pixels;
        
        // Handle special case where points are the same
        if (x0 == x1 && y0 == y1) {
            pixels.push_back(PixelPoint(x0, y0));
            return pixels;
        }
        
        // Calculate deltas
        int dx = x1 - x0;
        int dy = y1 - y0;
        
        // Handle horizontal lines
        if (dy == 0) {
            int xStart = std::min(x0, x1);
            int xEnd = std::max(x0, x1);
            for (int x = xStart; x <= xEnd; x++) {
                pixels.push_back(PixelPoint(x, y0));
            }
            return pixels;
        }
        
        // Handle vertical lines
        if (dx == 0) {
            int yStart = std::min(y0, y1);
            int yEnd = std::max(y0, y1);
            for (int y = yStart; y <= yEnd; y++) {
                pixels.push_back(PixelPoint(x0, y));
            }
            return pixels;
        }
        
        // Determine the octant/direction flags
        bool steep = std::abs(dy) > std::abs(dx);
        
        // If the line is steep, swap x and y
        if (steep) {
            std::swap(x0, y0);
            std::swap(x1, y1);
        }
        
        // Make sure we're always drawing left to right
        if (x0 > x1) {
            std::swap(x0, x1);
            std::swap(y0, y1);
        }
        
        // Recalculate deltas after potential swaps
        dx = x1 - x0;
        dy = y1 - y0;
        
        // Determine y step direction
        int yStep = (dy >= 0) ? 1 : -1;
        
        // Use absolute value of dy for calculations
        dy = std::abs(dy);
        
        // Step 1: Input two endpoints, store left one as (x0, y0) - already done
        
        // Step 2: Plot first point (x0, y0)
        if (steep) {
            pixels.push_back(PixelPoint(y0, x0)); // Swap back to original coordinate system
        } else {
            pixels.push_back(PixelPoint(x0, y0));
        }
        
        // Step 3: Calculate decision parameter d0 = 2*dy - dx
        int twoTimesDy = 2 * dy;
        int twoTimesDyMinusDx = twoTimesDy - 2 * dx;
        int d = twoTimesDy - dx;  // Initial decision parameter
        
        int y = y0;
        
        // Step 4 & 5: For each x from x0+1 to x1, decide which pixel to plot
        for (int x = x0 + 1; x <= x1; x++) {
            if (d < 0) {
                // Choose pixel at (x, y)
                d += twoTimesDy;  // Update d = d + 2*dy
            } else {
                // Choose pixel at (x, y+yStep)
                y += yStep;
                d += twoTimesDyMinusDx;  // Update d = d + 2*dy - 2*dx
            }
            
            // Plot the point, swapping x and y if the line is steep
            if (steep) {
                pixels.push_back(PixelPoint(y, x));
            } else {
                pixels.push_back(PixelPoint(x, y));
            }
        }
        
        return pixels;
    }
};

#endif // LINE_RASTERIZER_H