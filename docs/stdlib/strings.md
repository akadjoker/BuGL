# Strings

## Creation

```bulang
var s = "Hello, World!";
var ml = "line1\nline2\nline3";
var uni = "Caf\u00E9";   // Café
```

Strings use **double quotes only**. Single quotes are not supported.

## Concatenation

```bulang
var full = "Hello" + ", " + "World!";

// Mixing types — use str() to convert
var msg = "Score: " + str(score);
```

## Methods

| Method | Description |
|--------|-------------|
| `.upper()` | Uppercase |
| `.lower()` | Lowercase |
| `.trim()` | Remove leading/trailing whitespace |
| `.length()` | Character count |
| `.sub(start, len)` | Substring |
| `.indexOf(str)` | Position of substring (-1 if not found) |
| `.contains(str)` | True if substring exists |
| `.replace(old, new)` | Replace occurrences |
| `.split(sep)` | Split into array |
| `.join(sep)` | Join array into string |
| `.repeat(n)` | Repeat string N times |
| `.startsWith(str)` | True if starts with |
| `.endsWith(str)` | True if ends with |
| `.at(i)` | Character at index |
| `.concat(str)` | Concatenate |

## Global `len()`

```bulang
len(s);   // same as s.length()
```

## `write()` — Format Strings

```bulang
write("Name: {} HP: {} MP: {}\n", name, hp, mp);
```

Use `{}` as placeholder, in order.

## Examples

```bulang
var s = "  Hello World  ";
s = s.trim().upper();           // "HELLO WORLD"
var pos = s.indexOf("WORLD");   // 6
var sub = s.sub(0, 5);          // "HELLO"
var parts = "a,b,c".split(","); // ["a", "b", "c"]
var rep = "ha".repeat(3);       // "hahaha"
```
