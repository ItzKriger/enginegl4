#pragma once
#include <GL/glew.h>
#include <string>
#include <unordered_map>
#include <vector>
#include <iostream>

class CGL430SSBO {
public:
    CGL430SSBO() = default;
    ~CGL430SSBO();

    void Create(GLsizeiptr size, GLenum usage, GLuint bindingIndex);
    void Bind() const;
    void* Map(GLenum access = GL_WRITE_ONLY);
    void Unmap();

    void UploadData(GLintptr offset, GLsizeiptr size, const void* data);

    // Reflect array struct: structName[] in SSBO
    bool ReflectStructArray(GLuint program, const std::string& blockName, const std::string& arrayName);

    // Reflect top-level flat fields
    bool ReflectFields(GLuint program, const std::string& blockName);

    GLint GetArrayStride() const;
    GLint GetFieldOffset(const std::string& fieldName, int arrayIndex = 0) const;

private:
    GLuint m_BufferID = 0;
    GLenum m_Usage = GL_DYNAMIC_DRAW;
    GLuint m_BindingIndex = 0;

    struct FieldInfo
    {
        std::string Name;
        GLint Offset;
    };

    GLint m_ArrayStride = 0;
    std::unordered_map<std::string, FieldInfo> m_FieldMap;
};
