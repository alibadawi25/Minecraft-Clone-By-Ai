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
    FragColor = vec4(lighting * texColor.rgb, texColor.a);
}
