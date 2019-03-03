#include "shader.hpp"
#include <assert.h>
#include "logger.hpp"

namespace Shader
{

GLuint
LoadFromFile(const char* path, LinearAllocator allocator)
{
    assert(path);

    LOG_DEBUG("Making shader program for %s\n", path);

    GLuint vertex_shader, fragment_shader, program = 0;
    GLchar info[512] = {};
    GLint success;

    char* shader_string = nullptr;
    {
        FILE* fp = fopen(path, "rb");
        assert(fp && "shader file should exist");

        // take the size of the file
        fseek(fp, 0, SEEK_END);
        size_t file_size = ftell(fp);
        fseek(fp, 0, SEEK_SET);

        // allocate enough memory
        shader_string = (char*)allocator.Allocate(file_size);
        assert(shader_string && "there should be enough memory here");

        // read the file into the buffer
        size_t nread = fread(shader_string, 1, file_size, fp);
        assert(nread == file_size && "the correct amount of bytes should be read");

        fclose(fp);
    }

    vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);

    if (vertex_shader == 0 || fragment_shader == 0) {
        LOG_ERROR("failed to create shaders (glCreateShader)\n");
        goto cleanup_shaders;
    }

    {
        const char *vertex_string[3] = {
            "#version 330 core\n",
            "#define VERTEX_SHADER\n",
            shader_string,
        };
        glShaderSource(vertex_shader, 3, &vertex_string[0], NULL);

        const char *fragment_string[3] = {
            "#version 330 core\n",
            "#define FRAGMENT_SHADER\n",
            shader_string,
        };
        glShaderSource(fragment_shader, 3, &fragment_string[0], NULL);
    }

    glCompileShader(vertex_shader);
    glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &success);

    if (!success) {
        glGetShaderInfoLog(vertex_shader, 512, NULL, info);
        LOG_ERROR("Vertex shader compilation failed: %s\n", info);
        program = 0;
        goto cleanup_shaders;
    }

    glCompileShader(fragment_shader);
    glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &success);

    if (!success) {
        glGetShaderInfoLog(fragment_shader, 512, NULL, info);
        LOG_ERROR("Fragment shader compilation failed: %s\n", info);
        program = 0;
        goto cleanup_shaders;
    }

    program = glCreateProgram();
    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
    glLinkProgram(program);
    glGetProgramiv(program, GL_LINK_STATUS, &success);

    if (!success) {
        glGetShaderInfoLog(fragment_shader, 512, nullptr, info);
        LOG_ERROR("Shader linking failed: %s\n", info);
        program = 0;
        goto cleanup_shaders;
    }

cleanup_shaders:
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);

    return program;
}

}