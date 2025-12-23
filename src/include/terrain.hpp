#ifndef TERRAIN_H
#define TERRAIN_H

#include "include/shader.hpp"

// ==================== 顶点结构 ====================
struct TerrainVertex {
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 TexCoords;
};

class Terrain {
public:
    Terrain(
        const std::string& heightmapPath,
        float heightScale = 1800.0f,  // 最大海拔
        float gridScale = 1.0f  // 每个像素代表多少米
    )
        : heightScale(heightScale),
        gridScale(gridScale)
    {
        loadHeightmap(heightmapPath);

        // 计算地形半尺寸（用于 XZ 居中）
        halfWidth = (width - 1) * gridScale * 0.5f;
        halfHeight = (height - 1) * gridScale * 0.5f;

        buildMesh();
        setupMesh();
    }

    void Draw(Shader& shader) {
        glBindVertexArray(VAO);
        glDrawElements(
            GL_TRIANGLES,
            static_cast<unsigned int>(indices.size()),
            GL_UNSIGNED_INT,
            0
        );
        glBindVertexArray(0);
    }

    // ==================== 世界坐标高度查询 ====================
    float getHeightWorld(float worldX, float worldZ) const {
        // 世界坐标 → heightmap 网格坐标
        float gridX = (worldX + halfWidth) / gridScale;
        float gridZ = (worldZ + halfHeight) / gridScale;

        if (gridX < 0.0f || gridZ < 0.0f ||
            gridX > width - 1 || gridZ > height - 1)
        {
            return 0.0f; // 或你需要的默认值
        }

        int x0 = static_cast<int>(floor(gridX));
        int z0 = static_cast<int>(floor(gridZ));
        int x1 = std::min(x0 + 1, width - 1);
        int z1 = std::min(z0 + 1, height - 1);

        float sx = gridX - x0;
        float sz = gridZ - z0;

        float h00 = getHeightGrid(x0, z0);
        float h10 = getHeightGrid(x1, z0);
        float h01 = getHeightGrid(x0, z1);
        float h11 = getHeightGrid(x1, z1);

        float h0 = glm::mix(h00, h10, sx);
        float h1 = glm::mix(h01, h11, sx);
        return glm::mix(h0, h1, sz);
    }

private:
    // ==================== 数据 ====================
    std::vector<TerrainVertex> vertices;
    std::vector<unsigned int> indices;

    unsigned int VAO = 0, VBO = 0, EBO = 0;

    int width = 0;
    int height = 0;
    int channels = 0;

    float heightScale;
    float gridScale;

    float halfWidth = 0.0f;
    float halfHeight = 0.0f;

    std::vector<float> heightData;

    // ==================== 高度图加载 ====================
    void loadHeightmap(const std::string& path) {
        unsigned short* data = stbi_load_16(
            path.c_str(),
            &width,
            &height,
            &channels,
            1   // 强制单通道
        );

        if (!data) {
            std::cerr << "Failed to load heightmap: " << path << std::endl;
            return;
        }

        heightData.resize(width * height);

        for (int z = 0; z < height; ++z) {
            for (int x = 0; x < width; ++x) {
                int index = z * width + x;
                heightData[index] = data[index] / 65535.0f;
            }
        }

        stbi_image_free(data);
    }


    // ==================== 构建网格（XZ 居中） ====================
    void buildMesh() {
        vertices.resize(width * height);

        for (int z = 0; z < height; ++z) {
            for (int x = 0; x < width; ++x) {
                int index = z * width + x;

                float y = heightData[index] * heightScale;

                TerrainVertex v;
                v.Position = glm::vec3(
                    x * gridScale - halfWidth,
                    y,
                    z * gridScale - halfHeight
                );

                v.TexCoords = glm::vec2(
                    static_cast<float>(x) / (width - 1),
                    static_cast<float>(z) / (height - 1)
                );

                v.Normal = calculateNormal(x, z);
                vertices[index] = v;
            }
        }

        for (int z = 0; z < height - 1; ++z) {
            for (int x = 0; x < width - 1; ++x) {
                int tl = z * width + x;
                int tr = tl + 1;
                int bl = (z + 1) * width + x;
                int br = bl + 1;

                indices.push_back(tl);
                indices.push_back(bl);
                indices.push_back(tr);

                indices.push_back(tr);
                indices.push_back(bl);
                indices.push_back(br);

            }
        }
    }

    // ==================== 法线计算（grid 坐标） ====================
    glm::vec3 calculateNormal(int x, int z) {
        float hl = getHeightGrid(x - 1, z);
        float hr = getHeightGrid(x + 1, z);
        float hd = getHeightGrid(x, z - 1);
        float hu = getHeightGrid(x, z + 1);

        glm::vec3 normal(hl - hr, 2.0f, hd - hu);
        return glm::normalize(normal);
    }

    // ==================== 内部：heightmap 网格高度 ====================
    float getHeightGrid(int x, int z) const {
        x = glm::clamp(x, 0, width - 1);
        z = glm::clamp(z, 0, height - 1);
        return heightData[z * width + x] * heightScale;
    }

    // ==================== OpenGL ====================
    void setupMesh() {
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        glBindVertexArray(VAO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER,
            vertices.size() * sizeof(TerrainVertex),
            vertices.data(),
            GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,
            indices.size() * sizeof(unsigned int),
            indices.data(),
            GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
            sizeof(TerrainVertex), (void*)offsetof(TerrainVertex, Position));

        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE,
            sizeof(TerrainVertex), (void*)offsetof(TerrainVertex, Normal));

        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE,
            sizeof(TerrainVertex), (void*)offsetof(TerrainVertex, TexCoords));

        glBindVertexArray(0);
    }
};

#endif
