#pragma once
#include <glm/glm.hpp>
#include <glad/glad.h>
#include <string>
#include <vector>
#include <shader.h>

using std::string;
using std::vector;

struct Vertex {
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec3 texCoords;

};

struct Texture {
	GLuint id;
	string type;
};

class Mesh {
public:
	
	vector<Vertex> vertices;
	vector<GLuint> indices;
	vector<Texture> textures;

	Mesh(vector<Vertex> vertices, vector<GLuint> inidices, vector<Texture> textures);
	void draw(Shader& shader);

private:
	unsigned int vao, vbo, ebo;

	void setupMesh();
};