#version 330 core

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;

out vec4 FragColor;

uniform sampler2D blockTexture;
uniform vec3 lightDirection;
uniform vec3 lightColor;
uniform vec3 ambientColor;

void main()
{
    // Sample texture
    vec4 texColor = texture(blockTexture, TexCoord);

    // Basic directional lighting
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(-lightDirection);
    float diff = max(dot(norm, lightDir), 0.0);

    // Combine ambient and diffuse lighting
    vec3 ambient = ambientColor;
    vec3 diffuse = diff * lightColor;
    vec3 lighting = ambient + diffuse;

    // Apply lighting to texture
    vec3 finalColor = lighting * texColor.rgb;

    // Water transparency: detect water blocks by their blue color
    // Water texture is primarily blue (texture coordinates 3,0 in the atlas)
    float waterThreshold = 0.6; // Threshold for detecting blue water texture
    bool isWater = (texColor.b > waterThreshold && texColor.b > texColor.r && texColor.b > texColor.g);

    float alpha = texColor.a;
    if (isWater) {
        alpha = 0.7; // Make water 70% opaque (30% transparent)
    }

    FragColor = vec4(finalColor, alpha);
}
