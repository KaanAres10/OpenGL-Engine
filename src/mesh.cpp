#include "mesh.h"

Mesh::Mesh(vector<Vertex> vertices, vector<GLuint> inidices, vector<Texture> textures)
{
	this->vertices = vertices;
	this->indices = inidices;
	this->textures = textures;

	setupMesh();
}

void Mesh::draw(Shader& shader)
{
	GLuint diffuseNr = 1;
	GLuint specularNr = 1;
	GLuint normalNr = 1;
	bool hasNormal = false;
	bool hasSpecular = false;

	for (GLuint i = 0; i < textures.size(); i++)
	{
		glActiveTexture(GL_TEXTURE0 + i);

		string number;
		string name = textures[i].type;

		if (name == "texture_diffuse") {
			number = std::to_string(diffuseNr++);
			shader.setInt("diffuseMap", i);
		}
		else if (name == "texture_specular") {
			number = std::to_string(specularNr++);
			shader.setInt("specularMap", i);
			hasSpecular = true;
			
		} else if (name == "texture_normal") {
			number = std::to_string(normalNr++);
			shader.setInt("normalMap", i);
			hasNormal = true;
		}

		shader.setInt(("material." + name + number).c_str(), i);
		glBindTexture(GL_TEXTURE_2D, textures[i].id);
	}
	glActiveTexture(GL_TEXTURE0);

	shader.setBool("hasNormalMap", hasNormal);
	shader.setBool("hasSpecularMap", hasSpecular);


	glBindVertexArray(vao);
	glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);
}

void Mesh::setInstanceData(const std::vector<glm::mat4>& models)
{
	glBindBuffer(GL_ARRAY_BUFFER, instanceVbo);
	glBufferData(GL_ARRAY_BUFFER,
		models.size() * sizeof(glm::mat4),
		models.data(),
		GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void Mesh::drawInstanced(Shader& shader, GLsizei instanceCount)
{
	GLuint diffuseNr = 1, specularNr = 1;
	for (GLuint i = 0; i < textures.size(); i++) {
		glActiveTexture(GL_TEXTURE0 + i);
		std::string number =
			(textures[i].type == "texture_diffuse") ?
			std::to_string(diffuseNr++) :
			std::to_string(specularNr++);
		shader.setInt(("material." + textures[i].type + number).c_str(), i);
		glBindTexture(GL_TEXTURE_2D, textures[i].id);
	}
	glActiveTexture(GL_TEXTURE0);

	// instanced draw
	glBindVertexArray(vao);
	glDrawElementsInstanced(
		GL_TRIANGLES,
		static_cast<GLsizei>(indices.size()),
		GL_UNSIGNED_INT,
		0,
		instanceCount
	);
	glBindVertexArray(0);
}

void Mesh::setupMesh()
{
	glGenVertexArrays(1, &vao);
	glGenBuffers(1, &vbo);
	glGenBuffers(1, &ebo);

	glBindVertexArray(vao);

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), &indices[0], GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);

	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));

	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoords));

	glEnableVertexAttribArray(3);
	glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE,	sizeof(Vertex), (void*)offsetof(Vertex, tangent));

	glGenBuffers(1, &instanceVbo);
	glBindBuffer(GL_ARRAY_BUFFER, instanceVbo);
	glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_STATIC_DRAW);

	constexpr GLsizei vec4Size = sizeof(glm::vec4);
	GLuint baseLoc = 5;   
	for (GLuint i = 0; i < 4; ++i) {
		glEnableVertexAttribArray(baseLoc + i);
		glVertexAttribPointer(
			baseLoc + i,             
			4, GL_FLOAT, GL_FALSE,
			sizeof(glm::mat4),
			(void*)(i * vec4Size)
		);
		glVertexAttribDivisor(baseLoc + i, 1);
	}

	glBindVertexArray(0);
}
