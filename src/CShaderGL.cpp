#include "CShaderGL.h"

#include "CEngine.h"
#include "U_Log.h"

#include <fstream>
#include <sstream>
#include "GL/glew.h"
#include <iterator>
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/quaternion.hpp"
#include "glm/gtc/type_ptr.hpp"

CShaderGL::CShaderGL() {  }

CShaderGL::CShaderGL(const std::string& vertex_path, const std::string& frag_path, const std::string& geom_path)
{
	LoadFromFile(vertex_path, frag_path, geom_path);
}

void CShaderGL::SetProperty(const std::string& name, const std::string& value)
{
	for (Property& prop : Properties)
	{
		if (prop.Name == name)
		{
			prop.Value = value;
			return;
		}
	}

	Properties.push_back({ name, value });
}

void CShaderGL::DeleteProperty(const std::string& name)
{
	for (int i = 0; i < Properties.size(); i++)
	{
		if (Properties.at(i).Name == name)
		{
			Properties.erase(Properties.begin() + i);
			return;
		}
	}
}

void CShaderGL::ReloadShadersProperties()
{
	std::string vertCode = OriginalVertexShader, fragCode = OriginalFragmentShader, geomCode = OriginalGeometryShader;

	if (!fragCode.empty()) { usingFrag = true; }
	if (!geomCode.empty()) { usingGeom = true; }

	std::string propString;
	for (Property& prop : Properties)
	{
		propString += "#define " + prop.Name + " " + prop.Value + "\r\n";
	}

	std::string toFind = "#version 430\r\n";
	size_t index = vertCode.find(toFind);
	vertCode.insert(index + toFind.length(), propString);

	if (usingFrag)
	{
		std::string toFind = "#version 430\r\n";
		size_t index = fragCode.find(toFind);
		fragCode.insert(index + toFind.length(), propString);
	}

	if (usingGeom)
	{
		std::string toFind = "#version 430\r\n";
		size_t index = geomCode.find(toFind);
		geomCode.insert(index + toFind.length(), propString);
	}

	Compile(vertCode, fragCode, geomCode);
}

void CShaderGL::SetDefaultShaderProperties()
{
	
}

void CShaderGL::LoadFromFile(const std::string& vertex_path, const std::string& frag_path, const std::string& geom_path)
{
	if (!frag_path.empty()) { usingFrag = true; }
	if (!geom_path.empty()) { usingGeom = true; }

	vertPath = vertex_path;
	fragPath = frag_path;
	geomPath = geom_path;

	std::string vertCode, fragCode, geomCode;
	std::ifstream vertStream, fragStream, geomStream;

	std::string propString;

	for (Property& prop : Properties)
	{
		propString += "#define " + prop.Name + " " + prop.Value + "\r\n";
	}

	vertStream.open(vertex_path);

	if (vertStream.is_open())
	{
		std::stringstream vertStringStream;
		vertStringStream << vertStream.rdbuf();
		vertStream.close();

		vertCode = vertStringStream.str();

		OriginalVertexShader = vertCode;
		
		std::string toFind = "#version 430\r\n";
		size_t index = vertCode.find(toFind);
		vertCode.insert(index + toFind.length(), propString);

		if (vertex_path == "resources\\shaders\\default.vs")
		{
			std::ofstream fstr;
			fstr.open("vs.txt");
			fstr << vertCode;
			fstr.close();
		}
	}

	if (usingFrag)
	{
		fragStream.open(frag_path);

		if (fragStream.is_open())
		{
			std::stringstream fragStringStream;
			fragStringStream << fragStream.rdbuf();
			fragStream.close();

			fragCode = fragStringStream.str();

			OriginalFragmentShader = fragCode;

			std::string toFind = "#version 430\r\n";
			size_t index = fragCode.find(toFind);
			fragCode.insert(index + toFind.length(), propString);

			if (vertex_path == "resources\\shaders\\default.vs")
			{
				std::ofstream fstr;
				fstr.open("fs.txt");
				fstr << fragCode;
				fstr.close();
			}
		}
	}

	if (usingGeom)
	{
		geomStream.open(geom_path);

		if (geomStream.is_open())
		{
			std::stringstream geomStringStream;
			geomStringStream << geomStream.rdbuf();
			geomStream.close();

			geomCode = geomStringStream.str();

			OriginalGeometryShader = geomCode;

			std::string toFind = "#version 430\r\n";
			size_t index = geomCode.find(toFind);
			geomCode.insert(index + toFind.length(), propString);
		}
	}

	Compile(vertCode, fragCode, geomCode);
}

void CShaderGL::Compile(const std::string& vertCode, const std::string& fragCode, const std::string& geomCode)
{
	if (Compiled) { DestructProgram(); Compiled = false; }
	
	unsigned int vertex, fragment, geometry;
	int success;
	char infoLog[512];

	const char* vertcode = vertCode.c_str();

	vertex = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertex, 1, &vertcode, NULL);
	glCompileShader(vertex);

	glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(vertex, 512, NULL, infoLog);
		Log::Errln("Can't compile vertex shader \"" + vertPath + "\":\n" + std::string(infoLog));

		std::ofstream fstr;
		fstr.open("errordvs.txt");
		fstr << vertCode;
		fstr.close();
	}
	else
	{
		Log::Outln("Vertex shader compiled");
	}

	if (usingFrag)
	{
		const char* fragcode = fragCode.c_str();

		fragment = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(fragment, 1, &fragcode, NULL);
		glCompileShader(fragment);

		glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
		if (!success)
		{
			glGetShaderInfoLog(fragment, 512, NULL, infoLog);
			Log::Errln("Can't compile fragment shader \"" + fragPath + "\":\n" + std::string(infoLog));

			std::ofstream fstr;
			fstr.open("errordfs.txt");
			fstr << fragCode;
			fstr.close();
		}
		else
		{
			Log::Outln("Fragment shader compiled");
		}
	}

	if (usingGeom)
	{
		const char* geomcode = geomCode.c_str();

		geometry = glCreateShader(GL_GEOMETRY_SHADER);
		glShaderSource(geometry, 1, &geomcode, NULL);
		glCompileShader(geometry);

		glGetShaderiv(geometry, GL_COMPILE_STATUS, &success);
		if (!success)
		{
			glGetShaderInfoLog(geometry, 512, NULL, infoLog);
			Log::Errln("Can't compile geometry shader \"" + geomPath + "\":\n" + std::string(infoLog));
		}
		else
		{
			Log::Outln("Geometry shader compiled");
		}
	}

	ID = glCreateProgram();
	glAttachShader(ID, vertex);

	if (usingFrag) { glAttachShader(ID, fragment); }
	if (usingGeom) { glAttachShader(ID, geometry); }

	glLinkProgram(ID);

	glGetProgramiv(ID, GL_LINK_STATUS, &success);
	if (!success)
	{
		glGetProgramInfoLog(ID, 512, NULL, infoLog);
		Log::Errln("Can't link shader:\n" + std::string(infoLog));
	}
	else
	{
		Compiled = true;
		Log::Outln("Shader linked");
	}

	glDeleteShader(vertex);
	if (usingFrag) { glDeleteShader(fragment); }
	if (usingGeom) { glDeleteShader(geometry); }
}

CShaderGL::~CShaderGL()
{
	DestructProgram();
}

unsigned int CShaderGL::GetShaderID() { return ID; }

void CShaderGL::DestructProgram()
{
	glDeleteProgram(ID);
}

void CShaderGL::Bind()
{
	glUseProgram(ID);
}

void CShaderGL::UnbindAll()
{
	glUseProgram(0);
}

int CShaderGL::GetUniformLocation(const std::string& name)
{
	for (Uniform& u : Uniforms)
	{
		if (u.Name == name)
		{
			return u.Location;
		}
	}

	int loc = glGetUniformLocation(ID, name.c_str());

	if (loc != -1)
	{
		Log::Instance() << "Found uniform \"" << name << "\"\n";
		Uniforms.push_back({ name, loc });
	}

	return loc;
}

int CShaderGL::GetUniformBlockLocation(const std::string& name)
{
	for (Uniform& u : Uniforms)
	{
		if (u.Name == name)
		{
			return u.Location;
		}
	}

	int loc = glGetUniformBlockIndex(ID, name.c_str());

	if (loc != -1)
	{
		Uniforms.push_back({ name, loc });
	}

	return loc;
}

void CShaderGL::setUniform(const std::string& name, float x)
{
	int loc = GetUniformLocation(name);

	if (loc != -1)
	{
		glUniform1f(loc, x);
	}
}

void CShaderGL::setUniform(const std::string& name, int x)
{
	int loc = GetUniformLocation(name);

	if (loc != -1)
	{
		glUniform1i(loc, x);
	}
}

void CShaderGL::setUniformLegacyTextureID(const std::string& name, int x)
{
	int loc = GetUniformLocation(name);

	if (loc != -1)
	{
		glUniform1i(loc, x);
	}
}

void CShaderGL::setUniform(const std::string& name, unsigned int x)
{
	int loc = GetUniformLocation(name);

	if (loc != -1)
	{
		glUniform1ui(loc, x);
	}
}

void CShaderGL::setUniform(const std::string& name, bool x)
{
	int loc = GetUniformLocation(name);

	if (loc != -1)
	{
		glUniform1i(loc, static_cast<int>(x));
	}
}

void CShaderGL::setUniform(const std::string& name, const glm::vec2& v)
{
	int loc = GetUniformLocation(name);

	if (loc != -1)
	{
		glUniform2f(loc, v.x, v.y);
	}
}

void CShaderGL::setUniform(const std::string& name, const glm::vec3& v)
{
	int loc = GetUniformLocation(name);

	if (loc != -1)
	{
		glUniform3f(loc, v.x, v.y, v.z);
	}
}

void CShaderGL::setUniform(const std::string& name, const glm::vec4& v)
{
	int loc = GetUniformLocation(name);

	if (loc != -1)
	{
		glUniform4f(loc, v.x, v.y, v.z, v.w);
	}
}

void CShaderGL::setUniform(const std::string& name, const glm::ivec2& v)
{
	int loc = GetUniformLocation(name);

	if (loc != -1)
	{
		glUniform2i(loc, v.x, v.y);
	}
}

void CShaderGL::setUniform(const std::string& name, const glm::ivec3& v)
{
	int loc = GetUniformLocation(name);

	if (loc != -1)
	{
		glUniform3i(loc, v.x, v.y, v.z);
	}
}

void CShaderGL::setUniform(const std::string& name, const glm::ivec4& v)
{
	int loc = GetUniformLocation(name);

	if (loc != -1)
	{
		glUniform4i(loc, v.x, v.y, v.z, v.w);
	}
}

void CShaderGL::setUniform(const std::string& name, const glm::uvec2& v)
{
	int loc = GetUniformLocation(name);

	if (loc != -1)
	{
		glUniform2ui(loc, v.x, v.y);
	}
}

void CShaderGL::setUniform(const std::string& name, const glm::uvec3& v)
{
	int loc = GetUniformLocation(name);

	if (loc != -1)
	{
		glUniform3ui(loc, v.x, v.y, v.z);
	}
}

void CShaderGL::setUniform(const std::string& name, const glm::uvec4& v)
{
	int loc = GetUniformLocation(name);

	if (loc != -1)
	{
		glUniform4ui(loc, v.x, v.y, v.z, v.w);
	}
}

void CShaderGL::setUniform(const std::string& name, const glm::mat3& matrix)
{
	int loc = GetUniformLocation(name);

	if (loc != -1)
	{
		glUniformMatrix3fv(loc, 1, GL_FALSE, glm::value_ptr(matrix));
	}
}

void CShaderGL::setUniform(const std::string& name, const glm::mat4& matrix)
{
	int loc = GetUniformLocation(name);

	if (loc != -1)
	{
		glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(matrix));
	}
}

void CShaderGL::setTextureHandleUniform(const std::string& name, std::uint64_t handle)
{
	int loc = GetUniformLocation(name);

	if (loc != -1)
	{
		glUniform2ui(loc, reinterpret_cast<unsigned int*>(&handle)[0], reinterpret_cast<unsigned int*>(&handle)[1]);
	}
}

void CShaderGL::setUniformBuffer(const std::string& name, GLuint id, GLuint binding)
{
	int loc = GetUniformBlockLocation(name);
	//Game.Console.Print("loc of \"" + name + "\" is " + std::to_string(loc) + " and id is " + std::to_string(id));
	if (loc != -1)
	{
		glUniformBlockBinding(ID, loc, binding);
		glBindBufferBase(GL_UNIFORM_BUFFER, binding, id);
	}
}

int CShaderGL::getUboLayoutSize(const std::string& name)
{
	GLint blockSize;
	glGetActiveUniformBlockiv(ID, GetUniformBlockLocation(name), GL_UNIFORM_BLOCK_DATA_SIZE, &blockSize);

	return blockSize;
}

std::vector<GLuint> CShaderGL::getUboLayoutIndices(const std::string& name, std::vector<std::string> names)
{
	char** result = new char* [names.size()];
	for (int index = 0; index < names.size(); index++) 
	{
		result[index] = const_cast<char*>(names[index].c_str());
	}
	
	std::vector<GLuint> ret = getUboLayoutIndices(name, const_cast<const char**>(result), names.size());

	delete[] result;
	return ret;
}

std::vector<int> CShaderGL::getUboLayoutOffsets(const std::string& name, std::vector<GLuint> indices)
{
	return getUboLayoutOffsets(name, indices.data(), indices.size());
}

std::vector<GLuint> CShaderGL::getUboLayoutIndices(const std::string& name, const char** names, size_t count)
{
	GLuint* indices = new GLuint[count];
	glGetUniformIndices(ID, count, names, indices);

	std::vector<GLuint> ret;
	ret.assign(indices, indices + count);

	delete[] indices;
	return ret;
}

std::vector<int> CShaderGL::getUboLayoutOffsets(const std::string& name, const GLuint* indices, size_t count)
{
	GLint* offset = new GLint[count];
	glGetActiveUniformsiv(ID, count, indices, GL_UNIFORM_OFFSET, offset);

	std::vector<GLint> ret;
	ret.assign(offset, offset + count);

	delete[] offset;
	return ret;
}
