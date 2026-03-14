# Operators

## Arithmetic

```bulang
var a = 10 + 2;    // 12
var b = 10 - 2;    // 8
var c = 10 * 2;    // 20
var d = 10 / 2;    // 5
var e = 10 % 3;    // 1  (modulo)
```

## Compound Assignment

```bulang
x += 5;    x -= 5;
x *= 2;    x /= 2;
x %= 3;
```

## Increment / Decrement

```bulang
x++;    // postfix increment
++x;    // prefix increment
x--;    // postfix decrement
--x;    // prefix decrement
```

## Comparison

```bulang
x == y    // equal
x != y    // not equal
x <  y    // less than
x >  y    // greater than
x <= y    // less or equal
x >= y    // greater or equal
```

## Logical

```bulang
a && b    // AND
a || b    // OR
!a        // NOT
```

## Bitwise

```bulang
a & b     // AND
a | b     // OR
a ^ b     // XOR
~a        // NOT
a << 2    // left shift
a >> 2    // right shift
```

## Operator Precedence

Highest to lowest:

1. `!` `~` (unary)
2. `*` `/` `%`
3. `+` `-`
4. `<<` `>>`
5. `<` `>` `<=` `>=`
6. `==` `!=`
7. `&`
8. `^`
9. `|`
10. `&&`
11. `||`
12. `=` `+=` `-=` `*=` `/=` `%=`
