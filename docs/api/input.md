# Input

## Keyboard

```bulang
if (IsKeyDown(KEY_RIGHT))    { x += speed; }  // held
if (IsKeyPressed(KEY_SPACE)) { jump(); }       // just pressed
if (IsKeyReleased(KEY_SPACE)){ land(); }       // just released
```

### Key Constants

`KEY_SPACE`, `KEY_ESCAPE`, `KEY_ENTER`, `KEY_TAB`,
`KEY_BACKSPACE`, `KEY_DELETE`, `KEY_INSERT`,
`KEY_UP`, `KEY_DOWN`, `KEY_LEFT`, `KEY_RIGHT`,
`KEY_F1` ... `KEY_F12`,
`KEY_0` ... `KEY_9`,
`KEY_A` ... `KEY_Z`,
`KEY_LEFT_SHIFT`, `KEY_LEFT_CONTROL`, `KEY_LEFT_ALT`.

## Mouse

```bulang
var (mx, my) = GetMousePosition();
var mx = GetMouseX();
var my = GetMouseY();

if (IsMousePressed(MOUSE_LEFT))  { shoot(); }
if (IsMouseDown(MOUSE_RIGHT))    { aim(); }

var wheel = GetMouseWheelMoveV();
```

### Mouse Constants

`MOUSE_LEFT`, `MOUSE_RIGHT`, `MOUSE_MIDDLE`.

## Gamepad

```bulang
if (IsGamepadAvailable(0)) {
    var axis = GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_X);
    if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN)) {
        jump();
    }
}
```

### Gamepad Constants

**Buttons:** `GAMEPAD_BUTTON_LEFT_FACE_UP/DOWN/LEFT/RIGHT`,
`GAMEPAD_BUTTON_RIGHT_FACE_UP/DOWN/LEFT/RIGHT`,
`GAMEPAD_BUTTON_LEFT_TRIGGER_1/2`, `GAMEPAD_BUTTON_RIGHT_TRIGGER_1/2`,
`GAMEPAD_BUTTON_MIDDLE_LEFT/RIGHT/MIDDLE`,
`GAMEPAD_BUTTON_LEFT_THUMB`, `GAMEPAD_BUTTON_RIGHT_THUMB`.

**Axes:** `GAMEPAD_AXIS_LEFT_X/Y`, `GAMEPAD_AXIS_RIGHT_X/Y`,
`GAMEPAD_AXIS_LEFT_TRIGGER`, `GAMEPAD_AXIS_RIGHT_TRIGGER`.
