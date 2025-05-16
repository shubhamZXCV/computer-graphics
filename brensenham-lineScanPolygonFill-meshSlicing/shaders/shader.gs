#version 420 core

layout (triangles) in;
layout (triangle_strip, max_vertices = 72) out; // Conservative estimate

// Input from vertex shader
in vec3 VS_FragPos[];
in vec3 VS_FragNormal[];
in float VS_Visibility[];

// Output to fragment shader
out vec3 FragPos;
out vec3 FragNormal;
out float Visibility;
out vec3 SliceColor;

// Uniforms
uniform vec4 slicePlanes[4];
uniform int numPlanes;
uniform bool separateParts;
uniform float separationDistance;

// Debug counters
// uniform int debugTriangleCount;

struct Vertex {
    vec3 position;
    vec3 normal;
    float visibility;
};

Vertex findIntersection(Vertex v0, Vertex v1, vec4 plane) {
    vec3 planeNormal = normalize(plane.xyz);
    float d0 = dot(v0.position, planeNormal) + plane.w;
    float d1 = dot(v1.position, planeNormal) + plane.w;
    
    // Safe division with epsilon
    float t = clamp(d0 / (d0 - d1 + 1e-10), 0.0, 1.0);
    
    return Vertex(
        mix(v0.position, v1.position, t),
        normalize(mix(v0.normal, v1.normal, t)),
        mix(v0.visibility, v1.visibility, t)
    );
}

bool isInside(vec3 point, vec4 plane) {
    return dot(plane.xyz, point) + plane.w >= 0.0;
}

vec3 applySeparation(vec3 pos, vec4 plane, bool isPositiveSide) {
    if (!separateParts) return pos;
    vec3 normal = normalize(plane.xyz);
    return pos + normal * separationDistance * (isPositiveSide ? 1.0 : -1.0);
}

void emitVertex(Vertex v, vec3 color, bool isPositiveSide, vec4 plane) {
    FragPos = applySeparation(v.position, plane, isPositiveSide);
    FragNormal = v.normal;
    Visibility = v.visibility;
    SliceColor = color;
    gl_Position = vec4(FragPos, 1.0);
    EmitVertex();
}

void main() {
    // Early exit if no planes or all vertices invisible
    if (numPlanes == 0 || 
       (VS_Visibility[0] <= 0.0 && VS_Visibility[1] <= 0.0 && VS_Visibility[2] <= 0.0)) {
        for (int i = 0; i < 3; i++) {
            emitVertex(
                Vertex(VS_FragPos[i], VS_FragNormal[i], VS_Visibility[i]),
                vec3(0.0), true, vec4(0.0)
            );
        }
        EndPrimitive();
        return;
    }

    // Initialize with input triangle
    Vertex triangle[3] = {
        Vertex(VS_FragPos[0], VS_FragNormal[0], VS_Visibility[0]),
        Vertex(VS_FragPos[1], VS_FragNormal[1], VS_Visibility[1]),
        Vertex(VS_FragPos[2], VS_FragNormal[2], VS_Visibility[2])
    };

    // Temporary buffers for clipping
    Vertex inputList[4];
    Vertex outputList[4];
    
    // Copy initial triangle
    for (int i = 0; i < 3; i++) {
        inputList[i] = triangle[i];
    }
    int inputSize = 3;

    // Clip against each active plane
    for (int p = 0; p < min(numPlanes, 4); p++) {
        vec4 plane = slicePlanes[p];
        int outputSize = 0;
        
        for (int i = 0; i < inputSize; i++) {
            int j = (i + 1) % inputSize;
            Vertex current = inputList[i];
            Vertex next = inputList[j];
            
            bool currentInside = isInside(current.position, plane);
            bool nextInside = isInside(next.position, plane);
            
            if (currentInside) {
                outputList[outputSize++] = current;
            }
            
            if (currentInside != nextInside) {
                outputList[outputSize++] = findIntersection(current, next, plane);
            }
        }
        
        // Copy output to input for next plane
        inputSize = outputSize;
        for (int i = 0; i < inputSize; i++) {
            inputList[i] = outputList[i];
        }
        
        // Early exit if completely clipped
        if (inputSize < 3) break;
    }

    // Emit the resulting polygon (as triangle fan)
    if (inputSize >= 3) {
        bool isPositiveSide = numPlanes > 0 ? 
            isInside(inputList[0].position, slicePlanes[numPlanes-1]) : true;
            
        for (int i = 2; i < inputSize; i++) {
            emitVertex(inputList[0], vec3(0.0), isPositiveSide, 
                      numPlanes > 0 ? slicePlanes[numPlanes-1] : vec4(0.0));
            emitVertex(inputList[i-1], vec3(0.0), isPositiveSide, 
                      numPlanes > 0 ? slicePlanes[numPlanes-1] : vec4(0.0));
            emitVertex(inputList[i], vec3(0.0), isPositiveSide, 
                      numPlanes > 0 ? slicePlanes[numPlanes-1] : vec4(0.0));
            EndPrimitive();
        }
    }

    
}