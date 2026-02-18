#version 460 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;
layout (location = 2) in float aSelected;

uniform mat4 u_ViewProjection;
uniform float u_PointSize;

out vec3 vColor;
out float vSelected;

void main() {
    vColor = aColor;
    vSelected = aSelected;

    if (aSelected > 0.5) {
        gl_PointSize = u_PointSize * 2.5; 
    } else {
        gl_PointSize = u_PointSize;
    }

    gl_Position = u_ViewProjection * vec4(aPos, 1.0);
}
