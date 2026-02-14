#include "rmt/io/mesh/import/AssimpMeshLoader.h"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <cctype>
#include <string>
#include <vector>

#include "MeshImportInternals.h"

namespace rmt {

std::string toLowerCopy(const std::string& value) {
    std::string lowered = value;
    for (std::size_t i = 0; i < lowered.size(); ++i) {
        lowered[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(lowered[i])));
    }
    return lowered;
}

std::string getFileExtensionLower(const std::string& path) {
    const std::string::size_type dot = path.find_last_of('.');
    if (dot == std::string::npos) {
        return std::string();
    }
    return toLowerCopy(path.substr(dot));
}

bool loadMeshTrianglesWithAssimp(const std::string& path,
                                 std::vector<Vec3f>& vertices,
                                 std::vector<Vec3ui>& faces,
                                 std::string& outError) {
    vertices.clear();
    faces.clear();

    Assimp::Importer importer;
    const unsigned int flags =
        aiProcess_Triangulate |
        aiProcess_JoinIdenticalVertices |
        aiProcess_PreTransformVertices |
        aiProcess_ImproveCacheLocality |
        aiProcess_RemoveRedundantMaterials |
        aiProcess_SortByPType |
        aiProcess_ValidateDataStructure;

    const aiScene* scene = importer.ReadFile(path, flags);
    if (scene == nullptr) {
        outError = importer.GetErrorString();
        return false;
    }
    if (!scene->HasMeshes()) {
        outError = "No mesh data found in file.";
        return false;
    }

    for (unsigned int meshIndex = 0; meshIndex < scene->mNumMeshes; ++meshIndex) {
        const aiMesh* mesh = scene->mMeshes[meshIndex];
        if (mesh == nullptr || mesh->mNumVertices == 0 || mesh->mNumFaces == 0) {
            continue;
        }

        const std::size_t baseVertex = vertices.size();
        vertices.reserve(vertices.size() + mesh->mNumVertices);
        for (unsigned int v = 0; v < mesh->mNumVertices; ++v) {
            const aiVector3D& p = mesh->mVertices[v];
            vertices.push_back(Vec3f(p.x, p.y, p.z));
        }

        faces.reserve(faces.size() + mesh->mNumFaces);
        for (unsigned int f = 0; f < mesh->mNumFaces; ++f) {
            const aiFace& face = mesh->mFaces[f];
            if (face.mNumIndices != 3) {
                continue;
            }

            const unsigned int i0 = face.mIndices[0];
            const unsigned int i1 = face.mIndices[1];
            const unsigned int i2 = face.mIndices[2];
            const unsigned int maxIndex = mesh->mNumVertices - 1;
            if (i0 > maxIndex || i1 > maxIndex || i2 > maxIndex) {
                continue;
            }

            faces.push_back(Vec3ui(
                static_cast<unsigned int>(baseVertex + i0),
                static_cast<unsigned int>(baseVertex + i1),
                static_cast<unsigned int>(baseVertex + i2)));
        }
    }

    if (vertices.empty() || faces.empty()) {
        outError = "Mesh contains no valid triangle geometry.";
        return false;
    }

    return true;
}

} // namespace rmt
