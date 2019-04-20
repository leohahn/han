#include "TextureCatalog.hpp"

#include "FileSystem.hpp"
#include "Logger.hpp"
#include "glad/glad.h"
#include "stb_image.h"

static Texture* LoadTextureFromFile(Allocator* allocator,
                                    Allocator* scratch_allocator,
                                    const StringView& texture_file);

void
TextureCatalog::Destroy()
{
    assert(allocator);
    
    for (size_t i = 0; i < textures.len; ++i) {
        textures[i]->name.Destroy();
        allocator->Delete(textures[i]);
    }

    textures.Destroy();
    allocator = nullptr;
}

Texture*
TextureCatalog::GetTexture(const StringView& name)
{
    Texture* tex = nullptr;

    for (size_t i = 0; i < textures.len; ++i) {
        if (textures[i]->name == name) {
            tex = textures[i];
            break;
        }
    }
    
    assert(tex);
    return tex;
}

void
TextureCatalog::LoadTexture(const StringView& texture_file)
{
    assert(texture_file.data[texture_file.len] == 0);
    Texture* new_texture = LoadTextureFromFile(allocator, scratch_allocator, texture_file);
    textures.PushBack(new_texture);
}

static Texture*
LoadTextureFromFile(Allocator* allocator,
                    Allocator* scratch_allocator,
                    const StringView& texture_file)
{
    assert(allocator);
    assert(scratch_allocator);

    Path resources_path = FileSystem::GetResourcesPath(scratch_allocator);

    Path full_asset_path(scratch_allocator);
    full_asset_path.Push(resources_path);
    full_asset_path.Push(texture_file);

    Texture* texture = allocator->New<Texture>(allocator);
    texture->name.Append(texture_file);

    size_t texture_buffer_size;
    uint8_t* texture_buffer =
        FileSystem::LoadFileToMemory(allocator, full_asset_path, &texture_buffer_size);
    int texture_width, texture_height, texture_channel_count;

    assert(texture_buffer);

    // memory was read, now load it into an opengl buffer

    // tell stb_image.h to flip loaded texture's on the y-axis.
    stbi_set_flip_vertically_on_load(true);
    uint8_t* data = stbi_load_from_memory(texture_buffer,
                                          texture_buffer_size,
                                          &texture_width,
                                          &texture_height,
                                          &texture_channel_count,
                                          0);

    texture->width = texture_width;
    texture->height = texture_height;

    if (!data) {
        LOG_ERROR("Failed to load texture at %s", full_asset_path.data);
        goto cleanup_texture_buffer;
    }

    glGenTextures(1, &texture->handle);
    glBindTexture(GL_TEXTURE_2D, texture->handle);

    glTexParameteri(GL_TEXTURE_2D,
                    GL_TEXTURE_WRAP_S,
                    GL_REPEAT); // set texture wrapping to GL_REPEAT (default wrapping method)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    // set texture filtering parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage2D(
        GL_TEXTURE_2D, 0, GL_RGB, texture_width, texture_height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    free(data);
    
cleanup_texture_buffer:
    allocator->Deallocate(texture_buffer);
    full_asset_path.Destroy();
    resources_path.Destroy();

    texture->loaded = true;
    return texture;
}
