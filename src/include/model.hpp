#include "include/shader.hpp"
#include <map>

// ———————— 网格类（Mesh） ————————
// 顶点类
struct Vertex {
    glm::vec3 Position; // 位置
    glm::vec3 Normal;   // 法线
    glm::vec2 TexCoords;
};

// 纹理类
struct Texture {
    unsigned int id;
    std::string type; 
    aiString path;
};

class Mesh {
public:
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<Texture> textures;
    unsigned int VAO, VBO, EBO;

    Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices, std::vector<Texture> textures)
        : vertices(std::move(vertices)), indices(std::move(indices)), textures(std::move(textures)) {
        setupMesh();
    }

    void Draw(Shader& shader) {
        unsigned int diffuseNr = 1;

        // 绑定纹理
        for (unsigned int i = 0; i < textures.size(); i++) {
            glActiveTexture(GL_TEXTURE0 + i); // 激活相应的纹理单元
            std::string number;
            std::string name = textures[i].type;
            if (name == "texture_diffuse") number = std::to_string(diffuseNr++);

            shader.setInt((name + number).c_str(), i);
            glBindTexture(GL_TEXTURE_2D, textures[i].id);
        }

        // 绘制 Mesh
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, static_cast<unsigned int>(indices.size()), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        // 重置/清理状态
        glActiveTexture(GL_TEXTURE0);
    }

private:
    void setupMesh() {
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

        // position
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
        // normal
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));
        // tex coords
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));

        glBindVertexArray(0);
    }
};

// ———————— Model 类（使用 Assimp） ————————

unsigned int TextureFromFile(const char* path, const std::string& directory) {
    std::string filename = std::string(path);
    filename = directory + "/" + filename; // 拼接完整路径

    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    // 注意：stbi_load 不支持 .etc_x.png 这种非标准的文件名，它只看文件内容。
    // 如果您的纹理是 ETC 压缩格式并使用 stb_image 加载，可能会失败。
    // 但对于标准 PNG，这是可行的。
    unsigned char* data = stbi_load(filename.c_str(), &width, &height, &nrComponents, 0);
    if (data) {
        GLenum format;
        if (nrComponents == 1) { format = GL_RED; } //std::cerr << "nrComponents == 1" << filename << std::endl; }
        else if (nrComponents == 3) { format = GL_RGB; }// std::cerr << "nrComponents == 3" << filename << std::endl; }
        else if (nrComponents == 4) { format = GL_RGBA; }//std::cerr << "nrComponents == 4" << filename << std::endl;}
        else {
            std::cerr << "Texture format not supported for " << filename << std::endl;
            stbi_image_free(data);
            return 0;
        }

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        // 设置纹理参数
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else {
        std::cerr << "Texture failed to load at path: " << filename << std::endl;
        stbi_image_free(data);
        return 0;
    }

    return textureID;
}


class Model {
public:
    std::vector<Texture> textures_loaded;
    std::vector<Mesh> meshes;
    std::string directory;
    std::map<std::string, std::string> textureMap;
    std::string TEXTURES_DIR;

    Model(const std::string& path, const std::string& texture_path, std::map<std::string, std::string> textureMap) : TEXTURES_DIR(texture_path), textureMap(std::move(textureMap)) {
        loadModel(path);
    }

    void Draw(Shader& shader) {
        for (unsigned int i = 0; i < meshes.size(); i++)
            meshes[i].Draw(shader);
    }

private:
    void loadModel(const std::string& path) {
        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_CalcTangentSpace | aiProcess_PreTransformVertices); 

        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
            std::cout << "Assimp error: " << importer.GetErrorString() << std::endl;
            return;
        }

        directory = path.substr(0, path.find_last_of('/'));
        processNode(scene->mRootNode, scene);
    }

    void processNode(aiNode* node, const aiScene* scene) {
        for (unsigned int i = 0; i < node->mNumMeshes; i++) {
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            meshes.push_back(processMesh(mesh, scene));
        }
        for (unsigned int i = 0; i < node->mNumChildren; i++) {
            processNode(node->mChildren[i], scene);
        }
    }

    Mesh processMesh(aiMesh* mesh, const aiScene* scene) {
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;
        std::vector<Texture> textures;

        // 处理顶点
        for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
            Vertex vertex;
            // 位置
            vertex.Position = glm::vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);
            // 法线
            if (mesh->mNormals)
                vertex.Normal = glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
            else
                vertex.Normal = glm::vec3(0.0f, 1.0f, 0.0f);
            // 纹理坐标 (仅处理第一组 UV)
            if (mesh->mTextureCoords[0]) {
                vertex.TexCoords = glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);
            }
            else {
                vertex.TexCoords = glm::vec2(0.0f, 0.0f);
            }
            vertices.push_back(vertex);
        }

        // 处理索引
        for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
            aiFace face = mesh->mFaces[i];
            for (unsigned int j = 0; j < face.mNumIndices; j++)
                indices.push_back(face.mIndices[j]);
        }

        // 处理材质
        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

        // 我们只关注漫反射贴图 (Diffuse Maps)
        std::vector<Texture> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse");
        textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());

        return Mesh(vertices, indices, textures);
    }

    std::vector<Texture> loadMaterialTextures(aiMaterial* mat, aiTextureType type, std::string typeName) {
        std::vector<Texture> textures;
        for (unsigned int i = 0; i < mat->GetTextureCount(type); i++) {
            aiString str;
            mat->GetTexture(type, i, &str);
            std::string textureKey = str.C_Str();

            // Step 1: 检查是否已加载
            bool skip = false;
            for (const auto& loadedTex : textures_loaded) {
                if (loadedTex.path == str) {
                    textures.push_back(loadedTex);
                    skip = true;
                    break;
                }
            }
            if (skip) continue;

            Texture texture;
            texture.type = typeName;
            texture.path = str; // 保留原始路径用于查重

            std::string actualFilename;

            // Step 2: 判断是内部索引还是外部文件
            if (textureKey.rfind("*", 0) == 0) { //内部索引
                if (textureMap.find(textureKey) != textureMap.end()) {
                    actualFilename = textureMap[textureKey]; // 从用户提供的映射表获取真实文件名
                }
                else {
                    std::cerr << "Error: No mapping found for internal texture: " << textureKey << std::endl;
                    continue;
                }
            }
            else {
                // 普通外部文件路径（如 "Cat_diffuse.jpg"）
                actualFilename = textureKey;
            }

            // Step 3: 加载纹理
            unsigned int textureID = TextureFromFile(actualFilename.c_str(), TEXTURES_DIR);
            if (textureID == 0) {
                std::cerr << "Failed to load texture: " << actualFilename << " from dir: " << TEXTURES_DIR << std::endl;
                continue;
            }

            texture.id = textureID;
            textures.push_back(texture);
            textures_loaded.push_back(texture);
        }
        return textures;
    }
};