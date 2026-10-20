#include "idk/gfx/RenderEngine.hpp"
#include "idk/asset/ModelImporter.hpp"
#include "idk/EngineAPI.hpp"
#include "libidk/gpu/gl.hpp"
#include <SDL3/SDL.h>
#include <glm/gtc/matrix_transform.hpp>
#include <array>
#include <cstdio>
#include <cstring>

using namespace idk::gfx;

namespace
{
    int errors = 0;

    void APIENTRY debugCallback(GLenum, GLenum type, GLuint, GLenum, GLsizei, const GLchar *, const void *)
    {
        if (type == GL_DEBUG_TYPE_ERROR)
            ++errors;
    }

#define CHECK(value, message)                                                                                          \
    do                                                                                                                 \
    {                                                                                                                  \
        if (!(value))                                                                                                  \
        {                                                                                                              \
            std::fprintf(stderr, "%s\n", message);                                                                     \
            return false;                                                                                              \
        }                                                                                                              \
    } while (false)

    std::array<uint8_t, 4> pixel(int x, int y)
    {
        std::array<uint8_t, 4> result{};
        glReadPixels(x, y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, result.data());
        return result;
    }

    bool checkerChecks(RenderEngine &renderer, idk::EngineAPI &api)
    {
        using namespace idk::asset;
        Vertex vertices[4];
        vertices[0].position = {-1, -1, 0};
        vertices[1].position = {1, -1, 0};
        vertices[2].position = {1, 1, 0};
        vertices[3].position = {-1, 1, 0};
        vertices[0].uv = {0, 0};
        vertices[1].uv = {1, 0};
        vertices[2].uv = {1, 1};
        vertices[3].uv = {0, 1};
        uint32_t indices[] = {0, 1, 2, 0, 2, 3};
        const VertexAttribute attributes[] = {
            {0, VertexAttribType::Float3, offsetof(Vertex, position)},
            {1, VertexAttribType::Float2, offsetof(Vertex, uv)}};
        auto mesh = renderer.CreateMesh({vertices, 4, sizeof(Vertex), indices, 6, attributes});
        MaterialDesc description;
        description.shader = renderer.GetDefaultShader();
        description.baseColor = {0, 1, 0, 1};
        description.pattern = MaterialPattern::Checker;
        auto material = renderer.CreateMaterial(description);
        CHECK(mesh.isValid() && material.isValid(), "Could not create procedural material resources");
        renderer.SetCamera({});
        renderer.onPreRender(api);
        CHECK(renderer.Submit({mesh, material}), "Checker submission failed");
        renderer.onMidRender(api);
        auto light = pixel(4, 4), dark = pixel(12, 4);
        CHECK(light[1] > 240 && dark[1] > 45 && dark[1] < 70 &&
                  light[0] < 10 && dark[0] < 10 && dark[3] > 240,
              "Procedural material must alternate tinted checks without a texture");
        renderer.onPreRender(api);
        CHECK(renderer.Submit({mesh, material, glm::scale(glm::mat4(1), glm::vec3(3, 2, 1))}),
              "Scaled checker submission failed");
        renderer.onMidRender(api);
        CHECK(pixel(4, 4) == light && pixel(12, 4) == dark,
              "Nonuniform scaling must not stretch procedural checks");
        const glm::vec3 axes[] = {{1, 0, 0}, {0, 1, 0}};
        for (auto axis : axes)
        {
            auto rotation = glm::rotate(glm::mat4(1), glm::radians(90.0f), axis);
            renderer.SetCamera({glm::inverse(rotation), glm::mat4(1)});
            renderer.onPreRender(api);
            CHECK(renderer.Submit({mesh, material, rotation}), "Side-face checker submission failed");
            renderer.onMidRender(api);
            auto a = pixel(4, 4), b = pixel(12, 4);
            CHECK((a[1] > 240 && b[1] > 45 && b[1] < 70) ||
                      (b[1] > 240 && a[1] > 45 && a[1] < 70),
                  "Checker must project onto vertical and horizontal faces");
            renderer.onPreRender(api);
            CHECK(renderer.Submit({mesh, material, glm::scale(rotation, glm::vec3(3, 2, 1))}),
                  "Scaled side-face checker submission failed");
            renderer.onMidRender(api);
            CHECK(pixel(4, 4) == a && pixel(12, 4) == b,
                  "Scaling a side face changed the world-space pattern");
        }
        renderer.SetCamera({});
        renderer.DestroyMaterial(material);
        renderer.DestroyMesh(mesh);
        description.pattern = MaterialPattern(99);
        CHECK(!renderer.CreateMaterial(description).isValid(), "Unknown material patterns must be rejected");

        Primitive primitive{"Checker", vertices, indices, 0};
        Material source;
        source.baseColor = {0, 1, 0, 1};
        source.pattern = MaterialPattern::Checker;
        uint32_t ref = 0;
        Node node;
        node.primitives = {&ref, 1};
        Model model;
        model.primitives = {&primitive, 1};
        model.materials = {&source, 1};
        model.nodes = {&node, 1};
        auto uploaded = renderer.UploadModel(model);
        CHECK(uploaded.isValid(), "Procedural model upload failed");
        renderer.onPreRender(api);
        CHECK(renderer.SubmitModel(uploaded), "Procedural model submission failed");
        renderer.onMidRender(api);
        CHECK(pixel(4, 4) == light && pixel(12, 4) == dark, "Model upload lost the procedural material");
        renderer.DestroyModel(uploaded);

        for (auto &vertex : vertices)
        {
            vertex.uv *= 64.0f;
            vertex.position *= glm::vec3(3, 2, 1);
        }
        uploaded = renderer.UploadModel(model);
        CHECK(uploaded.isValid(), "Baked procedural model upload failed");
        renderer.onPreRender(api);
        CHECK(renderer.SubmitModel(uploaded), "Baked procedural model submission failed");
        renderer.onMidRender(api);
        CHECK(pixel(4, 4) == light && pixel(12, 4) == dark,
              "Baked geometry and UV density must not change world-space check size");
        renderer.DestroyModel(uploaded);

        for (auto &vertex : vertices)
            vertex.position *= 64.0f;
        Camera distant;
        distant.proj = glm::scale(glm::mat4(1), glm::vec3(1.0f / 64, 1.0f / 64, 1));
        renderer.SetCamera(distant);
        uploaded = renderer.UploadModel(model);
        CHECK(uploaded.isValid(), "Minified procedural model upload failed");
        renderer.onPreRender(api);
        CHECK(renderer.SubmitModel(uploaded), "Minified procedural model submission failed");
        renderer.onMidRender(api);
        CHECK(pixel(8, 8)[1] > 150 && pixel(8, 8)[1] < 160 &&
                  pixel(24, 8)[1] > 150 && pixel(24, 8)[1] < 160,
              "Subpixel checks should filter to their average colour");
        renderer.DestroyModel(uploaded);
        renderer.SetCamera({});
        return true;
    }
}

bool runChecks(RenderEngine &renderer, idk::EngineAPI &api)
{
    glDebugMessageCallback(debugCallback, nullptr);

    struct Vertex
    {
        glm::vec3 pos;
        glm::vec2 uv;
    };

    const Vertex vertices[] = {
        {{-0.2f, -0.2f, 0}, {0, 0}},
        {{0.2f, -0.2f, 0}, {1, 0}},
        {{0, 0.2f, 0}, {0.5f, 1}},
    };
    const uint32_t indices[] = {0, 1, 2};
    const VertexAttribute attributes[] = {
        {0, VertexAttribType::Float3, offsetof(Vertex, pos)},
        {1, VertexAttribType::Float2, offsetof(Vertex, uv)},
    };
    MeshDesc meshDesc{vertices, 3, sizeof(Vertex), indices, 3, attributes};
    auto mesh = renderer.CreateMesh(meshDesc);
    CHECK(mesh.isValid(), "First generation-zero mesh must be valid");
    CHECK(!renderer.Submit({mesh, {}}), "Submissions outside pre-render must fail");
    auto invalidDesc = meshDesc;
    const uint32_t badIndices[] = {0, 1, 3};
    invalidDesc.indices = badIndices;
    CHECK(!renderer.CreateMesh(invalidDesc).isValid(), "Out-of-bounds mesh indices must fail");

    // Width 3 exercises tightly packed RGB rows and unpack-state restoration.
    const uint8_t texels[] = {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255};
    glPixelStorei(GL_UNPACK_ALIGNMENT, 8);
    auto texture = renderer.CreateTexture({3, 2, TexFormat::RGB8, texels, false});
    GLint unpack = 0;
    glGetIntegerv(GL_UNPACK_ALIGNMENT, &unpack);
    CHECK(unpack == 8, "Texture upload must restore unpack alignment");
    CHECK(texture.isValid(), "Texture must be created");

    MaterialDesc materialDesc;
    materialDesc.shader = renderer.GetDefaultShader();
    materialDesc.texture = texture;
    materialDesc.baseColor = {1, 0, 0, 1};
    auto material = renderer.CreateMaterial(materialDesc);
    CHECK(material.isValid(), "Material must be created");

    // A nonsymmetric model/view/projection combination catches transpose mistakes.
    Camera camera;
    camera.view = glm::translate(glm::mat4(1), glm::vec3(0.1f, 0, 0));
    camera.proj = glm::scale(glm::mat4(1), glm::vec3(0.75f, 0.9f, 1));
    renderer.SetCamera(camera);
    renderer.SetClearColor({0, 0, 0, 1});
    renderer.onPreRender(api);
    CHECK(renderer.Submit({mesh, material, glm::translate(glm::mat4(1), glm::vec3(0.5f, 0, 0))}),
          "Valid draw must be accepted");
    renderer.onMidRender(api);
    auto center = pixel(64, 64);
    auto translated = pixel(93, 64);
    CHECK(center[0] < 10 && translated[0] > 240 && translated[1] < 10,
          "Slang camera/model matrices or texture/material bindings are wrong");

    // Failed replacement must preserve the existing linked program.
    CHECK(!renderer.ReloadShader(renderer.GetDefaultShader(), {}), "Empty shader replacement must fail");
    renderer.onPreRender(api);
    renderer.Submit({mesh, material, glm::translate(glm::mat4(1), glm::vec3(0.5f, 0, 0))});
    renderer.onMidRender(api);
    CHECK(pixel(93, 64)[0] > 240, "Failed shader replacement must preserve drawing");

    // Material sorting draws near red first, then far green: depth must keep red.
    renderer.SetCamera({});
    MaterialDesc greenDesc = materialDesc;
    greenDesc.baseColor = {0, 1, 0, 1};
    auto green = renderer.CreateMaterial(greenDesc);
    renderer.onPreRender(api);
    renderer.Submit({mesh, material, glm::translate(glm::mat4(1), glm::vec3(0, 0, -0.4f))});
    renderer.Submit({mesh, green, glm::translate(glm::mat4(1), glm::vec3(0, 0, 0.4f))});
    renderer.onMidRender(api);
    center = pixel(64, 64);
    CHECK(center[0] > 240 && center[1] < 10, "Depth testing must preserve the nearest draw");

    // Submit near red before far green; sorting must blend far green first.
    MaterialDesc alphaDesc = materialDesc;
    alphaDesc.blend = BlendMode::Alpha;
    alphaDesc.baseColor = {1, 0, 0, 0.5f};
    auto alphaRed = renderer.CreateMaterial(alphaDesc);
    alphaDesc.baseColor = {0, 1, 0, 0.5f};
    auto alphaGreen = renderer.CreateMaterial(alphaDesc);
    renderer.onPreRender(api);
    renderer.Submit({mesh, alphaRed, glm::translate(glm::mat4(1), glm::vec3(0, 0, -0.2f))});
    renderer.Submit({mesh, alphaGreen, glm::translate(glm::mat4(1), glm::vec3(0, 0, -0.6f))});
    renderer.onMidRender(api);
    center = pixel(64, 64);
    CHECK(center[0] >= 125 && center[0] <= 130 && center[1] >= 61 && center[1] <= 67,
          "Transparent draws must blend back-to-front without depth writes");
    renderer.DestroyMaterial(alphaRed);
    renderer.DestroyMaterial(alphaGreen);

    // Queued resource destruction and slot reuse must never alias a stale handle.
    renderer.onPreRender(api);
    renderer.Submit({mesh, material});
    renderer.DestroyMesh(mesh);
    auto replacement = renderer.CreateMesh(meshDesc);
    CHECK(replacement.id == mesh.id && replacement.gen != mesh.gen, "Pool must increment generations on reuse");
    CHECK(!renderer.Submit({mesh, material}), "Stale handles must be rejected");
    renderer.onMidRender(api);
    CHECK(pixel(64, 64)[0] < 10, "Destroyed queued mesh must not draw its replacement");
    renderer.DestroyMesh(replacement);
    renderer.DestroyMaterial(material);
    renderer.DestroyMaterial(green);
    renderer.DestroyTexture(texture);
    // Exercise the complete disk -> CPU scene -> GPU resources -> pixels path.
    ModelHandle uploaded;
    {
        alignas(std::max_align_t) static std::byte modelBytes[2 * 1024 * 1024];
        alignas(std::max_align_t) static std::byte scratchBytes[4 * 1024 * 1024];
        idk::Arena output(modelBytes), scratch(scratchBytes);
        idk::asset::Model imported;
        idk::asset::ImportError error;
        char path[1024];
        std::snprintf(path, sizeof(path), "%s/scene.glb", IDK_ASSET_TEST_DATA);
        CHECK(idk::asset::ImportModel(path, output, scratch, imported, error), error.message);
        uploaded = renderer.UploadModel(imported);
        CHECK(uploaded.isValid(), "Imported model upload failed");
        output.reset();
        std::memset(modelBytes, 0, sizeof(modelBytes));
    } // The GPU model must not retain pointers into the CPU asset.
    renderer.SetCamera({});
    renderer.onPreRender(api);
    CHECK(renderer.SubmitModel(uploaded), "Imported model submission failed");
    renderer.onMidRender(api);
    for (int x : {35, 93})
    {
        auto topRowTexture = pixel(x, 61);
        auto bottomRowTexture = pixel(x, 74);
        CHECK(topRowTexture[0] > 195 && topRowTexture[2] < 10 && bottomRowTexture[2] > 240 && bottomRowTexture[0] < 10,
              "Imported texture orientation, parent transform, or mirrored instance is wrong");
    }
    renderer.onPreRender(api);
    CHECK(renderer.SubmitModel(uploaded), "Valid model must submit before destruction");
    renderer.DestroyModel(uploaded);
    CHECK(!renderer.SubmitModel(uploaded), "Destroyed model must reject new submissions");
    renderer.onMidRender(api);
    CHECK(pixel(35, 61)[0] < 10, "Queued destroyed model must not render");

    // Masked materials must discard; opaque materials ignore their alpha value.
    auto maskMesh = renderer.CreateMesh(meshDesc);
    MaterialDesc maskDesc;
    maskDesc.shader = renderer.GetDefaultShader();
    maskDesc.baseColor = {1, 0, 0, .25f};
    maskDesc.blend = BlendMode::Mask;
    auto maskMaterial = renderer.CreateMaterial(maskDesc);
    renderer.onPreRender(api);
    renderer.Submit({maskMesh, maskMaterial});
    renderer.onMidRender(api);
    CHECK(pixel(64, 64)[0] < 10, "Alpha mask must discard fragments below its cutoff");
    renderer.DestroyMaterial(maskMaterial);
    maskDesc.blend = BlendMode::Opaque;
    maskMaterial = renderer.CreateMaterial(maskDesc);
    renderer.onPreRender(api);
    renderer.Submit({maskMesh, maskMaterial});
    renderer.onMidRender(api);
    CHECK(pixel(64, 64)[0] > 240 && pixel(64, 64)[3] > 240, "Opaque alpha must be ignored");
    // An editor canvas occupies a subrectangle of the same drawable.
    renderer.onPreRender(api);
    renderer.SetViewport(64, 32, 64, 64);
    renderer.Submit({maskMesh, maskMaterial});
    renderer.onMidRender(api);
    CHECK(pixel(96, 64)[0] > 240 && pixel(64, 64)[0] < 10, "Scene must map into the editor viewport");
    renderer.onPreRender(api);
    renderer.SetViewport(0, 0, 0, 0);
    renderer.Submit({maskMesh, maskMaterial});
    renderer.onMidRender(api);
    CHECK(pixel(64, 64)[0] < 10, "Hidden editor viewport must not draw");
    renderer.onPreRender(api);
    renderer.Submit({maskMesh, maskMaterial});
    renderer.onMidRender(api);
    CHECK(pixel(64, 64)[0] > 240, "Viewport must reset to full window every frame");
    renderer.DestroyMaterial(maskMaterial);
    renderer.DestroyMesh(maskMesh);
    CHECK(checkerChecks(renderer, api), "Procedural material checks failed");

    // Capacity failure is explicit and cannot grow a hidden draw vector.
    auto capacityMesh = renderer.CreateMesh(meshDesc);
    MaterialDesc capacityDesc;
    capacityDesc.shader = renderer.GetDefaultShader();
    auto capacityMaterial = renderer.CreateMaterial(capacityDesc);
    renderer.onPreRender(api);
    for (uint32_t i = 0; i < RenderLimits::MaxDraws; ++i)
        CHECK(renderer.Submit({capacityMesh, capacityMaterial}), "Draw queue filled prematurely");
    CHECK(!renderer.Submit({capacityMesh, capacityMaterial}), "Draw queue must report overflow");
    renderer.onPreRender(api);
    renderer.DestroyMesh(capacityMesh);
    renderer.DestroyMaterial(capacityMaterial);

    CHECK(errors == 0 && glGetError() == GL_NO_ERROR, "Rendering produced an OpenGL error");
    // Detach stack-owned callback data before it leaves scope.
    glDebugMessageCallback(nullptr, nullptr);
    return true;
}

int main()
{
    idk::ServiceManager services;
    idk::VideoManager video("Renderer integration test", 128, 128);
    RenderEngine renderer;
    services.addService(&video);
    services.addService(&renderer);
    idk::EngineAPI api(&services);
    video.onInit(api);
    video.setWindowVisibility(false);
    renderer.onInit(api);
    bool ok = renderer.IsInitialized() && runChecks(renderer, api);
    if (renderer.GetContextHandle())
        glDebugMessageCallback(nullptr, nullptr);
    renderer.onShutdown(api);
    video.onShutdown(api);
    SDL_Quit();
    if (ok)
        std::puts("Renderer integration checks passed");
    return ok ? 0 : 1;
}
