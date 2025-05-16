#version 330 core

layout (location = 0) in vec3 Position;
layout (location = 1) in vec3 Normal;

uniform mat4 gWorld;

out vec3 VS_FragPos;
out vec3 VS_FragNormal;
out float VS_Visibility;

void main()
{
    // Transform vertex position
    vec4 worldPos = gWorld * vec4(Position, 1.0);
    VS_FragPos = worldPos.xyz;
    
    // Transform normal
    VS_FragNormal = mat3(transpose(inverse(gWorld))) * Normal;
    
    // All vertices start as visible
    VS_Visibility = 1.0;
    
    // Output transformed position
    gl_Position = worldPos;
}