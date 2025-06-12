#version 330 core

in vec3 skyCoord;
out vec4 FragColor;

void main()
{
    // Normalize y coordinate to 0-1 range (0 = bottom, 1 = top)
    float t = clamp((skyCoord.y + 1.0) * 0.5, 0.0, 1.0);

    // Sky gradient colors
    // Bottom: #87CEEB (Sky Blue) = (0.529, 0.808, 0.922)
    // Top: #E0F6FF (Light Sky Blue) = (0.878, 0.965, 1.0)
    vec3 bottomColor = vec3(0.529, 0.808, 0.922);
    vec3 topColor = vec3(0.878, 0.965, 1.0);

    // Linear interpolation between bottom and top colors
    vec3 skyColor = mix(bottomColor, topColor, t);

    FragColor = vec4(skyColor, 1.0);
}
