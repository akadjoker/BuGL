# Variables & Types

## Declaration

All variables are declared with `var`:

```bulang
var x = 10;           // integer
var f = 3.14;         // float
var s = "hello";      // string
var b = true;         // boolean
var n = nil;          // null
var h = 0xFF;         // hex integer
```

## Types

| Type | Example | Notes |
|------|---------|-------|
| Integer | `42`, `0xFF` | 64-bit |
| Float | `3.14`, `1.0` | 64-bit double |
| String | `"hello"` | UTF-8, double quotes only |
| Boolean | `true`, `false` | |
| Nil | `nil` | absence of value |
| Array | `[1, 2, 3]` | dynamic |
| Map | `{"a": 1}` | hash map |
| Buffer | `Buffer(64)` | binary buffer |

## String Escapes

```bulang
var s = "line1\nline2";
var t = "tab\there";
var u = "unicode: \u0041";   // A
var x = "hex: \x41";         // A
```

| Escape | Meaning |
|--------|---------|
| `\n` | Newline |
| `\t` | Tab |
| `\r` | Carriage return |
| `\\` | Backslash |
| `\"` | Quote |
| `\xHH` | Hex byte |
| `\uHHHH` | Unicode (4 hex digits) |
| `\UHHHHHHHH` | Unicode (8 hex digits) |

## Multiple Return Assignment

```bulang
var (x, y) = GetMousePosition();
```

## Type Checking

```bulang
var t = type(x);   // returns "INTEGER", "FLOAT", "STRING", etc.
```
