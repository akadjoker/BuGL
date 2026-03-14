# Functions

## Basic Definition

```bulang
def add(a, b) {
    return a + b;
}

var result = add(3, 4);   // 7
```

## Recursion

```bulang
def factorial(n) {
    if (n <= 1) return 1;
    return n * factorial(n - 1);
}

factorial(6);   // 720
```

## Nested Functions

Functions can be defined inside other functions:

```bulang
def outer(x) {
    def inner(y) {
        return x + y;   // captures x from outer
    }
    return inner(10);
}

outer(5);   // 15
```

## Closures

Nested functions capture variables from the enclosing scope:

```bulang
def makeCounter() {
    var count = 0;
    def increment() {
        count++;
        return count;
    }
    return increment;
}

var c = makeCounter();
c();   // 1
c();   // 2
c();   // 3
```

## Functions as Arguments

```bulang
def apply(func, value) {
    return func(value);
}

def double(x) { return x * 2; }
def square(x) { return x * x; }

apply(double, 5);   // 10
apply(square, 5);   // 25
```

## Multiple Return Values

```bulang
// Use arrays or maps for multiple returns
def minMax(arr) {
    var mn = arr[0];
    var mx = arr[0];
    foreach (var v in arr) {
        if (v < mn) { mn = v; }
        if (v > mx) { mx = v; }
    }
    return [mn, mx];
}

var r = minMax([3, 1, 4, 1, 5, 9]);
print(r[0]);   // 1
print(r[1]);   // 9
```

!!! note
    Functions without an explicit `return` return `nil`.
