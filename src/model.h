#include <mesh.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
class Model
{
public:
	vector<Mesh> meshes;

	Model(){}

	Model(char* path)
	{
		loadModel(path);
	}
	void draw(Shader& shader);

	void loadModel(string path);

	void setInstanceData(const std::vector<glm::mat4>& models);

	void drawInstanced(Shader& shader, GLsizei count);

private:

	vector<Texture> textures_loaded;
	string directory;

	void processNode(aiNode* node, const aiScene* scene);
	Mesh processMesh(aiMesh* mesh, const aiScene* scene);
	vector<Texture> loadMaterialTextures(aiMaterial* mat, aiTextureType type, const string& typeName, const aiScene* scene);
};