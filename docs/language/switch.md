# Switch

## Basic Usage

!!! warning "No `break` needed"
    Unlike C/C++, BuGL `switch` does **not** fall through.
    Each `case` is isolated — no `break` required.

```bulang
switch (x) {
    case 1:
        print("one");
    case 2:
        print("two");
    case 3:
        print("three");
    default:
        print("other");
}
```

## With Return

```bulang
def describe(x) {
    switch (x) {
        case 1: return "one";
        case 2: return "two";
        case 3: return "three";
        default: return "unknown";
    }
}
```

## String Cases

```bulang
switch (state) {
    case "idle":    playIdleAnim();
    case "run":     playRunAnim();
    case "attack":  playAttackAnim();
    default:        playIdleAnim();
}
```

## Inside Classes

```bulang
class Enemy {
    def react(event) {
        switch (event) {
            case "hit":   self.hp -= 10;
            case "heal":  self.hp += 5;
            case "death": self.active = false;
        }
    }
}
```

!!! tip
    `switch` works with integers, floats, and strings.
