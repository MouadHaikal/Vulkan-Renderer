#include <model.hpp>
#include <scene.hpp>

#include <logger.hpp>




Model::Model(std::variant<std::string, RawMesh*> source, Scene* parentScene) {
    if (std::holds_alternative<RawMesh*>(source)) {
        // Create single mesh model
        meshes.emplace_back(std::make_unique<Mesh>(std::get<RawMesh*>(source), &modelMatrix));
    } 
    else {
        // Load entire model and create meshes (and textures) accordingly
        loadModel((dirModels/std::get<std::string>(source)).string(), parentScene);
    }
}


void Model::setUniformMaterial(const Material& material){
    for (auto& mesh : meshes) {
        mesh->setMaterial(material);
    }
}


void Model::draw(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, const glm::vec3& cameraPos) const{
    for (const auto& mesh : meshes) {
        mesh->draw(commandBuffer, pipelineLayout, cameraPos);
    }
}


void Model::loadModel(const std::string& path, Scene* parentScene){
    Assimp::Importer importer;

    const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_JoinIdenticalVertices);
    if (!scene) { 
        LOG_ERROR("Failed to load model (" + path + ")");
        return;
    }

    std::string modelName = utils::getFileName(path);

    loadTextures(scene, parentScene, modelName);
    loadNodeRec(scene->mRootNode, scene, parentScene, modelName);

    LOG_INFO("Model loaded (" + modelName + ")");
    LOG_DEBUG(SEP);
}

void Model::loadTextures(const aiScene* scene, Scene* parentScene, const std::string& modelName){
    for (size_t i = 0; i < scene->mNumMaterials; ++i) {
        aiMaterial* material = scene->mMaterials[i];

        if (material->GetTextureCount(aiTextureType_DIFFUSE)) {
            aiString path;

            if (material->GetTexture(aiTextureType_DIFFUSE, 0, &path) == AI_SUCCESS) {
                const aiTexture* texture = scene->GetEmbeddedTexture(path.C_Str());

                if (texture) {   // Embedded texture
                    std::string textureName = modelName + path.C_Str();

                    if (texture->mHeight) {   // Not compressed
                        parentScene->textureManager.addEmbeddedTexture(textureName,
                                                                       texture->pcData, 
                                                                       texture->mWidth, texture->mHeight
                        );
                    }
                    else {    // Compressed
                        int width, height, channels;
                        stbi_uc* pixels = stbi_load_from_memory(reinterpret_cast<stbi_uc*>(texture->pcData), 
                                                                texture->mWidth, &width, &height, &channels, 
                                                                STBI_rgb_alpha
                        );

                        parentScene->textureManager.addEmbeddedTexture(textureName, 
                                                                       pixels, 
                                                                       width, height
                        );
                    }
                }
                else {    // External texture
                    parentScene->textureManager.addTexture(utils::getFileName(path.C_Str())); 
                }
            } 
        }

        if (material->GetTextureCount(aiTextureType_NORMALS)) {
            LOG_WARNING("3ndo normal map");
        }
    }
}

void Model::loadNodeRec(const aiNode* node, const aiScene* scene, Scene* parentScene, const std::string& modelName){
    for (size_t i = 0; i < node->mNumMeshes; ++i) {
        auto mesh = scene->mMeshes[node->mMeshes[i]];

        RawMesh meshData{};
        meshData.vertices.resize(mesh->mNumVertices);

        // Vertices
        for (size_t j = 0; j < mesh->mNumVertices; ++j) {
            meshData.vertices[j].pos = { mesh->mVertices[j].x, mesh->mVertices[j].y, mesh->mVertices[j].z };

            if (mesh->HasTextureCoords(0)) {
                meshData.vertices[j].texCoord = { mesh->mTextureCoords[0][j].x, mesh->mTextureCoords[0][j].y };
            }

            if (mesh->HasNormals()) {
                meshData.vertices[j].normal = { mesh->mNormals[j].x, mesh->mNormals[j].y, mesh->mNormals[j].z };
            }
        }
    
        // Indices
        for (size_t j = 0; j < mesh->mNumFaces; ++j) {
            aiFace face = mesh->mFaces[j];

            for (size_t k = 0; k < face.mNumIndices; ++k) {
                meshData.indices.push_back(face.mIndices[k]);
            }
        }        

        // Material
        if (mesh->HasTextureCoords(0)) {
            aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

            if (material->GetTextureCount(aiTextureType_DIFFUSE)) {
                aiString path;

                if (material->GetTexture(aiTextureType_DIFFUSE, 0, &path) == AI_SUCCESS) {
                    const aiTexture* texture = scene->GetEmbeddedTexture(path.C_Str());

                    // Get texture name according to texture type (embedded/external)
                    std::string textureName = texture ? modelName + path.C_Str() : utils::getFileName(path.C_Str());

                    int textureIndex = parentScene->textureManager.getTextureIndex(textureName);
                    meshData.material.setTextureIndex(textureIndex);
                } 
            }
        }
        else {
            meshData.material.setTextureIndex(-1);
        }


        meshes.emplace_back(std::make_unique<Mesh>(&meshData, &modelMatrix));
    }

    for (size_t i = 0; i < node->mNumChildren; ++i) {
        loadNodeRec(node->mChildren[i], scene, parentScene, modelName);
    }
}


void Model::cleanup(){
    for (auto& mesh : meshes) {
        mesh->cleanup();
    }
}
