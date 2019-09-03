#include "Camera.hpp"
#include "Han/FileSystem.hpp"
#include "InputSystem.hpp"
#include "Han/MallocAllocator.hpp"
#include "OpenGL.hpp"
#include "PlayerInput.hpp"
#include "Program.hpp"
#include "Renderer.hpp"
#include "ResourceManager.hpp"
#include "ResourceFile.hpp"
#include "Texture.hpp"
#include "TriangleMesh.hpp"
#include "Math.hpp"
#include "Utils.hpp"
#include <SDL.h>
#include <SDL_opengl.h>
#include <assert.h>
#include <stdio.h>
#include "Json.hpp"
#include "Renderer/Buffer.hpp"
#include "Renderer/LowLevel.hpp"
#include "Han/Application.hpp"
#include "GameInstance.hpp"

#define SCREEN_WIDTH 1024
#define SCREEN_HEIGHT 768

// HACK: remove windows.h from here
#include <Windows.h>

static TriangleMesh
SetupPlane(Allocator* allocator, Allocator* scratch_allocator, Material* material)
{
    // clang-format off
    static const uint32_t indices[] =
    {
        0, 1, 2, 2, 3, 0
    };

    static const Vec3 vertices[] =
    {
        Vec3(-1.0f, -1.0f,  0.0f), // 0
        Vec3( 1.0f, -1.0f,  0.0f), // 1
        Vec3( 1.0f,  1.0f,  0.0f), // 2
        Vec3(-1.0f,  1.0f,  0.0f), // 3
    };

    static const Vec2 uvs[] =
    {
        Vec2(0.0f, 0.0f), // 0
        Vec2(1.0f, 0.0f), // 1
        Vec2(1.0f, 1.0f), // 2
        Vec2(0.0f, 1.0f), // 3
    };

    static_assert(ARRAY_SIZE(vertices) == ARRAY_SIZE(uvs),
                  "vertices and tex_coords should be the same size");

    // clang-format on
    TriangleMesh mesh(allocator);
    mesh.name = SID("Plane");

    // Fill indices up
    for (size_t i = 0; i < ARRAY_SIZE(indices); ++i) {
        mesh.indices.PushBack(indices[i]);
    }

    // Fill vertices up
    for (size_t i = 0; i < ARRAY_SIZE(vertices); ++i) {
        mesh.vertices.PushBack(vertices[i]);
    }

    // Fill uvs up
    for (size_t i = 0; i < ARRAY_SIZE(uvs); ++i) {
        mesh.uvs.PushBack(uvs[i]);
    }

    assert(mesh.vertices.len == mesh.uvs.len);

    //--------------------------------------------
    // Mesh setup
    //--------------------------------------------

    // Build the buffer that is going to be uploaded to the GPU.
    Array<OpenGL::Vertex_PT> buffer(scratch_allocator);
    for (size_t i = 0; i < mesh.vertices.len; ++i) {
        buffer.PushBack(OpenGL::Vertex_PT(mesh.vertices[i], mesh.uvs[i]));
    }

    auto vbo = VertexBuffer::Create(allocator, (float*)buffer.data, buffer.len * sizeof(OpenGL::Vertex_PT));
    vbo->SetLayout(BufferLayout(allocator, {
        BufferLayoutDataType::Vec3, // postion
        BufferLayoutDataType::Vec2, // texture coord
    }));

    auto ibo = IndexBuffer::Create(allocator, mesh.indices.data, mesh.indices.len);

    SubMesh submesh = {};
    submesh.vao = VertexArray::Create(allocator);
    submesh.vao->SetIndexBuffer(ibo);
    submesh.vao->SetVertexBuffer(vbo);
    submesh.start_index = 0;
    submesh.num_indices = mesh.indices.len;
    submesh.material = material;
    mesh.sub_meshes.PushBack(std::move(submesh));

    return mesh;
}

// TODO: receive a texture catalog
static TriangleMesh
SetupCube(Allocator* allocator, Allocator* scratch_allocator, Material* material)
{
    ASSERT(material, "material should exist");
    ASSERT(allocator, "allocator should exist");
    ASSERT(scratch_allocator, "scratch allocator should exist");
    // clang-format off
    static const uint32_t indices[] =
    {
        0,  1,  2,  2,  3,  0,
        4,  5,  6,  6,  7,  4,
        8,  9, 10, 10, 11,  8,
        12, 13, 14, 14, 15, 12,
        16, 17, 18, 18, 19, 16,
        20, 21, 22, 22, 23, 20,
    };
    
    static const Vec3 vertices[] =
    {
        // FRONT
        Vec3(-1, -1,  1), // 0
        Vec3(1, -1,  1), // 1
        Vec3(1,  1,  1), // 2
        Vec3(-1,  1,  1), // 3
        // BACK
        Vec3(1, -1, -1), // 4
        Vec3(-1, -1, -1), // 5
        Vec3(-1,  1, -1), // 6
        Vec3(1,  1, -1), // 7
        // TOP
        Vec3(-1,  1,  1), // 8
        Vec3(1,  1,  1), // 9
        Vec3(1,  1, -1), // 10
        Vec3(-1,  1, -1), // 11
        // BOTTOM
        Vec3(1, -1,  1), // 12
        Vec3(-1, -1,  1), // 13
        Vec3(-1, -1, -1), // 14
        Vec3(1, -1, -1), // 15
        // LEFT
        Vec3(-1, -1, -1), // 16
        Vec3(-1, -1,  1), // 17
        Vec3(-1,  1,  1), // 18
        Vec3(-1,  1, -1), // 19
        // RIGHT
        Vec3(1, -1,  1), // 20
        Vec3(1, -1, -1), // 21
        Vec3(1,  1, -1), // 22
        Vec3(1,  1,  1), // 23
    };
    
    static const Vec2 uvs[] =
    {
        // FRONT
        Vec2(0, 0), // 0
        Vec2(1, 0),// 1
        Vec2(1, 1),// 2
        Vec2(0, 1),// 3
        // BACK
        Vec2(0, 0), // 4
        Vec2(1, 0), // 5
        Vec2(1, 1), // 6
        Vec2(0, 1), // 7
        // TOP
        Vec2(0, 0), // 8
        Vec2(1, 0), // 9
        Vec2(1, 1), // 10
        Vec2(0, 1), // 11
        // BOTTOM
        Vec2(0, 0), // 12
        Vec2(1, 0), // 13
        Vec2(1, 1), // 14
        Vec2(0, 1), // 15
        // LEFT
        Vec2(0, 0), // 16
        Vec2(1, 0), // 17
        Vec2(1, 1), // 18
        Vec2(0, 1), // 19
        // RIGHT
        Vec2(0, 0), // 20
        Vec2(1, 0), // 21
        Vec2(1, 1), // 22
        Vec2(0, 1), // 23
    };

    // clang-format on

    TriangleMesh mesh(allocator);
    mesh.name = SID("Cube");

    // Fill indices up
    for (size_t i = 0; i < ARRAY_SIZE(indices); ++i) {
        mesh.indices.PushBack(indices[i]);
    }

    // Fill vertices up
    for (size_t i = 0; i < ARRAY_SIZE(vertices); ++i) {
        mesh.vertices.PushBack(vertices[i]);
    }

    // Fill uvs up
    for (size_t i = 0; i < ARRAY_SIZE(uvs); ++i) {
        mesh.uvs.PushBack(uvs[i]);
    }

    assert(mesh.vertices.len == mesh.uvs.len);

    //--------------------------------------------
    // Mesh setup
    //--------------------------------------------

    // Build the buffer that is going to be uploaded to the GPU.
    Array<OpenGL::Vertex_PT> buffer(scratch_allocator);
    for (size_t i = 0; i < mesh.vertices.len; ++i) {
        buffer.PushBack(OpenGL::Vertex_PT(mesh.vertices[i], mesh.uvs[i]));
    }

    auto vbo = VertexBuffer::Create(allocator, (float*)buffer.data, buffer.len * sizeof(OpenGL::Vertex_PT));
    vbo->SetLayout(BufferLayout(allocator, {
        BufferLayoutDataType::Vec3, // position
        BufferLayoutDataType::Vec2, // texture
    }));

    auto ibo = IndexBuffer::Create(allocator, mesh.indices.data, mesh.indices.len);

    SubMesh submesh = {};
    submesh.vao = VertexArray::Create(allocator);
    submesh.vao->SetIndexBuffer(ibo);
    submesh.vao->SetVertexBuffer(vbo);
    submesh.start_index = 0;
    submesh.num_indices = mesh.indices.len;
    submesh.material = material;
    mesh.sub_meshes.PushBack(std::move(submesh));

    return mesh;
}

static void
OnApplicationQuit(SDL_Event ev, void* user_data)
{
    bool* running = (bool*)user_data;
    *running = false;
}

//ResourceManager* g_debug_resource_manager = nullptr;

int
main(int argc, char** argv)
{
	// unused args
	(void)argc;
	(void)argv;

    // Instantiate a new program with preallocated memory
    Program program;
    InitProgram(&program, MEGABYTES(128), SCREEN_WIDTH, SCREEN_HEIGHT);

#ifdef _DEBUG
//    g_debug_resource_manager = &resource_manager;
#endif

    // Create the main camera
    Camera camera(Vec3(0, 0, 5), Vec3(0, 0, -1), (float)SCREEN_WIDTH / SCREEN_HEIGHT, 60.0f, 0.1f, 500.0f);

    //
    // Load shaders
    //
    LOG_DEBUG("Loading shaders\n");

    program.resource_manager->LoadShader(SID("flat_color.glsl"));
    Shader* flat_color_shader = program.resource_manager->GetShader(SID("flat_color.glsl"));
    assert(flat_color_shader && flat_color_shader->IsValid() && "program should be valid");
    flat_color_shader->AddUniform("u_model");
    flat_color_shader->AddUniform("u_view_projection");
    flat_color_shader->AddUniform("u_flat_color");

    program.resource_manager->LoadShader(SID("basic.glsl"));
    Shader* basic_shader = program.resource_manager->GetShader(SID("basic.glsl"));
    assert(basic_shader && basic_shader->IsValid() && "program should be valid");
    basic_shader->AddUniform("u_model");
    basic_shader->AddUniform("u_view");
    basic_shader->AddUniform("u_projection");
    basic_shader->AddUniform("u_input_texture");

    program.resource_manager->LoadShader(SID("gltf.glsl"));
    Shader* gltf_shader = program.resource_manager->GetShader(SID("gltf.glsl"));
    assert(gltf_shader && gltf_shader->IsValid() && "program should be valid");
    gltf_shader->AddUniform("u_model");
    gltf_shader->AddUniform("u_view");
    gltf_shader->AddUniform("u_projection");
    gltf_shader->AddUniform("u_input_texture");

    program.resource_manager->LoadShader(SID("pbr.glsl"));
    Shader* pbr_shader = program.resource_manager->GetShader(SID("pbr.glsl"));
    assert(pbr_shader && pbr_shader->IsValid() && "program should be valid");
    pbr_shader->AddUniform("u_model");
    pbr_shader->AddUniform("u_view_projection");
    pbr_shader->AddUniform("u_albedo_texture");
    pbr_shader->AddUniform("u_normal_texture");
    pbr_shader->AddUniform("u_metallic_roughness_texture");
    pbr_shader->AddUniform("u_occlusion_texture");
    pbr_shader->AddUniform("u_camera_position");
    pbr_shader->AddUniform("u_light_position");
    pbr_shader->AddUniform("u_light_color");
    pbr_shader->AddUniform("u_metallic_factor");
    pbr_shader->AddUniform("u_roughness_factor");

    //
    // Create the input system
    //
    InputSystem input_system;
    input_system.Create(&program.main_allocator);
    input_system.AddKeyboardEventListener(
        kKeyboardEventButtonDown, SDLK_q, OnApplicationQuit, &program.running, &program.main_allocator);

    PlayerInput player_input;
    player_input.RegisterInputs(&input_system, &program.main_allocator);

    basic_shader->Bind();
    basic_shader->SetUniformMat4(SID("u_projection"), camera.projection_matrix);

    gltf_shader->Bind();
    gltf_shader->SetUniformMat4(SID("u_projection"), camera.projection_matrix);

    //------------------------------
    // Create the texture catalog and textures
    //------------------------------
    program.resource_manager->LoadTexture(SID("wall.jpg"), LoadTextureFlags_FlipVertically|LoadTextureFlags_LinearSpace);

    Texture* wall_texture = program.resource_manager->GetTexture(SID("wall.jpg"));
    LOG_DEBUG("Loaded texture named: %s", wall_texture->name.GetStr());
    LOG_DEBUG("       width: %d", wall_texture->width);
    LOG_DEBUG("       height: %d", wall_texture->height);

    // HACK: load this material from a file instead of doing it like this
    {
        Material* material = program.resource_manager->allocator->New<Material>(program.resource_manager->allocator);
        material->name = SID("wall");
        material->shader = basic_shader;
        material->AddValue(SID("u_input_texture"), MaterialValue(wall_texture));
        program.resource_manager->materials.Add(material->name, material);
    }

    // DEBUG meshes for testing
    Material* wall_material = program.resource_manager->GetMaterial(SID("wall"));
    assert(wall_material);
    TriangleMesh floor_mesh = SetupPlane(&program.main_allocator, &program.temp_allocator, wall_material);
    TriangleMesh cube_mesh = SetupCube(&program.main_allocator, &program.temp_allocator, wall_material);

    {
        Material* material = program.resource_manager->allocator->New<Material>(program.resource_manager->allocator);
        material->name = SID("flat_color");
        material->shader = flat_color_shader;
        material->AddValue(SID("u_flat_color"), MaterialValue(Vec4(1.0f)));
        program.resource_manager->materials.Add(material->name, material);
    }

    TriangleMesh light_mesh = SetupCube(&program.main_allocator, &program.temp_allocator, program.resource_manager->GetMaterial(SID("flat_color")));

    Model alpine_chalet = program.resource_manager->LoadModel(SID("Alpine_chalet.model"));
    Model hammer = program.resource_manager->LoadModel(SID("hammer.model"));
    Model nanosuit = program.resource_manager->LoadModel(SID("nanosuit.model"));

    LOG_DEBUG("Starting main loop");
    Graphics::LowLevelApi::SetClearColor(Vec4(0.2f, 0.2f, 0.2f, 1.0f));

    // Now, load a game instance
    GameInstance* game_instance = GameInstance::Create(&program.main_allocator, program.game_dll_path.data);
    if (!game_instance->Load()) {
        LOG_ERROR("Failed to load game library");
        UNREACHABLE;
    }

    while (program.running) {
        input_system.Update();

        if (input_system.ReceivedQuitEvent()) {
            LOG_INFO("Received quit event, exiting application");
            break;
        }

        if (player_input.IsMovingLeft()) {
            camera.MoveLeft(0.01f);
        }

        if (player_input.IsMovingRight()) {
            camera.MoveRight(0.01f);
        }

        if (player_input.IsTurningLeft()) {
            camera.Rotate(Vec3(0, 1, 0));
        }

        if (player_input.IsTurningRight()) {
            camera.Rotate(Vec3(0, -1, 0));
        }

        if (player_input.IsTurningAbove()) {
            camera.Rotate(camera.right);
        }

        if (player_input.IsTurningBelow()) {
            camera.Rotate(-camera.right);
        }

        if (player_input.IsMovingForwards()) {
            camera.MoveForwards(0.01f);
        }

        if (player_input.IsMovingBackwards()) {
            camera.MoveBackwards(0.01f);
        }

        //
        // Update the game instance
        //
        game_instance->GetApplication()->Update();

        //
        // Start rendering part of the main loop
        //
        Graphics::LowLevelApi::ClearBuffers();

        auto ticks = SDL_GetTicks();

        auto view_matrix = camera.GetViewMatrix();
        auto view_projection_matrix = camera.GetViewProjectionMatrix(view_matrix);

        basic_shader->Bind();
        basic_shader->SetUniformMat4(SID("u_view"), camera.GetViewMatrix());

        // TODO: add real values here for the parameters
        Vec3 cube_position(10.0f, 0.0f, 0.0f);
        Quaternion cube_orientation =
            Quaternion::Rotation(Math::DegreesToRadians(ticks * 0.035f), Vec3(0, 1, 0));
        float cube_scale = 1.0f;
        RenderMesh(cube_mesh, *basic_shader, cube_position, cube_orientation, cube_scale);

        Vec3 floor_position(0, -5, 3);
        Quaternion floor_orientation =
            Quaternion::Rotation(Math::DegreesToRadians(90), Vec3(-1, 0, 0));
        float floor_scale = 50.0f;
        RenderMesh(floor_mesh, *basic_shader, floor_position, floor_orientation, floor_scale);

        Vec3 nanosuit_position(-10, 0, 0);
        Quaternion nanosuit_orientation = Quaternion::Identity();
        assert(nanosuit.meshes.len == 1);
        RenderMesh(*nanosuit.meshes[0], *basic_shader, nanosuit_position, nanosuit_orientation, 1.0f);

        Vec3 light_position(0.0f, 10.0f, 10.0f);
        flat_color_shader->Bind();
        flat_color_shader->SetUniformMat4(SID("u_view_projection"), view_projection_matrix);
        RenderMesh(light_mesh, *flat_color_shader, light_position, Quaternion::Identity(), 0.4f);

        gltf_shader->Bind();
        gltf_shader->SetUniformMat4(SID("u_view"), camera.GetViewMatrix());

        pbr_shader->Bind();
        pbr_shader->SetUniformMat4(SID("u_view_projection"), view_projection_matrix);
        pbr_shader->SetVector(SID("u_camera_position"), camera.position);
        pbr_shader->SetVector(SID("u_light_position"), light_position);
        pbr_shader->SetVector(SID("u_light_color"), Vec3(1.0f));
        //RenderModel(alpine_chalet, *gltf_shader, Vec3::Zero(), Quaternion::Identity(), 1.0f);
        auto hammer_rotation = Quaternion::Rotation(Math::DegreesToRadians(90), Vec3(1, 0, 0));
        //auto hammer_rotation = Quaternion::Identity();
        RenderModel(hammer, *pbr_shader, Vec3::Zero(), hammer_rotation, 1.0f);

        RenderModel(alpine_chalet, *pbr_shader, Vec3(20, 1, 0), Quaternion::Identity(), 1.0f);

        program.window->SwapBuffers();
    }

    LOG_INFO("Deallocating main resources");
    LOG_DEBUG("Total memory used by heap allocator: %s",
              Utils::GetPrettySize(program.temp_allocator.GetBytesWaterMark()));

    game_instance->GetApplication()->OnShutdown();

    input_system.Destroy();
    TerminateProgram(&program);
	return 0;
}
