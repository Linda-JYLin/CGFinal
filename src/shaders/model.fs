#version 330 core
out vec4 FragColor;

in vec2 TexCoords;
in vec3 Normal; // 必须与 .vs 中的 out 对应

uniform sampler2D texture_diffuse1;
uniform vec3 Maretial_baseColor; // 注意：检查你 C++ 里是 Maretial 还是 Material
uniform bool hasTexture;

void main()
{    
    vec3 result;
    if(hasTexture) {
        vec4 texColor = texture(texture_diffuse1, TexCoords);
        if(texColor.a < 0.1) 
            result = Maretial_baseColor;
        else
            result = texColor.rgb;
    } else {
        result = Maretial_baseColor;
    }

    // 简单的光照计算，防止模型全黑
    vec3 lightDir = normalize(vec3(1.0, 1.0, 1.0));
    float diff = max(dot(normalize(Normal), lightDir), 0.0);
    vec3 ambient = 0.3 * result;
    vec3 diffuse = diff * result;

    FragColor = vec4(ambient + diffuse, 1.0);
}