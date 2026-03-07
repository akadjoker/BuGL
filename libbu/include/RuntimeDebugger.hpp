class RuntimeDebugger
{
private:
    Interpreter *vm;
    bool enabled;
    bool stepMode;
    std::set<size_t> breakpoints;         // Line numbers
    std::map<std::string, Value> watches; // Watched variables

public:
    RuntimeDebugger(Interpreter *vm)
        : vm(vm), enabled(false), stepMode(false) {}

    void enable() { enabled = true; }
    void disable() { enabled = false; }

    // Breakpoints
    void addBreakpoint(size_t line)
    {
        breakpoints.insert(line);
        printf("Breakpoint set at line %zu\n", line);
    }

    void removeBreakpoint(size_t line)
    {
        breakpoints.erase(line);
    }

    bool hasBreakpoint(size_t line)
    {
        return breakpoints.find(line) != breakpoints.end();
    }

    // Step mode
    void enableStepMode() { stepMode = true; }
    void disableStepMode() { stepMode = false; }
    bool isStepMode() { return stepMode; }

    // Stack inspection
    void printStack(ProcessExec *fiber)
    {
        printf("\n=== STACK (top to bottom) ===\n");

        if (fiber->stackTop == fiber->stack)
        {
            printf("  (empty)\n");
            return;
        }

        int index = 0;
        for (Value *slot = fiber->stackTop - 1;
             slot >= fiber->stack; slot--)
        {
            printf("  [%2d] ", index++);
            printValue(*slot);
            printf("\n");
        }
        printf("=============================\n");
    }

    // Call frames
    void printCallFrames(ProcessExec *fiber)
    {
        printf("\n=== CALL FRAMES ===\n");

        for (int i = fiber->frameCount - 1; i >= 0; i--)
        {
            CallFrame *frame = &fiber->frames[i];
            Function *func = frame->func;

            size_t instruction = frame->ip - func->chunk.code - 1;
            int line = func->chunk.lines[instruction];

            printf("  [%d] %s() at line %d\n",
                   i,
                   func->name ? func->name->chars() : "<script>",
                   line);
        }
        printf("===================\n");
    }

    // Local variables
    void printLocals(ProcessExec *fiber)
    {
        printf("\n=== LOCAL VARIABLES ===\n");

        if (fiber->frameCount == 0)
        {
            printf("  (no active frame)\n");
            return;
        }

        CallFrame *frame = &fiber->frames[fiber->frameCount - 1];
        Value *slots = frame->slots;
        int slotCount = fiber->stackTop - slots;

        for (int i = 0; i < slotCount; i++)
        {
            printf("  local[%d] = ", i);
            printValue(slots[i]);
            printf("\n");
        }
        printf("=======================\n");
    }

    // Global variables
    void printGlobals()
    {
        printf("\n=== GLOBAL VARIABLES ===\n");

        for (auto &[name, value] : vm->globals)
        {
            printf("  %s = ", name->chars());
            printValue(value);
            printf("\n");
        }
        printf("========================\n");
    }

    // Processes (WDIV)
    void printProcesses()
    {
        printf("\n=== PROCESSES ===\n");
        printf("Alive: %zu\n", vm->aliveProcesses.size());

        for (Process *proc : vm->aliveProcesses)
        {
            printf("  [%u] %s (state=%d)\n",
                   proc->id,
                   proc->name ? proc->name->chars() : "<unnamed>",
                   (int)proc->state);

            // Print privates
            printf("    x=%.0f y=%.0f angle=%.0f\n",
                   proc->privates[0].asDouble(),
                   proc->privates[1].asDouble(),
                   proc->privates[4].asDouble());
        }
        printf("=================\n");
    }

    // Fibers (WSCRIPT)
    void printFibers()
    {
        printf("\n=== FIBERS ===\n");
        printf("Count: %zu\n", vm->aliveProcesses.size());

        for (Process *proc : vm->aliveProcesses)
        {
            ProcessExec *f = proc;
            printf("  [%u] state=%d frames=%d stack=%d\n",
                   proc->id,
                   (int)f->state,
                   f->frameCount,
                   (int)(f->stackTop - f->stack));
        }
        printf("==============\n");
    }

    // Watch variable
    void watch(const char *varName)
    {
        watches[varName] = Value::makeNil();
        printf("Watching variable '%s'\n", varName);
    }

    void updateWatches()
    {
        for (auto &[name, oldValue] : watches)
        {
            String *nameStr = vm->stringPool.intern(name.c_str());
            Value *newValue = vm->globals.get(nameStr);

            if (newValue && !valuesEqual(oldValue, *newValue))
            {
                printf("WATCH: '%s' changed from ", name.c_str());
                printValue(oldValue);
                printf(" to ");
                printValue(*newValue);
                printf("\n");

                watches[name] = *newValue;
            }
        }
    }

    // Interactive prompt
    void prompt(ProcessExec *fiber, size_t line)
    {
        printf("\n>>> Breakpoint at line %zu\n", line);

        while (true)
        {
            printf("(debug) ");

            char input[256];
            if (!fgets(input, sizeof(input), stdin))
                break;

            // Remove newline
            input[strcspn(input, "\n")] = 0;

            if (strcmp(input, "c") == 0 || strcmp(input, "continue") == 0)
            {
                break;
            }
            else if (strcmp(input, "s") == 0 || strcmp(input, "step") == 0)
            {
                stepMode = true;
                break;
            }
            else if (strcmp(input, "stack") == 0)
            {
                printStack(fiber);
            }
            else if (strcmp(input, "frames") == 0)
            {
                printCallFrames(fiber);
            }
            else if (strcmp(input, "locals") == 0)
            {
                printLocals(fiber);
            }
            else if (strcmp(input, "globals") == 0)
            {
                printGlobals();
            }
            else if (strcmp(input, "processes") == 0)
            {
                printProcesses();
            }
            else if (strcmp(input, "fibers") == 0)
            {
                printFibers();
            }
            else if (strncmp(input, "watch ", 6) == 0)
            {
                watch(input + 6);
            }
            else if (strncmp(input, "print ", 6) == 0)
            {
                // Evaluate expression (futuro)
                printf("Expression evaluation not implemented\n");
            }
            else if (strcmp(input, "help") == 0 || strcmp(input, "h") == 0)
            {
                printf("Commands:\n");
                printf("  c, continue  - Continue execution\n");
                printf("  s, step      - Step to next line\n");
                printf("  stack        - Print stack\n");
                printf("  frames       - Print call frames\n");
                printf("  locals       - Print local variables\n");
                printf("  globals      - Print global variables\n");
                printf("  processes    - Print processes (WDIV)\n");
                printf("  fibers       - Print fibers (WSCRIPT)\n");
                printf("  watch <var>  - Watch variable\n");
                printf("  help, h      - Show this help\n");
            }
            else
            {
                printf("Unknown command. Type 'help' for commands.\n");
            }
        }
    }

    // Called before each instruction
    void checkBreakpoint(ProcessExec *fiber)
    {
        if (!enabled)
            return;

        if (fiber->frameCount == 0)
            return;

        CallFrame *frame = &fiber->frames[fiber->frameCount - 1];
        size_t instruction = frame->ip - frame->func->chunk.code;
        int line = frame->func->chunk.lines[instruction];

        // Update watches
        updateWatches();

        // Check breakpoint or step mode
        if (hasBreakpoint(line) || stepMode)
        {
            stepMode = false; // Reset step mode
            prompt(fiber, line);
        }
    }
};
