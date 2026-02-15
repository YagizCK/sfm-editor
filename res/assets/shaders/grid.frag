#version 460 core

out vec4 FragColor;
in vec3 v_WorldPos;

uniform vec3 u_CameraPos;
uniform float u_GridSize;
uniform int u_IsLine;
uniform vec3 u_LineColor;

float drawGrid(vec2 coord, float spacing) {
    vec2 grid = abs(fract(coord / spacing - 0.5) - 0.5) / fwidth(coord / spacing);
    return 1.0 - min(min(grid.x, grid.y), 1.0);
}

void main() {
    float dist = distance(v_WorldPos, u_CameraPos);
    float fadeStart = u_GridSize * 0.2;
    float fadeEnd = u_GridSize * 0.45;
    float alpha = 1.0 - smoothstep(fadeStart, fadeEnd, dist);

    if (u_IsLine == 1) {
        FragColor = vec4(u_LineColor, alpha);
        return;
    }

    float distXZ = distance(v_WorldPos.xz, u_CameraPos.xz);
    float alphaGrid = 1.0 - smoothstep(fadeStart, fadeEnd, distXZ);

    vec4 color = vec4(0.0);
    float g1 = drawGrid(v_WorldPos.xz, 1.0);
    float g10 = drawGrid(v_WorldPos.xz, 10.0);

    if (g1 > 0.0) color = vec4(u_LineColor, 0.15);
    if (g10 > 0.0) color = vec4(u_LineColor, 0.4);

    if (color.a <= 0.0) discard;
    
    color.a *= alphaGrid;
    if (color.a < 0.01) discard;

    FragColor = color;
}
