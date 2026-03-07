#include "interpreter.hpp"
#include "compiler.hpp"
#include "debug.hpp"
#include "platform.hpp"
#include "utils.hpp"
#include <stdarg.h>

#ifndef BU_RUNTIME_ONLY
#define BU_RUNTIME_ONLY 0
#endif

Interpreter::Interpreter()
{
#if BU_RUNTIME_ONLY
  compiler = nullptr;
#else
  compiler = new Compiler(this);
#endif
  debugMode_ = false;
  hasFatalError_ = false;

  setPrivateTable();
  staticNames.resize((int)StaticNames::TOTAL_COUNT);
  staticNames[(int)StaticNames::PUSH] = createString("push");
  staticNames[(int)StaticNames::POP] = createString("pop");
  staticNames[(int)StaticNames::BACK] = createString("back");
  staticNames[(int)StaticNames::LENGTH] = createString("length");
  staticNames[(int)StaticNames::CLEAR] = createString("clear");
  staticNames[(int)StaticNames::REMOVE] = createString("remove");
  staticNames[(int)StaticNames::INSERT] = createString("insert");
  staticNames[(int)StaticNames::FIND] = createString("find");
  staticNames[(int)StaticNames::CONTAINS] = createString("contains");
  staticNames[(int)StaticNames::REVERSE] = createString("reverse");
  staticNames[(int)StaticNames::SLICE] = createString("slice");
  staticNames[(int)StaticNames::CONCAT] = createString("concat");
  staticNames[(int)StaticNames::JOIN] = createString("join");
  staticNames[(int)StaticNames::FIRST] = createString("first");
  staticNames[(int)StaticNames::LAST] = createString("last");
  staticNames[(int)StaticNames::FILL] = createString("fill");

  staticNames[(int)StaticNames::HAS] = createString("has");
  staticNames[(int)StaticNames::REMOVE] = createString("remove");
  staticNames[(int)StaticNames::KEYS] = createString("keys");
  staticNames[(int)StaticNames::VALUES] = createString("values");

  staticNames[(int)StaticNames::UPPER] = createString("upper");
  staticNames[(int)StaticNames::LOWER] = createString("lower");
  staticNames[(int)StaticNames::CONCAT] = createString("concat");
  staticNames[(int)StaticNames::SUB] = createString("sub");
  staticNames[(int)StaticNames::REPLACE] = createString("replace");
  staticNames[(int)StaticNames::AT] = createString("at");
  staticNames[(int)StaticNames::CONTAINS] = createString("contains");
  staticNames[(int)StaticNames::TRIM] = createString("trim");
  staticNames[(int)StaticNames::STARTWITH] = createString("startswith");
  staticNames[(int)StaticNames::ENDWITH] = createString("endswith");
  staticNames[(int)StaticNames::INDEXOF] = createString("indexof");
  staticNames[(int)StaticNames::REPEAT] = createString("repeat");
  staticNames[(int)StaticNames::SPLIT] = createString("split");

  staticNames[(int)StaticNames::INIT] = createString("init");

  staticNames[(int)StaticNames::FILL] = createString("fill");
  staticNames[(int)StaticNames::COPY] = createString("copy");
  staticNames[(int)StaticNames::SLICE] = createString("slice");
  staticNames[(int)StaticNames::SAVE] = createString("save");

  staticNames[(int)StaticNames::WRITE_BYTE] = createString("writeByte");
  staticNames[(int)StaticNames::READ_BYTE] = createString("readByte");
  staticNames[(int)StaticNames::WRITE_SHORT] = createString("writeShort");
  staticNames[(int)StaticNames::READ_SHORT] = createString("readShort");
  staticNames[(int)StaticNames::WRITE_INT] = createString("writeInt");
  staticNames[(int)StaticNames::READ_INT] = createString("readInt");
  staticNames[(int)StaticNames::WRITE_FLOAT] = createString("writeFloat");
  staticNames[(int)StaticNames::READ_FLOAT] = createString("readFloat");
  staticNames[(int)StaticNames::WRITE_DOUBLE] = createString("writeDouble");
  staticNames[(int)StaticNames::READ_DOUBLE] = createString("readDouble");
  staticNames[(int)StaticNames::WRITE_USHORT] = createString("writeUShort");
  staticNames[(int)StaticNames::READ_USHORT] = createString("readUShort");
  staticNames[(int)StaticNames::WRITE_UINT] = createString("writeUInt");
  staticNames[(int)StaticNames::READ_UINT] = createString("readUInt");
  staticNames[(int)StaticNames::WRITE_STRING] = createString("writeString");
  staticNames[(int)StaticNames::READ_STRING] = createString("readString");
  staticNames[(int)StaticNames::SEEK] = createString("seek");
  staticNames[(int)StaticNames::TELL] = createString("tell");
  staticNames[(int)StaticNames::REWIND] = createString("rewind");
  staticNames[(int)StaticNames::SKIP] = createString("skip");
  staticNames[(int)StaticNames::REMAINING] = createString("remaining");

  // OPTIMIZATION: Removed HashMap globals - using globalsArray directly
  // globals.set(createString("TYPE_UINT8"), makeInt(0));
  // globals.set(createString("TYPE_INT16"), makeInt(1));
  // globals.set(createString("TYPE_UINT16"), makeInt(2));
  // globals.set(createString("TYPE_INT32"), makeInt(3));
  // globals.set(createString("TYPE_UINT32"), makeInt(4));
  // globals.set(createString("TYPE_FLOAT"), makeInt(5));
  // globals.set(createString("TYPE_DOUBLE"), makeInt(6));
}

void Interpreter::freeInstances()
{
}

void Interpreter::freeFunctions()
{
  for (size_t i = 0; i < functions.size(); i++)
  {
    delete functions[i];
  }
  functions.clear();
  for (size_t i = 0; i < functionsClass.size(); i++)
  {
    delete functionsClass[i];
  }
  functionsClass.clear();
  functionsMap.destroy();
}

void Interpreter::freeRunningProcesses()
{
  for (size_t j = 0; j < cleanProcesses.size(); j++)
  {
    if (hooks.onDestroy)
    {
     // hooks.onDestroy(cleanProcesses[j], cleanProcesses[j]->exitCode);
    }
    ProcessPool::instance().destroy(cleanProcesses[j]);
  }
  cleanProcesses.clear();
  for (size_t i = 0; i < aliveProcesses.size(); i++)
  {
    if (hooks.onDestroy)
    {
      //hooks.onDestroy(aliveProcesses[i], aliveProcesses[i]->exitCode);
    }
    ProcessPool::instance().destroy(aliveProcesses[i]);
  }
  aliveProcesses.clear();
  ProcessPool::instance().clear();
  processesMap.destroy();
}

void Interpreter::freeBlueprints()
{
  // Classes
  for (size_t j = 0; j < classes.size(); j++)
  {
    delete classes[j];
  }
  classes.clear();

  // Native Structs
  for (size_t i = 0; i < nativeStructs.size(); i++)
  {
    delete nativeStructs[i];
  }
  nativeStructs.clear();

  natives.clear();

  // Structs
  for (size_t i = 0; i < structs.size(); i++)
  {
    structs[i]->names.destroy();
    delete structs[i];
  }
  structs.clear();

  // Native Classes
  for (size_t j = 0; j < nativeClasses.size(); j++)
  {
    delete nativeClasses[j];
  }
  nativeClasses.clear();

  // Process Definitions (Blueprints)
  for (size_t j = 0; j < processes.size(); j++)
  {
    processes[j]->release();
    delete processes[j];
  }
  processes.clear();

  // === LIMPEZA DE MAPAS ===
  
  structsMap.destroy();
  classesMap.destroy();
  nativeClassesMap.destroy();
  // globals.destroy();  // OPTIMIZATION: HashMap globals removed
}
void Interpreter::reset()
{
  openUpvalues = nullptr;
  // 1. Limpa processos em execução (RAM e Fibers)
  freeRunningProcesses();

  // 2. Limpa código compilado (Bytecode das funções)
  freeFunctions();

  // 2.1 Limpa classes/structs do script (evita ponteiros pendurados)
  for (size_t j = 0; j < classes.size(); j++)
  {
    delete classes[j];
  }
  classes.clear();
  classesMap.destroy();

  for (size_t i = 0; i < structs.size(); i++)
  {
    structs[i]->names.destroy();
    delete structs[i];
  }
  structs.clear();
  structsMap.destroy();

  clearAllGCObjects();

  gcObjects = nullptr;
  totalAllocated = 0;
  totalArrays = 0;
  totalStructs = 0;
  totalClasses = 0;
  totalMaps = 0;
  totalNativeClasses = 0;
  totalNativeStructs = 0;
  nextGC = 1024 * 4;
  gcInProgress = false;

  frameCount = 0;

  // 3. Limpa blueprints de processos (gerados pelo script anterior)
  // Nota: Mantivemos este loop aqui pois reset() pode não querer limpar
  // todas as Classes/Structs se forem partilhadas, mas os ProcessDefs do script sim.
  for (size_t j = 0; j < processes.size(); j++)
  {
    ProcessDef *proc = processes[j];
    proc->release();
    delete proc;
  }
  processes.clear();

  // 4. Reset de variáveis de estado
  currentProcess = nullptr;
  currentTime = 0.0f;
  hasFatalError_ = false;

#if !BU_RUNTIME_ONLY
  if (compiler)
  {
    compiler->clear();
  }
#endif
}

Interpreter::~Interpreter()
{
  dumpToFile("main.dump");
  Info("VM shutdown");
  Info("Memory allocated : %s", formatBytes(totalAllocated));
  Info("Classes          : %zu", getTotalClasses());
  Info("Structs          : %zu", getTotalStructs());
  Info("Arrays           : %zu", getTotalArrays());
  Info("Maps             : %zu", getTotalMaps());
  Info("Native classes   : %zu", getTotalNativeClasses());
  Info("Native structs   : %zu", getTotalNativeStructs());
  Info("Buffers          : %zu", totalBuffers);
  Info("Processes        : %zu", aliveProcesses.size());
  Info("Globals          : %zu", globalsArray.size());
  
  unloadAllPlugins();
  for (size_t i = 0; i < modules.size(); i++)
  {
    ModuleDef *mod = modules[i];
    delete mod;
  }

  modules.clear();
#if !BU_RUNTIME_ONLY
  if (compiler)
  {
    delete compiler;
    compiler = nullptr;
  }
#endif

  freeInstances();
  freeRunningProcesses();
  freeFunctions();
  // globals.destroy();  // OPTIMIZATION: HashMap globals removed
  clearAllGCObjects();  // Must be called before freeBlueprints() so native destructors can access ClassDef/NativeClassDef
  freeBlueprints();

  openUpvalues = nullptr;
  // Info("Heap stats:");
  // arena.Stats();
  arena.Clear();
  // Info("String Heap stats:");
  stringPool.clear();
}

BufferInstance *Interpreter::createBuffer(int count, int typeRaw)
{
  checkGC();
  size_t size = sizeof(BufferInstance);
  void *mem = (BufferInstance *)arena.Allocate(size);

  BufferInstance *instance = new (mem) BufferInstance(count, (BufferType)typeRaw);
  instance->marked = 0;

  instance->next = gcObjects;
  gcObjects = instance;
  totalBuffers++;

  totalAllocated += size;
  totalAllocated += (count * instance->elementSize); // Conta também os dados raw!

  return instance;
}

void Interpreter::freeBuffer(BufferInstance *b)
{
  size_t size = sizeof(BufferInstance);
  size_t dataSize = b->count * b->elementSize;

  b->~BufferInstance();
  arena.Free(b, size);

  totalBuffers--;

  totalAllocated -= (size + dataSize);
}

void Interpreter::setFileLoader(FileLoaderCallback loader, void *userdata)
{
  fileLoaderCallback_ = loader;
  fileLoaderUserdata_ = userdata;
#if !BU_RUNTIME_ONLY
  if (compiler)
  {
    compiler->setFileLoader(loader, userdata);
  }
#endif
}

NativeClassDef *Interpreter::registerNativeClass(const char *name,
                                                 NativeConstructor ctor,
                                                 NativeDestructor dtor,
                                                 int argCount,
                                                 bool persistent)
{
  NativeClassDef *klass = new NativeClassDef();
  klass->name = createString(name);
  int id = nativeClasses.size();
  klass->index = id;
  klass->constructor = ctor;
  klass->destructor = dtor;
  klass->argCount = argCount;
  klass->persistent = persistent;

  // Adiciona ao vector para lookup por id
  nativeClasses.push(klass);

  // Adiciona ao mapa para lookup por nome
  nativeClassesMap.set(klass->name, klass);

  // OPTIMIZATION: Removed HashMap globals - using globalsArray directly
  // globals.set(klass->name, makeNativeClass(id));

  // OPTIMIZATION: Also add to globalsArray for direct indexed access
  uint16 globalIndex = globalsArray.size();
  globalsArray.push(makeNativeClass(id));
  nativeGlobalIndices.set(klass->name, globalIndex);

  return klass;
}

void Interpreter::addNativeMethod(NativeClassDef *klass, const char *methodName,
                                  NativeMethod method)
{
  String *name = createString(methodName);
  klass->methods.set(name, method);
}

void Interpreter::addNativeProperty(NativeClassDef *klass, const char *propName,
                                    NativeGetter getter, NativeSetter setter)
{
  String *name = createString(propName);

  NativeProperty prop;
  prop.getter = getter;
  prop.setter = setter;

  klass->properties.set(name, prop);
}

Value Interpreter::createNativeStruct(int structId, int argc, Value *args)
{
  NativeStructDef *def = nativeStructs[structId];
  void *data = arena.Allocate(def->structSize);
  std::memset(data, 0, def->structSize);
  if (def->constructor)
  {
    def->constructor(this, data, argc, args);
  }

  Value literal = makeNativeStructInstance(def->persistent);
  NativeStructInstance *instance = literal.asNativeStructInstance();
  instance->def = def;
  instance->data = data;
  return literal;
}

NativeStructDef *Interpreter::registerNativeStruct(const char *name,
                                                   size_t structSize,
                                                   NativeStructCtor ctor,
                                                   NativeStructDtor dtor,
                                                   bool persistent)
{
  NativeStructDef *klass = new NativeStructDef();
  klass->name = createString(name);
  klass->constructor = ctor;
  klass->destructor = dtor;
  klass->structSize = structSize;
  klass->persistent = persistent;
  klass->id = nativeStructs.size();
  nativeStructs.push(klass);
  // OPTIMIZATION: Removed HashMap globals - using globalsArray directly
  // globals.set(klass->name, makeNativeStruct(klass->id));

  // OPTIMIZATION: Also add to globalsArray for direct indexed access
  uint16 globalIndex = globalsArray.size();
  globalsArray.push(makeNativeStruct(klass->id));
  nativeGlobalIndices.set(klass->name, globalIndex);

  return klass;
}

void Interpreter::addStructField(NativeStructDef *def, const char *fieldName,
                                 size_t offset, FieldType type, bool readOnly)
{
  String *name = createString(fieldName);
  NativeFieldDef field;
  field.offset = offset;
  field.type = type;
  field.readOnly = readOnly;
  if (def->fields.exist(name))
  {
    Warning("Field %s already exists in struct %s", fieldName,
            def->name->chars());
  }
  def->fields.set(name, field);
}

void Interpreter::printStack()
{
  ProcessExec *exec = currentExec();
  if (exec)
  {
    ProcessExec *fiber = exec;
    if (fiber->stackTop == fiber->stack)
      printf("  (empty)\n");
    else
      printf("          ");
    for (Value *slot = fiber->stack; slot < fiber->stackTop; slot++)
    {
      printf("[ ");
      printValue(*slot);
      printf(" ]");
    }
    printf("\n");
  }
}

void Interpreter::disassemble()
{

  printf("\n");
  printf("========================================\n");
  printf("         BYTECODE DUMP\n");
  printf("========================================\n\n");

  // ========== FUNCTIONS ==========
  printf(">>> FUNCTIONS: %zu\n\n", functions.size());

  for (size_t i = 0; i < functions.size(); i++)
  {
    Function *func = functions[i];
    if (!func)
      continue;

    printf("----------------------------------------\n");
    printf("Function #%zu: %s\n", i, func->name->chars());
    printf("  Arity: %d\n", func->arity);
    printf("  Has return: %s\n", func->hasReturn ? "yes" : "no");
    printf("----------------------------------------\n");

    // Constants
    printf("\nConstants (%zu):\n", func->chunk->constants.size());
    for (size_t j = 0; j < func->chunk->constants.size(); j++)
    {
      printf("  [%zu] = ", j);
      printValue(func->chunk->constants[j]);
      printf("\n");
    }

    // Bytecode usando Debug::disassembleInstruction
    printf("\nBytecode:\n");
    for (size_t offset = 0; offset < func->chunk->count;)
    {
      printf("  ");
      offset = Debug::disassembleInstruction(*func->chunk, offset);
    }

    printf("\n");
  }

  // ========== PROCESSES ==========
  printf("\n>>> PROCESSES: %zu\n\n", processes.size());

  for (size_t i = 1; i < processes.size(); i++)
  {
    ProcessDef *proc = processes[i];
    if (!proc)
      continue;

    printf("----------------------------------------\n");
    printf("Process #%zu: %s \n", i, proc->name->chars());
    printf("----------------------------------------\n");

    if (proc->frameCount > 0)
    {
      Function *func = proc->frames[0].func;

      printf("  Function: %s\n", func->name->chars());
      printf("  Arity: %d\n", func->arity);

      // Constants
      printf("\nConstants (%zu):\n", func->chunk->constants.size());
      for (size_t j = 0; j < func->chunk->constants.size(); j++)
      {
        printf("  [%zu] = ", j);
        printValue(func->chunk->constants[j]);
        printf("\n");
      }

      // Bytecode usando Debug::disassembleInstruction
      printf("\nBytecode:\n");
      for (size_t offset = 0; offset < func->chunk->count;)
      {
        printf("  ");
        offset = Debug::disassembleInstruction(*func->chunk, offset);
      }
    }

    printf("\n");
  }

  // ========== NATIVES ==========
  printf("\n>>> NATIVE FUNCTIONS: %zu\n\n", natives.size());

  for (size_t i = 0; i < natives.size(); i++)
  {
    printf("  [%2zu] %-20s (arity: %2d)\n", i, natives[i].name->chars(),
           natives[i].arity);
  }

  printf("\n========================================\n");
  printf("              END OF DUMP\n");
  printf("========================================\n\n");
}

int Interpreter::addGlobal(const char *name, Value value)
{
  String* str = createString(name);

  uint16 index = 0;
  if (nativeGlobalIndices.get(str, &index))
  {
    // Já existe: só atualiza o valor
    if (index >= globalsArray.size())
    {
      globalsArray.resize(index + 1);
    }
    if (globalsArray[index].isNative() || globalsArray[index].isNativeProcess() ||
        globalsArray[index].isNativeClass() || globalsArray[index].isNativeStruct())
    {
      Warning("Global '%s' is overriding a native symbol.", name);
    }
    globalsArray[index] = value;

    if (index >= globalIndexToName_.size())
    {
      globalIndexToName_.resize(index + 1);
    }
    globalIndexToName_[index] = str;
    return index;
  }

  // Novo global: cria índice estável
  index = globalsArray.size();
  globalsArray.push(value);
  nativeGlobalIndices.set(str, index);
  globalIndexToName_.push(str);

  return index;
}

bool Interpreter::setGlobal(const char *name, Value value)
{
  String* str = createString(name);
  uint16 index = 0;

  // Try native globals first
  if (!nativeGlobalIndices.get(str, &index))
  {
    // Try script globals via globalIndexToName_
    bool found = false;
    for (size_t i = 0; i < globalIndexToName_.size(); i++)
    {
      if (globalIndexToName_[i] && strcmp(globalIndexToName_[i]->chars(), name) == 0)
      {
        index = (uint16)i;
        found = true;
        break;
      }
    }
    if (!found) return false;
  }

  if (index >= globalsArray.size())
  {
    globalsArray.resize(index + 1);
  }

  if (globalsArray[index].isNative() || globalsArray[index].isNativeProcess() ||
      globalsArray[index].isNativeClass() || globalsArray[index].isNativeStruct())
  {
    Warning("Global '%s' is overriding a native symbol.", name);
  }

  globalsArray[index] = value;
  if (index >= globalIndexToName_.size())
  {
    globalIndexToName_.resize(index + 1);
  }
  globalIndexToName_[index] = str;
  return true;
}

Value Interpreter::getGlobal(const char *name)
{
  String* str = createString(name);
  uint16 index = 0;

  // Try native globals first
  if (nativeGlobalIndices.get(str, &index))
  {
    if (index < globalsArray.size()) return globalsArray[index];
    return makeNil();
  }

  // Try script globals via globalIndexToName_
  for (size_t i = 0; i < globalIndexToName_.size(); i++)
  {
    if (globalIndexToName_[i] && strcmp(globalIndexToName_[i]->chars(), name) == 0)
    {
      if (i < globalsArray.size()) return globalsArray[i];
      return makeNil();
    }
  }

  return makeNil();
}

Value Interpreter::getGlobal(uint32 index)
{
  if (index < globalsArray.size()) return globalsArray[index];
  return makeNil();
}

bool Interpreter::tryGetGlobal(const char *name, Value *value)
{
  String* str = createString(name);
  uint16 index = 0;

  // Try native globals first
  if (nativeGlobalIndices.get(str, &index))
  {
    if (index < globalsArray.size()) { *value = globalsArray[index]; return true; }
    return false;
  }

  // Try script globals via globalIndexToName_
  for (size_t i = 0; i < globalIndexToName_.size(); i++)
  {
    if (globalIndexToName_[i] && strcmp(globalIndexToName_[i]->chars(), name) == 0)
    {
      if (i < globalsArray.size()) { *value = globalsArray[i]; return true; }
      return false;
    }
  }

  return false;
}

void Interpreter::print(Value value) { printValue(value); }

float Interpreter::getCurrentTime() const { return currentTime; }

static void OsVPrintf(const char *fmt, va_list args)
{
  char buffer[1024];
  vsnprintf(buffer, sizeof(buffer), fmt, args);
  OsPrintf("%s", buffer);
}

void Interpreter::runtimeError(const char *format, ...)
{
  hasFatalError_ = true;

  if (!debugMode_)
  {
    // Release mode: minimal output
    OsPrintf("Runtime Error: ");
    va_list args;
    va_start(args, format);
    OsVPrintf(format, args);
    va_end(args);
    if (currentProcess && currentProcess->name)
      OsPrintf(" in '%s'", currentProcess->name->chars());
    OsPrintf("\n");
    return;
  }

  // Debug mode: full diagnostics
  OsPrintf("Runtime Error: ");
  va_list args;
  va_start(args, format);
  OsVPrintf(format, args);
  va_end(args);
  OsPrintf("\n");

  if (currentProcess)
  {
    OsPrintf("  Process: '%s' (id=%u)\n",
             currentProcess->name ? currentProcess->name->chars() : "?",
             currentProcess->id);
  }

  // Print stack trace with line numbers
  ProcessExec *exec = currentExec();
  if (exec && exec->frameCount > 0)
  {
    OsPrintf("\nStack trace:\n");
    for (int i = exec->frameCount - 1; i >= 0; i--)
    {
      CallFrame *frame = &exec->frames[i];
      Function *func = frame->func;

      if (func && func->chunk->code && frame->ip >= func->chunk->code)
      {
        size_t instruction = frame->ip - func->chunk->code;
        if (instruction > 0) instruction--;

        int line = func->chunk->lines[instruction];
        const char *funcName = func->name ? func->name->chars() : "<script>";

        OsPrintf("  [%d] %s() at line %d\n", i, funcName, line);
      }
    }
  }
}
bool Interpreter::throwException(Value error)
{
  ProcessExec *fiber = currentExec();
  if (!fiber)
  {
    return false;
  }

  while (fiber->tryDepth > 0)
  {
    TryHandler &handler = fiber->tryHandlers[fiber->tryDepth - 1];

    if (handler.inFinally)
    {
      handler.pendingError = error;
      handler.hasPendingError = true;
      fiber->tryDepth--;
      continue;
    }

    fiber->stackTop = handler.stackRestore; // Limpa a stack!

    if (handler.catchIP != nullptr && !handler.catchConsumed)
    {
      handler.catchConsumed = true;
      push(error);
      fiber->ip = handler.catchIP;
      return true;
    }
    else if (handler.finallyIP != nullptr)
    {
      handler.pendingError = error;
      handler.hasPendingError = true;
      handler.inFinally = true;
      fiber->ip = handler.finallyIP;
      return true;
    }

    fiber->tryDepth--;
  }

  return false;
}
void Interpreter::safetimeError(const char *format, ...)
{

  OsPrintf("Runtime Error: ");
  va_list args;
  va_start(args, format);
  OsVPrintf(format, args);
  va_end(args);
  OsPrintf("\n");
}

void Interpreter::resetFiber()
{
  ProcessExec *exec = currentExec();
  if (exec)
  {
    exec->stackTop = exec->stack;
    exec->frameCount = 0;
    exec->state = ProcessState::DEAD;
  }
  hasFatalError_ = false;
}

Function *Interpreter::compile(const char *source)
{
#if BU_RUNTIME_ONLY
  (void)source;
  safetimeError("compile: compiler disabled in runtime-only build");
  return nullptr;
#else
  ProcessDef *proc = compiler->compile(source);
  
  // Copy global index to name mapping from compiler (convert std::vector to Vector<String*>)
  const auto& compilerMapping = compiler->getGlobalIndexToName();
  globalIndexToName_.clear();
  globalIndexToName_.reserve(compilerMapping.size());
  for (const auto& name : compilerMapping)
  {
    String* str = createString(name.c_str());
    globalIndexToName_.push(str);
  }

  if (globalsArray.size() < globalIndexToName_.size())
  {
    globalsArray.resize(globalIndexToName_.size());
  }
  
  Function *mainFunc = proc->frames[0].func;
  return mainFunc;
#endif
}

Function *Interpreter::compileExpression(const char *source)
{
#if BU_RUNTIME_ONLY
  (void)source;
  safetimeError("compileExpression: compiler disabled in runtime-only build");
  return nullptr;
#else
  ProcessDef *proc = compiler->compileExpression(source);
  
  // Copy global index to name mapping from compiler
  const auto& compilerMapping = compiler->getGlobalIndexToName();
  globalIndexToName_.clear();
  globalIndexToName_.reserve(compilerMapping.size());
  for (const auto& name : compilerMapping)
  {
    String* str = createString(name.c_str());
    globalIndexToName_.push(str);
  }

  if (globalsArray.size() < globalIndexToName_.size())
  {
    globalsArray.resize(globalIndexToName_.size());
  }
  
  Function *mainFunc = proc->frames[0].func;
  return mainFunc;
#endif
}

bool Interpreter::run(const char *source, bool _dump)
{
#if BU_RUNTIME_ONLY
  (void)source;
  (void)_dump;
  safetimeError("run: source execution disabled in runtime-only build, load bytecode instead");
  return false;
#else
  reset();

  ProcessDef *proc = compiler->compile(source);
  if (!proc)
  {
    return false;
  }
  
  // Copy global name mapping for debug messages
  const auto& compilerMapping = compiler->getGlobalIndexToName();
  globalIndexToName_.clear();
  globalIndexToName_.reserve(compilerMapping.size());
  for (const auto& name : compilerMapping)
  {
    String* str = createString(name.c_str());
    globalIndexToName_.push(str);
  }

  if (globalsArray.size() < globalIndexToName_.size())
  {
    globalsArray.resize(globalIndexToName_.size());
  }

  if (_dump)
  {
    disassemble();
    // Function *mainFunc = proc->frames[0].func;
    //   Debug::dumpFunction(mainFunc);
  }

  mainProcess = spawnProcess(proc);
  currentProcess = mainProcess;

  //  Debug::disassembleChunk(*mainProcess->frames[0].func->chunk,"#main");
  run_process(mainProcess);

  return !hasFatalError_;
#endif
}
bool Interpreter::compile(const char *source, bool dump)
{
#if BU_RUNTIME_ONLY
  (void)source;
  (void)dump;
  safetimeError("compile: compiler disabled in runtime-only build");
  return false;
#else
  reset();

  ProcessDef *proc = compiler->compile(source);
  if (!proc)
  {
    return false;
  }
  
  // Copy global name mapping for debug messages
  const auto& compilerMapping = compiler->getGlobalIndexToName();
  globalIndexToName_.clear();
  globalIndexToName_.reserve(compilerMapping.size());
  for (const auto& name : compilerMapping)
  {
    String* str = createString(name.c_str());
    globalIndexToName_.push(str);
  }

  if (globalsArray.size() < globalIndexToName_.size())
  {
    globalsArray.resize(globalIndexToName_.size());
  }

  if (dump)
  {
    disassemble();
    // Function *mainFunc = proc->frames[0].func;
    //   Debug::dumpFunction(mainFunc);
  }

  return !hasFatalError_;
#endif
}

void Interpreter::setHooks(const VMHooks &h) { hooks = h; }

void Interpreter::initFiber(ProcessExec *fiber, Function *func)
{
  fiber->state = ProcessState::RUNNING;
  fiber->resumeTime = 0.0f;

  fiber->stackTop = fiber->stack;

  fiber->ip = func->chunk->code;

  fiber->frameCount = 1;
  fiber->frames[0].func = func;
  fiber->frames[0].closure = nullptr;
  fiber->frames[0].ip = nullptr;
  fiber->frames[0].slots = fiber->stack; // Base da stack
}

void Interpreter::setPrivateTable()
{
  privateIndexMap.set("x", 0);
  privateIndexMap.set("y", 1);
  privateIndexMap.set("z", 2);
  privateIndexMap.set("graph", 3);
  privateIndexMap.set("angle", 4);
  privateIndexMap.set("size", 5);
  privateIndexMap.set("flags", 6);
  privateIndexMap.set("id", 7);
  privateIndexMap.set("father", 8);
  privateIndexMap.set("red", 9);
  privateIndexMap.set("green", 10);
  privateIndexMap.set("blue", 11);
  privateIndexMap.set("alpha", 12);
  privateIndexMap.set("tag", 13);
  privateIndexMap.set("state", 14);
  privateIndexMap.set("speed", 15);
  privateIndexMap.set("group", 16);
}

StructDef *Interpreter::addStruct(String *name, int *id)
{

  if (structsMap.exist(name))
  {
    return nullptr;
  }
  StructDef *proc = new StructDef();
  structsMap.set(name, proc);
  proc->index = (int)structs.size();
  *id = (int)structs.size();
  structs.push(proc);
  return proc;
}

StructDef *Interpreter::registerStruct(String *name)
{
  if (structsMap.exist(name))
  {
    Warning("Struct with name '%s' already exists", name->chars());
    return nullptr;
  }

  StructDef *proc = new StructDef();

  proc->name = name;
  proc->argCount = 0;
  structsMap.set(name, proc);
  proc->index = (int)structs.size();

  structs.push(proc);

  return proc;
}

ClassDef *Interpreter::registerClass(String *name)
{
  if (structsMap.exist(name))
  {
    return nullptr;
  }

  ClassDef *proc = new ClassDef();
  proc->name = name;
  classesMap.set(name, proc);

  proc->index = (int)classes.size();

  classes.push(proc);

  return proc;
}

String *Interpreter::createString(const char *str, uint32 len)
{
  return stringPool.create(str, len);
}

String *Interpreter::createString(const char *str)
{
  return stringPool.create(str);
}

bool Interpreter::containsClassDefenition(String *name)
{
  return classesMap.exist(name);
}

bool Interpreter::getClassDefenition(String *name, ClassDef *result)
{
  return classesMap.get(name, &result);
}

bool Interpreter::tryGetClassDefenition(const char *name, ClassDef **out)
{
  String *pName = createString(name);
  bool result = false;
  if (classesMap.get(pName, out))
  {
    result = true;
  }
  return result;
}

bool Interpreter::tryGetNativeClassDef(const char *name, NativeClassDef **out)
{
  String *pName = createString(name);
  return nativeClassesMap.get(pName, out);
}

Value Interpreter::createClassInstance(const char *className, int argCount, Value *args)
{
  ClassDef *klass = nullptr;
  if (!tryGetClassDefenition(className, &klass))
  {
    runtimeError("Class '%s' not found", className);
    return makeNil();
  }
  return createClassInstance(klass, argCount, args);
}

Value Interpreter::createClassInstance(ClassDef *klass, int argCount, Value *args)
{
  if (!klass)
  {
    runtimeError("Cannot create instance of null class");
    return makeNil();
  }

  // Cria a instância
  Value value = makeClassInstance();
  ClassInstance *instance = value.asClassInstance();
  instance->klass = klass;
  instance->fields.reserve(klass->fieldCount);

  // Inicializa fields com valores default ou nil
  for (int i = 0; i < klass->fieldCount; i++)
  {
    if (i < (int)klass->fieldDefaults.size() && !klass->fieldDefaults[i].isNil())
    {
      instance->fields.push(klass->fieldDefaults[i]);
    }
    else
    {
      instance->fields.push(makeNil());
    }
  }

  // Se herda de NativeClass, cria os dados nativos
  NativeClassDef *nativeDef = nullptr;
  ClassDef *current = klass;
  while (current)
  {
    if (current->nativeSuperclass)
    {
      nativeDef = current->nativeSuperclass;
      break;
    }
    current = current->superclass;
  }

  if (nativeDef)
  {
    if (nativeDef->constructor)
    {
      instance->nativeUserData = nativeDef->constructor(this, 0, nullptr);
    }
    else
    {
      instance->nativeUserData = arena.Allocate(128);
      std::memset(instance->nativeUserData, 0, 128);
    }
  }

  // Chama o constructor (init) se existir
  if (klass->constructor)
  {
    if (argCount != klass->constructor->arity)
    {
      runtimeError("init() expects %d arguments, got %d", klass->constructor->arity, argCount);
      return makeNil();
    }

    // Guarda estado actual da fiber
    Process *proc = mainProcess;
    ProcessExec *fiber = proc;
    int savedFrameCount = fiber->frameCount;
    Value *savedStackTop = fiber->stackTop;

    // Prepara a stack: primeiro a instância (será slot 0 = self), depois os args
    push(value);
    for (int i = 0; i < argCount; i++)
    {
      push(args[i]);
    }

    // Cria um novo frame para o constructor
    if (fiber->frameCount >= FRAMES_MAX)
    {
      runtimeError("Stack overflow calling constructor");
      return makeNil();
    }

    CallFrame *frame = &fiber->frames[fiber->frameCount++];
    frame->func = klass->constructor;
    frame->closure = nullptr;
    frame->ip = klass->constructor->chunk->code;
    frame->slots = fiber->stackTop - argCount - 1; // self está antes dos args

    // Executa o constructor
    while (fiber->frameCount > savedFrameCount)
    {
      ProcessResult result = run_process(proc);
      if (result.reason == ProcessResult::PROCESS_DONE || result.reason == ProcessResult::ERROR)
      {
        break;
      }
    }

    // Limpa a stack (o constructor já fez pop do self)
    fiber->stackTop = savedStackTop;
  }

  return value;
}

Value Interpreter::createClassInstanceRaw(const char *className)
{
  ClassDef *klass = nullptr;
  if (!tryGetClassDefenition(className, &klass))
  {
    runtimeError("Class '%s' not found", className);
    return makeNil();
  }
  return createClassInstanceRaw(klass);
}

Value Interpreter::createClassInstanceRaw(ClassDef *klass)
{
  if (!klass)
  {
    runtimeError("Cannot create instance of null class");
    return makeNil();
  }

  Value value = makeClassInstance();
  ClassInstance *instance = value.asClassInstance();
  instance->klass = klass;
  instance->fields.reserve(klass->fieldCount);

  for (int i = 0; i < klass->fieldCount; i++)
  {
    if (i < (int)klass->fieldDefaults.size() && !klass->fieldDefaults[i].isNil())
    {
      instance->fields.push(klass->fieldDefaults[i]);
    }
    else
    {
      instance->fields.push(makeNil());
    }
  }

  // Se herda de NativeClass, cria os dados nativos
  NativeClassDef *nativeDef = nullptr;
  ClassDef *current = klass;
  while (current)
  {
    if (current->nativeSuperclass)
    {
      nativeDef = current->nativeSuperclass;
      break;
    }
    current = current->superclass;
  }

  if (nativeDef)
  {
    if (nativeDef->constructor)
    {
      instance->nativeUserData = nativeDef->constructor(this, 0, nullptr);
    }
    else
    {
      instance->nativeUserData = arena.Allocate(128);
      std::memset(instance->nativeUserData, 0, 128);
    }
  }

  // NÃO chama init() - seguro para chamar durante runtime
  return value;
}

void Interpreter::addFunctionsClasses(Function *fun)
{
  if (!fun)
  {
    return;
  }

  // Class methods must be part of the serialized function table.
  if (fun->index >= 0)
  {
    return;
  }

  fun->index = (int)functions.size();
  functions.push(fun);
}

bool Interpreter::findAndJumpToHandler(Value error, uint8 *&ip, ProcessExec *fiber)
{

  while (fiber->tryDepth > 0)
  {
    TryHandler &handler = fiber->tryHandlers[fiber->tryDepth - 1];

    if (handler.inFinally)
    {
      handler.pendingError = error;
      handler.hasPendingError = true;
      fiber->tryDepth--;
      continue;
    }

    fiber->stackTop = handler.stackRestore;

    if (handler.catchIP != nullptr)
    {
      push(error);
      ip = handler.catchIP;
      return true;
    }
    else if (handler.finallyIP != nullptr)
    {
      handler.pendingError = error;
      handler.hasPendingError = true;
      handler.inFinally = true;
      ip = handler.finallyIP;
      return true;
    }

    fiber->tryDepth--;
  }
  return false;
}

BufferInstance::BufferInstance(int count, BufferType type) : GCObject(GCObjectType::BUFFER)
{
  this->count = count;
  this->type = type;
  this->cursor = 0;

  switch (type)
  {
  case BufferType::UINT8:
    this->elementSize = 1;
    break;
  case BufferType::INT16:
  case BufferType::UINT16:
    this->elementSize = 2;
    break;
  case BufferType::INT32:
  case BufferType::UINT32:
  case BufferType::FLOAT:
    this->elementSize = 4;
    break;
  case BufferType::DOUBLE:
    this->elementSize = 8;
    break;
  default:
    this->elementSize = 1;
    break;
  }

  size_t byteSize = count * this->elementSize;

  // this->data = (uint8 *)calloc(count, this->elementSize * count);
  this->data = (uint8 *)malloc(byteSize);
  if (!this->data)
  {
    Error("Failed to allocate buffer");
    return;
  }
  memset(data, 0, byteSize);
}

BufferInstance::~BufferInstance()
{
  if (this->data)
  {
    free(this->data);
    this->data = nullptr;
  }
}

// bool ClassInstance::getMethod(String *name, Function **out)
// {
//   ClassDef *current = klass;

//   while (current)
//   {
//     if (current->methods.get(name, out))
//     {
//       return true;
//     }
//     current = current->superclass;
//   }

//   return false;
// }

// bool ClassInstance::getMethod(String *name, Function **out)
// {
//     if(klass->methods.get(name, out))
//     {
//         return true;
//     }

//     if (klass->inherited)
//     {
//         if(klass->superclass->methods.get(name,out));
//         {
//             return true;
//         }
//     }
//     return false;
// }

Function *ClassDef::canRegisterFunction(String *pName)
{

  if (methods.exist(pName))
  {
    return nullptr;
  }
  Function *func = new Function();
  func->index = -1;
  func->arity = 0;
  func->hasReturn = false;
  func->name = pName;
  func->chunk = new Code(16);
  methods.set(pName, func);
  return func;
}

StructDef::~StructDef()
{
  names.destroy();
}

ClassDef::~ClassDef()
{
  fieldNames.destroy();
  methods.destroy();
  superclass = nullptr;
}

NativeClassDef::~NativeClassDef()
{
  methods.destroy();
  properties.destroy();
}

void Interpreter::dumpToFile(const char *filename)
{

#if BU_ENABLE_BYTECODE_DUMP

  FILE *f = fopen(filename, "w");
  if (!f)
  {
    fprintf(stderr, "Failed to open %s for writing\n", filename);
    return;
  }

  fprintf(f, "========================================\n");
  fprintf(f, "BULANG BYTECODE DUMP\n");
  fprintf(f, "========================================\n\n");

  // Dump global functions
  dumpAllFunctions(f);

  // Dump classes e métodos
  dumpAllClasses(f);

  fprintf(f, "\n========================================\n");
  fprintf(f, "END OF DUMP\n");
  fprintf(f, "========================================\n");

  fclose(f);
  printf("Bytecode dumped to: %s\n", filename);

#endif
}

void Interpreter::dumpAllFunctions(FILE *f)
{
#if BU_ENABLE_BYTECODE_DUMP
  fprintf(f, "========================================\n");
  fprintf(f, "GLOBAL FUNCTIONS\n");
  fprintf(f, "========================================\n\n");

  functionsMap.forEach([&](String *name, Function *func)
                       {
        fprintf(f, "\n>>> Function: %s\n", name->chars());
        fprintf(f, "    Arity: %d\n", func->arity);
        fprintf(f, "    Has return: %s\n", func->hasReturn ? "yes" : "no");
        fprintf(f, "    Index: %d\n\n", func->index);
        
        // Constants
        fprintf(f, "  Constants (%zu):\n", func->chunk->constants.size());
        for (size_t i = 0; i < func->chunk->constants.size(); i++) {
            fprintf(f, "    [%zu] = ", i);
            
            Value v = func->chunk->constants[i];
            if (v.isString()) {
                fprintf(f, "\"%s\"", v.asString()->chars());
            } else if (v.isInt()) {
                fprintf(f, "%d", v.asInt());
            } else if (v.isDouble()) {
                fprintf(f, "%.2f", v.asDouble());
            } else if (v.isBool()) {
                fprintf(f, "%s", v.asBool() ? "true" : "false");
            } else if (v.isNil()) {
                fprintf(f, "nil");
            } else if (v.type == ValueType::CLASS) {
                int classId = v.asClassId();
                if (classId < (int)classes.size() && classes[classId]) {
                    fprintf(f, "<class '%s'>", classes[classId]->name->chars());
                } else {
                    fprintf(f, "<class %d>", classId);
                }
            } else if (v.type == ValueType::FUNCTION) {
                fprintf(f, "<function %d>", v.asFunctionId());
            } else if (v.type == ValueType::MODULEREFERENCE) {
                uint32_t packed = v.as.unsignedInteger;
                fprintf(f, "<module_reference %d %d %d>", 
                    packed >> 24, (packed >> 12) & 0xFFF, packed & 0xFFF);
            } else if (v.type == ValueType::STRUCT) {
                fprintf(f, "<struct %d>", v.asStructId());
            } else {
                fprintf(f, "<value type %d>", (int)v.type);
            }
            fprintf(f, "\n");
        }
        fprintf(f, "\n");
        
        // Bytecode
        fprintf(f, "  Bytecode:\n");
        for (size_t offset = 0; offset < func->chunk->count;) {
            fprintf(f, "    ");
            
            // Salva stdout, redireciona para file temporariamente
            FILE* old_stdout = stdout;
            stdout = f;
            offset = Debug::disassembleInstruction(*func->chunk, offset);
            stdout = old_stdout;
        }
        
        fprintf(f, "\n"); });
#endif
}

void Interpreter::dumpAllClasses(FILE *f)
{
#if BU_ENABLE_BYTECODE_DUMP
  fprintf(f, "\n========================================\n");
  fprintf(f, "CLASSES\n");
  fprintf(f, "========================================\n\n");

  classesMap.forEach([&](String *name, ClassDef *klass)
                     {
        fprintf(f, "\n>>> Class: %s\n", name->chars());
        fprintf(f, "    Index: %d\n", klass->index);
        fprintf(f, "    Field count: %d\n", klass->fieldCount);
        fprintf(f, "    Has superclass: %s\n", klass->superclass ? "yes" : "no");
        
        if (klass->superclass) {
            fprintf(f, "    Superclass: %s\n", klass->superclass->name->chars());
        }
        
        // Fields
        fprintf(f, "\n    Fields:\n");
        klass->fieldNames.forEach([&](String* fieldName, uint8_t index) {
            fprintf(f, "      [%u] %s\n", index, fieldName->chars());
        });
        
        // Constructor
        if (klass->constructor) {
            fprintf(f, "\n    >>> Constructor (init)\n");
            fprintf(f, "        Arity: %d\n\n", klass->constructor->arity);
            
            // Constants
            fprintf(f, "      Constants (%zu):\n", 
                   klass->constructor->chunk->constants.size());
            for (size_t i = 0; i < klass->constructor->chunk->constants.size(); i++) {
                fprintf(f, "        [%zu] = ", i);
                
                Value v = klass->constructor->chunk->constants[i];
                if (v.isString()) {
                    fprintf(f, "\"%s\"", v.asString()->chars());
                } else if (v.isInt()) {
                    fprintf(f, "%d", v.asInt());
                } else if (v.isDouble()) {
                    fprintf(f, "%.2f", v.asDouble());
                } else {
                    fprintf(f, "<value>");
                }
                fprintf(f, "\n");
            }
            fprintf(f, "\n");
            
            // Bytecode
            fprintf(f, "      Bytecode:\n");
            for (size_t offset = 0; offset < klass->constructor->chunk->count;) {
                fprintf(f, "        ");
                
                FILE* old_stdout = stdout;
                stdout = f;
                offset = Debug::disassembleInstruction(*klass->constructor->chunk, offset);
                stdout = old_stdout;
            }
        }
        
        // Methods
        fprintf(f, "\n    Methods:\n");
        klass->methods.forEach([&](String* methodName, Function* method) {
            fprintf(f, "\n    >>> Method: %s\n", methodName->chars());
            fprintf(f, "        Arity: %d\n", method->arity);
            fprintf(f, "        Has return: %s\n\n", method->hasReturn ? "yes" : "no");
            
            // Constants
            fprintf(f, "      Constants (%zu):\n", method->chunk->constants.size());
            for (size_t i = 0; i < method->chunk->constants.size(); i++) {
                fprintf(f, "        [%zu] = ", i);
                
                Value v = method->chunk->constants[i];
                if (v.isString()) {
                    fprintf(f, "\"%s\"", v.asString()->chars());
                } else if (v.isInt()) {
                    fprintf(f, "%d", v.asInt());
                } else if (v.isDouble()) {
                    fprintf(f, "%.2f", v.asDouble());
                } else {
                    fprintf(f, "<value>");
                }
                fprintf(f, "\n");
            }
            fprintf(f, "\n");
            
            // Bytecode
            fprintf(f, "      Bytecode:\n");
            for (size_t offset = 0; offset < method->chunk->count;) {
                fprintf(f, "        ");
                
                FILE* old_stdout = stdout;
                stdout = f;
                offset = Debug::disassembleInstruction(*method->chunk, offset);
                stdout = old_stdout;
            }
            fprintf(f, "\n");
        });
        
        fprintf(f, "\n"); });
#endif
}

// enum class BufferType : uint8
// {
//   UINT8, // 0: Byte
//   INT16, // 1: Short
//   UINT16, // 2: UShort
//   INT32, // 3: Int
//   UINT32, // 4: UInt
//   FLOAT, // 5: Float
//   DOUBLE // 6: Double
// };

size_t get_type_size(BufferType type)
{
  switch (type)
  {
  case BufferType::UINT8:
    return 1;
  case BufferType::INT16:
  case BufferType::UINT16:
    return 2;
  case BufferType::INT32:
  case BufferType::UINT32:
  case BufferType::FLOAT:
    return 4;
  case BufferType::DOUBLE:
    return 8;
  default:
    return 1;
  }
}

Closure::Closure() : GCObject(GCObjectType::CLOSURE),
                     functionId(-1),
                     upvalueCount(0) {}

Closure::~Closure()
{

  upvalues.destroy();
}

Upvalue::Upvalue(Value *loc) : GCObject(GCObjectType::UPVALUE)
{
  location = loc;
  nextOpen = nullptr;
  closed.type = ValueType::NIL;
}

// ============================================
// Array Extraction Helpers (for native bindings)
// ============================================

int Interpreter::getArrayLength(Value v)
{
    if (!v.isArray()) return -1;
    return (int)v.asArray()->values.size();
}

int Interpreter::getFloats(Value v, float* out, int maxCount)
{
    if (!v.isArray()) return -1;
    ArrayInstance* arr = v.asArray();
    int count = (int)arr->values.size();
    if (count > maxCount) count = maxCount;

    for (int i = 0; i < count; i++) {
        out[i] = (float)arr->values[i].asNumber();
    }
    return count;
}

int Interpreter::getInts(Value v, int* out, int maxCount)
{
    if (!v.isArray()) return -1;
    ArrayInstance* arr = v.asArray();
    int count = (int)arr->values.size();
    if (count > maxCount) count = maxCount;

    for (int i = 0; i < count; i++) {
        out[i] = (int)arr->values[i].asNumber();
    }
    return count;
}

int Interpreter::getDoubles(Value v, double* out, int maxCount)
{
    if (!v.isArray()) return -1;
    ArrayInstance* arr = v.asArray();
    int count = (int)arr->values.size();
    if (count > maxCount) count = maxCount;

    for (int i = 0; i < count; i++) {
        out[i] = arr->values[i].asNumber();
    }
    return count;
}

bool Interpreter::getVec2(Value v, float* out)
{
    if (!v.isArray()) return false;
    ArrayInstance* arr = v.asArray();
    if (arr->values.size() < 2) return false;

    out[0] = (float)arr->values[0].asNumber();
    out[1] = (float)arr->values[1].asNumber();
    return true;
}

bool Interpreter::getVec3(Value v, float* out)
{
    if (!v.isArray()) return false;
    ArrayInstance* arr = v.asArray();
    if (arr->values.size() < 3) return false;

    out[0] = (float)arr->values[0].asNumber();
    out[1] = (float)arr->values[1].asNumber();
    out[2] = (float)arr->values[2].asNumber();
    return true;
}

bool Interpreter::getVec4(Value v, float* out)
{
    if (!v.isArray()) return false;
    ArrayInstance* arr = v.asArray();
    if (arr->values.size() < 4) return false;

    out[0] = (float)arr->values[0].asNumber();
    out[1] = (float)arr->values[1].asNumber();
    out[2] = (float)arr->values[2].asNumber();
    out[3] = (float)arr->values[3].asNumber();
    return true;
}

bool Interpreter::getMat3(Value v, float* out)
{
    if (!v.isArray()) return false;
    ArrayInstance* arr = v.asArray();
    if (arr->values.size() < 9) return false;

    for (int i = 0; i < 9; i++) {
        out[i] = (float)arr->values[i].asNumber();
    }
    return true;
}

bool Interpreter::getMat4(Value v, float* out)
{
    if (!v.isArray()) return false;
    ArrayInstance* arr = v.asArray();
    if (arr->values.size() < 16) return false;

    for (int i = 0; i < 16; i++) {
        out[i] = (float)arr->values[i].asNumber();
    }
    return true;
}
