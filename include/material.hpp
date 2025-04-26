#include <utilities.hpp>


class Material{
public:
    Material(glm::vec3 albedo = glm::vec3(0.5f), int32_t textureIndex = -1);

    int32_t getTextureIndex() const;
private:

    glm::vec3 albedo;
    int32_t   textureIndex;
};
