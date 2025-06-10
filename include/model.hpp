#pragma once

#include <mesh.hpp>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>


class Scene;  // Forward declaration due to circular dependency

class Model{
public:
    glm::mat4 modelMatrix = glm::mat4(1.0f);


    Model(std::variant<std::string, RawMesh*> source, Scene* parentScene);

    void setUniformMaterial(const Material& material);

private:
    std::vector<std::unique_ptr<Mesh>> meshes;

    void draw(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, const glm::vec3& cameraPos) const;
    void cleanup();

    friend class Scene;


    void loadModel(const std::string& path, Scene* parentScene);
    void loadTextures(const aiScene* scene, Scene* parentScene, const std::string& modelName);
    void loadNodeRec(const aiNode* node, const aiScene* scene, Scene* parentScene, const std::string& modelName);
};
