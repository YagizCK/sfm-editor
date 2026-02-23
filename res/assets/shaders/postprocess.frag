#version 460 core

in vec2 TexCoords;

uniform sampler2D u_ScreenTexture;
uniform int u_ModelId;
uniform float u_DistParams[8];
uniform vec2 u_Focal;
uniform vec2 u_Principal;
uniform vec2 u_Resolution;

out vec4 FragColor;

vec2 apply_distortion(vec2 p, int model) {
    float x = p.x; float y = p.y;
    float r2 = x*x + y*y; 
    float r4 = r2*r2;

    if (model == 6) { 
        float k1 = u_DistParams[0]; float k2 = u_DistParams[1]; 
        float p1 = u_DistParams[2]; float p2 = u_DistParams[3];
        float k3 = u_DistParams[4]; float k4 = u_DistParams[5];
        float k5 = u_DistParams[6]; float k6 = u_DistParams[7];
        
        float r_num = 1.0 + k1 * r2 + k2 * r4 + k3 * r4 * r2;
        float r_den = 1.0 + k4 * r2 + k5 * r4 + k6 * r4 * r2;
        float r_coeff = r_num / r_den;

        float x_dist = x * r_coeff + 2.0 * p1 * x * y + p2 * (r2 + 2.0 * x * x);
        float y_dist = y * r_coeff + p1 * (r2 + 2.0 * y * y) + 2.0 * p2 * x * y;
        return vec2(x_dist, y_dist);
    }
    else if (model == 2 || model == 3 || model == 4) {
        float k1 = u_DistParams[0]; float k2 = u_DistParams[1];
        float p1 = u_DistParams[2]; float p2 = u_DistParams[3];
        
        float r_coeff = 1.0 + k1 * r2 + k2 * r4;
        float x_dist = x * r_coeff + 2.0 * p1 * x * y + p2 * (r2 + 2.0 * x * x);
        float y_dist = y * r_coeff + p1 * (r2 + 2.0 * y * y) + 2.0 * p2 * x * y;
        return vec2(x_dist, y_dist);
    } 
    else if (model == 5) {
        float k1 = u_DistParams[0]; float k2 = u_DistParams[1];
        float k3 = u_DistParams[2]; float k4 = u_DistParams[3];
        
        float r = sqrt(r2);
        if (r > 1e-8) {
            float theta = atan(r);
            float theta2 = theta*theta; float theta4 = theta2*theta2; 
            float theta6 = theta4*theta2; float theta8 = theta4*theta4;
            float theta_d = theta * (1.0 + k1 * theta2 + k2 * theta4 + k3 * theta6 + k4 * theta8);
            return p * (theta_d / r);
        }
    }
    return p;
}

void main() {
    if (u_ModelId <= 1) { 
        FragColor = texture(u_ScreenTexture, TexCoords);
        return;
    }

    vec2 px = TexCoords * u_Resolution;
    vec2 p_dist = (px - u_Principal) / u_Focal;

    vec2 p_ideal = p_dist;
    for (int i = 0; i < 5; ++i) {
        vec2 p_est = apply_distortion(p_ideal, u_ModelId);
        p_ideal += (p_dist - p_est);
    }

    vec2 px_ideal = p_ideal * u_Focal + u_Principal;
    vec2 uv_ideal = px_ideal / u_Resolution;

    if (uv_ideal.x < 0.0 || uv_ideal.x > 1.0 || uv_ideal.y < 0.0 || uv_ideal.y > 1.0) {
        FragColor = vec4(0.0, 0.0, 0.0, 1.0);
    } else {
        FragColor = texture(u_ScreenTexture, uv_ideal);
    }
}
