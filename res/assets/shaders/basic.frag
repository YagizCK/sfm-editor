#version 460 core

in vec3 vColor;
in float vSelected;

out vec4 FragColor;

void main() {
    vec2 temp = gl_PointCoord - vec2(0.5);
    float dist = dot(temp, temp);

    if (dist > 0.25) {
        discard;
    }

    vec3 finalColor = vColor;

    if (vSelected > 0.5) {
        if (dist > 0.16) {
            finalColor = vec3(1.0, 0.8, 0.0);
        } else {
            finalColor = vColor * 1.2; 
        }
    }

    float alpha = smoothstep(0.25, 0.22, dist);

    FragColor = vec4(finalColor, alpha);
}
