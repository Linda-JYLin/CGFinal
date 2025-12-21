#version 330 core

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

out vec4 FragColor;

uniform vec3 lightDir = normalize(vec3(-0.3, -1.0, -0.2));
uniform vec3 color    = vec3(0.4, 0.8, 0.3);

void main() {
    float diff = max(dot(normalize(Normal), -lightDir), 0.0);
    vec3 lighting = color * (0.3 + 0.7 * diff);
    FragColor = vec4(lighting, 1.0);
}
