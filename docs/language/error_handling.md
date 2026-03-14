# Error Handling

## try / catch / finally

```bulang
try {
    var x = riskyOperation();
} catch (e) {
    write("Error: {}\n", e);
} finally {
    print("Always runs");
}
```

## throw

You can throw any value — string, number, or object:

```bulang
def divide(a, b) {
    if (b == 0) {
        throw "Division by zero!";
    }
    return a / b;
}

try {
    var r = divide(10, 0);
} catch (e) {
    print("Caught: " + e);
}
```

## Nested try/catch

```bulang
try {
    try {
        throw "inner error";
    } catch (e) {
        write("Inner catch: {}\n", e);
        throw "rethrown";
    }
} catch (e) {
    write("Outer catch: {}\n", e);
} finally {
    print("Outer finally");
}
```

## finally Always Runs

```bulang
def loadFile(path) {
    var f = openFile(path);
    try {
        return readData(f);
    } finally {
        closeFile(f);   // always called, even on throw
    }
}
```

!!! tip
    `finally` runs whether the `try` block succeeds, throws, or returns early.
