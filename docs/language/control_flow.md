# Control Flow

## if / elif / else

```bulang
if (x > 100) {
    print("large");
} elif (x > 50) {
    print("medium");
} elif (x > 10) {
    print("small");
} else {
    print("tiny");
}
```

!!! tip
    BuGL uses `elif` (not `else if`).

## Nested Conditions

```bulang
def classify(x) {
    if (x < 0) {
        if (x < -100) { return "extreme negative"; }
        return "negative";
    } elif (x == 0) {
        return "zero";
    } else {
        if (x > 100) { return "extreme positive"; }
        return "positive";
    }
}
```

## Truthiness

| Value | Truthy? |
|-------|---------|
| `true` | ✅ |
| `false` | ❌ |
| `nil` | ❌ |
| `0` | ❌ |
| `0.0` | ❌ |
| `""` | ❌ |
| Any other | ✅ |
