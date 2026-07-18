// ---------------------------------------------------------------------------
// Kemena3D standalone runtime ("player").
//
// This is the export template: a generic, game-agnostic executable. It loads a
// bundled project (data/scene.world + Library/) and runs it. The same binary
// ships with every exported game; only the data folder differs.
//
// Expected layout next to the executable:
//   <game>.exe
//   <game>.kpak              (packaged assets — preferred)
//   OR
//   data/
//     game.config            (window title / size / fullscreen)
//     scene.world
//     Library/ImportedAssets/*.glb
//     Library/Scripts/*.kbc
//   <dependency runtime libs>  (SDL3, assimp, angelscript, ...)
// ---------------------------------------------------------------------------

#include "kemena/kemena.h"

#include <nlohmann/json.hpp>

#include <fstream>
#include <filesystem>
#include <cstdio>

using namespace kemena;
namespace fs = std::filesystem;

/**
 * @brief Entry point for the Kemena3D standalone runtime ("player").
 *
 * Boots the generic export-template executable: resolves the bundled `data/`
 * folder relative to the executable, reads `game.config` (window title/size/
 * fullscreen), creates the window and renderer, loads `scene.world`, starts
 * physics and scripts, and runs the main game loop (event handling, fixed/
 * variable script updates, physics step, and rendering) until the window
 * closes. Cleans up scripts, physics, renderer, and window on exit.
 *
 * If a .kpak package file is found next to the executable, it is mounted
 * as the virtual file system and assets are streamed directly from it.
 *
 * @param argc Argument count; @p argv[0] is used to locate the executable directory.
 * @param argv Argument vector; @p argv[0] is the executable path.
 * @return 0 on normal exit.
 */
int main(int argc, char **argv)
{
    // Resolve the executable directory.
    fs::path exeDir = (argc > 0) ? fs::path(argv[0]).parent_path() : fs::current_path();
    if (exeDir.empty())
        exeDir = fs::current_path();

    // Initialize the virtual file system — auto-detects .kpak vs data/ folder.
    kFileSystem::init(exeDir.string(), "data");

    // ---- Read game.config --------------------------------------------------
    std::string title = "Kemena3D Game";
    int  width      = 1280;
    int  height     = 720;
    bool fullscreen = false;

    kString cfgJson = kFileSystem::readFileString("game.config");
    if (!cfgJson.empty())
    {
        try
        {
            nlohmann::json cfg = nlohmann::json::parse(cfgJson);
            title      = cfg.value("title", title);
            width      = cfg.value("width", width);
            height     = cfg.value("height", height);
            fullscreen = cfg.value("fullscreen", fullscreen);
        }
        catch (const std::exception &e)
        {
            printf("Runtime: bad game.config: %s\n", e.what());
        }
    }

    // ---- Boot the engine ---------------------------------------------------
    kWindow *window = createWindow(width, height, title, false,
                                   fullscreen ? kWindowType::WINDOW_FULLSCREEN
                                              : kWindowType::WINDOW_DEFAULT);
    kRenderer *renderer = createRenderer(window);
    renderer->setEnableShadow(true);
    renderer->setClearColor(kVec4(0.1f, 0.1f, 0.12f, 1.0f));

    kAssetManager *assetManager = createAssetManager();
    kWorld *world = createWorld(assetManager);

    // Load the world — VFS resolves scene.world from package or data/ folder.
    if (!world->loadFromFile("scene.world"))
        printf("Runtime: failed to load world.\n");

    // Choose the scene to render: first active scene, else the first one.
    kScene *scene = nullptr;
    for (kScene *s : world->getScenes())
        if (s->getActive()) { scene = s; break; }
    if (!scene && !world->getScenes().empty())
        scene = world->getScenes().front();

    // ---- Start gameplay ----------------------------------------------------
    world->startPhysics();
    world->startScripts();

    kSystemEvent event;
    while (window->getRunning())
    {
        while (event.hasEvent())
        {
            if (event.getType() == K_EVENT_QUIT)
                window->setRunning(false);
        }

        float dt = window->getTimer()->getDeltaTime();

        world->fixedUpdateScripts(dt);
        world->updatePhysics(dt);
        world->updateScripts(dt);

        if (scene)
            renderer->render(world, scene, 0, 0,
                             window->getWindowWidth(), window->getWindowHeight(),
                             dt, /*autoClearSwapWindow*/ true);
    }

    // ---- Shutdown ----------------------------------------------------------
    world->stopScripts();
    world->stopPhysics();
    renderer->destroy();
    window->destroy();
    kFileSystem::shutdown();
    return 0;
}
