#pragma once
#include <string>
#include "../shader.hpp"
#include "terrainSystem.hpp"


unsigned int loadTexture2D(const std::string& path)
{
    unsigned int texID;
    glGenTextures(1, &texID);

    int w, h, ch;
    unsigned char* data = stbi_load(path.c_str(), &w, &h, &ch, 0);

    GLenum format = (ch == 1) ? GL_RED :
        (ch == 3) ? GL_RGB : GL_RGBA;

    glBindTexture(GL_TEXTURE_2D, texID);
    glTexImage2D(GL_TEXTURE_2D, 0, format, w, h, 0, format,
        GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_image_free(data);
    return texID;
}

class Terrain {
public:
    Terrain(
        const std::string& heightmapPath,
        float heightScale,
        int chunkCountX, int chunkCountZ, int chunkSize, float gridScale
    );

    ~Terrain();

    void setLODDistances(float d0, float d1, float d2);
    void render(const glm::mat4& view, const glm::mat4& projection, const glm::vec3& cameraPos);

    float getHeightWorld(float worldX, float worldZ) const;     // 世界高度查询: 输入世界坐标(x, z)，输出世界高度y
    glm::vec3 getNormalWorld(float worldX, float worldZ) const; // 世界法线查询: 输入世界坐标(x, z)，输出该点的法线
    float getSlopeRadians(float worldX, float worldZ) const;    // 世界坡度查询: 输入世界坐标(x, z)，输出该点的坡度（弧度）
    float getSlopeDegrees(float worldX, float worldZ) const;    // 世界坡度查询: 输入世界坐标(x, z)，输出该点的坡度（角度）

private:
    void loadTextures();
    void setupShader();

    Heightmap heightmap;
    TerrainSystem terrainSystem;
    Shader terrainShader;

    unsigned int grassTex = 0;
    unsigned int rockTex = 0;
    unsigned int snowTex = 0;
    unsigned int noiseTex = 0;

    // Uniform parameters (可设为成员变量或外部传入)
    float uvScale = 32.0f;
    float grassMaxHeight = 200.0f;
    float snowMinHeight = 600.0f;
    float rockSlopeStart = 0.6f;
    float rockSlopeEnd = 0.85f;
    float noiseScale = 0.05f;
    float noiseStrength = 60.0f;
};

Terrain::Terrain(
    const std::string& heightmapPath,
    float heightScale,
    int chunkCountX, int chunkCountZ, int chunkSize, float gridScale
)
    : heightmap(heightmapPath, heightScale),
    terrainSystem(heightmap, chunkCountX, chunkCountZ, chunkSize, gridScale),
    terrainShader(SHADERS_FOLDER "terrain.vs", SHADERS_FOLDER "terrain.fs")
{
    loadTextures();
    setupShader();
    std::cout << "heightmap w,h = " << heightmap.width << "," << heightmap.height << "\n";
    std::cout << "terrain world size approx = "
        << (8 * (33 - 1) * 1.0f) << " x " << (8 * (33 - 1) * 1.0f) << "\n";
}

void Terrain::loadTextures() {
    // ------------- 加载地形纹理 ---------------------
    grassTex = loadTexture2D(ASSETS_FOLDER "terrain/grass_diff.png");
    rockTex = loadTexture2D(ASSETS_FOLDER "terrain/rock_diff.png");
    snowTex = loadTexture2D(ASSETS_FOLDER "terrain/snow_diff.png");
    noiseTex = loadTexture2D(ASSETS_FOLDER "terrain/noise.png");
}

void Terrain::setupShader() {
    terrainShader.use();
    terrainShader.setInt("grassTex", 0);
    terrainShader.setInt("rockTex", 1);
    terrainShader.setInt("snowTex", 2);
    terrainShader.setInt("noiseTex", 3);
}

void Terrain::setLODDistances(float d0, float d1, float d2) {
    terrainSystem.setLODDistances(d0, d1, d2);
}

void Terrain::render(const glm::mat4& view, const glm::mat4& projection, const glm::vec3& cameraPos) {
    terrainShader.use();
    glm::mat4 model = glm::translate(
        glm::mat4(1.0f),
        glm::vec3(
            -0.5f * terrainSystem.getWorldSizeX(),
            0.0f,
            -0.5f * terrainSystem.getWorldSizeZ()
        )
    );
    terrainShader.setMat4("model", model);
    terrainShader.setMat4("view", view);
    terrainShader.setMat4("projection", projection);

    terrainShader.setFloat("uvScale", uvScale);
    terrainShader.setFloat("grassMaxHeight", grassMaxHeight);
    terrainShader.setFloat("snowMinHeight", snowMinHeight);
    terrainShader.setFloat("rockSlopeStart", rockSlopeStart);
    terrainShader.setFloat("rockSlopeEnd", rockSlopeEnd);
    terrainShader.setFloat("noiseScale", noiseScale);
    terrainShader.setFloat("noiseStrength", noiseStrength);

    // Bind textures
    glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, grassTex);
    glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, rockTex);
    glActiveTexture(GL_TEXTURE2); glBindTexture(GL_TEXTURE_2D, snowTex);
    glActiveTexture(GL_TEXTURE3); glBindTexture(GL_TEXTURE_2D, noiseTex);

    terrainSystem.Draw(
        terrainShader,
        model,          
        view,
        projection,
        cameraPos
    );
}

float  Terrain::getHeightWorld(float worldX, float worldZ) const {
    return terrainSystem.getHeightWorld(worldX, worldZ);
}

glm::vec3  Terrain::getNormalWorld(float worldX, float worldZ) const {
    return terrainSystem.getNormalWorld(worldX, worldZ);
}

float  Terrain::getSlopeRadians(float worldX, float worldZ) const {
    return terrainSystem.getSlopeRadians(worldX, worldZ);
}

float  Terrain::getSlopeDegrees(float worldX, float worldZ) const {
    return terrainSystem.getSlopeDegrees(worldX, worldZ);
}

// ---------------- Destructor ----------------
Terrain::~Terrain() {
    glDeleteTextures(1, &grassTex);
    glDeleteTextures(1, &rockTex);
    glDeleteTextures(1, &snowTex);
    glDeleteTextures(1, &noiseTex);
}
