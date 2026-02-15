#version 460 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;

out vec3 vColor;

uniform mat4 u_ViewProjection;

void main() {
    gl_Position = u_ViewProjection * vec4(aPos, 1.0);
    vColor = aColor;
    
    gl_PointSize = 4.0; 
}
