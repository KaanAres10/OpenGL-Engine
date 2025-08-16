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

	GLenum internalFormat, dataFormat;
	if (nrComponents == 1) {
		internalFormat = GL_R8;        dataFormat = GL_RED;
	}
	else if (nrComponents == 3) {
		internalFormat = gammaCorrection ? GL_SRGB8 : GL_RGB8;
		dataFormat = GL_RGB;
	}
	else if (nrComponents == 4) {
		internalFormat = gammaCorrection ? GL_SRGB8_ALPHA8 : GL_RGBA8;
		dataFormat = GL_RGBA;
	}

	GLuint textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_2D, textureID);

	glTexImage2D(GL_TEXTURE_2D, 0,
	internalFormat,
		width, height, 0,
		dataFormat, GL_UNSIGNED_BYTE,
		data);
	glGenerateMipmap(GL_TEXTURE_2D);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	stbi_image_free(data);

	return textureID;
}

static GLuint findFirstOfType(const std::vector<Texture>& tex, const char* type)
{
	for (auto const& t : tex)
		if (t.type == type) return t.id;
	return 0;
}

static GLuint findFirstByFallback(const std::vector<Texture>& tex,
	std::initializer_list<const char*> types)
{
	for (auto name : types) {
		GLuint id = findFirstOfType(tex, name);
		if (id) return id;
	}
	return 0;
}

static std::string toLower(std::string s) {
	std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
	return s;
}
static bool containsI(const std::string& hay, const std::string& needle) {
	return toLower(hay).find(toLower(needle)) != std::string::npos;
}
static bool hasAny(const std::string& s, std::initializer_list<const char*> keys) {
	for (auto k : keys) if (containsI(s, k)) return true;
	return false;
}

static bool fileExists(const std::filesystem::path& p) {
	std::error_code ec; return std::filesystem::exists(p, ec) && std::filesystem::is_regular_file(p, ec);
}

static void pushTexture(std::vector<Texture>& dst, std::vector<Texture>& loaded,
	GLuint id, const std::string& path, const std::string& typeName)
{
	Texture t; t.id = id; t.type = typeName; t.path = path;
	auto it = std::find_if(loaded.begin(), loaded.end(), [&](const Texture& x) { return x.path == t.path; });
	if (it == loaded.end()) loaded.push_back(t);
	dst.push_back(t);
}

static std::string findFileByKeywords(const std::string& directory,
	std::initializer_list<const char*> keywords)
{
	std::error_code ec;
	for (auto const& entry : std::filesystem::directory_iterator(directory, ec)) {
		if (!entry.is_regular_file()) continue;
		auto ext = toLower(entry.path().extension().string());
		if (ext != ".png" && ext != ".jpg" && ext != ".jpeg" && ext != ".tga") continue;
		auto name = entry.path().filename().string();
		if (hasAny(name, keywords)) return entry.path().filename().string();
	}
	return {};
}

void Model::draw(Shader& shader) {
	GLint loc = glGetUniformLocation(shader.ID, "albedoMap");
	if (loc >= 0) {
		drawPBR(shader);
		return;
	}
	for (GLuint i = 0; i < meshes.size(); i++) {
		meshes[i].draw(shader);
	}
}

void Model::drawPBR(Shader& shader) const
{
	shader.setInt("albedoMap", 0);
	shader.setInt("normalMap", 1);
	shader.setInt("metallicMap", 2);
	shader.setInt("roughnessMap", 3);
	shader.setInt("aoMap", 4);

	for (auto const& m : meshes)
	{
		GLuint al = 0, nm = 0, me = 0, ro = 0, ao = 0;

		al = findFirstByFallback(m.textures, {
			"texture_albedo", "texture_basecolor", "texture_diffuse"
			});
		nm = findFirstByFallback(m.textures, {
			"texture_normal", "texture_normalmap", "texture_height" 
			});
		me = findFirstByFallback(m.textures, {
			"texture_metallic", "texture_specular" 
			});
		ro = findFirstByFallback(m.textures, {
			"texture_roughness", "texture_shininess" 
			});
		ao = findFirstByFallback(m.textures, {
			"texture_ao", "texture_ambient"
			});


		glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, al ? al : 0);
		glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, nm ? nm : 0);
		glActiveTexture(GL_TEXTURE2); glBindTexture(GL_TEXTURE_2D, me ? me : 0);
		glActiveTexture(GL_TEXTURE3); glBindTexture(GL_TEXTURE_2D, ro ? ro : 0);
		glActiveTexture(GL_TEXTURE4); glBindTexture(GL_TEXTURE_2D, ao ? ao : 0);

		m.drawGeometryOnly();
	}
}

void Model::loadModel(string path)
{
	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);

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

		if (mesh->HasTangentsAndBitangents()) {
			vertex.tangent = {
			  mesh->mTangents[i].x,
			  mesh->mTangents[i].y,
			  mesh->mTangents[i].z
			};
		}
		else {
			vertex.tangent = glm::vec3(1, 0, 0);
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


		// --- PBR ---
		auto base = loadMaterialTextures(material, aiTextureType_BASE_COLOR, "texture_albedo", scene);
		auto normN = loadMaterialTextures(material, aiTextureType_NORMALS, "texture_normal", scene);
		auto normH = loadMaterialTextures(material, aiTextureType_HEIGHT, "texture_normal", scene); 
		auto metal = loadMaterialTextures(material, aiTextureType_METALNESS, "texture_metallic", scene);
		auto rough = loadMaterialTextures(material, aiTextureType_DIFFUSE_ROUGHNESS, "texture_roughness", scene);
		auto occ = loadMaterialTextures(material, aiTextureType_AMBIENT_OCCLUSION, "texture_ao", scene);
		textures.insert(textures.end(), base.begin(), base.end());
		textures.insert(textures.end(), normN.begin(), normN.end());
		textures.insert(textures.end(), normH.begin(), normH.end());
		textures.insert(textures.end(), metal.begin(), metal.end());
		textures.insert(textures.end(), rough.begin(), rough.end());
		textures.insert(textures.end(), occ.begin(), occ.end());


		// --- Legacy ---
		if (base.empty()) {
			auto dif = loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse", scene);
			textures.insert(textures.end(), dif.begin(), dif.end());
		}

		if (base.empty()) {
			auto dif = loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse", scene);
			textures.insert(textures.end(), dif.begin(), dif.end());
		}
	}

	auto hasType = [&](const char* type) {
		return std::any_of(textures.begin(), textures.end(),
			[&](const Texture& t) { return t.type == type; });
		};

	if (!hasType("texture_albedo")) {
		if (auto f = findFileByKeywords(directory, { "basecolor","base_colour","albedo","diffuse","color","colour" }); !f.empty()) {
			GLuint id = TextureFromFile(f.c_str(), directory, /*gamma*/true);
			if (id) pushTexture(textures, textures_loaded, id, f, "texture_albedo");
		}
	}
	if (!hasType("texture_normal")) {
		if (auto f = findFileByKeywords(directory, { "normal","normalmap","_n","-n" }); !f.empty()) {
			GLuint id = TextureFromFile(f.c_str(), directory, /*gamma*/false);
			if (id) pushTexture(textures, textures_loaded, id, f, "texture_normal");
		}
	}
	if (!hasType("texture_metallic")) {
		if (auto f = findFileByKeywords(directory, { "metal","metallic","_m","-m" }); !f.empty()) {
			GLuint id = TextureFromFile(f.c_str(), directory, /*gamma*/false);
			if (id) pushTexture(textures, textures_loaded, id, f, "texture_metallic");
		}
	}
	if (!hasType("texture_roughness")) {
		if (auto f = findFileByKeywords(directory, { "rough","roughness","_r","-r","gloss","smooth" }); !f.empty()) {
			GLuint id = TextureFromFile(f.c_str(), directory, /*gamma*/false);
			if (id) pushTexture(textures, textures_loaded, id, f, "texture_roughness");
		}
	}
	if (!hasType("texture_ao")) {
		if (auto f = findFileByKeywords(directory, { "ao","occlusion","ambientocclusion" }); !f.empty()) {
			GLuint id = TextureFromFile(f.c_str(), directory, /*gamma*/false);
			if (id) pushTexture(textures, textures_loaded, id, f, "texture_ao");
		}
	}

	if ((!hasType("texture_metallic") || !hasType("texture_roughness"))) {
		if (auto f = findFileByKeywords(directory, { "orm","occlusionroughnessmetallic" }); !f.empty()) {
			GLuint id = TextureFromFile(f.c_str(), directory, /*gamma*/false);
			if (id) {
				if (!hasType("texture_metallic"))
					pushTexture(textures, textures_loaded, id, f, "texture_metallic");
				if (!hasType("texture_roughness"))
					pushTexture(textures, textures_loaded, id, f, "texture_roughness");
				if (!hasType("texture_ao"))
					pushTexture(textures, textures_loaded, id, f, "texture_ao");
			}
		}
	}

	return Mesh(vertices, indices, textures);
}

void Model::setInstanceData(const std::vector<glm::mat4>& models) {
	for (auto& mesh : meshes)
		mesh.setInstanceData(models);
}

void Model::drawInstanced(Shader& shader, GLsizei count) {
	for (auto& mesh : meshes)
		mesh.drawInstanced(shader, count);
}

vector<Texture> Model::loadMaterialTextures(aiMaterial* mat,
	aiTextureType type,
	const string& typeName,
	const aiScene* scene)
{
	bool isDiffuse = (type == aiTextureType_DIFFUSE);
	bool isBaseColor = (type == aiTextureType_DIFFUSE || type == aiTextureType_BASE_COLOR);

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

			GLenum internalFormat, dataFormat;
			if (comp == 1) {
				internalFormat = GL_R8;           
				dataFormat = GL_RED;
			}
			else if (comp == 3) {
				internalFormat = isBaseColor
					? GL_SRGB8   // decode on fetch
					: GL_RGB8;   // stay linear
				dataFormat = GL_RGB;
			}
			else if (comp == 4) {
				internalFormat = isBaseColor
					? GL_SRGB8_ALPHA8
					: GL_RGBA8;
				dataFormat = GL_RGBA;
			}

			glGenTextures(1, &tex.id);
			glBindTexture(GL_TEXTURE_2D, tex.id);
			glTexImage2D(GL_TEXTURE_2D, 0,
				internalFormat,
				w, h, 0,
				dataFormat, GL_UNSIGNED_BYTE,
				data);
			glGenerateMipmap(GL_TEXTURE_2D);

			// wrap/filter as before
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
				GL_LINEAR_MIPMAP_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			if (atex->mHeight == 0)
				stbi_image_free(data);   // only free when stb actually allocated

			tex.path = name;
		}
		else {
			// External file
			tex.id = TextureFromFile(name, directory, type == aiTextureType_DIFFUSE);
			tex.path = name;
		}

		tex.type = typeName;
		textures.push_back(tex);
		textures_loaded.push_back(tex);
	}
	return textures;
}

void Model::drawGeometryOnly(Shader& shader) const {
	for (auto const& m : meshes) {
		m.drawGeometryOnly(); // no texture binding, just geometry
	}
}

