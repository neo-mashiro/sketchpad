//#pragma once
//
//#include <string>
//
//#include "core/log.h"
//#include "components/component.h"
//// #include "components/mesh.h"
//
//#define STB_IMAGE_IMPLEMENTATION
//#include "stb_image.h"
//
//#include <assimp/Importer.hpp>
//#include <assimp/scene.h>
//#include <assimp/postprocess.h>
//
//namespace components {
//
//    // forward declaration
//    class Mesh;
//    class Shader;
//    class Texture;
//
//    class Model : public Component {
//      private:
//        // loads a model with supported ASSIMP extensions from file and stores the resulting meshes in the meshes vector.
//        void loadModel(std::string const& path);
//
//        // processes a node in a recursive fashion. Processes each individual mesh located at the node
//        // and repeats this process on its children nodes (if any).
//        void processNode(aiNode* node, const aiScene* scene);
//
//        Mesh processMesh(aiMesh* mesh, const aiScene* scene);
//
//        // checks all material textures of a given type and loads the textures if they're not loaded yet.
//        // the required info is returned as a Texture struct.
//        std::vector<Texture> loadMaterialTextures(aiMaterial* mat, aiTextureType type, std::string typeName);
//
//      public:
//        // model data
//        std::vector<Texture> textures_loaded;  // stores all the textures loaded so far, optimization to make sure textures aren't loaded more than once.
//        std::vector<Mesh>    meshes;
//        string directory;
//        bool gammaCorrection;
//
//        Model(std::string const& filepath, bool gamma = false);
//        ~Model() {}
//
//        Model(const Model&) = delete;
//        Model& operator=(const Model&) = delete;
//
//        Model(Model&& other) noexcept;
//        Model& operator=(Model&& other) noexcept;
//
//        void Draw(Shader& shader);
//    };
//
//}
