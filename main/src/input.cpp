
#include "pch.h"
#include "input.hpp"

bool Input::currentMouseState[MAX_MOUSE_BUTTONS] = {false};
bool Input::previousMouseState[MAX_MOUSE_BUTTONS] = {false};
Vec2 Input::mousePosition = {0.0f, 0.0f};
Vec2 Input::mousePreviousPosition = {0.0f, 0.0f};
Vec2 Input::mouseOffset = {0.0f, 0.0f};
Vec2 Input::mouseScale = {1.0f, 1.0f};
Vec2 Input::mouseWheel = {0.0f, 0.0f};
SDL_Cursor *Input::mouseCursor = nullptr;
MouseCursor Input::currentCursor = MouseCursor::DEFAULT;

// Teclado
bool Input::currentKeyState[MAX_KEYBOARD_KEYS] = {false};
bool Input::previousKeyState[MAX_KEYBOARD_KEYS] = {false};
KeyCode Input::keyPressedQueue[MAX_KEY_PRESSED_QUEUE] = {KEY_NULL};
int Input::keyPressedQueueCount = 0;
int Input::charPressedQueue[MAX_CHAR_PRESSED_QUEUE] = {0};
int Input::charPressedQueueCount = 0;

// Gamepad
SDL_GameController *Input::gamepads[MAX_GAMEPADS] = {nullptr};
SDL_JoystickID Input::gamepadInstanceIds[MAX_GAMEPADS] = {-1};
bool Input::currentGamepadButtonState[MAX_GAMEPADS][MAX_GAMEPAD_BUTTONS] = {{false}};
bool Input::previousGamepadButtonState[MAX_GAMEPADS][MAX_GAMEPAD_BUTTONS] = {{false}};
float Input::gamepadAxisState[MAX_GAMEPADS][MAX_GAMEPAD_AXIS] = {{0.0f}};
int Input::lastButtonPressed = GAMEPAD_BUTTON_UNKNOWN;

// ========== CONVERSÃO SDL -> RAYLIB ==========

static const KeyCode scancodeMap[232] = {
    KEY_NULL, KEY_NULL, KEY_NULL, KEY_NULL,
    KEY_A, KEY_B, KEY_C, KEY_D, KEY_E, KEY_F, KEY_G, KEY_H, KEY_I, KEY_J, KEY_K, KEY_L, KEY_M,
    KEY_N, KEY_O, KEY_P, KEY_Q, KEY_R, KEY_S, KEY_T, KEY_U, KEY_V, KEY_W, KEY_X, KEY_Y, KEY_Z,
    KEY_ONE, KEY_TWO, KEY_THREE, KEY_FOUR, KEY_FIVE, KEY_SIX, KEY_SEVEN, KEY_EIGHT, KEY_NINE, KEY_ZERO,
    KEY_ENTER, KEY_ESCAPE, KEY_BACKSPACE, KEY_TAB, KEY_SPACE,
    KEY_MINUS, KEY_EQUAL, KEY_LEFT_BRACKET, KEY_RIGHT_BRACKET, KEY_BACKSLASH, KEY_NULL,
    KEY_SEMICOLON, KEY_APOSTROPHE, KEY_GRAVE, KEY_COMMA, KEY_PERIOD, KEY_SLASH,
    KEY_CAPS_LOCK,
    KEY_F1, KEY_F2, KEY_F3, KEY_F4, KEY_F5, KEY_F6, KEY_F7, KEY_F8, KEY_F9, KEY_F10, KEY_F11, KEY_F12,
    KEY_PRINT_SCREEN, KEY_SCROLL_LOCK, KEY_PAUSE, KEY_INSERT, KEY_HOME, KEY_PAGE_UP,
    KEY_DELETE, KEY_END, KEY_PAGE_DOWN, KEY_RIGHT, KEY_LEFT, KEY_DOWN, KEY_UP,
    KEY_NUM_LOCK, KEY_KP_DIVIDE, KEY_KP_MULTIPLY, KEY_KP_SUBTRACT, KEY_KP_ADD, KEY_KP_ENTER,
    KEY_KP_1, KEY_KP_2, KEY_KP_3, KEY_KP_4, KEY_KP_5, KEY_KP_6, KEY_KP_7, KEY_KP_8, KEY_KP_9, KEY_KP_0,
    KEY_KP_DECIMAL};

static bool decode_utf8_codepoint(const unsigned char *&text, int &outCodepoint)
{
    const unsigned char c0 = text[0];
    if (c0 == 0)
        return false;

    if ((c0 & 0x80) == 0)
    {
        outCodepoint = (int)c0;
        text += 1;
        return true;
    }

    if ((c0 & 0xE0) == 0xC0)
    {
        const unsigned char c1 = text[1];
        if ((c1 & 0xC0) != 0x80)
            return false;
        outCodepoint = ((int)(c0 & 0x1F) << 6) | (int)(c1 & 0x3F);
        text += 2;
        return true;
    }

    if ((c0 & 0xF0) == 0xE0)
    {
        const unsigned char c1 = text[1];
        const unsigned char c2 = text[2];
        if (((c1 & 0xC0) != 0x80) || ((c2 & 0xC0) != 0x80))
            return false;
        outCodepoint = ((int)(c0 & 0x0F) << 12) |
                       ((int)(c1 & 0x3F) << 6) |
                       (int)(c2 & 0x3F);
        text += 3;
        return true;
    }

    if ((c0 & 0xF8) == 0xF0)
    {
        const unsigned char c1 = text[1];
        const unsigned char c2 = text[2];
        const unsigned char c3 = text[3];
        if (((c1 & 0xC0) != 0x80) || ((c2 & 0xC0) != 0x80) || ((c3 & 0xC0) != 0x80))
            return false;
        outCodepoint = ((int)(c0 & 0x07) << 18) |
                       ((int)(c1 & 0x3F) << 12) |
                       ((int)(c2 & 0x3F) << 6) |
                       (int)(c3 & 0x3F);
        text += 4;
        return true;
    }

    return false;
}

KeyCode Input::ConvertSDLScancode(SDL_Scancode scancode)
{
    const int scancodeCount = (int)(sizeof(scancodeMap) / sizeof(scancodeMap[0]));
    if (scancode >= 0 && scancode < scancodeCount)
        return scancodeMap[scancode];
    if (scancode == SDL_SCANCODE_LCTRL)
        return KEY_LEFT_CONTROL;
    if (scancode == SDL_SCANCODE_LSHIFT)
        return KEY_LEFT_SHIFT;
    if (scancode == SDL_SCANCODE_LALT)
        return KEY_LEFT_ALT;
    if (scancode == SDL_SCANCODE_LGUI)
        return KEY_LEFT_SUPER;
    if (scancode == SDL_SCANCODE_RCTRL)
        return KEY_RIGHT_CONTROL;
    if (scancode == SDL_SCANCODE_RSHIFT)
        return KEY_RIGHT_SHIFT;
    if (scancode == SDL_SCANCODE_RALT)
        return KEY_RIGHT_ALT;
    if (scancode == SDL_SCANCODE_RGUI)
        return KEY_RIGHT_SUPER;
    return KEY_NULL;
}

MouseButton Input::ConvertSDLButton(Uint8 button)
{
    if (button == SDL_BUTTON_LEFT)
        return MouseButton::LEFT;
    if (button == SDL_BUTTON_RIGHT)
        return MouseButton::RIGHT;
    if (button == SDL_BUTTON_MIDDLE)
        return MouseButton::MIDDLE;
    return MouseButton::LEFT;
}

GamepadButton Input::ConvertSDLGamepadButton(SDL_GameControllerButton button)
{
    switch (button)
    {
    case SDL_CONTROLLER_BUTTON_A:
        return GAMEPAD_BUTTON_RIGHT_FACE_DOWN;
    case SDL_CONTROLLER_BUTTON_B:
        return GAMEPAD_BUTTON_RIGHT_FACE_RIGHT;
    case SDL_CONTROLLER_BUTTON_X:
        return GAMEPAD_BUTTON_RIGHT_FACE_LEFT;
    case SDL_CONTROLLER_BUTTON_Y:
        return GAMEPAD_BUTTON_RIGHT_FACE_UP;
    case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:
        return GAMEPAD_BUTTON_LEFT_TRIGGER_1;
    case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER:
        return GAMEPAD_BUTTON_RIGHT_TRIGGER_1;
    case SDL_CONTROLLER_BUTTON_BACK:
        return GAMEPAD_BUTTON_MIDDLE_LEFT;
    case SDL_CONTROLLER_BUTTON_START:
        return GAMEPAD_BUTTON_MIDDLE_RIGHT;
    case SDL_CONTROLLER_BUTTON_GUIDE:
        return GAMEPAD_BUTTON_MIDDLE;
    case SDL_CONTROLLER_BUTTON_LEFTSTICK:
        return GAMEPAD_BUTTON_LEFT_THUMB;
    case SDL_CONTROLLER_BUTTON_RIGHTSTICK:
        return GAMEPAD_BUTTON_RIGHT_THUMB;
    case SDL_CONTROLLER_BUTTON_DPAD_UP:
        return GAMEPAD_BUTTON_LEFT_FACE_UP;
    case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
        return GAMEPAD_BUTTON_LEFT_FACE_DOWN;
    case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
        return GAMEPAD_BUTTON_LEFT_FACE_LEFT;
    case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
        return GAMEPAD_BUTTON_LEFT_FACE_RIGHT;
    default:
        return GAMEPAD_BUTTON_UNKNOWN;
    }
}

// ========== CALLBACKS SDL ==========

void Input::OnMouseDown(const SDL_MouseButtonEvent &event)
{
     if (event.button != SDL_BUTTON_LEFT && 
        event.button != SDL_BUTTON_RIGHT && 
        event.button != SDL_BUTTON_MIDDLE)
        return;  
    
    MouseButton btn = ConvertSDLButton(event.button);
    currentMouseState[btn] = true;
}

void Input::OnMouseUp(const SDL_MouseButtonEvent &event)
{
      if (event.button != SDL_BUTTON_LEFT && 
        event.button != SDL_BUTTON_RIGHT && 
        event.button != SDL_BUTTON_MIDDLE)
        return;   
    MouseButton btn = ConvertSDLButton(event.button);
    currentMouseState[btn] = false;
}

void Input::OnMouseMove(const SDL_MouseMotionEvent &event)
{
    mousePosition.x = event.x * mouseScale.x + mouseOffset.x;
    mousePosition.y = event.y * mouseScale.y + mouseOffset.y;
}

void Input::OnMouseWheel(const SDL_MouseWheelEvent &event)
{
    mouseWheel.x = (float)event.x;
    mouseWheel.y = (float)event.y;
}

void Input::OnKeyDown(const SDL_KeyboardEvent &event)
{
    KeyCode key = ConvertSDLScancode(event.keysym.scancode);
    if (key != KEY_NULL)
    {
        if (!currentKeyState[key] && keyPressedQueueCount < MAX_KEY_PRESSED_QUEUE)
        {
            keyPressedQueue[keyPressedQueueCount++] = key;
        }
        currentKeyState[key] = true;
    }
}

void Input::OnKeyUp(const SDL_KeyboardEvent &event)
{
    KeyCode key = ConvertSDLScancode(event.keysym.scancode);
    if (key != KEY_NULL)
    {
        currentKeyState[key] = false;
    }
}

// ========== MOUSE ==========

bool Input::IsMousePressed(MouseButton button)
{
    bool pressed = false;
    if ((currentMouseState[button] == true) && (previousMouseState[button] == false))
        pressed = true;
    return pressed;
}

bool Input::IsMouseDown(MouseButton button)
{
    return currentMouseState[button];
}

bool Input::IsMouseReleased(MouseButton button)
{
    return !currentMouseState[button] && previousMouseState[button];
}

bool Input::IsMouseUp(MouseButton button)
{
    return !currentMouseState[button];
}

Vec2 Input::GetMousePosition()
{
    return mousePosition;
}

Vec2 Input::GetMouseDelta()
{
    return {mousePosition.x - mousePreviousPosition.x, mousePosition.y - mousePreviousPosition.y};
}

int Input::GetMouseX()
{
    return (int)mousePosition.x;
}

int Input::GetMouseY()
{
    return (int)mousePosition.y;
}

void Input::SetMousePosition(int x, int y)
{
    mousePosition = {(float)x, (float)y};
}

bool Input::SetMouseRelative(bool enabled)
{
    return SDL_SetRelativeMouseMode(enabled ? SDL_TRUE : SDL_FALSE) == 0;
}

bool Input::GetMouseRelative()
{
    return SDL_GetRelativeMouseMode() == SDL_TRUE;
}

void Input::SetMouseOffset(int offsetX, int offsetY)
{
    mouseOffset = {(float)offsetX, (float)offsetY};
}

void Input::SetMouseScale(float scaleX, float scaleY)
{
    mouseScale = {scaleX, scaleY};
}

Vec2 Input::GetMouseWheelMove()
{
    return mouseWheel;
}

float Input::GetMouseWheelMoveV()
{
    return mouseWheel.y;
}

void Input::SetMouseCursor(MouseCursor cursor)
{
    static const int cursors[] = {
        SDL_SYSTEM_CURSOR_ARROW, SDL_SYSTEM_CURSOR_ARROW, SDL_SYSTEM_CURSOR_IBEAM,
        SDL_SYSTEM_CURSOR_CROSSHAIR, SDL_SYSTEM_CURSOR_HAND, SDL_SYSTEM_CURSOR_SIZEWE,
        SDL_SYSTEM_CURSOR_SIZENS, SDL_SYSTEM_CURSOR_SIZENWSE, SDL_SYSTEM_CURSOR_SIZENESW,
        SDL_SYSTEM_CURSOR_SIZEALL, SDL_SYSTEM_CURSOR_NO};

    if (mouseCursor)
        SDL_FreeCursor(mouseCursor);
    mouseCursor = SDL_CreateSystemCursor((SDL_SystemCursor)cursors[cursor]);
    SDL_SetCursor(mouseCursor);
    currentCursor = cursor;
}

// ========== TECLADO ==========

bool Input::IsKeyPressed(KeyCode key)
{
    if ((int)key >= MAX_KEYBOARD_KEYS)
        return false;
    return currentKeyState[key] && !previousKeyState[key];
}

bool Input::IsKeyDown(KeyCode key)
{
    if ((int)key >= MAX_KEYBOARD_KEYS)
        return false;
    return currentKeyState[key];
}

void Input::OnTextInput(const SDL_TextInputEvent &event)
{
    // SDL envia UTF-8 em event.text: converte para codepoints.
    const unsigned char *text = (const unsigned char *)event.text;
    while (*text && charPressedQueueCount < MAX_CHAR_PRESSED_QUEUE)
    {
        int codepoint = 0;
        if (!decode_utf8_codepoint(text, codepoint))
        {
            // Byte inválido: avança 1 para evitar loop infinito.
            text++;
            continue;
        }
        charPressedQueue[charPressedQueueCount++] = codepoint;
    }
}

bool Input::IsKeyReleased(KeyCode key)
{
    if ((int)key >= MAX_KEYBOARD_KEYS)
        return false;
    return !currentKeyState[key] && previousKeyState[key];
}

bool Input::IsKeyUp(KeyCode key)
{
    if ((int)key >= MAX_KEYBOARD_KEYS)
        return false;
    return !currentKeyState[key];
}

KeyCode Input::GetKeyPressed()
{
    if (keyPressedQueueCount <= 0)
        return KEY_NULL;

    KeyCode key = keyPressedQueue[0];
    for (int i = 1; i < keyPressedQueueCount; i++)
        keyPressedQueue[i - 1] = keyPressedQueue[i];
    keyPressedQueueCount--;
    keyPressedQueue[keyPressedQueueCount] = KEY_NULL;
    return key;
}

int Input::GetCharPressed()
{
    if (charPressedQueueCount <= 0)
        return 0;

    int codepoint = charPressedQueue[0];
    for (int i = 1; i < charPressedQueueCount; i++)
        charPressedQueue[i - 1] = charPressedQueue[i];
    charPressedQueueCount--;
    charPressedQueue[charPressedQueueCount] = 0;
    return codepoint;
}

// ========== GAMEPAD ==========

bool Input::IsGamepadAvailable(int gamepad)
{
    return gamepad >= 0 && gamepad < MAX_GAMEPADS && gamepads[gamepad] != nullptr;
}

const char *Input::GetGamepadName(int gamepad)
{
    if (!IsGamepadAvailable(gamepad))
        return "";
    return SDL_GameControllerName(gamepads[gamepad]);
}

bool Input::IsGamepadButtonPressed(int gamepad, GamepadButton button)
{
    if (!IsGamepadAvailable(gamepad) || (int)button >= MAX_GAMEPAD_BUTTONS)
        return false;
    return currentGamepadButtonState[gamepad][button] && !previousGamepadButtonState[gamepad][button];
}

bool Input::IsGamepadButtonDown(int gamepad, GamepadButton button)
{
    if (!IsGamepadAvailable(gamepad) || (int)button >= MAX_GAMEPAD_BUTTONS)
        return false;
    return currentGamepadButtonState[gamepad][button];
}

bool Input::IsGamepadButtonReleased(int gamepad, GamepadButton button)
{
    if (!IsGamepadAvailable(gamepad) || (int)button >= MAX_GAMEPAD_BUTTONS)
        return false;
    return !currentGamepadButtonState[gamepad][button] && previousGamepadButtonState[gamepad][button];
}

bool Input::IsGamepadButtonUp(int gamepad, GamepadButton button)
{
    if (!IsGamepadAvailable(gamepad) || (int)button >= MAX_GAMEPAD_BUTTONS)
        return false;
    return !currentGamepadButtonState[gamepad][button];
}

int Input::GetGamepadButtonPressed()
{
    return lastButtonPressed;
}

int Input::GetGamepadAxisCount(int gamepad)
{
    if (!IsGamepadAvailable(gamepad))
        return 0;
    return MAX_GAMEPAD_AXIS;
}

float Input::GetGamepadAxisMovement(int gamepad, GamepadAxis axis)
{
    if (!IsGamepadAvailable(gamepad) || (int)axis >= MAX_GAMEPAD_AXIS)
        return 0.0f;
    return gamepadAxisState[gamepad][axis];
}

int Input::FindFreeGamepadSlot()
{
    for (int i = 0; i < MAX_GAMEPADS; i++)
    {
        if (gamepads[i] == nullptr)
            return i;
    }
    return -1;
}

int Input::FindGamepadSlotByInstanceId(SDL_JoystickID instanceId)
{
    for (int i = 0; i < MAX_GAMEPADS; i++)
    {
        if (gamepads[i] != nullptr && gamepadInstanceIds[i] == instanceId)
            return i;
    }
    return -1;
}

void Input::OnGamepadDeviceAdded(const SDL_ControllerDeviceEvent &event)
{
    if (!SDL_IsGameController(event.which))
        return;

    SDL_JoystickID instanceId = SDL_JoystickGetDeviceInstanceID(event.which);
    if (instanceId >= 0 && FindGamepadSlotByInstanceId(instanceId) >= 0)
        return;

    int slot = FindFreeGamepadSlot();
    if (slot < 0)
        return;

    SDL_GameController *controller = SDL_GameControllerOpen(event.which);
    if (!controller)
        return;

    SDL_Joystick *joystick = SDL_GameControllerGetJoystick(controller);
    gamepads[slot] = controller;
    gamepadInstanceIds[slot] = (instanceId >= 0) ? instanceId : SDL_JoystickInstanceID(joystick);

    for (int i = 0; i < MAX_GAMEPAD_BUTTONS; i++)
    {
        currentGamepadButtonState[slot][i] = false;
        previousGamepadButtonState[slot][i] = false;
    }
    for (int i = 0; i < MAX_GAMEPAD_AXIS; i++)
        gamepadAxisState[slot][i] = 0.0f;
}

void Input::OnGamepadDeviceRemoved(const SDL_ControllerDeviceEvent &event)
{
    int slot = FindGamepadSlotByInstanceId(event.which);
    if (slot < 0)
        return;

    if (gamepads[slot])
        SDL_GameControllerClose(gamepads[slot]);

    gamepads[slot] = nullptr;
    gamepadInstanceIds[slot] = -1;
    for (int i = 0; i < MAX_GAMEPAD_BUTTONS; i++)
    {
        currentGamepadButtonState[slot][i] = false;
        previousGamepadButtonState[slot][i] = false;
    }
    for (int i = 0; i < MAX_GAMEPAD_AXIS; i++)
        gamepadAxisState[slot][i] = 0.0f;
}

void Input::OnGamepadButtonDown(const SDL_ControllerButtonEvent &event)
{
    int slot = FindGamepadSlotByInstanceId(event.which);
    if (slot < 0)
        return;

    GamepadButton button = ConvertSDLGamepadButton((SDL_GameControllerButton)event.button);
    if (button == GAMEPAD_BUTTON_UNKNOWN || (int)button >= MAX_GAMEPAD_BUTTONS)
        return;

    currentGamepadButtonState[slot][button] = true;
    lastButtonPressed = (int)button;
}

void Input::OnGamepadButtonUp(const SDL_ControllerButtonEvent &event)
{
    int slot = FindGamepadSlotByInstanceId(event.which);
    if (slot < 0)
        return;

    GamepadButton button = ConvertSDLGamepadButton((SDL_GameControllerButton)event.button);
    if (button == GAMEPAD_BUTTON_UNKNOWN || (int)button >= MAX_GAMEPAD_BUTTONS)
        return;

    currentGamepadButtonState[slot][button] = false;
}

void Input::OnGamepadAxisMotion(const SDL_ControllerAxisEvent &event)
{
    int slot = FindGamepadSlotByInstanceId(event.which);
    if (slot < 0)
        return;

    int axis = -1;
    switch ((SDL_GameControllerAxis)event.axis)
    {
    case SDL_CONTROLLER_AXIS_LEFTX:
        axis = GAMEPAD_AXIS_LEFT_X;
        break;
    case SDL_CONTROLLER_AXIS_LEFTY:
        axis = GAMEPAD_AXIS_LEFT_Y;
        break;
    case SDL_CONTROLLER_AXIS_RIGHTX:
        axis = GAMEPAD_AXIS_RIGHT_X;
        break;
    case SDL_CONTROLLER_AXIS_RIGHTY:
        axis = GAMEPAD_AXIS_RIGHT_Y;
        break;
    case SDL_CONTROLLER_AXIS_TRIGGERLEFT:
        axis = GAMEPAD_AXIS_LEFT_TRIGGER;
        break;
    case SDL_CONTROLLER_AXIS_TRIGGERRIGHT:
        axis = GAMEPAD_AXIS_RIGHT_TRIGGER;
        break;
    default:
        return;
    }

    float value = 0.0f;
    if (event.axis == SDL_CONTROLLER_AXIS_TRIGGERLEFT || event.axis == SDL_CONTROLLER_AXIS_TRIGGERRIGHT)
    {
        if (event.value > 0)
            value = (float)event.value / 32767.0f;
    }
    else
    {
        if (event.value == -32768)
            value = -1.0f;
        else
            value = (float)event.value / 32767.0f;
    }

    gamepadAxisState[slot][axis] = value;

    if (event.axis == SDL_CONTROLLER_AXIS_TRIGGERLEFT)
        currentGamepadButtonState[slot][GAMEPAD_BUTTON_LEFT_TRIGGER_2] = (value > 0.5f);
    else if (event.axis == SDL_CONTROLLER_AXIS_TRIGGERRIGHT)
        currentGamepadButtonState[slot][GAMEPAD_BUTTON_RIGHT_TRIGGER_2] = (value > 0.5f);
}

void Input::Init()
{
    memset(currentMouseState, 0, sizeof(currentMouseState));
    memset(previousMouseState, 0, sizeof(previousMouseState));
    mousePosition = {0.0f, 0.0f};
    mousePreviousPosition = {0.0f, 0.0f};
    mouseOffset = {0.0f, 0.0f};
    mouseScale = {1.0f, 1.0f};
    mouseWheel = {0.0f, 0.0f};

    for (int i = 0; i < MAX_GAMEPADS; i++)
    {
        gamepads[i] = nullptr;
        gamepadInstanceIds[i] = -1;
        for (int k = 0; k < MAX_GAMEPAD_BUTTONS; k++)
        {
            currentGamepadButtonState[i][k] = false;
            previousGamepadButtonState[i][k] = false;
        }
        for (int k = 0; k < MAX_GAMEPAD_AXIS; k++)
            gamepadAxisState[i][k] = 0.0f;
    }

    for (int i = 0; i < MAX_KEYBOARD_KEYS; i++)
    {
        previousKeyState[i] = false;
        currentKeyState[i] = false;
    }
    for (int i = 0; i < MAX_KEY_PRESSED_QUEUE; i++)
    {
        keyPressedQueue[i] = KEY_NULL;
    }
    for (int i = 0; i < MAX_CHAR_PRESSED_QUEUE; i++)
        charPressedQueue[i] = 0;
    keyPressedQueueCount = 0;
    charPressedQueueCount = 0;
    lastButtonPressed = GAMEPAD_BUTTON_UNKNOWN;

    SDL_StartTextInput();
    SDL_GameControllerEventState(SDL_ENABLE);

    const int joystickCount = SDL_NumJoysticks();
    for (int joystickIndex = 0; joystickIndex < joystickCount; joystickIndex++)
    {
        SDL_ControllerDeviceEvent event = {};
        event.which = joystickIndex;
        OnGamepadDeviceAdded(event);
    }
}

// ========== END FRAME ==========

void Input::Update()
{

    for (int i = 0; i < MAX_GAMEPADS; i++)
    {
        for (int k = 0; k < MAX_GAMEPAD_BUTTONS; k++)
        {

            previousGamepadButtonState[i][k] = currentGamepadButtonState[i][k];
        }
    }
    for (int i = 0; i < MAX_KEYBOARD_KEYS; i++)
    {
        previousKeyState[i] = currentKeyState[i];
    }

    for (int i = 0; i < MAX_MOUSE_BUTTONS; i++)
        previousMouseState[i] = currentMouseState[i];

    mousePreviousPosition = mousePosition;
    mouseWheel = {0.0f, 0.0f};
    keyPressedQueueCount = 0;
    charPressedQueueCount = 0;
    lastButtonPressed = GAMEPAD_BUTTON_UNKNOWN;
}

void Input::Shutdown()
{
    for (int i = 0; i < MAX_GAMEPADS; i++)
    {
        if (gamepads[i])
        {
            SDL_GameControllerClose(gamepads[i]);
            gamepads[i] = nullptr;
        }
        gamepadInstanceIds[i] = -1;
    }

    if (mouseCursor)
    {
        SDL_FreeCursor(mouseCursor);
        mouseCursor = nullptr;
    }

    SDL_StopTextInput();
}
