#include <model.hpp>
#include <scene.hpp>

#include <logger.hpp>
#include <string>




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


void Model::draw(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout) const{
    for (const auto& mesh : meshes) {
        mesh->draw(commandBuffer, pipelineLayout);
    }
}


void Model::loadModel(const std::string& path, Scene* parentScene){
    Assimp::Importer importer;

    const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_JoinIdenticalVertices);
    if (!scene) { 
        LOG_ERROR("Failed to load model  (" + path + ")");
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
                    std::string textureName = modelName + path.C_Str();   // example.obj*0

                    if (texture->mHeight) {   // Not compressed
                        parentScene->textureManager.addEmbeddedTexture(textureName,
                                                                       texture->pcData, 
                                                                       texture->mWidth, texture->mHeight
                        );
                    }
                    else {    // Compressed
                        // LOG_WARNING("Compressed embedded textures are not supported yet (" + textureName + ")");
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
    }
}

void Model::loadNodeRec(const aiNode* node, const aiScene* scene, Scene* parentScene, const std::string& modelName){
    for (size_t i = 0; i < node->mNumMeshes; ++i) {
        auto mesh = scene->mMeshes[node->mMeshes[i]];

        RawMesh meshData{};
        meshData.vertices.resize(mesh->mNumVertices);

        // Vertices
        for (size_t i = 0; i < mesh->mNumVertices; ++i) {
            meshData.vertices[i].pos = { mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z };

            if (mesh->HasTextureCoords(0)) {
                meshData.vertices[i].texCoord = { mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y };
            }
        }
    
        // Indices
        for (size_t i = 0; i < mesh->mNumFaces; ++i) {
            aiFace face = mesh->mFaces[i];

            for (size_t j = 0; j < face.mNumIndices; ++j) {
                meshData.indices.push_back(face.mIndices[j]);
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
