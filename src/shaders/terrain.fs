#version 330 core

out vec4 FragColor;

in VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoords;
} fs_in;

// ================== 纹理 ==================
uniform sampler2D grassTex;
uniform sampler2D rockTex;
uniform sampler2D snowTex;
uniform sampler2D noiseTex;

// ================== 参数 ==================
uniform float uvScale = 32.0;

// 高度阈值（世界单位）
uniform float grassMaxHeight = 200.0;
uniform float snowMinHeight  = 600.0;

// 坡度阈值
uniform float rockSlopeStart = 0.6;
uniform float rockSlopeEnd   = 0.85;

// 噪声
uniform float noiseScale    = 0.05; // 世界尺度
uniform float noiseStrength = 60.0; // 高度扰动（米）

// 光照（基础）
uniform vec3 lightDir = normalize(vec3(-0.3, -1.0, -0.4));
uniform vec3 lightColor = vec3(1.0);

void main()
{
    // ---------- 法线与坡度 ----------
    vec3 N = normalize(fs_in.Normal);
    float slope = 1.0 - dot(N, vec3(0.0, 1.0, 0.0)); 
    // slope: 0 = 平地, 1 = 垂直

    // ---------- 高度 ----------
    vec2 noiseUV = fs_in.FragPos.xz * noiseScale;
    float noise = texture(noiseTex, noiseUV).r * 2.0 - 1.0;

    float h = fs_in.FragPos.y + noise * noiseStrength;

    // ---------- UV ----------
    vec2 uv = fs_in.TexCoords * uvScale;

    vec4 grass = texture(grassTex, uv);
    vec4 rock  = texture(rockTex,  uv);
    vec4 snow  = texture(snowTex,  uv);

    // ---------- 高度权重 ----------
    float grassH = 1.0 - smoothstep(
        grassMaxHeight * 0.8,
        grassMaxHeight,
        h
    );

    float snowH = smoothstep(
        snowMinHeight,
        snowMinHeight * 1.2,
        h
    );

    // ---------- 坡度权重（岩石） ----------
    float rockS = smoothstep(rockSlopeStart, rockSlopeEnd, slope);

    // ---------- 混合 ----------
    vec4 baseColor = mix(grass, rock, rockS);
    baseColor = mix(baseColor, snow, snowH);

    // ---------- 光照（Lambert） ----------
    float diff = max(dot(N, -lightDir), 0.0);
    vec3 lighting = diff * lightColor;

    FragColor = vec4(baseColor.rgb * lighting, 1.0);
}
