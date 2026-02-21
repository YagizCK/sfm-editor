#version 460 core

flat in int v_PointID;

out vec4 FragColor;

void main() {
    vec2 temp = gl_PointCoord - vec2(0.5);
    if (dot(temp, temp) > 0.25) discard;

    int id = v_PointID + 1;
    
    float r = float(id & 0x0000FF) / 255.0;
    float g = float((id >> 8) & 0x0000FF) / 255.0;
    float b = float((id >> 16) & 0x0000FF) / 255.0;

    FragColor = vec4(r, g, b, 1.0);
}
