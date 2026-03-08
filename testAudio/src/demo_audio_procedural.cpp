#include "bugl_audio.hpp"

#include <chrono>
#include <iostream>
#include <string>
#include <thread>

static void runForMs(bugl::audio::Engine &engine, int ms)
{
    const int stepMs = 16;
    int elapsed = 0;
    while (elapsed < ms)
    {
        engine.update();
        std::this_thread::sleep_for(std::chrono::milliseconds(stepMs));
        elapsed += stepMs;
    }
}

int main()
{
    bugl::audio::Engine engine;
    if (!engine.init())
    {
        std::cerr << "Failed to initialize audio engine.\n";
        return 1;
    }

    engine.setMasterVolume(1.0f);
    engine.setSfxVolume(0.85f);
    engine.enableSfxDelay(true, 0.35f);

    const auto uiBip = engine.createWaveform(bugl::audio::Engine::WAVE_SINE, 0.20f, 980.0f);
    const auto laser = engine.createWaveform(bugl::audio::Engine::WAVE_SAW, 0.16f, 320.0f);
    const auto alert = engine.createWaveform(bugl::audio::Engine::WAVE_SQUARE, 0.10f, 660.0f);
    const auto wind = engine.createNoise(bugl::audio::Engine::NOISE_PINK, 12345, 0.10f);
    const auto rumble = engine.createNoise(bugl::audio::Engine::NOISE_BROWNIAN, 54321, 0.06f);

    if (uiBip == 0 || laser == 0 || alert == 0 || wind == 0 || rumble == 0)
    {
        std::cerr << "Failed to create procedural sounds.\n";
        engine.shutdown();
        return 2;
    }

    std::cout << "Demo procedural audio:\n";
    std::cout << "1) UI bip\n2) Laser\n3) Alert\n4) Wind+Rumble ambience\n";
    std::cout << "Running sequence...\n\n";

    auto h = engine.playSfx(uiBip, 0.9f, 1.0f, 0.0f);
    runForMs(engine, 180);
    engine.stop(h);

    h = engine.playSfx(laser, 0.8f, 1.7f, -0.20f);
    runForMs(engine, 140);
    engine.stop(h);

    h = engine.playSfx(laser, 0.8f, 1.0f, 0.20f);
    runForMs(engine, 140);
    engine.stop(h);

    h = engine.playSfx(alert, 0.7f, 1.0f, 0.0f);
    runForMs(engine, 120);
    engine.stop(h);
    h = engine.playSfx(alert, 0.7f, 0.85f, 0.0f);
    runForMs(engine, 120);
    engine.stop(h);

    auto hWind = engine.playSfx(wind, 0.45f, 0.9f, 0.0f);
    auto hRumble = engine.playSfx(rumble, 0.55f, 0.7f, 0.0f);
    std::cout << "Ambience ON for 5 seconds...\n";
    runForMs(engine, 5000);
    engine.stop(hWind);
    engine.stop(hRumble);

    engine.enableSfxDelay(false);
    engine.shutdown();

    std::cout << "Done.\n";
    return 0;
}

