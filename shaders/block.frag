#version 330 core

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;
in float viewDistance;

out vec4 FragColor;

uniform sampler2D blockTexture;
uniform vec3 lightDirection;
uniform vec3 lightColor;
uniform vec3 ambientColor;

// Fog uniforms
uniform float fogNear;
uniform float fogFar;
uniform vec3 fogColor;

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
    vec3 lighting = ambient + diffuse;    // Apply lighting to texture
    vec3 finalColor = lighting * texColor.rgb;

    // Water transparency: detect water blocks by texture coordinates
    // Water texture is at position (3,0) in 16x16 atlas
    // Each texture is 1/16 = 0.0625 units wide
    vec2 atlasPos = floor(TexCoord * 16.0) / 16.0;
    bool isWater = (atlasPos.x >= 0.1875 && atlasPos.x < 0.25 && atlasPos.y >= 0.0 && atlasPos.y < 0.0625);

    float alpha = texColor.a;
    if (isWater) {
        alpha = 0.7; // Make water 70% opaque (30% transparent)
    }

    // Apply fog
    float fogFactor = clamp((fogFar - viewDistance) / (fogFar - fogNear), 0.0, 1.0);
    vec3 finalColorWithFog = mix(fogColor, finalColor, fogFactor);

    FragColor = vec4(finalColorWithFog, alpha);
}
