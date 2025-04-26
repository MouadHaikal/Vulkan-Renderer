#include <material.hpp>


Material::Material(glm::vec3 albedo, int32_t textureIndex) : albedo(albedo), textureIndex(textureIndex){}

int32_t Material::getTextureIndex() const{ return textureIndex; }
