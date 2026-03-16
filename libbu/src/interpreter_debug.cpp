#include "interpreter.hpp"
#include "compiler.hpp"

#include <algorithm>
#include <cstdio>
#include <cstring>

namespace
{
static const char *debugFunctionName(const Function *func)
{
    if (!func || !func->name)
        return "<script>";

    const char *rawName = func->name->chars();
    const char *separator = std::strrchr(rawName, '$');
    if (separator && separator[1] != '\0')
        return separator + 1;
    return rawName;
}

static bool fillDebugLocation(const ProcessExec *exec, const CallFrame *frame, DebugLocation *out)
{
    if (!exec || !frame || !out || !frame->func || !frame->func->chunk || !frame->func->chunk->code || !frame->ip)
        return false;

    ptrdiff_t instruction = frame->ip - frame->func->chunk->code;
    if (instruction < 0)
        return false;
    if (instruction > 0)
        instruction--;
    if (instruction >= static_cast<ptrdiff_t>(frame->func->chunk->count))
        instruction = static_cast<ptrdiff_t>(frame->func->chunk->count) - 1;
    if (instruction < 0)
        return false;

    out->functionName = debugFunctionName(frame->func);
    out->instruction = static_cast<int>(instruction);
    out->line = frame->func->chunk->lines[static_cast<size_t>(instruction)];
    return true;
}

static bool fillDebugFrameRange(const ProcessExec *exec,
                                int frameIndexFromTop,
                                const CallFrame **outFrame,
                                Value **outEnd)
{
    if (!exec || exec->frameCount <= 0 || frameIndexFromTop < 0 || frameIndexFromTop >= exec->frameCount)
        return false;

    const int frameIndex = exec->frameCount - 1 - frameIndexFromTop;
    const CallFrame *frame = &exec->frames[frameIndex];
    if (!frame->slots)
        return false;

    Value *frameEnd = exec->stackTop;
    if (frameIndex + 1 < exec->frameCount)
        frameEnd = exec->frames[frameIndex + 1].slots;
    if (!frameEnd || frameEnd < frame->slots)
        return false;

    if (outFrame)
        *outFrame = frame;
    if (outEnd)
        *outEnd = frameEnd;
    return true;
}
} // namespace

void Interpreter::enableDebugger(bool enabled)
{
    debuggerEnabled_ = enabled;
    debuggerPaused_ = false;
    debuggerStepMode_ = false;
    debuggerPauseRequested_ = false;
    debuggerPauseState_ = {};
    debuggerStepStart_ = {};
    debuggerSkipOnce_ = {};
    if (!enabled)
        clearDebugBreakpoints();
}

void Interpreter::addDebugBreakpoint(int line)
{
    if (line <= 0)
        return;
    if (std::find(debugBreakpoints_.begin(), debugBreakpoints_.end(), line) == debugBreakpoints_.end())
        debugBreakpoints_.push(line);
}

void Interpreter::removeDebugBreakpoint(int line)
{
    for (size_t i = 0; i < debugBreakpoints_.size(); ++i)
    {
        if (debugBreakpoints_[i] != line)
            continue;
        for (size_t j = i + 1; j < debugBreakpoints_.size(); ++j)
            debugBreakpoints_[j - 1] = debugBreakpoints_[j];
        debugBreakpoints_.pop();
        return;
    }
}

void Interpreter::clearDebugBreakpoints()
{
    debugBreakpoints_.clear();
}

bool Interpreter::hasDebugBreakpoint(int line) const
{
    if (line <= 0)
        return false;
    for (size_t i = 0; i < debugBreakpoints_.size(); ++i)
    {
        if (debugBreakpoints_[i] == line)
            return true;
    }
    return false;
}

void Interpreter::requestDebugPause()
{
    if (!debuggerEnabled_)
        return;
    debuggerPauseRequested_ = true;
}

void Interpreter::continueDebug()
{
    if (!debuggerPaused_)
        return;

    debuggerSkipOnce_ = debuggerPauseState_.location;
    if (debuggerPauseState_.reason == DebugPauseReason::BREAKPOINT)
        debuggerSkipOnce_.instruction = -1;
    debuggerPaused_ = false;
    debuggerPauseRequested_ = false;
    debuggerStepMode_ = false;
    debuggerPauseState_ = {};
}

void Interpreter::stepDebug()
{
    if (!debuggerPaused_)
    {
        debuggerStepMode_ = true;
        debuggerPauseRequested_ = false;
        return;
    }

    debuggerSkipOnce_ = debuggerPauseState_.location;
    debuggerStepStart_ = debuggerPauseState_.location;
    debuggerPaused_ = false;
    debuggerPauseRequested_ = false;
    debuggerStepMode_ = true;
    debuggerPauseState_ = {};
}

bool Interpreter::prepareSourceRun(const char *source, bool dump)
{
#if BU_RUNTIME_ONLY
    (void)source;
    (void)dump;
    safetimeError("prepareSourceRun: source execution disabled in runtime-only build");
    return false;
#else
    reset();

    ProcessDef *proc = compiler->compile(source);
    if (!proc)
        return false;

    const auto &compilerMapping = compiler->getGlobalIndexToName();
    globalIndexToName_.clear();
    globalIndexToName_.reserve(compilerMapping.size());
    for (const auto &name : compilerMapping)
    {
        String *str = createString(name.c_str());
        globalIndexToName_.push(str);
    }

    if (globalsArray.size() < globalIndexToName_.size())
        ensureGlobalsArraySize(globalIndexToName_.size());

    if (dump)
        disassemble();

    mainProcess = spawnProcess(proc);
    if (!mainProcess)
        return false;

    currentProcess = mainProcess;
    return !hasFatalError_;
#endif
}

bool Interpreter::continueExecution()
{
    if (!mainProcess || mainProcess->state == ProcessState::DEAD)
        return !hasFatalError_;

    currentProcess = mainProcess;
    ProcessResult result = run_process(mainProcess);
    if (result.reason == ProcessResult::DEBUG_BREAK)
    {
        mainProcess->state = ProcessState::SUSPENDED;
        return true;
    }
    return !hasFatalError_;
}

bool Interpreter::isMainProcessFinished() const
{
    return !mainProcess || mainProcess->state == ProcessState::DEAD;
}

bool Interpreter::debugShouldPauseBeforeInstruction(ProcessExec *fiber, CallFrame *frame, uint8 *ip)
{
    if (!debuggerEnabled_ || debuggerPaused_ || !fiber || !frame || !ip)
        return false;

    DebugLocation location;
    if (!fillDebugLocation(fiber, frame, &location))
        return false;

    bool isSkippedInstruction = false;
    if (debuggerSkipOnce_.line > 0 && debuggerSkipOnce_.functionName == location.functionName)
    {
        if (debuggerSkipOnce_.instruction < 0)
            isSkippedInstruction = debuggerSkipOnce_.line == location.line;
        else
            isSkippedInstruction =
                debuggerSkipOnce_.line == location.line &&
                debuggerSkipOnce_.instruction == location.instruction;
    }

    if (isSkippedInstruction)
        return false;

    if (debuggerSkipOnce_.line > 0)
        debuggerSkipOnce_ = {};

    DebugPauseReason reason = DebugPauseReason::NONE;
    if (debuggerPauseRequested_)
    {
        reason = DebugPauseReason::MANUAL;
        debuggerPauseRequested_ = false;
    }
    else if (debuggerStepMode_)
    {
        const bool sameLocation =
            debuggerStepStart_.instruction >= 0 &&
            debuggerStepStart_.line == location.line &&
            debuggerStepStart_.functionName == location.functionName;

        if (!sameLocation)
        {
            reason = DebugPauseReason::STEP;
            debuggerStepMode_ = false;
        }
    }
    else if (hasDebugBreakpoint(location.line))
    {
        reason = DebugPauseReason::BREAKPOINT;
    }

    if (reason == DebugPauseReason::NONE)
        return false;

    debuggerPaused_ = true;
    debuggerPauseState_.reason = reason;
    debuggerPauseState_.location = location;
    return true;
}

bool Interpreter::getCurrentLocation(DebugLocation *out) const
{
    if (!out)
        return false;

    *out = {};

    const ProcessExec *exec = currentExec();
    if (!exec || exec->frameCount <= 0)
        return false;

    const CallFrame *frame = &exec->frames[exec->frameCount - 1];
    return fillDebugLocation(exec, frame, out);
}

int Interpreter::getCallFrameCount() const
{
    const ProcessExec *exec = currentExec();
    if (!exec)
        return 0;
    return exec->frameCount;
}

bool Interpreter::getCallFrameInfo(int frameIndexFromTop, DebugFrameInfo *out) const
{
    if (!out)
        return false;

    *out = {};

    const ProcessExec *exec = currentExec();
    if (!exec || exec->frameCount <= 0 || frameIndexFromTop < 0 || frameIndexFromTop >= exec->frameCount)
        return false;

    const CallFrame *frame = nullptr;
    Value *frameEnd = nullptr;
    if (!fillDebugFrameRange(exec, frameIndexFromTop, &frame, &frameEnd))
        return false;

    DebugLocation location;
    if (!fillDebugLocation(exec, frame, &location))
        return false;

    out->functionName = location.functionName;
    out->line = location.line;
    out->instruction = location.instruction;

    out->slotCount = static_cast<int>(frameEnd - frame->slots);

    return true;
}

bool Interpreter::getFrameSlotValue(int frameIndexFromTop, int slotIndex, Value *out) const
{
    if (!out)
        return false;

    *out = Value{};

    if (slotIndex < 0)
        return false;

    const ProcessExec *exec = currentExec();
    if (!exec)
        return false;

    const CallFrame *frame = nullptr;
    Value *frameEnd = nullptr;
    if (!fillDebugFrameRange(exec, frameIndexFromTop, &frame, &frameEnd))
        return false;

    const int slotCount = static_cast<int>(frameEnd - frame->slots);
    if (slotIndex >= slotCount)
        return false;

    *out = frame->slots[slotIndex];
    return true;
}

int Interpreter::getGlobalCount() const
{
    return static_cast<int>(globalIndexToName_.size());
}

bool Interpreter::getGlobalInfo(int index, DebugGlobalInfo *out) const
{
    if (!out)
        return false;

    *out = {};

    if (index < 0)
        return false;

    const size_t idx = static_cast<size_t>(index);
    if (idx >= globalIndexToName_.size() || idx >= globalsArray.size())
        return false;
    if (!globalIndexToName_[idx])
        return false;

    out->name = globalIndexToName_[idx]->chars();
    out->value = globalsArray[idx];
    return true;
}

void Interpreter::dumpCallFrames(FILE *output) const
{
    FILE *out = output ? output : stdout;
    const ProcessExec *exec = currentExec();
    if (!exec || exec->frameCount <= 0)
    {
        std::fprintf(out, "(no call frames)\n");
        return;
    }

    std::fprintf(out, "Call frames (%d):\n", exec->frameCount);
    for (int i = 0; i < exec->frameCount; ++i)
    {
        DebugFrameInfo info;
        if (!getCallFrameInfo(i, &info))
            continue;

        std::fprintf(out,
                     "  [%d] %s line=%d instruction=%d slots=%d\n",
                     i,
                     info.functionName ? info.functionName : "<script>",
                     info.line,
                     info.instruction,
                     info.slotCount);
    }
}

void Interpreter::dumpFrameLocals(int frameIndexFromTop, FILE *output) const
{
    FILE *out = output ? output : stdout;
    DebugFrameInfo info;
    if (!getCallFrameInfo(frameIndexFromTop, &info))
    {
        std::fprintf(out, "(no frame locals)\n");
        return;
    }

    std::fprintf(out,
                 "Locals frame=%d function=%s line=%d:\n",
                 frameIndexFromTop,
                 info.functionName ? info.functionName : "<script>",
                 info.line);

    for (int slotIndex = 0; slotIndex < info.slotCount; ++slotIndex)
    {
        Value value;
        if (!getFrameSlotValue(frameIndexFromTop, slotIndex, &value))
            continue;

        std::fprintf(out, "  $%d = ", slotIndex);
        printValue(value);
        std::fprintf(out, "\n");
    }
}

void Interpreter::dumpGlobals(FILE *output) const
{
    FILE *out = output ? output : stdout;
    const int count = getGlobalCount();

    std::fprintf(out, "Globals:\n");
    for (int i = 0; i < count; ++i)
    {
        DebugGlobalInfo info;
        if (!getGlobalInfo(i, &info))
            continue;

        std::fprintf(out, "  %s = ", info.name ? info.name : "<unnamed>");
        printValue(info.value);
        std::fprintf(out, "\n");
    }
}
