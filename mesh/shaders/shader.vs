#version 330

layout (location = 0) in vec3 Position;
layout (location = 1) in vec3 Normal;

uniform mat4 gModel;
uniform mat4 gView;

out vec3 GS_FragPos;
out vec3 GS_FragNormal;
out float GS_FragDepth;

void main()
{
    // Calculate position in world space (but don't apply gWorld yet - geometry shader will do that)
    vec4 worldPos = gModel * vec4(Position, 1.0);
    GS_FragPos = worldPos.xyz;
    
    // Transform normal to world space
    mat3 normalMatrix = transpose(inverse(mat3(gModel)));
    GS_FragNormal = normalize(normalMatrix * Normal);
    
    // Calculate depth (distance from camera)
    vec4 viewPos = gView * worldPos;
    GS_FragDepth = -viewPos.z;
    
    // Note: We don't set gl_Position here as the geometry shader will handle it
}