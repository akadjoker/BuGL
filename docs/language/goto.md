# Goto & Gosub

`goto` and `gosub` provide low-level jump control, useful for state machines and legacy-style code.

!!! warning
    Prefer `if/elif/else`, `switch`, or `while` for normal control flow.
    Use `goto`/`gosub` only for state machines or performance-critical loops.

## goto

Unconditional jump to a label:

```bulang
var i = 0;

label loop_start;
    print(i);
    i++;
    if (i < 5) goto loop_start;

print("done");
```

## gosub / return

`gosub` jumps to a label and saves the return address. `return` comes back:

```bulang
gosub draw_hud;
gosub draw_hud;   // can call multiple times
goto end;

label draw_hud;
    DrawText("HP: 100", 10, 10, 16, 255, 255, 255, 255);
    DrawText("MP: 50",  10, 30, 16, 100, 100, 255, 255);
return;

label end;
```

## State Machine Example

```bulang
var state = "idle";

process enemy() {
    while (Running()) {
        if      (state == "idle")   { goto st_idle;   }
        elif    (state == "chase")  { goto st_chase;  }
        elif    (state == "attack") { goto st_attack; }
        goto st_idle;

        label st_idle;
            // stand still
            frame; goto loop_end;

        label st_chase;
            // move towards player
            x += 1;
            frame; goto loop_end;

        label st_attack;
            // deal damage
            frame; goto loop_end;

        label loop_end;
    }
}
```

!!! tip
    For state machines, `switch` is usually cleaner than `goto`.
