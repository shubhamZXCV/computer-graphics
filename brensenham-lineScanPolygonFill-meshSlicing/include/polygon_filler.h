#ifndef POLYGON_FILLER_H
#define POLYGON_FILLER_H

#include <vector>
#include <algorithm>
#include <iostream>
#include <GL/glew.h>
#include "line_rasterizer.h"

using namespace std;

// Structure to represent an edge for scan-line algorithm with integer-only arithmetic
struct Edge {
    int ymin;     // Minimum y coordinate
    int ymax;     // Maximum y coordinate
    int x;        // Current x coordinate (multiplied by precision factor)
    int dx;       // Change in x (∆x)
    int dy;       // Change in y (∆y)
    int counter;  // Error counter
    
    // Constructor using Bresenham-like approach
    Edge(int y1, int y2, int x1, int x2) {
        // Ensure y1 <= y2
        if (y1 <= y2) {
            ymin = y1;
            ymax = y2;
            x = x1 << 8; // Scale for precision (8-bit fractional part)
            dx = x2 - x1;
            dy = y2 - y1;
        } else {
            ymin = y2;
            ymax = y1;
            x = x2 << 8; // Scale for precision
            dx = x1 - x2;
            dy = y1 - y2;
        }
        
        // Initialize counter
        counter = 0;
    }
    
    // Get current x-coordinate (in screen space)
    int getCurrentX() const {
        return (x + 128) >> 8; // Add half for rounding
    }
    
    // Update x for the next scan line using Bresenham approach
    void updateForNextScanline() {
        counter += abs(dx);
        if (counter >= dy) {
            int steps = counter / dy;
            counter %= dy;
            
            // Increment or decrement x based on direction
            if (dx >= 0) {
                x += steps << 8;
            } else {
                x -= steps << 8;
            }
        }
    }
    
    // Comparator for sorting edges by ymin, then by x
    bool operator<(const Edge& other) const {
        if (ymin != other.ymin)
            return ymin < other.ymin;
        return x < other.x;
    }
};

class PolygonFiller {
public:
    // Fill a polygon defined by its vertices
    static std::vector<PixelPoint> fillPolygon(const std::vector<ImVec2>& vertices, int width, int height) {
        for(auto pixel : vertices) {
            cout << "Pixel: " << pixel.x << ", " << pixel.y << endl;
        }
        
        if (vertices.size() < 3)
            return {};  // Need at least 3 vertices to form a polygon
        
        std::vector<PixelPoint> filledPixels;
        std::vector<Edge> allEdges;
        
        // Create edges from vertices
        for (size_t i = 0; i < vertices.size(); i++) {
            size_t next = (i + 1) % vertices.size();
            int y1 = static_cast<int>(vertices[i].y);
            int y2 = static_cast<int>(vertices[next].y);
            int x1 = static_cast<int>(vertices[i].x);
            int x2 = static_cast<int>(vertices[next].x);
            
            // Skip horizontal edges
            if (y1 == y2)
                continue;
            
            allEdges.push_back(Edge(y1, y2, x1, x2));
        }
        
        // Find min and max y
        int ymin = height;
        int ymax = 0;
        for (const auto& vertex : vertices) {
            ymin = std::min(ymin, static_cast<int>(vertex.y));
            ymax = std::max(ymax, static_cast<int>(vertex.y));
        }
        
        // Clip to canvas boundaries
        ymin = std::max(0, ymin);
        ymax = std::min(height - 1, ymax);
        
        // Sort edges by ymin
        std::sort(allEdges.begin(), allEdges.end());
        
        // Active Edge Table (AET) - stores active edges for current scanline
        std::vector<Edge> activeEdges;
        
        // Process each scan line
        for (int y = ymin; y <= ymax; y++) {
            // Add new edges that start at this scanline to AET
            for (size_t i = 0; i < allEdges.size(); i++) {
                if (allEdges[i].ymin == y) {
                    activeEdges.push_back(allEdges[i]);
                }
            }
            
            // Remove edges that end at previous scanline
            for (size_t i = 0; i < activeEdges.size(); i++) {
                if (activeEdges[i].ymax <= y) {
                    activeEdges.erase(activeEdges.begin() + i);
                    i--; // Adjust index after removal
                }
            }
            
            // Sort active edges by x-coordinate
            std::sort(activeEdges.begin(), activeEdges.end(), 
                [](const Edge& a, const Edge& b) {
                    return a.x < b.x;
                });
            
            // Fill pixels between pairs of intersections
            for (size_t i = 0; i < activeEdges.size(); i += 2) {
                if (i + 1 >= activeEdges.size())
                    break;
                    
                int x1 = std::max(0, activeEdges[i].getCurrentX());
                int x2 = std::min(width - 1, activeEdges[i+1].getCurrentX());
                
                // Add all pixels in this span
                for (int x = x1; x <= x2; x++) {
                    filledPixels.push_back(PixelPoint(x, y));
                }
            }
            
            // Update x-coordinates for the next scanline
            for (size_t i = 0; i < activeEdges.size(); i++) {
                activeEdges[i].updateForNextScanline();
            }
        }
        
        return filledPixels;
    }
};

#endif // POLYGON_FILLER_H