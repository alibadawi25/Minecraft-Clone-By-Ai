#version 330 core

out vec4 FragColor;

void main()
{
    // Grass green color
    vec3 grassColor = vec3(0.34, 0.7, 0.34);
    FragColor = vec4(grassColor, 1.0);
}
