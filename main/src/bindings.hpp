#pragma once

#include "interpreter.hpp"
#include <SDL2/SDL.h> 

namespace SDLBindings
{
    void set_gl_debug_enabled(bool enabled);
    bool get_gl_debug_enabled();
    bool check_gl_errors(Interpreter *vm, const char *fnName);

    void registerAll(Interpreter &vm);
    void register_core(ModuleBuilder &mod);
    void register_window(ModuleBuilder &module);
    void register_renderer(ModuleBuilder &module);
    void register_device_input(ModuleBuilder &module);
    void register_opengl(ModuleBuilder &module);
    void register_opengl_legacy(ModuleBuilder &module);
    void register_opengl_vertexbuffer(ModuleBuilder &module);
    void register_opengl_texture(ModuleBuilder &module);
    void register_opengl_rendertarget(ModuleBuilder &module);
    void register_opengl_shader(ModuleBuilder &module);
    void register_opengl_instancing(ModuleBuilder &module);
    void register_opengl_query(ModuleBuilder &module);
    void register_opengl_ubo(ModuleBuilder &module);
    void register_sdl_opengl(ModuleBuilder &module);
    void register_stb(ModuleBuilder &module);
    void register_stb_rect_pack(ModuleBuilder &module, Interpreter &vm);
    void register_stb_truetype(ModuleBuilder &module, Interpreter &vm);
    void register_msf_gif(Interpreter &vm);
    void register_poly2tri(Interpreter &vm);
    void register_box2d(Interpreter &vm);
    void register_box2d_joints(Interpreter &vm);
    void register_ode(Interpreter &vm);
    void register_raymath(Interpreter &vm);
    void register_audio(ModuleBuilder &module);
 
}
 
