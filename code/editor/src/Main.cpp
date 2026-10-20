#include "idk/engine/Engine.hpp"
#include "idk/editor/Editor.hpp"
#include <cstdio>
#include <cstring>

int main(int argc, char **argv)
{
    const char *configPath = nullptr;
    if (argc == 2 && !std::strcmp(argv[1], "--help"))
    {
        std::puts("Usage: editor [--config path.cfg]\nF5 reloads the timing configuration.");
        return 0;
    }
    if (argc == 3 && !std::strcmp(argv[1], "--config"))
        configPath = argv[2];
    else if (argc != 1)
    {
        std::fputs("Usage: editor [--config path.cfg]\n", stderr);
        return 2;
    }

    idk::Engine engine(configPath);
    idk::editor::EditorApplication editor;
    if (!engine.startApplication(&editor))
        return 1;

    return 0;
}
