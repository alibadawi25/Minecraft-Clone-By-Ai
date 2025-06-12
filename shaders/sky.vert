#version 330 core

layout(location = 0) in vec3 aPos;

uniform mat4 view;
uniform mat4 projection;

out vec3 skyCoord;

void main()
{
    // Remove translation from view matrix to keep sky at infinity
    mat4 skyView = mat4(mat3(view));
    
    vec4 pos = projection * skyView * vec4(aPos, 1.0);
    gl_Position = pos.xyww; // Set z to w to ensure sky is always at far plane
    
    skyCoord = aPos;
}
