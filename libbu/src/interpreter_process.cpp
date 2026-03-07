#include "interpreter.hpp"
#include "pool.hpp"

static uint64_t PROCESS_IDS = 0;

void ProcessDef::finalize()
{
    ProcessExec *fiber = this;
    if (fiber->frameCount <= 0)
    {
        return;
    }

    for (int j = 0; j < fiber->frameCount; j++)
    {
        CallFrame *frame = &fiber->frames[j];
        if (frame->func && frame->ip == nullptr)
        {
            frame->ip = frame->func->chunk->code;
        }
    }

    if (fiber->ip == nullptr && fiber->frames[0].func)
    {
        fiber->ip = fiber->frames[0].func->chunk->code;
    }
}
void ProcessDef::release()
{
}

void Process::reset()
{
    this->id = 0;
    this->blueprint = -1;
    this->exitCode = 0;
    this->initialized = false;
    name = nullptr;

    state = ProcessState::DEAD; // Estado do PROCESSO (frame)
    stackTop = stack;
    frameCount = 0;
    ip = nullptr;
    resumeTime = 0.0f;        // Quando acorda (frame)
    gosubTop = 0;
    tryDepth = 0;
}

int Interpreter::getProcessPrivateIndex(const char *name)
{
   // if (!name || name[0] == '\0') return -1;

    switch (name[0])
    {
    case 'x':
        if (name[1] == '\0') return (int)PrivateIndex::X;
        if (strcmp(name, "xold") == 0) return (int)PrivateIndex::XOLD;
        return -1;

    case 'y':
        if (name[1] == '\0') return (int)PrivateIndex::Y;
        if (strcmp(name, "yold") == 0) return (int)PrivateIndex::YOLD;
        return -1;

    case 'z':
        return (name[1] == '\0') ? (int)PrivateIndex::Z : -1;

    case 'g':
        if (strcmp(name, "graph") == 0) return (int)PrivateIndex::GRAPH;
        if (strcmp(name, "green") == 0) return (int)PrivateIndex::iGREEN;
        if (strcmp(name, "group") == 0) return (int)PrivateIndex::GROUP;
        return -1;

    case 'a':
        if (strcmp(name, "angle") == 0) return (int)PrivateIndex::ANGLE;
        if (strcmp(name, "alpha") == 0) return (int)PrivateIndex::iALPHA;
        if (strcmp(name, "active") == 0) return (int)PrivateIndex::ACTIVE;
        return -1;

    case 's':
        if (strcmp(name, "size") == 0) return (int)PrivateIndex::SIZE;
        if (strcmp(name, "sizex") == 0) return (int)PrivateIndex::SIZEX;
        if (strcmp(name, "sizey") == 0) return (int)PrivateIndex::SIZEY;
        if (strcmp(name, "state") == 0) return (int)PrivateIndex::STATE;
        if (strcmp(name, "speed") == 0) return (int)PrivateIndex::SPEED;
        if (strcmp(name, "show") == 0) return (int)PrivateIndex::SHOW;
        return -1;

    case 'f':
        if (strcmp(name, "flags") == 0) return (int)PrivateIndex::FLAGS;
        if (strcmp(name, "father") == 0) return (int)PrivateIndex::FATHER;
        return -1;

    case 'i':
        return (strcmp(name, "id") == 0) ? (int)PrivateIndex::ID : -1;

    case 'r':
        return (strcmp(name, "red") == 0) ? (int)PrivateIndex::iRED : -1;

    case 'b':
        return (strcmp(name, "blue") == 0) ? (int)PrivateIndex::iBLUE : -1;

    case 't':
        return (strcmp(name, "tag") == 0) ? (int)PrivateIndex::TAG : -1;

    case 'v':
        if (strcmp(name, "velx") == 0) return (int)PrivateIndex::VELX;
        if (strcmp(name, "vely") == 0) return (int)PrivateIndex::VELY;
        return -1;

    case 'h':
        return (strcmp(name, "hp") == 0) ? (int)PrivateIndex::HP : -1;

    case 'p':
        return (strcmp(name, "progress") == 0) ? (int)PrivateIndex::PROGRESS : -1;

    case 'l':
        return (strcmp(name, "life") == 0) ? (int)PrivateIndex::LIFE : -1;
    }

    return -1;
}

ProcessDef *Interpreter::addProcess(const char *name, Function *func)
{
    String *pName = createString(name);
    ProcessDef *existing = nullptr;
    if (processesMap.get(pName, &existing))
    {

        return existing;
    }

    ProcessDef *proc = new ProcessDef();

    if (proc == nullptr)
    {
        runtimeError("Critical: Out of memory creating process!");
        return nullptr;
    }

    proc->name = pName;
    proc->index = processes.size();

    proc->privates[0] = makeDouble(0); // x
    proc->privates[1] = makeDouble(0); // y
    proc->privates[2] = makeInt(0);    // z
    proc->privates[3] = makeInt(-1);   // graph
    proc->privates[4] = makeInt(0);    // angle
    proc->privates[5] = makeInt(100);  // size
    proc->privates[6] = makeInt(0);    // flags
    proc->privates[7] = makeInt(-1);   // id
    proc->privates[8] = makeInt(-1);   // father
    proc->privates[9] = makeInt(255);  // red
    proc->privates[10] = makeInt(255); // green
    proc->privates[11] = makeInt(255); // blue
    proc->privates[12] = makeInt(255); // alpha
    proc->privates[13] = makeInt(0);   // tag
    proc->privates[14] = makeInt(0);   // state
    proc->privates[15] = makeDouble(0); // speed
    proc->privates[16] = makeInt(0);   // group
    proc->privates[17] = makeDouble(0); // velx
    proc->privates[18] = makeDouble(0); // vely
    proc->privates[19] = makeInt(0);    // hp
    proc->privates[20] = makeDouble(0); // progress
    proc->privates[21] = makeInt(100);  // life
    proc->privates[22] = makeInt(1);    // active
    proc->privates[23] = makeInt(1);    // show
    proc->privates[24] = makeInt(0);    // xold
    proc->privates[25] = makeInt(0);    // yold
    proc->privates[26] = makeDouble(1.0);    // sizex
    proc->privates[27] = makeDouble(1.0);    // sizex


    initFiber(proc, func);

    processesMap.set(pName, proc);
    processes.push(proc);
    return proc;
}

Process *Interpreter::spawnProcess(ProcessDef *blueprint)
{
    Process *instance = ProcessPool::instance().create();

    if (instance == nullptr)
    {
        runtimeError("Critical: Out of memory spawning process!");
        return nullptr;
    }

    instance->name = blueprint->name;
    instance->blueprint = blueprint->index;
    instance->id = PROCESS_IDS++;
    instance->state = ProcessState::RUNNING;
    instance->resumeTime = 0;
    instance->initialized = false;
    instance->exitCode = 0;

    // Clona privates
    for (int i = 0; i < MAX_PRIVATES; i++)
    {
        instance->privates[i] = blueprint->privates[i];
    }

    ProcessExec *srcFiber = blueprint;
    ProcessExec *dstFiber = instance;

    if (srcFiber->frameCount <= 0 || srcFiber->frames[0].func == nullptr)
    {
        runtimeError("Process blueprint has no executable fiber");
        ProcessPool::instance().recycle(instance);
        return nullptr;
    }

    if (srcFiber->state == ProcessState::DEAD)
    {
        dstFiber->state = ProcessState::DEAD;
        dstFiber->stackTop = dstFiber->stack;
        dstFiber->frameCount = 0;
        dstFiber->ip = nullptr;
        dstFiber->resumeTime = 0;
        dstFiber->gosubTop = 0;
        dstFiber->tryDepth = 0;
    }
    else
    {
        dstFiber->state = srcFiber->state;
        dstFiber->resumeTime = srcFiber->resumeTime;
        dstFiber->frameCount = srcFiber->frameCount;
        dstFiber->tryDepth = srcFiber->tryDepth;

        size_t stackSize = srcFiber->stackTop - srcFiber->stack;
        if (stackSize > 0)
        {
            memcpy(dstFiber->stack, srcFiber->stack, stackSize * sizeof(Value));
        }
        dstFiber->stackTop = dstFiber->stack + stackSize;

        dstFiber->gosubTop = srcFiber->gosubTop;
        if (srcFiber->gosubTop > 0)
        {
            memcpy(dstFiber->gosubStack, srcFiber->gosubStack,
                   srcFiber->gosubTop * sizeof(uint8 *));
        }

        for (int j = 0; j < srcFiber->frameCount; j++)
        {
            dstFiber->frames[j].func = srcFiber->frames[j].func;
            dstFiber->frames[j].closure = srcFiber->frames[j].closure;

            if (srcFiber->frames[j].ip != nullptr)
            {
                dstFiber->frames[j].ip = srcFiber->frames[j].ip;
            }
            else if (dstFiber->frames[j].func && dstFiber->frames[j].func->chunk)
            {
                dstFiber->frames[j].ip = dstFiber->frames[j].func->chunk->code;
            }
            else
            {
                dstFiber->frames[j].ip = nullptr;
            }

            ptrdiff_t offset = srcFiber->frames[j].slots - srcFiber->stack;
            dstFiber->frames[j].slots = dstFiber->stack + offset;
        }

        if (dstFiber->frameCount > 0)
        {
            dstFiber->ip = dstFiber->frames[dstFiber->frameCount - 1].ip;
        }
        else
        {
            dstFiber->ip = nullptr;
        }
    }

    aliveProcesses.push(instance);

    return instance;
}
uint32 Interpreter::getTotalProcesses() const
{
    return static_cast<uint32>(processes.size());
}

uint32 Interpreter::getTotalAliveProcesses() const
{
    return uint32(aliveProcesses.size());
}


void Interpreter::killAliveProcess()
{
    // if (aliveProcesses.size() == 1)
    //     return;

    for (size_t i = 0; i < aliveProcesses.size(); i++)
    {
        Process *proc = aliveProcesses[i];
        if (proc)
        {
            proc->state = ProcessState::DEAD;
        }
    }
    return;
}

Process *Interpreter::findProcessById(uint32 id)
{
    for (size_t i = 0; i < aliveProcesses.size(); i++)
    {
        Process *proc = aliveProcesses[i];
        if (proc && proc->id == id)
            return proc;
    }
    return nullptr;
}

void Interpreter::update(float deltaTime)
{
    // if(    asEnded)
    //     return;
    Process *savedCurrentProcess = currentProcess;
    const bool isReentrantUpdate = (savedCurrentProcess != nullptr);

    currentTime += deltaTime;
    lastFrameTime = deltaTime;
    frameCount++;

    size_t i = 0;
    while (i < aliveProcesses.size())
    {
        Process *proc = aliveProcesses[i];

        // If update() is called from inside a running process (e.g. ticks()),
        // never step that same process re-entrantly.
        if (isReentrantUpdate && proc == savedCurrentProcess)
        {
            i++;
            continue;
        }

        // Frozen? -> skip entirely
        if (proc->state == ProcessState::FROZEN)
        {
            i++;
            continue;
        }

        // Suspended?
        if (proc->state == ProcessState::SUSPENDED)
        {
            if (currentTime >= proc->resumeTime)
                proc->state = ProcessState::RUNNING;
            else
            {
                i++;
                continue;
            }
        }

        // Dead? -> remove da lista
        if (proc->state == ProcessState::DEAD)
        {
            // remove sem manter ordem
            //   Info(" Process (id=%u) is dead. Cleaning up. ",   proc->id);
            aliveProcesses[i] = aliveProcesses.back();
            cleanProcesses.push(proc);
            aliveProcesses.pop();
            continue;
        }

        currentProcess = proc;
        if (!currentProcess)
        {
            i++;
            continue;
        }

        run_process_step(proc);
        if (hooks.onUpdate)
            hooks.onUpdate(this,proc, deltaTime);

        i++;
    }

    for (size_t j = 0; j < cleanProcesses.size(); j++)
    {
        Process *proc = cleanProcesses[j];
        if (hooks.onDestroy)
            hooks.onDestroy(this,proc, proc->exitCode);

        if (currentProcess == proc)
        {
            currentProcess = nullptr;
        }

        ProcessPool::instance().recycle(proc);
    }
    cleanProcesses.clear();

    if (frameCount % 300 == 0)
    {
        size_t poolSize = ProcessPool::instance().size();
        
        if (poolSize > ProcessPool::MIN_POOL_SIZE * 2)
        {
            Info("Pool has %zu processes, shrinking...", poolSize);
            ProcessPool::instance().shrink();
        }
    }

    currentProcess = savedCurrentProcess;
}

void Interpreter::run_process_step(Process *proc)
{
    if (proc->state == ProcessState::DEAD)
    {
        return;
    }

    // Process e contexto de execução são o mesmo objeto.
    if (proc->state == ProcessState::SUSPENDED)
    {
        if (currentTime < proc->resumeTime)
        {
            return;
        }
        proc->state = ProcessState::RUNNING;
    }

    if (proc->state != ProcessState::RUNNING)
    {
        return;
    }

    currentProcess = proc;

    // Reset fatal error before each process step to prevent cascade
    hasFatalError_ = false;

    ProcessResult result = run_process(proc);

    if (proc->state == ProcessState::DEAD)
    {
        proc->initialized = false;
        return;
    }

    if (result.reason == ProcessResult::ERROR)
    {
        // Runtime error occurred - kill this process cleanly
        if (debugMode_)
        {
            Info("  Process '%s' (id=%u) killed due to runtime error",
                 proc->name ? proc->name->chars() : "?", proc->id);
        }
        proc->state = ProcessState::DEAD;
        proc->initialized = false;
        hasFatalError_ = false;
        return;
    }

    if (result.reason == ProcessResult::PROCESS_FRAME)
    {
        proc->state = ProcessState::SUSPENDED;
        proc->resumeTime = currentTime + (lastFrameTime * (result.framePercent - 100) / 100.0f);

        if (!proc->initialized)
        {
            proc->initialized = true;
             if (hooks.onStart)
                hooks.onStart(this,proc);
        }

        return;
    }

    if (result.reason == ProcessResult::PROCESS_DONE)
    {
        proc->state = ProcessState::DEAD;
        proc->initialized = false;
        return;
    }
}

void Interpreter::render()
{
    if (!hooks.onRender)
        return;
    for (size_t i = 0; i < aliveProcesses.size(); i++)
    {
        Process *proc = aliveProcesses[i];
        if (proc->state != ProcessState::DEAD && proc->initialized)
        {
            hooks.onRender(this,proc);
        }
    }
}
