#version 330 core

layout(location = 0) in vec3 aPos;

out vec3 skyCoord;

void main()
{
    // For fullscreen quad, pass position directly (already in NDC space)
    gl_Position = vec4(aPos, 1.0);

    // Use position for gradient calculation
    skyCoord = aPos;
}
