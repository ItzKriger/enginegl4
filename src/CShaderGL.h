#pragma once
#include <string>
#include <vector>
#include "GL/glew.h"
#include "glm/glm.hpp"

class CShaderGL
{
public:
	struct Property
	{
		std::string Name, Value;
	};

	CShaderGL();
	CShaderGL(const std::string& vertex_path, const std::string& frag_path = "", const std::string& geom_path = "");
	~CShaderGL();

	void SetDefaultShaderProperties();
	void SetProperty(const std::string& name, const std::string& value);
	void DeleteProperty(const std::string& name);

	void DestructProgram();

	void LoadFromFile(const std::string& vertex_path, const std::string& frag_path = "", const std::string& geom_path = "");
	void Bind();
	static void UnbindAll();

	void setUniform(const std::string& name, float x);
	void setUniform(const std::string& name, int x);
	void setUniform(const std::string& name, unsigned int x);
	void setUniformLegacyTextureID(const std::string& name, int x);
	void setUniform(const std::string& name, bool x);
	void setUniform(const std::string& name, const glm::vec2& v);
	void setUniform(const std::string& name, const glm::vec3& v);
	void setUniform(const std::string& name, const glm::vec4& v);
	void setUniform(const std::string& name, const glm::ivec2& v);
	void setUniform(const std::string& name, const glm::ivec3& v);
	void setUniform(const std::string& name, const glm::ivec4& v);
	void setUniform(const std::string& name, const glm::uvec2& v);
	void setUniform(const std::string& name, const glm::uvec3& v);
	void setUniform(const std::string& name, const glm::uvec4& v);
	void setUniform(const std::string& name, const glm::mat3& matrix);
	void setUniform(const std::string& name, const glm::mat4& matrix);
	void setTextureHandleUniform(const std::string& name, std::uint64_t handle);
	void setUniformBuffer(const std::string& name, GLuint id, GLuint binding);

	int getUboLayoutSize(const std::string& name);
	std::vector<GLuint> getUboLayoutIndices(const std::string& name, std::vector<std::string> names);
	std::vector<int> getUboLayoutOffsets(const std::string& name, std::vector<GLuint> indices);

	std::vector<GLuint> getUboLayoutIndices(const std::string& name, const char** names, size_t count);
	std::vector<int> getUboLayoutOffsets(const std::string& name, const GLuint* indices, size_t count);

	void ReloadShadersProperties();

	unsigned int GetShaderID();

	std::vector<Property> Properties;
private:
	bool usingFrag = false;
	bool usingGeom = false;

	bool Compiled = false;

	unsigned int ID = 0;

	struct Uniform
	{
		std::string Name;
		int Location = 0;
	};

	std::vector<Uniform> Uniforms;

	std::string OriginalVertexShader;
	std::string OriginalFragmentShader;
	std::string OriginalGeometryShader;

	std::string vertPath, fragPath, geomPath;

	void Compile(const std::string& vertCode, const std::string& fragCode, const std::string& geomCode);
	int GetUniformLocation(const std::string& name);
	int GetUniformBlockLocation(const std::string& name);
};
