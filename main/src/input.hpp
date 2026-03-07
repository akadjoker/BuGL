#pragma once

#include "config.hpp"
#include <SDL2/SDL.h>
#include <string>
#include <vector>
#include <unordered_map>

// ========== ENUMS ==========

// Mouse buttons
enum MouseButton
{
    LEFT = 0,
    RIGHT = 1,
    MIDDLE = 2
};

// Mouse cursor types
enum MouseCursor
{
    DEFAULT = 0,
    ARROW = 1,
    IBEAM = 2,
    CROSSHAIR = 3,
    POINTING_HAND = 4,
    RESIZE_EW = 5,
    RESIZE_NS = 6,
    RESIZE_NWSE = 7,
    RESIZE_NESW = 8,
    RESIZE_ALL = 9,
    NOT_ALLOWED = 10
};

enum KeyCode
{
    KEY_NULL = 0,
    // Alphanumeric keys
    KEY_APOSTROPHE = 39,
    KEY_COMMA = 44,
    KEY_MINUS = 45,
    KEY_PERIOD = 46,
    KEY_SLASH = 47,
    KEY_ZERO = 48,
    KEY_ONE = 49,
    KEY_TWO = 50,
    KEY_THREE = 51,
    KEY_FOUR = 52,
    KEY_FIVE = 53,
    KEY_SIX = 54,
    KEY_SEVEN = 55,
    KEY_EIGHT = 56,
    KEY_NINE = 57,
    KEY_SEMICOLON = 59,
    KEY_EQUAL = 61,
    KEY_A = 65,
    KEY_B = 66,
    KEY_C = 67,
    KEY_D = 68,
    KEY_E = 69,
    KEY_F = 70,
    KEY_G = 71,
    KEY_H = 72,
    KEY_I = 73,
    KEY_J = 74,
    KEY_K = 75,
    KEY_L = 76,
    KEY_M = 77,
    KEY_N = 78,
    KEY_O = 79,
    KEY_P = 80,
    KEY_Q = 81,
    KEY_R = 82,
    KEY_S = 83,
    KEY_T = 84,
    KEY_U = 85,
    KEY_V = 86,
    KEY_W = 87,
    KEY_X = 88,
    KEY_Y = 89,
    KEY_Z = 90,
    KEY_LEFT_BRACKET = 91,
    KEY_BACKSLASH = 92,
    KEY_RIGHT_BRACKET = 93,
    KEY_GRAVE = 96,
    // Function keys
    KEY_SPACE = 32,
    KEY_ESCAPE = 256,
    KEY_ENTER = 257,
    KEY_TAB = 258,
    KEY_BACKSPACE = 259,
    KEY_INSERT = 260,
    KEY_DELETE = 261,
    KEY_RIGHT = 262,
    KEY_LEFT = 263,
    KEY_DOWN = 264,
    KEY_UP = 265,
    KEY_PAGE_UP = 266,
    KEY_PAGE_DOWN = 267,
    KEY_HOME = 268,
    KEY_END = 269,
    KEY_CAPS_LOCK = 280,
    KEY_SCROLL_LOCK = 281,
    KEY_NUM_LOCK = 282,
    KEY_PRINT_SCREEN = 283,
    KEY_PAUSE = 284,
    KEY_F1 = 290,
    KEY_F2 = 291,
    KEY_F3 = 292,
    KEY_F4 = 293,
    KEY_F5 = 294,
    KEY_F6 = 295,
    KEY_F7 = 296,
    KEY_F8 = 297,
    KEY_F9 = 298,
    KEY_F10 = 299,
    KEY_F11 = 300,
    KEY_F12 = 301,
    // Keypad keys
    KEY_KP_0 = 320,
    KEY_KP_1 = 321,
    KEY_KP_2 = 322,
    KEY_KP_3 = 323,
    KEY_KP_4 = 324,
    KEY_KP_5 = 325,
    KEY_KP_6 = 326,
    KEY_KP_7 = 327,
    KEY_KP_8 = 328,
    KEY_KP_9 = 329,
    KEY_KP_DECIMAL = 330,
    KEY_KP_DIVIDE = 331,
    KEY_KP_MULTIPLY = 332,
    KEY_KP_SUBTRACT = 333,
    KEY_KP_ADD = 334,
    KEY_KP_ENTER = 335,
    KEY_KP_EQUAL = 336,
    // Modifier keys
    KEY_LEFT_SHIFT = 340,
    KEY_LEFT_CONTROL = 341,
    KEY_LEFT_ALT = 342,
    KEY_LEFT_SUPER = 343,
    KEY_RIGHT_SHIFT = 344,
    KEY_RIGHT_CONTROL = 345,
    KEY_RIGHT_ALT = 346,
    KEY_RIGHT_SUPER = 347,
    KEY_KB_MENU = 348
};

// Gamepad buttons
enum GamepadButton
{
    GAMEPAD_BUTTON_UNKNOWN = 0,
    GAMEPAD_BUTTON_LEFT_FACE_UP,
    GAMEPAD_BUTTON_LEFT_FACE_RIGHT,
    GAMEPAD_BUTTON_LEFT_FACE_DOWN,
    GAMEPAD_BUTTON_LEFT_FACE_LEFT,
    GAMEPAD_BUTTON_RIGHT_FACE_UP,
    GAMEPAD_BUTTON_RIGHT_FACE_RIGHT,
    GAMEPAD_BUTTON_RIGHT_FACE_DOWN,
    GAMEPAD_BUTTON_RIGHT_FACE_LEFT,
    GAMEPAD_BUTTON_LEFT_TRIGGER_1,
    GAMEPAD_BUTTON_LEFT_TRIGGER_2,
    GAMEPAD_BUTTON_RIGHT_TRIGGER_1,
    GAMEPAD_BUTTON_RIGHT_TRIGGER_2,
    GAMEPAD_BUTTON_MIDDLE_LEFT,
    GAMEPAD_BUTTON_MIDDLE,
    GAMEPAD_BUTTON_MIDDLE_RIGHT,
    GAMEPAD_BUTTON_LEFT_THUMB,
    GAMEPAD_BUTTON_RIGHT_THUMB
};

// Gamepad axis
enum GamepadAxis
{
    GAMEPAD_AXIS_LEFT_X = 0,
    GAMEPAD_AXIS_LEFT_Y = 1,
    GAMEPAD_AXIS_RIGHT_X = 2,
    GAMEPAD_AXIS_RIGHT_Y = 3,
    GAMEPAD_AXIS_LEFT_TRIGGER = 4,
    GAMEPAD_AXIS_RIGHT_TRIGGER = 5
};

struct Vec2
{
    float x, y;
};

class Input
{
public:
    Input() = delete;

    static void OnMouseDown(const SDL_MouseButtonEvent &event);
    static void OnMouseUp(const SDL_MouseButtonEvent &event);
    static void OnMouseMove(const SDL_MouseMotionEvent &event);
    static void OnMouseWheel(const SDL_MouseWheelEvent &event);
    static void OnKeyDown(const SDL_KeyboardEvent &event);
    static void OnKeyUp(const SDL_KeyboardEvent &event);
    static void OnTextInput(const SDL_TextInputEvent &event);
    static void OnGamepadDeviceAdded(const SDL_ControllerDeviceEvent &event);
    static void OnGamepadDeviceRemoved(const SDL_ControllerDeviceEvent &event);
    static void OnGamepadButtonDown(const SDL_ControllerButtonEvent &event);
    static void OnGamepadButtonUp(const SDL_ControllerButtonEvent &event);
    static void OnGamepadAxisMotion(const SDL_ControllerAxisEvent &event);

    static bool IsMousePressed(MouseButton button);
    static bool IsMouseDown(MouseButton button);
    static bool IsMouseReleased(MouseButton button);
    static bool IsMouseUp(MouseButton button);

    static Vec2 GetMousePosition();
    static Vec2 GetMouseDelta();
    static int GetMouseX();
    static int GetMouseY();
    static void SetMousePosition(int x, int y);
    static void SetMouseOffset(int offsetX, int offsetY);
    static void SetMouseScale(float scaleX, float scaleY);

    static Vec2 GetMouseWheelMove();
    static float GetMouseWheelMoveV();

    static void SetMouseCursor(MouseCursor cursor);

    static bool IsKeyPressed(KeyCode key);
    static bool IsKeyDown(KeyCode key);
    static bool IsKeyReleased(KeyCode key);
    static bool IsKeyUp(KeyCode key);

    static KeyCode GetKeyPressed();
    static int GetCharPressed();

    static bool IsGamepadAvailable(int gamepad);
    static const char *GetGamepadName(int gamepad);

    static bool IsGamepadButtonPressed(int gamepad, GamepadButton button);
    static bool IsGamepadButtonDown(int gamepad, GamepadButton button);
    static bool IsGamepadButtonReleased(int gamepad, GamepadButton button);
    static bool IsGamepadButtonUp(int gamepad, GamepadButton button);

    static int GetGamepadButtonPressed();
    static int GetGamepadAxisCount(int gamepad);
    static float GetGamepadAxisMovement(int gamepad, GamepadAxis axis);

    static void Update();
    static void Init();
    static void Shutdown();

private:
    static constexpr int MAX_MOUSE_BUTTONS = 3;
    static constexpr int MAX_KEYBOARD_KEYS = 512;
    static constexpr int MAX_GAMEPADS = 4;
    static constexpr int MAX_GAMEPAD_BUTTONS = 32;
    static constexpr int MAX_GAMEPAD_AXIS = 8;
    static constexpr int MAX_KEY_PRESSED_QUEUE = 16;
    static constexpr int MAX_CHAR_PRESSED_QUEUE = 16;

    // Estados mouse
    static bool currentMouseState[MAX_MOUSE_BUTTONS];
    static bool previousMouseState[MAX_MOUSE_BUTTONS];
    static Vec2 mousePosition;
    static Vec2 mousePreviousPosition;
    static Vec2 mouseOffset;
    static Vec2 mouseScale;
    static Vec2 mouseWheel;
    static SDL_Cursor *mouseCursor;
    static MouseCursor currentCursor;

    // Estados teclado
    static bool currentKeyState[MAX_KEYBOARD_KEYS];
    static bool previousKeyState[MAX_KEYBOARD_KEYS];
    static KeyCode keyPressedQueue[MAX_KEY_PRESSED_QUEUE];
    static int keyPressedQueueCount;
    static int charPressedQueue[MAX_CHAR_PRESSED_QUEUE];
    static int charPressedQueueCount;

    // Estados gamepad
    static SDL_GameController *gamepads[MAX_GAMEPADS];
    static SDL_JoystickID gamepadInstanceIds[MAX_GAMEPADS];
    static bool currentGamepadButtonState[MAX_GAMEPADS][MAX_GAMEPAD_BUTTONS];
    static bool previousGamepadButtonState[MAX_GAMEPADS][MAX_GAMEPAD_BUTTONS];
    static float gamepadAxisState[MAX_GAMEPADS][MAX_GAMEPAD_AXIS];
    static int lastButtonPressed;

    // Helpers
    static MouseButton ConvertSDLButton(Uint8 button);
    static KeyCode ConvertSDLScancode(SDL_Scancode scancode);
    static GamepadButton ConvertSDLGamepadButton(SDL_GameControllerButton button);
    static int FindFreeGamepadSlot();
    static int FindGamepadSlotByInstanceId(SDL_JoystickID instanceId);
};
