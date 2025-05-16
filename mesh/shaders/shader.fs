#version 330

in vec3 FragPos;
in vec3 FragNormal;
in float FragDepth;

out vec4 FragColor;

// Light properties
#define MAX_LIGHTS 3
uniform vec3 lightPositions[MAX_LIGHTS];
uniform vec3 lightColors[MAX_LIGHTS];
uniform float lightIntensities[MAX_LIGHTS];
uniform bool lightEnabled[MAX_LIGHTS];
uniform int numActiveLights;

// Material properties
uniform vec3 viewPos;           // Camera position for specular
uniform float ambientStrength;  // Ambient light strength
uniform float specularStrength; // Specular reflection strength
uniform float shininess;        // Shininess factor for specular
uniform bool useBlinnPhong;     // Toggle Blinn-Phong vs Phong
uniform bool useDepthColor;     // Toggle depth coloring
uniform float minDepth;         // Min depth for color mapping
uniform float maxDepth;         // Max depth for color mapping

void main()
{
    // Calculate depth color
    vec3 depthColor;
    if (useDepthColor) {
        float normalizedDepth = clamp((FragDepth - minDepth) / (maxDepth - minDepth), 0.0, 1.0);
        // Use blue to red color gradient based on depth
        depthColor = mix(vec3(0.0, 0.0, 1.0), vec3(1.0, 0.0, 0.0), normalizedDepth);
    } else {
        depthColor = vec3(1.0); // White if not using depth coloring
    }
    
    // Normalize the normal
    vec3 normal = normalize(FragNormal);
    
    // Ambient component - always present
    vec3 ambient = ambientStrength * vec3(1.0, 1.0, 1.0);
    
    // Accumulate lighting from all active lights
    vec3 lighting = ambient;
    
    for (int i = 0; i < numActiveLights; i++) {
        if (lightEnabled[i]) {
            // Calculate light direction from fragment to light
            vec3 lightDir = normalize(lightPositions[i] - FragPos);
            
            // Diffuse component
            float diff = max(dot(normal, lightDir), 0.0);
            vec3 diffuse = diff * lightColors[i] * lightIntensities[i];
            
            // Specular component
            float spec = 0.0;
            
            // View direction (from fragment to camera)
            vec3 viewDir = normalize(viewPos - FragPos);
            
            if (useBlinnPhong) {
                // Blinn-Phong: use halfway vector between view and light
                vec3 halfway = normalize(lightDir + viewDir);
                spec = pow(max(dot(normal, halfway), 0.0), shininess * 4);  // *4 to make Blinn-Phong consistent with Phong
            } else {
                // Original Phong: use reflection vector
                vec3 reflectDir = reflect(-lightDir, normal);
                spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
            }
            
            // Add specular component
            vec3 specular = specularStrength * spec * lightColors[i] * lightIntensities[i];
            
            // Add this light's contribution
            lighting += diffuse + specular;
        }
    }
    
    // Combine lighting with depth color
    vec3 finalColor = lighting * depthColor;
    
    // Ensure color doesn't exceed 1.0
    finalColor = min(finalColor, vec3(1.0));
    
    FragColor = vec4(finalColor, 1.0);
}