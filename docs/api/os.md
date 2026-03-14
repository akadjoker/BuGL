# OS Module

```bulang
import os;
```

## Platform

```bulang
print(os.platform);   // "linux", "windows", "macos"
```

## Running External Processes

```bulang
// Fire and forget
var pid = os.spawn("convert", "input.png", "output.jpg");

// Via shell
os.spawn_shell("ls -la");

// Capture output
var result = os.spawn_capture("git log --oneline -5");
print(result["output"]);
print(result["code"]);

// Wait for process
var code = os.wait(pid);
```

## Process Control

| Function | Description |
|----------|-------------|
| `os.spawn(cmd, args...)` | Start process, returns PID |
| `os.spawn_shell(cmd)` | Start via shell, returns PID |
| `os.spawn_capture(cmd)` | Capture stdout, returns Map `{output, code}` |
| `os.wait(pid)` | Wait for process, returns exit code |
| `os.poll(pid)` | Check status (exit code or nil if running) |
| `os.is_alive(pid)` | True if process is running |
| `os.kill(pid)` | Terminate process |
| `os.execute(cmd)` | system() call, returns exit code |

## Environment & Filesystem

```bulang
var path = os.getenv("PATH");
os.setenv("DEBUG", "1");

var cwd = os.getcwd();
os.chdir("/tmp");
```

| Function | Description |
|----------|-------------|
| `os.getenv(name)` | Get environment variable |
| `os.setenv(name, val)` | Set environment variable |
| `os.getcwd()` | Get current directory |
| `os.chdir(path)` | Change directory |
| `os.quit(code)` | Exit BuGL application |
