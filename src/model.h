// #pragma once
//
// #include <GL/glew.h>
// #include <glm/glm.hpp>
// #include <glm/gtc/type_ptr.hpp>
// #include <glm/gtc/matrix_transform.hpp>
//
// #define STB_IMAGE_IMPLEMENTATION
// #include "stb_image.h"
// // #include <stb_image.h>
//
// #include <assimp/Importer.hpp>
// #include <assimp/scene.h>
// #include <assimp/postprocess.h>
//
// #include <string>
// #include <fstream>
// #include <sstream>
// #include <iostream>
// #include <map>
// #include <vector>
//
// #include "mesh.h"
// #include "shader.h"
//
// namespace Sketchpad {
//     unsigned int TextureFromFile(const char *path, const string &directory, bool gamma = false);
//
//     class Model {
//       public:
//         // model data
//         std::vector<Texture> textures_loaded;	// stores all the textures loaded so far, optimization to make sure textures aren't loaded more than once.
//         std::vector<Mesh>    meshes;
//         string directory;
//         bool gammaCorrection;
//
//         Model(string const &filepath, bool gamma = false);
//         void Draw(Shader &shader);
//
//       private:
//         // loads a model with supported ASSIMP extensions from file and stores the resulting meshes in the meshes vector.
//         void loadModel(string const &path);
//
//         // processes a node in a recursive fashion. Processes each individual mesh located at the node and repeats this process on its children nodes (if any).
//         void processNode(aiNode *node, const aiScene *scene);
//
//         Mesh processMesh(aiMesh *mesh, const aiScene *scene);
//
//         // checks all material textures of a given type and loads the textures if they're not loaded yet.
//         // the required info is returned as a Texture struct.
//         vector<Texture> loadMaterialTextures(aiMaterial *mat, aiTextureType type, string typeName);
//     };
//
//
//     unsigned int TextureFromFile(const char *path, const string &directory, bool gamma);
// }
