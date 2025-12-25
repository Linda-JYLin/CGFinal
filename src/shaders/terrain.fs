#version 330 core
out vec4 FragColor;

in VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoords;
} fs_in;

// ========== 纹理 ==========
uniform sampler2D grassTex;
uniform sampler2D snowTex;
uniform sampler2D noiseTex;

// ========== 参数 ==========
uniform float uvScale;


// 低于 grassMaxHeight -> 纯草地
// 高于 snowMinHeight  -> 纯雪地
// 中间区域          -> 草地过渡到雪地
uniform float grassMaxHeight; 
uniform float snowMinHeight;

uniform float noiseScale;
uniform float noiseStrength;

// 光照
uniform vec3 lightDir;
uniform vec3 lightColor;

void main()
{
    // ---------- 法线 ----------
    vec3 N = normalize(fs_in.Normal);
    

    // ---------- 高度 + 噪声 ----------
    float noise = texture(noiseTex, fs_in.FragPos.xz * noiseScale).r;
    noise = noise * 2.0 - 1.0; // 映射到 -1 到 1

    float h = fs_in.FragPos.y + noise * noiseStrength;

    // ---------- 高度权重 (混合因子) ----------
    // smoothstep 返回 0.0 到 1.0 之间的值
    // 如果 h < grassMaxHeight，返回 0.0 (全草)
    // 如果 h > snowMinHeight， 返回 1.0 (全雪)
    float snowFactor = smoothstep(grassMaxHeight, snowMinHeight, h);

    // ---------- 采样 ----------
    vec2 uv = fs_in.TexCoords * uvScale;

    vec4 grass = texture(grassTex, uv);
    vec4 snow  = texture(snowTex,  uv);
    

    // ---------- 混合 ----------
    vec4 finalColor = mix(grass, snow, snowFactor);

    // ---------- 光照 ----------
    float diff = max(dot(N, -normalize(lightDir)), 0.0);
    vec3 lighting = diff * lightColor;

    // 加上环境光(Ambient)防止背光面全黑:
    vec3 ambient = 0.1 * lightColor;
    FragColor = vec4(finalColor.rgb * (lighting + ambient), 1.0);
}