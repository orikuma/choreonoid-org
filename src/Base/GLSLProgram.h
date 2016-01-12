/**
   @author Shin'ichiro Nakaoka
*/

#ifndef CNOID_BASE_GLSL_PROGRAM_H
#define CNOID_BASE_GLSL_PROGRAM_H

/**
   The following header and the corresponding .c file were generated by OpenGL Loader Generator (glLoadGen)
   with the command: lua LoadGen.lua -style=pointer_c -spec=gl -version=3.3 -profile=core core_3_3.
*/
#include "gl_core_3_3.h"

#include <stdexcept>

namespace cnoid {

class GLSLProgram
{
public:
    class Exception : public std::runtime_error {
    public:
        Exception(const std::string& msg) : std::runtime_error(msg) { }
    };
    
    GLSLProgram();
    ~GLSLProgram();

    void release();
    void loadVertexShader(const char* filename) throw (Exception);
    void loadFragmentShader(const char* filename) throw (Exception);
    void loadShader(const char* filename, int shaderType) throw(Exception);
    void link() throw (Exception);
    void validate() throw(Exception);
    void use() throw (Exception);
    
    GLint handle() const { return programHandle; }
    bool isLinked() const { return isLinked_; }

    GLuint getUniformLocation(const char* name) const {
        return glGetUniformLocation(programHandle, name);
    }

private:
    GLuint programHandle;
    bool isLinked_;

    GLSLProgram(const GLSLProgram& other) { }
    GLSLProgram& operator=(const GLSLProgram& other) { return *this; }
};

}

#endif
