#include "interpreter.hpp"

#ifdef BU_ENABLE_OS

#include <string>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#include <direct.h>
#define getcwd _getcwd
#define chdir _chdir
#else
#include <cerrno>
#include <csignal>
#include <unistd.h>
#include <sys/wait.h>
#endif

int native_os_execute(Interpreter *vm, int argCount, Value *args)
{
    if (!args[0].isString())
    {
        vm->push(vm->makeInt(-1));
        return 1;
    }

    int result = system(args[0].asStringChars());
    vm->push(vm->makeInt(result));
    return 1;
}

int native_os_getenv(Interpreter *vm, int argCount, Value *args)
{
    if (!args[0].isString())
        return 0;

    const char *value = getenv(args[0].asStringChars());
    if (value)
    {
        vm->push(vm->makeString(value));
        return 1;
    }
    return 0;
}

int native_os_setenv(Interpreter *vm, int argCount, Value *args)
{
    if (!args[0].isString() || !args[1].isString())
    {
        vm->push(vm->makeBool(false));
        return 1;
    }

#ifdef _WIN32
    vm->push(vm->makeBool(_putenv_s(args[0].asStringChars(),
                                    args[1].asStringChars()) == 0));
#else
    vm->push(vm->makeBool(setenv(args[0].asStringChars(),
                                 args[1].asStringChars(), 1) == 0));
#endif
    return 1;
}

int native_os_getcwd(Interpreter *vm, int argCount, Value *args)
{
    char buffer[4096];
    if (getcwd(buffer, sizeof(buffer)))
    {
        vm->push(vm->makeString(buffer));
        return 1;
    }
    return 0;
}

int native_os_chdir(Interpreter *vm, int argCount, Value *args)
{
    if (!args[0].isString())
    {
        vm->push(vm->makeBool(false));
        return 1;
    }

    vm->push(vm->makeBool(chdir(args[0].asStringChars()) == 0));
    return 1;
}

int native_os_exit(Interpreter *vm, int argCount, Value *args)
{
    int code = args[0].isInt() ? args[0].asInt() : 0;
    exit(code);
    return 0;
}

// ============================================
// OS.SPAWN - Versão Completa
// ============================================

int native_os_spawn(Interpreter *vm, int argCount, Value *args)
{
    if (argCount < 1 || !args[0].isString())
    {
        vm->runtimeError("os.spawn expects at least command string");
        vm->push(vm->makeInt(-1));
        return 1;
    }

    const char *command = args[0].asStringChars();

    if (!command || strlen(command) == 0)
    {
        vm->runtimeError("os.spawn: empty command");
        vm->push(vm->makeInt(-1));
        return 1;
    }

#ifdef _WIN32
    STARTUPINFOA si = {0};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi = {0};

    std::string cmdLine;
    cmdLine.reserve(1024);

    cmdLine += "\"";
    cmdLine += command;
    cmdLine += "\"";

    for (int i = 1; i < argCount; i++)
    {
        if (!args[i].isString())
            continue;

        const char *arg = args[i].asStringChars();
        cmdLine += " \"";

        for (const char *p = arg; *p; p++)
        {
            if (*p == '"')
                cmdLine += "\\\"";
            else
                cmdLine += *p;
        }

        cmdLine += "\"";
    }

    std::vector<char> cmdBuf(cmdLine.begin(), cmdLine.end());
    cmdBuf.push_back(0);

    BOOL success = CreateProcessA(
        NULL, cmdBuf.data(), NULL, NULL, FALSE,
        CREATE_NO_WINDOW, NULL, NULL, &si, &pi);

    if (!success)
    {
        DWORD error = GetLastError();
        vm->runtimeError("os.spawn failed: error code %lu", error);
        vm->push(vm->makeInt(-1));
        return 1;
    }

    DWORD pid = pi.dwProcessId;

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    vm->push(vm->makeInt((int)pid));
    return 1;

#else
    pid_t pid = fork();

    if (pid == -1)
    {
        vm->runtimeError("os.spawn: fork failed");
        vm->push(vm->makeInt(-1));
        return 1;
    }
    else if (pid == 0)
    {
        std::vector<char *> argv;
        argv.reserve(argCount + 1);

        argv.push_back((char *)command);

        for (int i = 1; i < argCount; i++)
        {
            if (args[i].isString())
                argv.push_back((char *)args[i].asStringChars());
        }

        argv.push_back(NULL);

        execvp(command, argv.data());

        perror("execvp");
        _exit(127);
    }
    else
    {
        vm->push(vm->makeInt((int)pid));
        return 1;
    }
#endif
}

// ============================================
// OS.SPAWN_SHELL - Executa via shell
// ============================================

int native_os_spawn_shell(Interpreter *vm, int argCount, Value *args)
{
    if (argCount < 1 || !args[0].isString())
    {
        vm->runtimeError("os.spawn_shell expects command string");
        vm->push(vm->makeInt(-1));
        return 1;
    }

    const char *command = args[0].asStringChars();

#ifdef _WIN32
    STARTUPINFOA si = {0};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi = {0};

    std::string cmdLine = "cmd.exe /C \"";
    cmdLine += command;
    cmdLine += "\"";

    std::vector<char> cmdBuf(cmdLine.begin(), cmdLine.end());
    cmdBuf.push_back(0);

    if (!CreateProcessA(NULL, cmdBuf.data(), NULL, NULL, FALSE,
                        CREATE_NO_WINDOW, NULL, NULL, &si, &pi))
    {
        vm->push(vm->makeInt(-1));
        return 1;
    }

    DWORD pid = pi.dwProcessId;
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    vm->push(vm->makeInt((int)pid));
    return 1;

#else
    pid_t pid = fork();

    if (pid == -1)
    {
        vm->push(vm->makeInt(-1));
        return 1;
    }
    else if (pid == 0)
    {
        execl("/bin/sh", "sh", "-c", command, (char *)NULL);
        _exit(127);
    }
    else
    {
        vm->push(vm->makeInt((int)pid));
        return 1;
    }
#endif
}

// ============================================
// OS.SPAWN_CAPTURE - Executa e captura output
// ============================================

int native_os_spawn_capture(Interpreter *vm, int argCount, Value *args)
{
    if (argCount < 1 || !args[0].isString())
    {
        vm->runtimeError("os.spawn_capture expects command");
        return 0;
    }

    const char *command = args[0].asStringChars();

#ifdef _WIN32
    FILE *pipe = _popen(command, "r");
    if (!pipe)
        
        {
            vm->push(vm->makeInt(-1));
            return 1;
        }

    std::string output;
    char buffer[4096];

    while (fgets(buffer, sizeof(buffer), pipe))
    {
        output += buffer;
    }

    int exitCode = _pclose(pipe);

    Value result = vm->makeMap();
    MapInstance *map = result.asMap();

    map->table.set(vm->makeString("output").asString(), vm->makeString(output.c_str()));
    map->table.set(vm->makeString("stdout").asString(), vm->makeString(output.c_str()));
    map->table.set(vm->makeString("code").asString(), vm->makeInt(exitCode));

    vm->push(result);
    return 1;

#else
    FILE *pipe = popen(command, "r");
    if (!pipe)
        {
            vm->push(vm->makeInt(-1));
            return 1;
        }

    std::string output;
    char buffer[4096];

    while (fgets(buffer, sizeof(buffer), pipe))
    {
        output += buffer;
    }

    int status = pclose(pipe);
    int exitCode = WIFEXITED(status) ? WEXITSTATUS(status) : -1;

    Value result = vm->makeMap();
    MapInstance *map = result.asMap();

    map->table.set(vm->makeString("output").asString(), vm->makeString(output.c_str()));
    map->table.set(vm->makeString("stdout").asString(), vm->makeString(output.c_str()));
    map->table.set(vm->makeString("code").asString(), vm->makeInt(exitCode));
    map->table.set(vm->makeString("status").asString(), vm->makeInt(status));

    vm->push(result);
    return 1;
#endif
}

// ============================================
// OS.KILL - Termina processo
// ============================================

int native_os_kill(Interpreter *vm, int argCount, Value *args)
{
    if (argCount < 1 || !args[0].isInt())
    {
        vm->runtimeError("os.kill expects process ID");
        vm->push(vm->makeBool(false));
        return 1;
    }

    int pid = args[0].asInt();
    if (pid <= 0)
    {
        vm->push(vm->makeBool(false));
        return 1;
    }

#ifdef _WIN32
    int exitCode = 1;
    if (argCount >= 2 && args[1].isInt())
    {
        exitCode = args[1].asInt();
    }

    HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
    if (!hProcess)
    {
        vm->push(vm->makeBool(false));
        return 1;
    }

    BOOL result = TerminateProcess(hProcess, (UINT)exitCode);
    CloseHandle(hProcess);

    vm->push(vm->makeBool(result != 0));
    return 1;

#else
    int sig = SIGTERM;
    if (argCount >= 2 && args[1].isInt())
    {
        sig = args[1].asInt();
    }
    vm->push(vm->makeBool(kill(pid, sig) == 0));
    return 1;
#endif
}

static int os_status_to_exit_code(int status)
{
#ifdef _WIN32
    (void)status;
    return -1;
#else
    if (WIFEXITED(status))
    {
        return WEXITSTATUS(status);
    }
    if (WIFSIGNALED(status))
    {
        return -WTERMSIG(status);
    }
    return -1;
#endif
}

int native_os_wait(Interpreter *vm, int argCount, Value *args)
{
    if (argCount < 1 || !args[0].isInt())
    {
        vm->push(vm->makeInt(-1));
        return 1;
    }

    int pid = args[0].asInt();
    if (pid <= 0)
    {
        vm->push(vm->makeInt(-1));
        return 1;
    }

    int timeoutMs = -1;
    if (argCount >= 2)
    {
        if (args[1].isInt())
        {
            timeoutMs = args[1].asInt();
        }
        else if (args[1].isDouble())
        {
            timeoutMs = (int)args[1].asDouble();
        }
    }

#ifdef _WIN32
    HANDLE hProcess = OpenProcess(SYNCHRONIZE | PROCESS_QUERY_INFORMATION,
                                  FALSE, pid);
    if (!hProcess)
    {
        vm->push(vm->makeInt(-1));
        return 1;
    }

    DWORD waitTimeout = (timeoutMs < 0) ? INFINITE : (DWORD)timeoutMs;
    DWORD waitResult = WaitForSingleObject(hProcess, waitTimeout);
    if (waitResult == WAIT_TIMEOUT)
    {
        CloseHandle(hProcess);
        vm->pushNil();
        return 1;
    }
    if (waitResult != WAIT_OBJECT_0)
    {
        CloseHandle(hProcess);
        vm->push(vm->makeInt(-1));
        return 1;
    }

    DWORD exitCode;
    GetExitCodeProcess(hProcess, &exitCode);
    CloseHandle(hProcess);

    vm->push(vm->makeInt((int)exitCode));
    return 1;
#else
    int status = 0;
    if (timeoutMs < 0)
    {
        if (waitpid(pid, &status, 0) == -1)
        {
            vm->push(vm->makeInt(-1));
            return 1;
        }
    }
    else
    {
        int elapsedMs = 0;
        const int stepMs = 10;
        while (true)
        {
            pid_t ret = waitpid(pid, &status, WNOHANG);
            if (ret == pid)
            {
                break;
            }
            if (ret == -1)
            {
                vm->push(vm->makeInt(-1));
                return 1;
            }
            if (elapsedMs >= timeoutMs)
            {
                vm->pushNil();
                return 1;
            }
            usleep(stepMs * 1000);
            elapsedMs += stepMs;
        }
    }

    vm->push(vm->makeInt(os_status_to_exit_code(status)));
    return 1;
#endif
}

int native_os_poll(Interpreter *vm, int argCount, Value *args)
{
    if (argCount < 1 || !args[0].isInt())
    {
        vm->push(vm->makeInt(-1));
        return 1;
    }

    int pid = args[0].asInt();
    if (pid <= 0)
    {
        vm->push(vm->makeInt(-1));
        return 1;
    }

#ifdef _WIN32
    HANDLE hProcess = OpenProcess(SYNCHRONIZE | PROCESS_QUERY_INFORMATION,
                                  FALSE, pid);
    if (!hProcess)
    {
        vm->push(vm->makeInt(-1));
        return 1;
    }

    DWORD exitCode = 0;
    if (!GetExitCodeProcess(hProcess, &exitCode))
    {
        CloseHandle(hProcess);
        vm->push(vm->makeInt(-1));
        return 1;
    }
    CloseHandle(hProcess);

    if (exitCode == STILL_ACTIVE)
    {
        vm->pushNil();
        return 1;
    }
    vm->push(vm->makeInt((int)exitCode));
    return 1;
#else
    int status = 0;
    pid_t ret = waitpid(pid, &status, WNOHANG);
    if (ret == 0)
    {
        vm->pushNil();
        return 1;
    }
    if (ret == -1)
    {
        vm->push(vm->makeInt(-1));
        return 1;
    }
    vm->push(vm->makeInt(os_status_to_exit_code(status)));
    return 1;
#endif
}

int native_os_is_alive(Interpreter *vm, int argCount, Value *args)
{
    if (argCount < 1 || !args[0].isInt())
    {
        vm->push(vm->makeBool(false));
        return 1;
    }

    int pid = args[0].asInt();
    if (pid <= 0)
    {
        vm->push(vm->makeBool(false));
        return 1;
    }

#ifdef _WIN32
    HANDLE hProcess = OpenProcess(SYNCHRONIZE | PROCESS_QUERY_INFORMATION,
                                  FALSE, pid);
    if (!hProcess)
    {
        vm->push(vm->makeBool(false));
        return 1;
    }

    DWORD exitCode = 0;
    BOOL ok = GetExitCodeProcess(hProcess, &exitCode);
    CloseHandle(hProcess);
    vm->push(vm->makeBool(ok && exitCode == STILL_ACTIVE));
    return 1;
#else
    int result = kill(pid, 0);
    if (result == 0 || errno == EPERM)
    {
        vm->push(vm->makeBool(true));
        return 1;
    }
    vm->push(vm->makeBool(false));
    return 1;
#endif
}

void Interpreter::registerOS()
{
#ifdef _WIN32
    const char *platform = "windows";
#elif defined(__ANDROID__)
    const char *platform = "android";
#elif defined(__EMSCRIPTEN__)
    const char *platform = "emscripten";
#elif defined(__APPLE__)
    const char *platform = "macos";
#elif defined(__linux__)
    const char *platform = "linux";
#elif defined(__unix__)
    const char *platform = "unix";
#else
    const char *platform = "unknown";
#endif

    addModule("os")
        .addString("platform", platform)
        .addFunction("spawn", native_os_spawn, -1)                // Spawn processo
        .addFunction("spawn_shell", native_os_spawn_shell, 1)     // Via shell
        .addFunction("spawn_capture", native_os_spawn_capture, 1) // Captura output
        .addFunction("wait", native_os_wait, -1)                  // Espera processo (timeout opcional)
        .addFunction("poll", native_os_poll, 1)                   // Poll sem bloquear
        .addFunction("is_alive", native_os_is_alive, 1)           // Processo ainda vivo?
        .addFunction("kill", native_os_kill, -1)                  // Termina processo (signal opcional)
        .addFunction("execute", native_os_execute, 1)
        .addFunction("getenv", native_os_getenv, 1)
        .addFunction("setenv", native_os_setenv, 2)
        .addFunction("getcwd", native_os_getcwd, 0)
        .addFunction("chdir", native_os_chdir, 1)
        .addFunction("quit", native_os_exit, 1);
}

#endif
