# Maps

Maps are key-value stores (hash maps). Keys can be strings or integers.

## Creation

```bulang
var empty = {};
var m = {"name": "Hero", "hp": 100, "level": 5};
```

## Access & Mutation

```bulang
m["hp"] -= 10;
m["mp"] = 50;         // add new key

if (m.has("name")) {
    print(m["name"]);
}
```

## Methods

| Method | Description |
|--------|-------------|
| `.has(key)` | True if key exists |
| `.remove(key)` | Remove entry |
| `.keys()` | Array of all keys |
| `.values()` | Array of all values |
| `.length()` | Number of entries |
| `.clear()` | Remove all entries |

## Iterating

```bulang
foreach (var key in m.keys()) {
    write("{} = {}\n", key, m[key]);
}
```

## Nested Maps

```bulang
var config = {
    "window": {"width": 800, "height": 600},
    "audio":  {"volume": 80, "muted": false}
};

var w = config["window"]["width"];   // 800
```

## Map as Component/State

```bulang
var player = {
    "x": 400, "y": 300,
    "hp": 100, "mp": 50,
    "inventory": ["sword", "shield"]
};

player["hp"] -= 20;
player["inventory"].push("potion");
```
