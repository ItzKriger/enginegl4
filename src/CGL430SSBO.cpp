#include "CGL430SSBO.h"
#include "U_Log.h"

CGL430SSBO::~CGL430SSBO()
{
    if (m_BufferID) { glDeleteBuffers(1, &m_BufferID); }
}

void CGL430SSBO::Create(GLsizeiptr size, GLenum usage, GLuint bindingIndex)
{
    m_Usage = usage;
    m_BindingIndex = bindingIndex;

    glGenBuffers(1, &m_BufferID);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_BufferID);
    glBufferData(GL_SHADER_STORAGE_BUFFER, size, nullptr, m_Usage);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, m_BindingIndex, m_BufferID);
}

void CGL430SSBO::Bind() const
{
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, m_BindingIndex, m_BufferID);
}

void* CGL430SSBO::Map(GLenum access)
{
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_BufferID);
    return glMapBuffer(GL_SHADER_STORAGE_BUFFER, access);
}

void CGL430SSBO::Unmap()
{
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_BufferID);
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
}

void CGL430SSBO::UploadData(GLintptr offset, GLsizeiptr size, const void* data)
{
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_BufferID);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, offset, size, data);
}

bool CGL430SSBO::ReflectStructArray(GLuint program, const std::string& blockName, const std::string& arrayName)
{
    m_FieldMap.clear();
    m_ArrayStride = 0;

    GLuint blockIndex = glGetProgramResourceIndex(program, GL_SHADER_STORAGE_BLOCK, blockName.c_str());
    if (blockIndex == GL_INVALID_INDEX) { return false; }

    GLint numVars = 0;
    glGetProgramInterfaceiv(program, GL_BUFFER_VARIABLE, GL_ACTIVE_RESOURCES, &numVars);

    const GLenum props[] = { GL_NAME_LENGTH, GL_OFFSET, GL_ARRAY_STRIDE };

    for (int i = 0; i < numVars; ++i)
    {
        GLint nameLen = 0;
        glGetProgramResourceiv(program, GL_BUFFER_VARIABLE, i, 1, (GLenum[]){ GL_NAME_LENGTH }, 1, nullptr, &nameLen);

        std::vector<GLchar> name(nameLen);
        glGetProgramResourceName(program, GL_BUFFER_VARIABLE, i, nameLen, nullptr, name.data());
        std::string fullName(name.data());

        std::string prefix = arrayName + "[0].";
        if (!fullName.starts_with(prefix)) continue;

        std::string fieldName = fullName.substr(prefix.length());

        FieldInfo f;
        f.Name = fieldName;

        glGetProgramResourceiv(program, GL_BUFFER_VARIABLE, i, 1, (GLenum[]){ GL_OFFSET }, 1, nullptr, &f.Offset);

        GLint arrayStride = 0;
        GLenum strideProp = GL_ARRAY_STRIDE;
        glGetProgramResourceiv(program, GL_BUFFER_VARIABLE, i, 1, &strideProp, 1, nullptr, &arrayStride);

        if (arrayStride > 0)
        {
            m_ArrayStride = arrayStride;
        }

        m_FieldMap[fieldName] = f;
    }
    Log::Instance() << "Array stride is " << m_ArrayStride << Log::Endl;
    return true;
}

bool CGL430SSBO::ReflectFields(GLuint program, const std::string& blockName)
{
    m_FieldMap.clear();
    m_ArrayStride = 0;

    GLuint blockIndex = glGetProgramResourceIndex(program, GL_SHADER_STORAGE_BLOCK, blockName.c_str());
    if (blockIndex == GL_INVALID_INDEX) { return false; }

    GLint numVars = 0;
    glGetProgramInterfaceiv(program, GL_BUFFER_VARIABLE, GL_ACTIVE_RESOURCES, &numVars);

    const GLenum props[] = { GL_NAME_LENGTH, GL_OFFSET };

    for (int i = 0; i < numVars; ++i)
    {
        GLint nameLen = 0;
        glGetProgramResourceiv(program, GL_BUFFER_VARIABLE, i, 1, &props[0], 1, nullptr, &nameLen);

        std::vector<GLchar> name(nameLen);
        glGetProgramResourceName(program, GL_BUFFER_VARIABLE, i, nameLen, nullptr, name.data());
        std::string fieldName(name.data());

        GLint values[2];
        glGetProgramResourceiv(program, GL_BUFFER_VARIABLE, i, 2, &props[1], 2, nullptr, values);

        FieldInfo f;
        f.Name = fieldName;
        f.Offset = values[0];

        m_FieldMap[fieldName] = f;
    }

    return true;
}

GLint CGL430SSBO::GetArrayStride() const
{
    return m_ArrayStride;
}

GLint CGL430SSBO::GetFieldOffset(const std::string& fieldName, int arrayIndex) const
{
    auto it = m_FieldMap.find(fieldName);
    if (it == m_FieldMap.end()) { return -1; }

    return it->second.Offset + arrayIndex * m_ArrayStride;
}
