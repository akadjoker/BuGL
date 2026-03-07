#include "bindings.hpp"

namespace SDLBindings
{

    //

    void registerAll(Interpreter &vm)
    {
        auto *sdlEvent = vm.registerNativeStruct(
            "SDL_Event",
            sizeof(SDL_Event),
            nullptr,
            nullptr);

        vm.addStructField(sdlEvent, "type", offsetof(SDL_Event, type), FieldType::INT);
        vm.addStructField(sdlEvent, "key", offsetof(SDL_Event, key.keysym.sym), FieldType::INT);
        vm.addStructField(sdlEvent, "winEvent", offsetof(SDL_Event, window.event), FieldType::BYTE);
        // 2. Os dados genéricos (No caso de Resize: data1 = Width, data2 = Height)
        vm.addStructField(sdlEvent, "Width", offsetof(SDL_Event, window.data1), FieldType::INT);
        vm.addStructField(sdlEvent, "Height", offsetof(SDL_Event, window.data2), FieldType::INT);

        // 1. Mouse Motion (Quando o mouse mexe)
        // "motionX" e "motionY" são a posição absoluta na janela
        vm.addStructField(sdlEvent, "motionX", offsetof(SDL_Event, motion.x), FieldType::INT);
        vm.addStructField(sdlEvent, "motionY", offsetof(SDL_Event, motion.y), FieldType::INT);

        // "relX" é o relativo (quanto mexeu desde o último frame) - útil para câmeras 3D/FPS
        vm.addStructField(sdlEvent, "relX", offsetof(SDL_Event, motion.xrel), FieldType::INT);
        vm.addStructField(sdlEvent, "relY", offsetof(SDL_Event, motion.yrel), FieldType::INT);

        // 2. Mouse Button (Cliques)
        // "button" diz qual botão foi (1=Esq, 2=Meio, 3=Dir)
        vm.addStructField(sdlEvent, "button", offsetof(SDL_Event, button.button), FieldType::BYTE);
        // "btnState" diz se foi pressionado (1) ou solto (0)
        vm.addStructField(sdlEvent, "btnState", offsetof(SDL_Event, button.state), FieldType::BYTE);
        // Nota: O clique também tem X e Y, mas ficam num offset diferente do Motion!
        vm.addStructField(sdlEvent, "btnX", offsetof(SDL_Event, button.x), FieldType::INT);
        vm.addStructField(sdlEvent, "btnY", offsetof(SDL_Event, button.y), FieldType::INT);

        // 3. Mouse Wheel (Scroll)
        // wheelY > 0 (Scroll Cima), wheelY < 0 (Scroll Baixo)
        vm.addStructField(sdlEvent, "wheelY", offsetof(SDL_Event, wheel.y), FieldType::INT);

        ModuleBuilder module = vm.addModule("SDL");

        register_core(module);
        register_renderer(module);
        register_window(module);
        register_device_input(module);
        register_sdl_opengl(module);
        
        module = vm.addModule("OpenGl");
        register_opengl(module);
        register_opengl_shader(module);
        register_opengl_instancing(module);
        register_opengl_query(module);
        register_opengl_ubo(module);

        module = vm.addModule("STB");
        register_stb(module);
        register_stb_rect_pack(module, vm);
        register_stb_truetype(module, vm);
        register_msf_gif(vm);
        register_poly2tri(vm);
        register_raymath(vm);
        register_box2d(vm);
        register_box2d_joints(vm);
    }
}
