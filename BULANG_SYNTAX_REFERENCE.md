# BuLang Syntax Reference

> Comprehensive language specification extracted from the BuGL engine source code (`libbu/`).

---

## Table of Contents

1. [Lexical Elements](#1-lexical-elements)
2. [Types & Values](#2-types--values)
3. [Declarations](#3-declarations)
4. [Operators](#4-operators)
5. [Control Flow](#5-control-flow)
6. [Functions](#6-functions)
7. [Classes](#7-classes)
8. [Structs](#8-structs)
9. [Processes (Coroutines)](#9-processes-coroutines)
10. [Closures & Upvalues](#10-closures--upvalues)
11. [Generic Calls](#11-generic-calls)
12. [Module System](#12-module-system)
13. [Exception Handling](#13-exception-handling)
14. [Goto / Gosub / Labels](#14-goto--gosub--labels)
15. [Built-in Keywords & Functions](#15-built-in-keywords--functions)
16. [Built-in Methods (String)](#16-built-in-methods-string)
17. [Built-in Methods (Array)](#17-built-in-methods-array)
18. [Built-in Methods (Map)](#18-built-in-methods-map)
19. [Built-in Methods (Buffer)](#19-built-in-methods-buffer)
20. [Typed Arrays (Native Classes)](#20-typed-arrays-native-classes)
21. [Module: math](#21-module-math)
22. [Module: file](#22-module-file)
23. [Module: fs](#23-module-fs)
24. [Module: json](#24-module-json)
25. [Module: os](#25-module-os)
26. [Module: path](#26-module-path)
27. [Module: regex](#27-module-regex)
28. [Module: time](#28-module-time)
29. [Module: socket (net)](#29-module-socket-net)
30. [Module: zip](#30-module-zip)
31. [Global Constants](#31-global-constants)

---

## 1. Lexical Elements

### Keywords

```
var  def  if  elif  else  while  for  foreach  in
return  break  continue  do  loop  switch  case  default
true  false  nil  print  process  type  frame  len  free
proc  get_id  exit  label  goto  gosub  struct  class  enum
self  super  include  import  using  require
try  catch  finally  throw
sin  cos  asin  acos  atan  atan2  sqrt  pow  log  abs
floor  ceil  deg  rad  tan  exp  clock
```

> **Note:** `enum`, `this`, `push`, and `time` are defined as token types in the compiler but are **not** registered as lexer keywords at this time. `enum` is reserved for future use.

### Comments

```bu
// Single-line comment

/* Multi-line
   block comment */
```

### Number Literals

| Format | Example | Token |
|--------|---------|-------|
| Decimal integer | `42`, `-7` | `TOKEN_INT` |
| Hexadecimal | `0xFF`, `0x1A3F` | `TOKEN_INT` |
| Float/Double | `3.14`, `0.5` | `TOKEN_FLOAT` |

> **Note:** Binary integer literals (`0b...`) are **not** supported. Large integers that exceed 32-bit range are stored as `UINT`.

### String Literals

**Regular strings** — double-quoted, with escape sequences:

```bu
"hello\nworld"
```

**Escape sequences:**

| Escape | Meaning |
|--------|---------|
| `\n` | Newline |
| `\t` | Tab |
| `\r` | Carriage return |
| `\\` | Backslash |
| `\"` | Double quote |
| `\0` | Null byte |
| `\a` | Alert (bell) |
| `\b` | Backspace |
| `\f` | Form feed |
| `\v` | Vertical tab |
| `\e` | Escape (0x1B) |
| `\xHH` | Hex byte (e.g. `\x41` = `A`) |
| `\uHHHH` | Unicode codepoint 4-digit (UTF-8 encoded) |
| `\UHHHHHHHH` | Unicode codepoint 8-digit (UTF-8 encoded) |

**Verbatim strings** — prefixed with `@`, no escape processing, allows multi-line, `""` for literal quote:

```bu
@"C:\Users\name\file.txt"
@"She said ""hello"" to me"
@"Multi
line
string"
```

> **String interpolation** — use `f"..."` syntax with `{expr}` placeholders:

```bu
var name = "World";
var x = 42;
print(f"Hello {name}!");         // Hello World!
print(f"Math: {2 + 3}");         // Math: 5
print(f"Value is {x * 2}");      // Value is 84
print(f"Escaped: {{literal}}");  // Escaped: {literal}
```

Expressions inside `{...}` are compiled and automatically converted to strings. Use `{{` and `}}` to produce literal braces.

### Identifiers

Identifiers start with a letter or `_`, followed by alphanumerics or `_`. Maximum length: 255 characters.

### Punctuation & Tokens

```
(  )  {  }  [  ]  ,  ;  :  ?  .  @
+  -  *  /  %
=  ==  !=  <  <=  >  >=
!  &&  ||  &  |  ^  ~  <<  >>
++  --
+=  -=  *=  /=  %=
```

---

## 2. Types & Values

BuLang is dynamically typed. Every value has one of these runtime types:

| Type | Description | Literal Examples |
|------|-------------|------------------|
| `nil` | Null/nothing | `nil` |
| `bool` | Boolean | `true`, `false` |
| `int` | 32-bit signed integer | `42`, `0xFF` |
| `uint` | 32-bit unsigned integer | (auto for large hex) |
| `float` | 32-bit float | (from C++ bindings) |
| `double` | 64-bit float | `3.14` |
| `string` | Immutable UTF-8 string | `"hello"` |
| `array` | Dynamic array of values | `[1, 2, 3]` |
| `map` | String-keyed hash map | `{name: "John", age: 30}` |
| `buffer` | Typed binary buffer | `@(size, type)` |
| `struct` | Struct type/definition | — |
| `struct instance` | Struct instance | `MyStruct(args)` |
| `class` | Class type/definition | — |
| `class instance` | Class instance | `MyClass(args)` |
| `function` | Script function | — |
| `closure` | Function capturing upvalues | — |
| `process` | Process type/definition | — |
| `process instance` | Running process | — |
| `native class` | C++ class exposed to script | — |
| `native struct` | C++ struct exposed to script | — |
| `pointer` | Raw C pointer | (from C++ bindings) |

---

## 3. Declarations

### Variable Declaration

```bu
var x;              // Declares x, initialized to nil
var x = 10;         // Declares x with value
var a, b, c;        // Multiple declarations
var a = 1, b = 2;   // Multiple with initializers
```

### Multi-Return Variable Declaration

```bu
var (a, b, c) = someFunc();   // Unpacks multiple return values
```

### Scope

Variables declared inside `{ }` blocks are local. Top-level declarations are global. Variables **must** be declared with `var` before use — implicit globals are not allowed.

---

## 4. Operators

### Precedence (lowest to highest)

| Precedence | Operators | Description |
|-----------|-----------|-------------|
| `ASSIGNMENT` | `=`, `+=`, `-=`, `*=`, `/=`, `%=` | Assignment |
| `CONDITIONAL` | `? :` | Ternary |
| `OR` | `\|\|` | Logical OR |
| `AND` | `&&` | Logical AND |
| `BITWISE_OR` | `\|` | Bitwise OR |
| `BITWISE_XOR` | `^` | Bitwise XOR |
| `BITWISE_AND` | `&` | Bitwise AND |
| `EQUALITY` | `==`, `!=` | Equality |
| `COMPARISON` | `<`, `<=`, `>`, `>=` | Comparison |
| `SHIFT` | `<<`, `>>` | Bit shift |
| `TERM` | `+`, `-` | Addition, subtraction |
| `FACTOR` | `*`, `/`, `%` | Multiplication, division, modulo |
| `UNARY` | `!`, `-`, `~`, `++`, `--` | Unary |
| `CALL` | `()`, `.`, `[]` | Call, member access, subscript |

### Arithmetic

```bu
a + b    a - b    a * b    a / b    a % b
```

### Comparison

```bu
a == b   a != b   a < b   a <= b   a > b   a >= b
```

### Logical

```bu
a && b      // Short-circuit AND
a || b      // Short-circuit OR
!a          // Logical NOT
```

### Bitwise

```bu
a & b       // AND
a | b       // OR
a ^ b       // XOR
~a          // NOT (complement)
a << n      // Left shift
a >> n      // Right shift
```

### Ternary

```bu
var result = condition ? valueIfTrue : valueIfFalse;
```

### Increment / Decrement

```bu
++i;        // Prefix: increments then returns new value
--i;        // Prefix: decrements then returns new value
i++;        // Postfix: returns old value then increments
i--;        // Postfix: returns old value then decrements
```

Works on variables, properties (`++obj.x`), and private fields.

### Compound Assignment

```bu
x += 1;    x -= 1;    x *= 2;    x /= 2;    x %= 3;
```

Also work with properties (`obj.x += 1`) and subscripts (`arr[i] += 1`).

---

## 5. Control Flow

### if / elif / else

```bu
if (condition)
{
    // ...
}
elif (other_condition)
{
    // ...
}
else
{
    // ...
}
```

- Parentheses around condition are **required**.
- Braces are optional for single statements.
- Multiple `elif` branches are supported.

### while

```bu
while (condition)
{
    // body
}
```

### do-while

```bu
do
{
    // body (always executes at least once)
}
while (condition);
```

### loop (infinite)

```bu
loop
{
    // Only exits via break
    if (done) break;
}
```

### for (C-style)

```bu
for (var i = 0; i < 10; i++)
{
    // body
}

for (;;)   // infinite loop
{
    break;
}
```

### foreach

```bu
foreach (item in collection)
{
    print(item);
}
```

Works with arrays, maps, and strings (iterates over characters/entries).

### switch / case / default

```bu
switch (value)
{
    case 1:
        print("one");
    case 2:
        print("two");
    default:
        print("other");
}
```

- Cases **auto-exit** (no fall-through). No explicit `break` needed (or allowed).
- `default` is optional.

### break / continue

```bu
break;        // Exit current loop
continue;     // Skip to next iteration
```

### return

```bu
return;                  // Return nil
return value;            // Return single value
return (a, b, c);        // Return multiple values
```

Multi-return values are unpacked with `var (x, y, z) = func();`.

---

## 6. Functions

### Declaration

```bu
def myFunction(a, b, c)
{
    return a + b + c;
}
```

### Nested Functions

```bu
def outer()
{
    def inner()
    {
        // Can capture outer's locals (becomes a closure)
    }
    inner();
}
```

Nested function names are internally mangled as `outer$inner`.

### Calling

```bu
myFunction(1, 2, 3);
```

Maximum 255 arguments.

---

## 7. Classes

### Declaration

```bu
class Animal
{
    var name;
    var age;
    var sound = "...";           // Default value (literal only)

    def init(name, age)
    {
        self.name = name;
        self.age = age;
    }

    def speak()
    {
        print(self.name, " says ", self.sound);
    }
}
```

### Instantiation

```bu
var dog = Animal("Rex", 5);
dog.speak();
```

### `self` / `this` keyword

`self` (or its alias `this`) is used inside methods to refer to the current instance:

```bu
self.name = "Rex";
self.speak();
```

### Inheritance (`:`)

```bu
class Dog : Animal
{
    var breed;

    def init(name, age, breed)
    {
        super.init(name, age);
        self.breed = breed;
        self.sound = "Woof!";
    }
}
```

- Inherit from script classes or native C++ classes.
- `super.methodName(args)` calls the parent method.
- Single inheritance only.

### Field Defaults

Fields can have literal default values (int, float, string, bool, nil). Complex expressions must be set in `init()`:

```bu
class Foo
{
    var x = 0;
    var name = "default";
    var active = true;
    var data;                 // Defaults to nil
}
```

---

## 8. Structs

Lightweight value containers (no methods, no inheritance):

```bu
struct Vec2
{
    x, y
}

// With var keyword (optional):
struct Vec3
{
    var x, y, z;
}
```

### Instantiation

```bu
var v = Vec2(10, 20);
print(v.x);           // 10
v.y = 30;
```

Fields are positional — constructor arguments map to field order.

---

## 8b. Enums

Enums define named integer constants grouped under a single name. They compile to a map, so members are accessed with dot notation.

### Declaration

```bu
enum Color {
    Red = 1,
    Green = 2,
    Blue = 3
}
```

### Auto-increment

If no explicit value is given, members auto-increment from 0 (or from the last explicit value + 1):

```bu
enum Direction {
    Up,       // 0
    Down,     // 1
    Left,     // 2
    Right     // 3
}
```

### Mixed values

```bu
enum HttpStatus {
    OK = 200,
    NotFound = 404,
    Error = 500
}
```

### Access

Members are accessed with `EnumName.MemberName`:

```bu
print(Color.Red);          // 1
print(HttpStatus.NotFound); // 404

var dir = Direction.Up;
if (dir == Direction.Left) {
    // ...
}
```

### With f-strings

```bu
enum Fruit { Apple = 1, Banana = 2, Cherry = 3 }
print(f"Cherry = {Fruit.Cherry}");  // Cherry = 3
```

---

## 9. Processes (Coroutines)

Processes are the core concurrency primitive in BuLang, inspired by game engine "entities" with built-in lifecycle management.

### Declaration

```bu
process Enemy(x, y, hp)
{
    // Parameters matching private names (x, y, hp) auto-bind to privates
    
    loop
    {
        // Game logic
        x += velx;
        y += vely;
        frame;              // Yield control, resume next tick
    }
}
```

### Spawning

Processes are spawned by calling them like functions. Each call creates a new concurrent instance:

```bu
var id = Enemy(100, 200, 50);   // Spawns and returns process ID
```

### `frame` statement

Yields execution until the next tick. The process is suspended and resumed by the engine's `update()` call:

```bu
frame;           // Yield for one full frame (100%)
frame(50);       // Yield for 50% of a frame
```

### `exit` statement

Terminates the current process:

```bu
exit;            // Exit with code 0
exit(1);         // Exit with error code
```

### Process Private Variables

Every process instance has built-in private variables accessible by name:

| Private | Type | Default | Description |
|---------|------|---------|-------------|
| `x` | double | 0.0 | X position |
| `y` | double | 0.0 | Y position |
| `z` | int | 0 | Z-order / depth |
| `graph` | int | -1 | Graphic/sprite ID |
| `angle` | int | 0 | Rotation angle |
| `size` | int | 100 | Scale (percentage) |
| `sizex` | double | 1.0 | X scale |
| `sizey` | double | 1.0 | Y scale |
| `flags` | int | 0 | Bitfield flags |
| `id` | int | auto | Unique process ID (read-only) |
| `father` | int | -1 | Parent process ID (read-only) |
| `red` | int | 255 | Red color component |
| `green` | int | 255 | Green color component |
| `blue` | int | 255 | Blue color component |
| `alpha` | int | 255 | Alpha (opacity) |
| `tag` | int | 0 | User-defined tag |
| `state` | int | 0 | User-defined state |
| `speed` | double | 0.0 | Speed value |
| `group` | int | 0 | Group ID |
| `velx` | double | 0.0 | X velocity |
| `vely` | double | 0.0 | Y velocity |
| `hp` | int | 0 | Hit points |
| `progress` | double | 0.0 | Progress value |
| `life` | int | 100 | Life value |
| `active` | int | 1 | Active flag |
| `show` | int | 1 | Visible flag |
| `xold` | int | 0 | Previous X position |
| `yold` | int | 0 | Previous Y position |

### Process Lifecycle

1. **Spawn** — `Enemy(x, y, hp)` creates instance, clones blueprint
2. **Running** — Executes until `frame`, then suspends
3. **Suspended** — Waits for next `update()` tick
4. **Dead** — Reaches end of body, `exit`, or runtime error

### `goto` / `gosub` / `return` in Processes

- `goto label;` — jumps to label (within the process body)
- `gosub label;` — calls label as subroutine, returns via `return;`
- `return;` — in a process acts as "return from gosub" (`OP_RETURN_SUB`)

### Helper Built-ins for Processes

```bu
type ProcessName       // Spawn/reference process type
proc(id)               // Get process by ID (returns process reference)
get_id(process_ref)    // Get process ID from reference
```

---

## 10. Closures & Upvalues

Functions that capture variables from enclosing scopes become closures automatically:

```bu
def makeCounter()
{
    var count = 0;
    def increment()
    {
        count++;      // Captures 'count' as upvalue
        return count;
    }
    return increment;
}

var counter = makeCounter();
print(counter());   // 1
print(counter());   // 2
```

Captured variables are "closed over" — they live on the heap even after the enclosing function returns.

---

## 11. Generic Calls

BuLang supports a generic call syntax for passing types as arguments:

```bu
var obj = createWidget<MyClass>(arg1, arg2);
var val = holder.method<Vector3>();
```

The type name between `<` and `>` is resolved as an implicit first argument. This is syntactic sugar:

```bu
// func<Type>(args...)  is equivalent to  func(Type, args...)
```

---

## 12. Module System

### import

Makes a module available for qualified access (`module.function()`):

```bu
import math;
import math, os, time;      // Multiple modules
import *;                    // Import all available modules
```

### using

After importing, `using` allows unqualified access to module members:

```bu
import math;
using math;

var x = lerp(0, 100, 0.5);    // Instead of math.lerp(...)
var pi = PI;                   // Instead of math.PI
```

### Qualified Access

```bu
import math;
var x = math.lerp(0, 100, 0.5);
var pi = math.PI;
```

### include

Inlines another source file at compile time:

```bu
include "utils.bu";
```

Circular includes are detected and rejected.

### require

Loads native plugins (shared libraries) at compile time:

```bu
require "SDL";
require "glfw,rlgl";     // Multiple plugins (comma-separated)
require "glfw;rlgl;gtk";  // Multiple plugins (semicolon-separated)
```

---

## 13. Exception Handling

### try / catch / finally

```bu
try
{
    // Code that may throw
    var data = riskyOperation();
}
catch (e)
{
    // Handle error. 'e' is the thrown value.
    print("Error: ", e);
}
finally
{
    // Always runs (optional)
    cleanup();
}
```

- `catch` or `finally` (or both) must follow `try`.
- `catch (varname)` — the variable receives the thrown value.
- `finally` — always executes, even if catch handled the error.

### throw

```bu
throw "Something went wrong";
throw 42;
throw myErrorObject;
```

Any value can be thrown.

---

## 14. Goto / Gosub / Labels

### Labels

```bu
myLabel:
    print("at label");
```

Labels are defined with `identifier:` at statement position.

### goto

```bu
goto myLabel;     // Unconditional jump to label
```

### gosub

```bu
gosub mySubroutine;    // Jump to label, push return address
// ... execution returns here after 'return;'

mySubroutine:
    print("in subroutine");
    return;              // Returns to after the gosub call
```

Limits: max 32 labels, 32 gotos, 32 gosubs per function. Gosub stack depth: 16.

---

## 15. Built-in Keywords & Functions

### Keyword-level builtins (compiled to opcodes)

| Function | Syntax | Description |
|----------|--------|-------------|
| `print(args...)` | `print("hello", x);` | Print values (variadic) |
| `len(value)` | `len(arr)` | Length of array, string, map, or buffer |
| `free(value)` | `free(obj)` | Mark value for GC collection |
| `clock()` | `clock()` | High-resolution CPU time (seconds) |
| `type TypeName` | `type Enemy` | Reference a process type |
| `proc(id)` | `proc(42)` | Get process by ID |
| `get_id(ref)` | `get_id(p)` | Get process ID from reference |

### Math builtins (compiled to opcodes)

These are keywords, available without imports:

| Function | Args | Description |
|----------|------|-------------|
| `sin(x)` | 1 | Sine (radians) |
| `cos(x)` | 1 | Cosine |
| `tan(x)` | 1 | Tangent |
| `asin(x)` | 1 | Arc sine |
| `acos(x)` | 1 | Arc cosine |
| `atan(x)` | 1 | Arc tangent |
| `atan2(y, x)` | 2 | Two-argument arc tangent |
| `sqrt(x)` | 1 | Square root |
| `pow(base, exp)` | 2 | Power |
| `log(x)` | 1 | Natural logarithm |
| `abs(x)` | 1 | Absolute value |
| `floor(x)` | 1 | Floor |
| `ceil(x)` | 1 | Ceiling |
| `deg(x)` | 1 | Radians to degrees |
| `rad(x)` | 1 | Degrees to radians |
| `exp(x)` | 1 | e^x |

### Native functions (registered at startup)

| Function | Args | Description |
|----------|------|-------------|
| `format(fmt, args...)` | variadic | String formatting with `{}` placeholders |
| `write(fmt, args...)` | variadic | Like `format()` but prints directly (no newline) |
| `input([prompt])` | 0-1 | Read line from stdin |
| `str(value)` | 1 | Convert value to string |
| `int(value)` | 1 | Convert to integer |
| `real(value)` | 1 | Convert to double |
| `classname(instance)` | 1 | Get class/struct name as string |
| `typeid(type_or_instance)` | 1 | Get unique type ID (int) |
| `ticks(dt)` | 1 | Trigger an engine update tick |
| `_gc()` | 0 | Force garbage collection |
| `print_stack([label])` | 0-1 | Debug: dump VM stack |

---

## 16. Built-in Methods (String)

Strings have built-in methods accessible via dot notation:

| Method | Example | Description |
|--------|---------|-------------|
| `.length()` | `s.length()` | String length |
| `.upper()` | `s.upper()` | To uppercase |
| `.lower()` | `s.lower()` | To lowercase |
| `.concat(other)` | `s.concat("!")` | Concatenate strings |
| `.sub(start, len)` | `s.sub(0, 5)` | Substring |
| `.replace(old, new)` | `s.replace("a", "b")` | Replace occurrences |
| `.at(index)` | `s.at(0)` | Character at index |
| `.contains(substr)` | `s.contains("hello")` | Check if contains substring |
| `.trim()` | `s.trim()` | Remove leading/trailing whitespace |
| `.startswith(prefix)` | `s.startswith("He")` | Check prefix |
| `.endswith(suffix)` | `s.endswith(".txt")` | Check suffix |
| `.indexof(substr)` | `s.indexof("lo")` | Find index of substring (-1 if not found) |
| `.repeat(n)` | `s.repeat(3)` | Repeat string n times |
| `.split(delim)` | `s.split(",")` | Split into array |
| `.slice(start, end)` | `s.slice(1, 4)` | Slice substring |

---

## 17. Built-in Methods (Array)

| Method | Example | Description |
|--------|---------|-------------|
| `.push(value)` | `arr.push(42)` | Append element (optimized opcode) |
| `.pop()` | `arr.pop()` | Remove and return last element |
| `.back()` | `arr.back()` | Return last element without removing |
| `.length()` | `arr.length()` | Number of elements |
| `.clear()` | `arr.clear()` | Remove all elements |
| `.insert(index, value)` | `arr.insert(0, "first")` | Insert at index |
| `.remove(index)` | `arr.remove(2)` | Remove at index |
| `.find(value)` | `arr.find(42)` | Find index of value (-1 if not found) |
| `.contains(value)` | `arr.contains(42)` | Check if array contains value |
| `.reverse()` | `arr.reverse()` | Reverse in-place |
| `.slice(start, end)` | `arr.slice(1, 3)` | Return new sub-array |
| `.concat(other)` | `arr.concat(arr2)` | Concatenate arrays |
| `.join(sep)` | `arr.join(", ")` | Join elements into string |
| `.first()` | `arr.first()` | First element |
| `.last()` | `arr.last()` | Last element |
| `.fill(value)` | `arr.fill(0)` | Fill all elements with a value |

### Indexing

```bu
var x = arr[0];        // Get by index
arr[0] = 42;           // Set by index
arr[i] += 1;           // Compound assignment on index
```

---

## 18. Built-in Methods (Map)

| Method | Example | Description |
|--------|---------|-------------|
| `.has(key)` | `m.has("name")` | Check if key exists |
| `.remove(key)` | `m.remove("name")` | Remove key-value pair |
| `.keys()` | `m.keys()` | Return array of keys |
| `.values()` | `m.values()` | Return array of values |
| `.length()` | `m.length()` | Number of entries |
| `.clear()` | `m.clear()` | Remove all entries |

### Map Literals

```bu
var m = {name: "John", age: 30};
var m2 = {"key with spaces": 42, normal: true};
```

Keys can be identifiers or string literals. Values are any expression.

### Access

```bu
var name = m["name"];     // Subscript access
m["age"] = 31;            // Subscript assignment
```

---

## 19. Built-in Methods (Buffer)

Buffers are typed binary data containers. Created with `@(count, type)`:

```bu
var buf = @(1024, TYPE_UINT8);    // 1024 bytes
var buf = @(256, TYPE_FLOAT);     // 256 floats
```

| Method | Description |
|--------|-------------|
| `.length()` | Number of elements |
| `.fill(value)` | Fill all elements |
| `.copy(src)` | Copy from another buffer |
| `.slice(start, end)` | Sub-buffer |
| `.save(path)` | Save buffer to file |
| `.writeByte(value)` | Write byte at cursor |
| `.writeShort(value)` | Write int16 at cursor |
| `.writeUShort(value)` | Write uint16 at cursor |
| `.writeInt(value)` | Write int32 at cursor |
| `.writeUInt(value)` | Write uint32 at cursor |
| `.writeFloat(value)` | Write float at cursor |
| `.writeDouble(value)` | Write double at cursor |
| `.writeString(value)` | Write length-prefixed string at cursor |
| `.readByte()` | Read byte at cursor |
| `.readShort()` | Read int16 at cursor |
| `.readUShort()` | Read uint16 at cursor |
| `.readInt()` | Read int32 at cursor |
| `.readUInt()` | Read uint32 at cursor |
| `.readFloat()` | Read float at cursor |
| `.readDouble()` | Read double at cursor |
| `.readString()` | Read length-prefixed string at cursor |
| `.seek(position)` | Set cursor position |
| `.tell()` | Get cursor position |
| `.rewind()` | Reset cursor to 0 |
| `.skip(n)` | Advance cursor by n |
| `.remaining()` | Bytes remaining after cursor |

---

## 20. Typed Arrays (Native Classes)

BuLang provides typed array classes for efficient numeric storage:

| Class | Element Type | Element Size |
|-------|-------------|-------------|
| `Uint8Array` | uint8 | 1 byte |
| `Int16Array` | int16 | 2 bytes |
| `Uint16Array` | uint16 | 2 bytes |
| `Int32Array` | int32 | 4 bytes |
| `Uint32Array` | uint32 | 4 bytes |
| `Float32Array` | float | 4 bytes |
| `Float64Array` | double | 8 bytes |

### Constructor

```bu
var a = Uint8Array(1024);           // Allocate capacity for 1024 elements
var a = Float32Array([1.0, 2.0]);   // From array
var a = Int32Array(someBuffer);     // From buffer
```

### Methods

| Method | Args | Description |
|--------|------|-------------|
| `.add(value)` | 1+ | Append number(s), array, buffer, or typed array |
| `.clear()` | 0 | Reset count to 0 |
| `.reserve(n)` | 1 | Ensure capacity for n elements |
| `.pack()` | 0 | Shrink allocation to fit count |
| `.get(index)` | 1 | Get element at index |
| `.set(index, value)` | 2 | Set element at index |
| `.toBuffer()` | 0 | Convert to Buffer |

### Properties (read-only)

| Property | Description |
|----------|-------------|
| `.length` | Number of elements |
| `.capacity` | Allocated capacity |
| `.byteLength` | Total bytes used |
| `.ptr` | Raw pointer to underlying data |

---

## 21. Module: math

```bu
import math;
using math;
```

### Constants

| Constant | Value |
|----------|-------|
| `math.PI` | 3.14159265358979 |
| `math.E` | 2.71828182845905 |
| `math.TAU` | 6.28318530717958 |
| `math.SQRT2` | 1.41421356 |
| `math.MIN_INT` | -2147483648 |
| `math.MAX_INT` | 2147483647 |

### Functions

| Function | Args | Description |
|----------|------|-------------|
| `math.lerp(a, b, t)` | 3 | Linear interpolation |
| `math.catmull(p0, p1, p2, p3, t)` | 5 | Catmull-Rom interpolation |
| `math.map(x, inMin, inMax, outMin, outMax)` | 5 | Remap value between ranges |
| `math.sign(x)` | 1 | Returns -1, 0, or 1 |
| `math.hypot(dx, dy)` | 2 | Hypotenuse (overflow-safe) |
| `math.log10(x)` | 1 | Base-10 logarithm |
| `math.log2(x)` | 1 | Base-2 logarithm |
| `math.sinh(x)` | 1 | Hyperbolic sine |
| `math.cosh(x)` | 1 | Hyperbolic cosine |
| `math.tanh(x)` | 1 | Hyperbolic tangent |
| `math.smoothstep(t)` | 1 or 3 | Smooth Hermite interpolation |
| `math.smootherstep(t)` | 1 or 3 | Ken Perlin's smoother step |
| `math.hermite(v1, t1, v2, t2, amount)` | 5 | Hermite interpolation |
| `math.repeat(t, length)` | 2 | Repeat value in range |
| `math.ping_pong(t, length)` | 2 | Ping-pong value in range |
| `math.abs(x)` | 1 | Absolute value |
| `math.clamp(v, lo, hi)` | 3 | Clamp value to range |
| `math.min(a, b)` | 2 | Minimum of two values |
| `math.max(a, b)` | 2 | Maximum of two values |
| `math.seed(n)` | 1 | Set random seed |
| `math.rand([max])` or `math.rand(min, max)` | 0-2 | Random float (0-1, 0-max, or min-max) |
| `math.irand([max])` or `math.irand(min, max)` | 0-2 | Random integer |

---

## 22. Module: file

Binary file I/O with cursor-based reading/writing:

```bu
import file;
```

| Function | Args | Description |
|----------|------|-------------|
| `file.exists(path)` | 1 | Check if file exists → bool |
| `file.open(path, [mode])` | 1-2 | Open file → handle ID. Modes: `"r"`, `"w"`, `"rw"` |
| `file.close(id)` | 1 | Close file → bool |
| `file.save(id)` | 1 | Flush/write file to disk → bool |
| `file.seek(id, pos)` | 2 | Set cursor position → bool |
| `file.tell(id)` | 1 | Get cursor position → int |
| `file.size(id)` | 1 | Get file size → int |
| `file.write_byte(id, val)` | 2 | Write uint8 → bool |
| `file.write_int(id, val)` | 2 | Write int32 → bool |
| `file.write_float(id, val)` | 2 | Write float → bool |
| `file.write_double(id, val)` | 2 | Write double → bool |
| `file.write_bool(id, val)` | 2 | Write bool → bool |
| `file.write_string(id, val)` | 2 | Write length-prefixed string → bool |
| `file.read_byte(id)` | 1 | Read uint8 → int |
| `file.read_int(id)` | 1 | Read int32 → int |
| `file.read_float(id)` | 1 | Read float → double |
| `file.read_double(id)` | 1 | Read double → double |
| `file.read_bool(id)` | 1 | Read bool → bool |
| `file.read_string(id)` | 1 | Read length-prefixed string → string |

---

## 23. Module: fs

High-level filesystem operations:

```bu
import fs;
```

| Function | Args | Description |
|----------|------|-------------|
| `fs.read(path)` | 1 | Read entire file as string (nil on failure) |
| `fs.write(path, data)` | 2 | Write string to file → bool |
| `fs.append(path, data)` | 2 | Append string to file → bool |
| `fs.remove(path)` | 1 | Delete file → bool |
| `fs.mkdir(path)` | 1 | Create directory → bool |
| `fs.rmdir(path)` | 1 | Remove directory → bool |
| `fs.list(path)` | 1 | List directory contents → array of strings |
| `fs.stat(path)` | 1 | Get file info → map with `size`, `isdir`, `isfile`, `mode`, `mtime` |

---

## 24. Module: json

```bu
import json;
```

| Function | Args | Description |
|----------|------|-------------|
| `json.parse(str)` | 1 | Parse JSON string → value (array, map, string, number, bool, nil) |
| `json.stringify(value, [pretty])` | 1-2 | Serialize value to JSON string. `pretty`: bool or int (indent width) |

Supports: null, booleans, numbers (int/float), strings, arrays, maps (objects). Detects cyclic references.

---

## 25. Module: os

```bu
import os;
```

### Constant

| Constant | Description |
|----------|-------------|
| `os.platform` | `"windows"`, `"linux"`, `"macos"`, `"android"`, `"emscripten"`, etc. |

### Functions

| Function | Args | Description |
|----------|------|-------------|
| `os.execute(cmd)` | 1 | Run shell command → exit code |
| `os.getenv(name)` | 1 | Get environment variable → string |
| `os.setenv(name, value)` | 2 | Set environment variable → bool |
| `os.getcwd()` | 0 | Get current working directory → string |
| `os.chdir(path)` | 1 | Change directory → bool |
| `os.quit(code)` | 1 | Exit program with code |
| `os.spawn(cmd, [args...])` | variadic | Spawn child process → PID |
| `os.spawn_shell(cmd)` | 1 | Spawn via shell → PID |
| `os.spawn_capture(cmd)` | 1 | Run and capture output → map `{output, stdout, code, status}` |
| `os.wait(pid, [timeout_ms])` | 1-2 | Wait for process → exit code (nil on timeout) |
| `os.poll(pid)` | 1 | Non-blocking check → exit code or nil (still running) |
| `os.is_alive(pid)` | 1 | Check if process alive → bool |
| `os.kill(pid, [signal])` | 1-2 | Kill process → bool |

---

## 26. Module: path

```bu
import path;
```

| Function | Args | Description |
|----------|------|-------------|
| `path.join(parts...)` | variadic | Join path segments |
| `path.normalize(p)` | 1 | Normalize path (resolve `.` and `..`) |
| `path.dirname(p)` | 1 | Directory part of path |
| `path.basename(p)` | 1 | Filename part of path |
| `path.extname(p)` | 1 | File extension (e.g. `".txt"`) |
| `path.exists(p)` | 1 | Check if path exists → bool |
| `path.isdir(p)` | 1 | Check if path is directory → bool |
| `path.isfile(p)` | 1 | Check if path is file → bool |

---

## 27. Module: regex

```bu
import regex;
```

| Function | Args | Description |
|----------|------|-------------|
| `regex.match(pattern, text)` | 2 | Full match → bool |
| `regex.search(pattern, text)` | 2 | Partial match → bool |
| `regex.replace(pattern, replacement, text)` | 3 | Replace matches → string |
| `regex.findall(pattern, text)` | 2 | Find all matches → array of strings |

Uses C++ `<regex>` (ECMAScript syntax by default).

---

## 28. Module: time

```bu
import time;
```

| Function | Args | Description |
|----------|------|-------------|
| `time.now()` | 0 | Unix timestamp in seconds → int |
| `time.now_ms()` | 0 | Unix timestamp in milliseconds → int |
| `time.current()` | 0 | High-precision clock (nanosecond resolution) → double (seconds) |
| `time.sleep(seconds)` | 1 | Sleep (double seconds) |
| `time.sleep_ms(ms)` | 1 | Sleep (integer milliseconds) |
| `time.date([timestamp])` | 0-1 | Decompose timestamp → map `{year, month, day, hour, minute, second, weekday, yearday}` |
| `time.ftime(timestamp, [format])` | 1-2 | Format timestamp → string (default: `"%Y-%m-%d %H:%M:%S"`) |
| `time.parse(dateStr, formatStr)` | 2 | Parse date string → timestamp int |
| `time.diff(t1, t2)` | 2 | Difference between timestamps → int |

---

## 29. Module: socket (net)

```bu
import socket;
```

### Setup

| Function | Args | Description |
|----------|------|-------------|
| `socket.init()` | 0 | Initialize socket subsystem (Windows WSA) |
| `socket.quit()` | 0 | Cleanup all sockets |

### TCP

| Function | Args | Description |
|----------|------|-------------|
| `socket.tcp_listen([port, [backlog]])` | variadic | Create TCP server → socket ID |
| `socket.tcp_accept(server_id)` | 1 | Accept connection → client socket ID |
| `socket.tcp_connect(host, port)` | 2 | Connect to server → socket ID |

### UDP

| Function | Args | Description |
|----------|------|-------------|
| `socket.udp_create(port)` | 1 | Create UDP socket → socket ID |

### Send / Receive

| Function | Args | Description |
|----------|------|-------------|
| `socket.send(id, data)` | 2 | Send string data → bytes sent |
| `socket.receive(id, [maxLen])` | variadic | Receive data → string |
| `socket.sendto(id, data, host, port)` | 4 | UDP send → bytes sent |
| `socket.recvfrom(id, [maxLen])` | variadic | UDP receive → map |

### Options

| Function | Args | Description |
|----------|------|-------------|
| `socket.set_blocking(id, blocking)` | 2 | Set blocking mode → bool |
| `socket.set_nodelay(id, nodelay)` | 2 | Set TCP_NODELAY → bool |
| `socket.is_connected(id)` | 1 | Check connection → bool |
| `socket.info(id)` | 1 | Get socket info → map |
| `socket.close(id)` | 1 | Close socket → bool |

### HTTP (high-level)

| Function | Args | Description |
|----------|------|-------------|
| `socket.http_get(url, [options])` | variadic | HTTP GET → map `{status_code, body, headers, success}` |
| `socket.http_post(url, [options])` | variadic | HTTP POST → map |
| `socket.download_file(url, path, [options])` | variadic | Download file → bool |

Options map supports: `headers` (map), `params` (map), `timeout` (int), `user_agent` (string), `data` (string/map), `json` (map).

### Utilities

| Function | Args | Description |
|----------|------|-------------|
| `socket.ping(host, [port, [timeout]])` | variadic | TCP ping → bool |
| `socket.get_local_ip()` | 0 | Get local IP → string |
| `socket.resolve(hostname)` | 1 | DNS resolve → string (IP) |

---

## 30. Module: zip

```bu
import zip;
```

| Function | Args | Description |
|----------|------|-------------|
| `zip.list(archivePath)` | 1 | List entries → array of filenames |
| `zip.read(archivePath, entryName)` | 2 | Read entry as string |
| `zip.read_buffer(archivePath, entryName)` | 2 | Read entry as Buffer |
| `zip.extract(archivePath, outputDir)` | 2 | Extract all to directory → bool |
| `zip.create(archivePath, filesArray, [level])` | 2-3 | Create zip from file paths → bool. Level: 0-10, default 6 |

---

## 31. Global Constants

These constants are always available (no import needed):

### Buffer Type Constants

| Constant | Value | Description |
|----------|-------|-------------|
| `TYPE_UINT8` | 0 | Unsigned 8-bit |
| `TYPE_INT16` | 1 | Signed 16-bit |
| `TYPE_UINT16` | 2 | Unsigned 16-bit |
| `TYPE_INT32` | 3 | Signed 32-bit |
| `TYPE_UINT32` | 4 | Unsigned 32-bit |
| `TYPE_FLOAT` | 5 | 32-bit float |
| `TYPE_DOUBLE` | 6 | 64-bit double |

---

## Appendix: Complete Grammar Summary

```
program        → declaration* EOF

declaration    → "def" funDecl
               | "process" processDecl
               | "var" varDecl
               | "import" importStmt
               | "include" includeStmt
               | "using" usingStmt
               | "require" requireStmt
               | statement

statement      → exprStmt
               | printStmt
               | ifStmt
               | whileStmt
               | doWhileStmt
               | loopStmt
               | forStmt
               | foreachStmt
               | switchStmt
               | breakStmt
               | continueStmt
               | returnStmt
               | structDecl
               | enumDecl
               | classDecl
               | tryStmt
               | throwStmt
               | block
               | labelStmt
               | gotoStmt
               | gosubStmt
               | frameStmt
               | exitStmt

varDecl        → "var" ( "(" IDENT ("," IDENT)* ")" "=" expr ";" )
               | "var" IDENT ("=" expr)? ("," IDENT ("=" expr)?)* ";"

funDecl        → IDENT "(" params? ")" block
processDecl    → IDENT "(" params? ")" block

classDecl      → IDENT (":" IDENT)? "{" fieldDecls* methodDecls* "}"
structDecl     → IDENT "{" fieldList* "}"
enumDecl       → IDENT "{" (IDENT ("=" expr)? ","?)* "}"

block          → "{" declaration* "}"

expression     → assignment
assignment     → ternary ( "=" | "+=" | "-=" | "*=" | "/=" | "%=" ) assignment
               | ternary
ternary        → or ( "?" expression ":" expression )?
or             → and ( "||" and )*
and            → bitwiseOr ( "&&" bitwiseOr )*
bitwiseOr      → bitwiseXor ( "|" bitwiseXor )*
bitwiseXor     → bitwiseAnd ( "^" bitwiseAnd )*
bitwiseAnd     → equality ( "&" equality )*
equality       → comparison ( ( "==" | "!=" ) comparison )*
comparison     → shift ( ( "<" | "<=" | ">" | ">=" ) shift )*
shift          → term ( ( "<<" | ">>" ) term )*
term           → factor ( ( "+" | "-" ) factor )*
factor         → unary ( ( "*" | "/" | "%" ) unary )*
unary          → ( "!" | "-" | "~" | "++" | "--" ) unary | call
call           → primary ( "(" args? ")" | "." IDENT | "[" expr "]" | "<" TYPE ">" "(" args? ")" )*
primary        → INT | FLOAT | STRING | FSTRING | "true" | "false" | "nil"
               | IDENT | "(" expression ")" | "[" arrayElems? "]"
               | "{" mapElems? "}" | "@" "(" expr "," expr ")"
               | "self" | "super" "." IDENT "(" args? ")"
               | mathFunc | "clock()" | "len(" expr ")" | "free(" expr ")"
               | "type" IDENT | "proc(" expr ")" | "get_id(" expr ")"
```



