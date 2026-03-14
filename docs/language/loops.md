# Loops

## while

```bulang
var i = 0;
while (i < 10) {
    print(i);
    i++;
}
```

## do / loop

Runs the body at least once:

```bulang
var x = 0;
do {
    x++;
} loop (x < 5);
// x == 5
```

## for

```bulang
for (var i = 0; i < 10; i++) {
    print(i);
}
```

## foreach

Iterate over arrays:

```bulang
var fruits = ["apple", "banana", "cherry"];
foreach (var fruit in fruits) {
    print(fruit);
}
```

Iterate over map keys:

```bulang
var m = {"hp": 100, "mp": 50};
foreach (var key in m.keys()) {
    write("{} = {}\n", key, m[key]);
}
```

## break / continue

```bulang
var i = 0;
while (i < 100) {
    i++;
    if (i == 5)  { continue; }  // skip 5
    if (i == 10) { break; }     // stop at 10
    print(i);
}
```

## Nested Loops

```bulang
for (var row = 0; row < 3; row++) {
    for (var col = 0; col < 3; col++) {
        write("[{},{}] ", row, col);
    }
    print("");
}
```
