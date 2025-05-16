#version 330 core

in vec3 FragPos;
in vec3 FragNormal;
in float Visibility;
in vec3 SliceColor;

out vec4 FragColor;

void main()
{
    // Discard completely invisible fragments
    if (Visibility <= 0.0) {
        discard;
    }
    
    // If this is a slice face, use the special slice color
    if (length(SliceColor) > 0.0) {
        // Add lighting to the slice color
        vec3 lightDir = normalize(vec3(1.0, 1.0, 1.0));
        vec3 normal = normalize(FragNormal);
        float diff = max(dot(normal, lightDir), 0.0);
        vec3 diffuse = diff * SliceColor;
        vec3 ambient = SliceColor * 0.3;
        FragColor = vec4(ambient + diffuse, 1.0);
        return;
    }
    
    // Normal lighting for regular surfaces
    vec3 lightDir = normalize(vec3(1.0, 1.0, 1.0));
    vec3 normal = normalize(FragNormal);
    
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diff * vec3(0.8, 0.8, 0.8);
    vec3 ambient = vec3(0.2, 0.2, 0.2);
    
    vec3 result = (ambient + diffuse);
    FragColor = vec4(result, 1.0);
}