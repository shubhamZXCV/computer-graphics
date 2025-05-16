#version 330 core

layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

in vec3 GS_FragPos[];
in vec3 GS_FragNormal[];
in float GS_FragDepth[];

out vec3 FragPos;
out vec3 FragNormal;
out float FragDepth;

uniform mat4 gWorld;  // Need this to transform the offset vertices
uniform float explosionFactor;
uniform bool isExploded;

void main() {
    // Calculate the center of the triangle for explosion direction
    vec3 center = (GS_FragPos[0] + GS_FragPos[1] + GS_FragPos[2]) / 3.0;
    
    // Apply explosion effect if enabled
    if (isExploded && explosionFactor > 0.0) {
        // Calculate explosion direction (from origin to center)
        vec3 explosion_dir = normalize(center);
        
        // Apply the explosion offset to each vertex
        vec3 offset = explosion_dir * explosionFactor;
        
        // Output transformed vertices with offset
        for (int i = 0; i < 3; i++) {
            // Apply offset to position and transform with world matrix
            gl_Position = gWorld * vec4(GS_FragPos[i] + offset, 1.0);
            FragPos = GS_FragPos[i] + offset;
            FragNormal = GS_FragNormal[i];
            FragDepth = GS_FragDepth[i];
            EmitVertex();
        }
    } else {
        // No explosion, just pass through data
        for (int i = 0; i < 3; i++) {
            gl_Position = gWorld * vec4(GS_FragPos[i], 1.0);
            FragPos = GS_FragPos[i];
            FragNormal = GS_FragNormal[i];
            FragDepth = GS_FragDepth[i];
            EmitVertex();
        }
    }
    
    EndPrimitive();
}