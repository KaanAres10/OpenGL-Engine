#include <model.h>
#include <stb_image.h>

static GLuint TextureFromFile(const char* filename, const std::string& directory, bool gammaCorrection = false)
{
	std::string filepath = directory + "/" + filename;

	int width, height, nrComponents;
	stbi_set_flip_vertically_on_load(true);
	unsigned char* data = stbi_load(filepath.c_str(), &width, &height, &nrComponents, 0);
	if (!data)
	{
		std::cerr << "Texture failed to load at path: " << filepath << "\n";
		stbi_image_free(data);
		return 0;
	}

	GLenum format;
	if (nrComponents == 1)       format = GL_RED;
	else if (nrComponents == 3)  format = GL_RGB;
	else if (nrComponents == 4)  format = GL_RGBA;

	GLuint textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_2D, textureID);

	glTexImage2D(GL_TEXTURE_2D, 0, gammaCorrection ? GL_SRGB : format,
		width, height, 0, format, GL_UNSIGNED_BYTE, data);
	glGenerateMipmap(GL_TEXTURE_2D);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	stbi_image_free(data);

	return textureID;
}

void Model::draw(Shader& shader) {
	for (GLuint i = 0; i < meshes.size(); i++) {
		meshes[i].draw(shader);
	}
}

void Model::loadModel(string path)
{
	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs);

	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
	{
		std::cout << "ERRRO::ASSIMP::" << importer.GetErrorString() << std::endl;
		return;
	}
	directory = path.substr(0, path.find_last_of('/'));

	processNode(scene->mRootNode, scene);
}

void Model::processNode(aiNode* node, const aiScene* scene)
{
	for (GLuint i = 0; i < node->mNumMeshes; i++) {
		aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
		meshes.push_back(processMesh(mesh, scene));
	}

	for (GLuint i = 0; i < node->mNumChildren; i++) {
		processNode(node->mChildren[i], scene);
	}
}

Mesh Model::processMesh(aiMesh* mesh, const aiScene* scene)
{
	vector<Vertex> vertices;
	vector<GLuint> indices;
	vector<Texture> textures;

	for (GLuint i = 0; i < mesh->mNumVertices; i++) {
		Vertex vertex;
		
		vertex.position= glm::vec3(mesh->mVertices[i].x,
								   mesh->mVertices[i].y,
		                           mesh->mVertices[i].z);

		vertex.normal = glm::vec3(mesh->mNormals[i].x,
								  mesh->mNormals[i].y,
								  mesh->mNormals[i].z);

		if (mesh->mTextureCoords[0]) {
			vertex.texCoords = {
				mesh->mTextureCoords[0][i].x,
				1.0f - mesh->mTextureCoords[0][i].y
			};
		}
		else {
			vertex.texCoords = { 0.0f, 0.0f };
		}

		vertices.push_back(vertex);
	}

	for (GLuint i = 0; i < mesh->mNumFaces; i++) {
		aiFace face = mesh->mFaces[i];
		for (GLuint j = 0; j < face.mNumIndices; j++) {
			indices.push_back(face.mIndices[j]);
		}
	}

	if (mesh->mMaterialIndex >= 0)
	{
		aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

		vector<Texture> diffuseMaps = loadMaterialTextures(material,
			aiTextureType_DIFFUSE, "texture_diffuse", scene);
		textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());

		vector<Texture> specularMaps = loadMaterialTextures(material,
			aiTextureType_SPECULAR, "texture_specular", scene);
		textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
	}

	return Mesh(vertices, indices, textures);
}

vector<Texture> Model::loadMaterialTextures(aiMaterial* mat,
	aiTextureType type,
	const string& typeName,
	const aiScene* scene)
{
	vector<Texture> textures;
	for (unsigned i = 0; i < mat->GetTextureCount(type); ++i) {
		aiString str;
		mat->GetTexture(type, i, &str);

		bool skip = false;
		for (auto& loaded : textures_loaded) {
			if (loaded.path == str.C_Str()) {
				textures.push_back(loaded);
				skip = true;
				break;
			}
		}
		if (skip) continue;

		Texture tex;
		const char* name = str.C_Str();

		// Embedded
		if (name[0] == '*') {
			// index into scene->mTextures
			unsigned idx = std::atoi(name + 1);
			const aiTexture* atex = scene->mTextures[idx];

			// compressed (mHeight==0) or raw RGBA data
			int w, h, comp;
			unsigned char* data;
			if (atex->mHeight == 0) {
				// compressed (PNG/JPEG) in pcData, size = mWidth
				data = stbi_load_from_memory(
					reinterpret_cast<const unsigned char*>(atex->pcData),
					atex->mWidth, &w, &h, &comp, 0);
			}
			else {
				// raw RGBA8888 in pcData, size = mWidth * mHeight * 4
				w = atex->mWidth;
				h = atex->mHeight;
				comp = 4;
				data = reinterpret_cast<unsigned char*>(atex->pcData);
			}

			GLenum fmt = (comp == 1 ? GL_RED
				: comp == 3 ? GL_RGB
				: GL_RGBA);
			glGenTextures(1, &tex.id);
			glBindTexture(GL_TEXTURE_2D, tex.id);
			glTexImage2D(GL_TEXTURE_2D, 0, fmt, w, h, 0, fmt,
				GL_UNSIGNED_BYTE, data);
			glGenerateMipmap(GL_TEXTURE_2D);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
				GL_LINEAR_MIPMAP_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			if (atex->mHeight == 0)
				stbi_image_free(data); 

			tex.path = name;
		}
		else {
			// External file
			tex.id = TextureFromFile(name, directory);
			tex.path = name;
		}

		tex.type = typeName;
		textures.push_back(tex);
		textures_loaded.push_back(tex);
	}
	return textures;
}