#include "bugl_audio.hpp"

#include <iostream>
#include <string>

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        std::cout << "Usage: testAudio <audio-file> [--loop]\n";
        return 1;
    }

    const std::string path = argv[1];
    const bool loop = (argc >= 3 && std::string(argv[2]) == "--loop");

    bugl::audio::Engine engine;
    if (!engine.init())
    {
        std::cerr << "Failed to initialize audio engine.\n";
        return 2;
    }

    const auto musicId = engine.createMusic(path);
    if (musicId == 0)
    {
        std::cerr << "Failed to register music file: " << path << "\n";
        engine.shutdown();
        return 3;
    }

    const auto musicHandle = engine.playMusic(musicId, loop, 1.0f);
    if (musicHandle == 0)
    {
        std::cerr << "Failed to play file: " << path << "\n";
        engine.shutdown();
        return 4;
    }

    std::cout << "Playing musicId=" << musicId << ": " << path << (loop ? " (loop)\n" : "\n");
    std::cout << "Press Enter to stop...\n";

    std::string line;
    std::getline(std::cin, line);

    engine.shutdown();
    return 0;
}
