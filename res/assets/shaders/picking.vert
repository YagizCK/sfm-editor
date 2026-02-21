#version 460 core

layout (location = 0) in vec3 aPos;

uniform mat4 u_ViewProjection;
uniform float u_PointSize;

flat out int v_PointID; 

void main() {
    v_PointID = gl_VertexID;
    gl_PointSize = u_PointSize; 
    gl_Position = u_ViewProjection * vec4(aPos, 1.0);
}
