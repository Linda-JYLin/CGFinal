#version 330 core
out vec4 FragColor;

in vec2 TexCoords;
in vec3 Normal;

uniform sampler2D texture_diffuse1;
uniform vec3 Material_baseColor; // 确保拼写为 Material
uniform bool hasTexture;

void main()
{    
    vec3 result;
    vec3 lightDir = normalize(vec3(0.5, 1.0, 0.5)); // 调整光源方向
    vec3 norm = normalize(Normal);

    if(hasTexture) {
        vec4 texColor = texture(texture_diffuse1, TexCoords);
        
        // 关键逻辑：如果贴图是透明的（Alpha通道），混合材质基础色
        // 这样 Logo 之外的地方才会显示迈凯伦橙
        result = mix(Material_baseColor, texColor.rgb, texColor.a);
    } else {
        result = Material_baseColor;
    }

    // 增强环境光，PBR 模型在普通着色器下需要更多的光
    vec3 ambient = 0.5 * result;
    
    // 漫反射
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * result;

    // 简单的高光（让赛车有金属质感）
    vec3 viewDir = normalize(vec3(0.0, 0.0, 1.0)); // 简化视线方向
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specular = 0.2 * spec * vec3(1.0); 

    FragColor = vec4(ambient + diffuse + specular, 1.0);
}