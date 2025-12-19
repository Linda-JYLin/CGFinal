#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

struct Light {
    vec3 position;
    vec3 color;
};

uniform Light light;
uniform vec3 viewPos;
uniform sampler2D texture_diffuse1; // 来自 Assimp 的漫反射贴图

void main() {
    // 1. 从纹理获取基础颜色（漫反射底色）
    vec3 textureColor = texture(texture_diffuse1, TexCoords).rgb;

    // 2. 环境光
    vec3 ambient = 0.1 * light.color * textureColor;

    // 3. 漫反射
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(light.position - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * light.color * textureColor;

    // 4. 镜面反射（通常不乘纹理颜色，因为高光是光源属性）
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 16);
    vec3 specular = spec * light.color; // 注意：这里没乘 textureColor

    // 5. 合成最终颜色
    vec3 result = ambient + diffuse + specular;
    FragColor = vec4(result, 1.0);

}