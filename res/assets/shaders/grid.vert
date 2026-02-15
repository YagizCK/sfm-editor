#version 460 core

layout (location = 0) in vec3 aPos;

uniform mat4 u_Model;
uniform mat4 u_View;
uniform mat4 u_Projection;

out vec3 v_WorldPos;

void main() {
    vec4 worldPos = u_Model * vec4(aPos, 1.0);
    v_WorldPos = worldPos.xyz;
    gl_Position = u_Projection * u_View * worldPos;
}
