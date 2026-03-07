#pragma once
#include "arena.hpp"
#include "code.hpp"
#include "config.hpp"
#include "map.hpp"
#include "list.hpp"
#include "ordermap.hpp"
#include "pool.hpp"
#include "string.hpp"
#include "types.hpp"
#include "vector.hpp"
#include <new>

#ifdef NDEBUG
#define WDIV_ASSERT(condition, ...) ((void)0)
#else
#define WDIV_ASSERT(condition, ...)                                      \
  do                                                                     \
  {                                                                      \
    if (!(condition))                                                    \
    {                                                                    \
      Error("ASSERT FAILED: %s:%d: %s", __FILE__, __LINE__, #condition); \
      assert(false);                                                     \
    }                                                                    \
  } while (0)
#endif

struct Function;
struct CallFrame;
struct ProcessExec;
struct Process;
class Interpreter;
class Compiler;

enum class FieldType : uint8_t
{
  BYTE,    // byte
  INT,     // int
  UINT,    // uint
  FLOAT,   // float
  DOUBLE,  // double
  BOOL,    // bool
  POINTER, // void*
  STRING,  // String*
};

enum class StaticNames : uint8 {
    // === COMMON / ARRAY / LIST ===
    PUSH = 0,
    POP,
    BACK,
    LENGTH,
    CLEAR,
    
    // === NOVOS (ARRAY/LIST) ===
    INSERT,       // Novo
    FIND,         // Novo (pode ser predicado ou valor)
    REVERSE,      // Novo
    JOIN,         // Novo
    FIRST,        // Novo
    LAST,         // Novo
    COUNT,        // Novo (para contar ocorrências, ex: arr.count(10))

    // === MAP / SET ===
    HAS,
    REMOVE,       //  Map e Array)
    KEYS,
    VALUES,

    // === STRING ===
    UPPER,
    LOWER,
    CONCAT,       // Já existia (agora serve para String e Array)
    SUB,
    REPLACE,
    AT,
    CONTAINS,     //  String e Array)
    TRIM,
    STARTWITH,
    ENDWITH,
    INDEXOF,
    REPEAT,
    SPLIT,
    INIT,         // Geralmente para Classes/Structs

    // === BUFFER ===
    FILL,         //   Buffer e Array)
    COPY,
    SLICE,        //   Buffer, String e Array)
    SAVE,

    // === BUFFER READ/WRITE OPERATIONS ===
    WRITE_BYTE,
    WRITE_SHORT,
    WRITE_USHORT,
    WRITE_INT,
    WRITE_UINT,
    WRITE_FLOAT,
    WRITE_DOUBLE,
    
    READ_BYTE,
    READ_SHORT,
    READ_USHORT,
    READ_INT,
    READ_UINT,
    READ_FLOAT,
    READ_DOUBLE,
    
    WRITE_STRING,
    READ_STRING,
    
    SEEK,
    TELL,
    REWIND,
    SKIP,
    REMAINING,

    // === META ===
    TOTAL_COUNT  
};  

  // startsWith
  // endsWith
  // indexOf

  typedef int(*NativeFunction)(Interpreter * vm, int argCount, Value *args);
  typedef int(*NativeFunctionProcess)(Interpreter * vm, Process *process, int argCount, Value *args);
  typedef int(*NativeMethod)(Interpreter * vm, void *instance, int argCount,
                               Value *args);
  typedef void *(*NativeConstructor)(Interpreter * vm, int argCount, Value *args);
  typedef void(*NativeDestructor)(Interpreter * vm, void *instance);
  typedef Value(*NativeGetter)(Interpreter * vm, void *instance);
  typedef void(*NativeSetter)(Interpreter * vm, void *instance, Value value);
  typedef void(*NativeStructCtor)(Interpreter * vm, void *buffer, int argc,
                                  Value *args);
  typedef void(*NativeStructDtor)(Interpreter * vm, void *buffer);

  struct NativeProperty{
      NativeGetter getter;
      NativeSetter setter; // null = read-only
  };
struct Function
{
  int index;
  int arity{-1};
  Code *chunk{nullptr};
  String *name{nullptr};
  bool hasReturn{false};
  int upvalueCount{0};
  ~Function();
};

struct NativeDef
{
  String *name{nullptr};
  NativeFunction func;
  int arity{0};
  uint32 index{0};
};

struct NativeProcessDef
{
  String *name{nullptr};
  NativeFunctionProcess func;
  int arity{0};
  uint32 index{0};
};

struct StructDef
{
  int index;
  String *name;
  // HashMap<String *, uint8, StringHasher, StringEq> names;
  List<String *, uint8> names;
  uint8 argCount;
  ~StructDef();
};

// Forward declaration para NativeClassDef
struct NativeClassDef;

struct ClassDef
{
  int index;
  String *name{nullptr};
  String *parent{nullptr};
  bool inherited{false};
  int fieldCount;                 // Número de fields
  Function *constructor{nullptr}; // existe na tabela
  ClassDef *superclass;           // Herança de outra ClassDef
  NativeClassDef *nativeSuperclass{nullptr}; // Herança de NativeClass (híbrido)

  // HashMap<String *, Function *, StringHasher, StringEq> methods;
  // HashMap<String *, uint8_t, StringHasher, StringEq> fieldNames; // field name → index

  List<String *, Function *> methods;
  List<String *, uint8> fieldNames;
  Vector<Value> fieldDefaults;    // Default values for fields (nil if no default)

  Function *canRegisterFunction(String *pName);
  ~ClassDef();
};

struct NativeClassDef
{
  int index;
  String *name;
  NativeConstructor constructor;
  NativeDestructor destructor;
  bool persistent;  // Se true, instâncias não são coletadas pelo GC

  List<String *, NativeMethod> methods;
  List<String *, NativeProperty> properties;

  ~NativeClassDef();

  int argCount; // Args do constructor
};

struct NativeFieldDef
{
  size_t offset;
  FieldType type;
  bool readOnly;
};

struct NativeStructDef
{
  int id;
  String *name;
  size_t structSize;
  bool persistent;  // Se true, instâncias não são coletadas pelo GC
  List<String *, NativeFieldDef> fields;
  NativeStructCtor constructor; // nullable
  NativeStructDtor destructor;  // nullable
};

struct NativeFunctionDef
{
  NativeFunction ptr;
  int arity;
};

class ModuleDef
{
private:
  String *name;
  Interpreter *vm;
  HashMap<String *, uint16, StringHasher, StringEq> functionNames;
  Vector<Value> constants;
  HashMap<String *, uint16, StringHasher, StringEq> constantNames;
  friend class Interpreter;

  void clear();

public:
  Vector<NativeFunctionDef> functions;
  ModuleDef(String *name, Interpreter *vm);
  uint16 addFunction(const char *name, NativeFunction func, int arity);
  uint16 addConstant(const char *name, Value value);
  NativeFunctionDef *getFunction(uint16 id);
  Value *getConstant(uint16 id);

  bool getFunctionId(String *name, uint16 *outId);
  bool getConstantId(String *name, uint16 *outId);
  bool getFunctionId(const char *name, uint16 *outId);
  bool getConstantId(const char *name, uint16 *outId);

  bool getFunctionName(uint16 id, String **outName);
  bool getConstantName(uint16 id, String **outName);

  String *getName() const { return name; }
};

class ModuleBuilder
{
private:
  ModuleDef *module;
  Interpreter *vm;
  friend class Interpreter;

public:
  ModuleBuilder(ModuleDef *module, Interpreter *vm);
  ModuleBuilder &addFunction(const char *name, NativeFunction func, int arity);
  ModuleBuilder &addInt(const char *name, int value);
  ModuleBuilder &addByte(const char *name, uint8 value);

  ModuleBuilder &addFloat(const char *name, float value);
  ModuleBuilder &addDouble(const char *name, double value);
  ModuleBuilder &addBool(const char *name, bool value);
  ModuleBuilder &addString(const char *name, const char *value);
};

enum class GCObjectType : uint8
{
  STRUCT,
  CLASS,
  ARRAY,
  MAP,
  BUFFER,
  NATIVE_CLASS,
  NATIVE_STRUCT,
  CLOSURE,
  UPVALUE
};

struct GCObject
{
  GCObjectType type;
  uint8 marked;
  GCObject *next;

  GCObject(GCObjectType t) : type(t), marked(0), next(nullptr) {}
};

struct StructInstance : GCObject
{
  StructDef *def;
  Vector<Value> values;

  StructInstance() : GCObject(GCObjectType::STRUCT) {}
};

struct ClassInstance : GCObject
{
  ClassDef *klass;
  Vector<Value> fields;
  void *nativeUserData{nullptr};  // Dados nativos quando herda de NativeClass

  ClassInstance() : GCObject(GCObjectType::CLASS) {}

  ~ClassInstance() {}

  FORCE_INLINE bool getMethod(String *name, Function **out)
  {
    ClassDef *current = klass;
    while (current)
    {
      if (current->methods.get(name, out))
      {
        return true;
      }
      current = current->superclass;
    }
    return false;
  }
  
  // Procura método nativo na superclass nativa
  FORCE_INLINE bool getNativeMethod(String *name, NativeMethod *out)
  {
    ClassDef *current = klass;
    while (current)
    {
      if (current->nativeSuperclass)
      {
        if (current->nativeSuperclass->methods.get(name, out))
        {
          return true;
        }
      }
      current = current->superclass;
    }
    return false;
  }
  
  // Retorna o NativeClassDef se existir na cadeia de herança
  FORCE_INLINE NativeClassDef* getNativeSuperclass()
  {
    ClassDef *current = klass;
    while (current)
    {
      if (current->nativeSuperclass)
      {
        return current->nativeSuperclass;
      }
      current = current->superclass;
    }
    return nullptr;
  }
  
  // Procura propriedade nativa na superclass nativa
  FORCE_INLINE bool getNativeProperty(String *name, NativeProperty *out)
  {
    ClassDef *current = klass;
    while (current)
    {
      if (current->nativeSuperclass)
      {
        if (current->nativeSuperclass->properties.get(name, out))
        {
          return true;
        }
      }
      current = current->superclass;
    }
    return false;
  }
};

struct ArrayInstance : GCObject
{
  Vector<Value> values;

  ArrayInstance() : GCObject(GCObjectType::ARRAY) {}
};

enum class BufferType : uint8
{
  UINT8,  // 0: Byte
  INT16,  // 1: Short
  UINT16, // 2: UShort
  INT32,  // 3: Int
  UINT32, // 4: UInt
  FLOAT,  // 5: Float
  DOUBLE  // 6: Double

};

struct BufferInstance : GCObject
{

  BufferType type;
  int count;       // Quantos elementos tem
  int elementSize; // Tamanho em bytes de 1 elemento (cache)
  int cursor;
  uint8 *data;
  BufferInstance(int count, BufferType type);
  ~BufferInstance();
};

struct MapInstance : GCObject
{
  HashMap<String *, Value, StringHasher, StringEq> table;

  MapInstance() : GCObject(GCObjectType::MAP) {}
};
struct NativeClassInstance : GCObject
{
  NativeClassDef *klass;
  void *userData;
  bool persistent;      // Se true, não é coletado pelo GC
  bool ownsUserData;    // Se true, o destrutor é chamado ao libertar

  NativeClassInstance() : GCObject(GCObjectType::NATIVE_CLASS), persistent(false), ownsUserData(true) {}
};

struct NativeStructInstance : GCObject
{
  NativeStructDef *def;
  void *data;
  bool persistent;  // Se true, não é coletado pelo GC

  NativeStructInstance() : GCObject(GCObjectType::NATIVE_STRUCT), persistent(false) {}
};

struct Upvalue : GCObject
{
  Value *location;
  Value closed;
  Upvalue *nextOpen;

  Upvalue(Value *loc);
};
struct Closure : GCObject
{
  int functionId;
  int upvalueCount;
  Vector<Upvalue *> upvalues;

  Closure();

  ~Closure();
};

struct CallFrame
{
  Function *func{nullptr};
  uint8 *ip{nullptr};
  Value *slots{nullptr};
  Closure *closure{nullptr};
};

struct VMHooks
{
  void (*onCreate)(Interpreter *vm, Process *p) = nullptr;
  void (*onStart)(Interpreter *vm,Process *p) = nullptr;
  void (*onUpdate)(Interpreter *vm,Process *p, float dt) = nullptr;
  void (*onRender)(Interpreter *vm,Process *p) = nullptr;
  void (*onDestroy)(Interpreter *vm,Process *p, int exitCode) = nullptr;
};

struct ProcessResult
{
  enum Reason : uint8
  {
    PROCESS_FRAME, // frame(N)
    CALL_RETURN,   // return to native C++ caller boundary
    PROCESS_DONE,    // return/end
    ERROR
  };

  Reason reason;
  int framePercent; // Se PROCESS_FRAME
};

struct TryHandler
{
  static constexpr int MAX_PENDING_RETURNS = 16;
  uint8_t *catchIP;
  uint8_t *finallyIP;
  Value *stackRestore;
  bool inFinally;
  bool hasPendingError;
  Value pendingError;
  bool catchConsumed;
  Value pendingReturns[MAX_PENDING_RETURNS];
  uint8_t pendingReturnCount;
  bool hasPendingReturn;

  TryHandler() : catchIP(nullptr), finallyIP(nullptr),
                 stackRestore(nullptr), inFinally(false),
                 hasPendingError(false), pendingReturnCount(0)
  {
    pendingError.as.byte = 0;
    pendingError.type = ValueType::NIL;
    catchConsumed = false;
    hasPendingReturn = false;
  }
};

struct ProcessExec
{

  ProcessState state; // Estado de execução do processo
  float resumeTime;   // Quando acorda (yield/frame)

  uint8 *ip;
  Value stack[STACK_MAX];
  Value *stackTop;
  CallFrame frames[FRAMES_MAX];
  int frameCount;
  uint8_t *gosubStack[GOSUB_MAX];
  int gosubTop{0};
  TryHandler tryHandlers[TRY_MAX];
  int tryDepth;

  ProcessExec()
      : state(ProcessState::DEAD), resumeTime(0), ip(nullptr), stackTop(stack),
        frameCount(0), gosubTop(0), tryDepth(0) {}
};
enum class PrivateIndex : uint8
{
  X = 0,
  Y = 1,
  Z = 2,
  GRAPH = 3,
  ANGLE = 4,
  SIZE = 5,
  FLAGS = 6,
  ID = 7,
  FATHER = 8,
  iRED = 9,
  iGREEN = 10,
  iBLUE = 11,
  iALPHA = 12,
  TAG = 13,
  STATE = 14,
  SPEED = 15,
  GROUP = 16,
  VELX = 17,
  VELY = 18,
  HP = 19,
  PROGRESS = 20,
  LIFE = 21,
  ACTIVE = 22,
  SHOW = 23,
  XOLD = 24,
  YOLD = 25,
  SIZEX =26,
  SIZEY =27,
  

};

struct ProcessDef : public ProcessExec
{
  int index;
  Vector<uint8> argsNames;
  String *name{nullptr};
  Value privates[MAX_PRIVATES];
  void finalize();
  void release();
};

struct Process : public ProcessExec
{

  String *name{nullptr};
  uint32 id{0};
  int blueprint{-1}; // Índice do ProcessDef que é a "blueprint" desse processo
  void *userData{nullptr}; 
  Value privates[MAX_PRIVATES];

  int exitCode = 0;

  bool initialized = false;

  void release();

  void reset();
};

class Interpreter
{

  HashMap<String *, Function *, StringHasher, StringEq> functionsMap;
  HashMap<String *, ProcessDef *, StringHasher, StringEq> processesMap;
  HashMap<String *, NativeDef, StringHasher, StringEq> nativesMap;
  HashMap<String *, NativeProcessDef, StringHasher, StringEq> nativeProcessesMap;
  HashMap<String *, StructDef *, StringHasher, StringEq> structsMap;
  HashMap<String *, ClassDef *, StringHasher, StringEq> classesMap;
  HashMap<String *, NativeClassDef *, StringHasher, StringEq> nativeClassesMap;
  HashMap<const char *, int, CStringHash, CStringEq> privateIndexMap;

  Vector<NativeDef> natives;
  Vector<NativeProcessDef> nativeProcesses; 
  Vector<Function *> functions;
  Vector<Function *> functionsClass;
  Vector<ProcessDef *> processes;
  Vector<StructDef *> structs;
  Vector<ClassDef *> classes;
  Vector<NativeClassDef *> nativeClasses;
  Vector<NativeStructDef *> nativeStructs;

  // gc begin

  size_t totalAllocated = 0;
  size_t totalStructs = 0;
  size_t totalClasses = 0;
  size_t totalClosures = 0;
  size_t totalUpvalues = 0;
  size_t totalMaps = 0;
  size_t totalArrays = 0;
  size_t totalBuffers = 0;
  size_t totalNativeStructs = 0;
  size_t totalNativeClasses = 0;
  size_t nextGC = 1024 * 1024;
  static constexpr size_t MIN_GC_THRESHOLD = 512 * 1024;         
  static constexpr size_t MAX_GC_THRESHOLD = 512 * 1024 * 1024;  
  static constexpr double GC_GROWTH_FACTOR = 2.0;
  bool gcInProgress = false;
  bool enbaledGC = true;
  GCObject *gcObjects = nullptr;
  int frameCount = 0;
  Vector<GCObject *> grayStack;

  // gc end

  HashMap<String *, uint16, StringHasher, StringEq> moduleNames; // Nome  ID
  Vector<ModuleDef *> modules;                                   // Array de módulos!
  HashMap<String *, Value, StringHasher, StringEq> globals;      // For named lookups (debug, reflection)
  Vector<Value> globalsArray;                                    // OPTIMIZATION: Direct indexed access
  HashMap<String *, uint16, StringHasher, StringEq> nativeGlobalIndices; // Native name -> globalsArray index
  Vector<String*> globalIndexToName_;                            // For debug: index -> name mapping (VM strings)

  // Plugin system internals
  static constexpr int MAX_PLUGIN_PATHS = 8;
  static constexpr int MAX_PATH_LEN = 256;
  static constexpr int MAX_PLUGINS = 32;

  struct LoadedPlugin {
      void* handle;
      const char* name;
      void (*cleanup)();
  };
  LoadedPlugin loadedPlugins[MAX_PLUGINS];
  int loadedPluginCount = 0;
  char pluginSearchPaths[MAX_PLUGIN_PATHS][MAX_PATH_LEN];
  int pluginSearchPathCount = 0;
  char lastPluginError[512];

  Vector<Process *> aliveProcesses;
  Vector<Process *> cleanProcesses;

  HeapAllocator arena;

  StringPool stringPool;

  float currentTime;
  float lastFrameTime;
  float accumulator = 0.0f;
  const float FIXED_DT = 1.0f / 60.0f;

  Process *currentProcess;
  Process *mainProcess;
  // Internal boundary used by C++->script calls (callFunction/callMethod).
  // When the target frame returns, run_process stops with CALL_RETURN instead
  // of continuing to execute the caller frame.
  bool stopOnCallReturn_{false};
  Process *callReturnProcess_{nullptr};
  int callReturnTargetFrameCount_{-1};
  bool hasFatalError_;
  bool debugMode_;

  Compiler *compiler;
  FileLoaderCallback fileLoaderCallback_ = nullptr;
  void *fileLoaderUserdata_ = nullptr;
  Upvalue *openUpvalues;

  VMHooks hooks;

  Vector<String*> staticNames;

  FORCE_INLINE ProcessExec *currentExec()
  {
    if (currentProcess)
      return static_cast<ProcessExec *>(currentProcess);
    if (mainProcess)
      return static_cast<ProcessExec *>(mainProcess);
    return nullptr;
  }

  FORCE_INLINE const ProcessExec *currentExec() const
  {
    if (currentProcess)
      return static_cast<const ProcessExec *>(currentProcess);
    if (mainProcess)
      return static_cast<const ProcessExec *>(mainProcess);
    return nullptr;
  }

  void freeInstances();
  void freeBlueprints();
  void freeFunctions();
  void freeRunningProcesses();
  void checkGC();
  void blackenObject(GCObject *obj);
  void traceReferences();

  void resetFiber();
  void initFiber(ProcessExec *fiber, Function *func);
  void setPrivateTable();
  void checkType(int index, ValueType expected, const char *funcName);

  void addFunctionsClasses(Function *fun);
  bool findAndJumpToHandler(Value error, uint8 *&ip, ProcessExec *fiber);

  friend class Compiler;
  friend class ModuleBuilder;

  void dumpAllFunctions(FILE *f);
  void dumpAllClasses(FILE *f);

  size_t countObjects() const;
  void clearAllGCObjects();

  FORCE_INLINE ClassInstance *creatClass()
  {

    checkGC();
    size_t size = sizeof(ClassInstance);
    void *mem = (MapInstance *)arena.Allocate(size); // 40kb
    ClassInstance *instance = new (mem) ClassInstance();

    totalClasses++;
    instance->next = gcObjects;
    gcObjects = instance;

    totalAllocated += size;

    return instance;
  }

  FORCE_INLINE Upvalue *createUpvalue(Value *loc)
  {
    checkGC();
    size_t size = sizeof(Upvalue);
    void *mem = arena.Allocate(size);
    Upvalue *upvalue = new (mem) Upvalue(loc);

    upvalue->next = (Upvalue *)gcObjects;
    gcObjects = upvalue;

    totalAllocated += size;
    totalUpvalues++;

    return upvalue;
  }

  FORCE_INLINE void freeUpvalue(Upvalue *upvalue)
  {
    size_t size = sizeof(Upvalue);
    upvalue->~Upvalue();
    arena.Free(upvalue, size);
    totalAllocated -= size;
    totalUpvalues--;
  }

  FORCE_INLINE Closure *createClosure()
  {
    checkGC();
    size_t size = sizeof(Closure);
    void *mem = arena.Allocate(size);
    Closure *closure = new (mem) Closure();
    closure->type = GCObjectType::CLOSURE;
    closure->marked = 0;

    closure->next = gcObjects;
    gcObjects = closure;

    totalAllocated += size;
    return closure;
  }

  FORCE_INLINE void freeClosure(Closure *c)
  {

    size_t size = sizeof(Closure);
    c->~Closure();
    arena.Free(c, size);
    totalAllocated -= size;
  }

  FORCE_INLINE void freeClass(ClassInstance *c)
  {
    size_t size = sizeof(ClassInstance);
    
    // Se herda de NativeClass, chama destructor nativo
    if (c->nativeUserData)
    {
      NativeClassDef *nativeDef = c->getNativeSuperclass();
      if (nativeDef && nativeDef->destructor)
      {
        nativeDef->destructor(this, c->nativeUserData);
      }
      // Nota: nativeUserData foi alocado pela arena ou pelo constructor nativo
      // A arena limpa automaticamente na shutdown
    }
    
    c->fields.destroy();
    c->klass = nullptr;
    c->~ClassInstance();
    arena.Free(c, size);
    totalAllocated -= size;
    totalClasses--;
  }

  FORCE_INLINE StructInstance *createStruct()
  {
    checkGC();
    size_t size = sizeof(StructInstance);
    void *mem = (StructInstance *)arena.Allocate(size); // 40kb
    StructInstance *instance = new (mem) StructInstance();
    instance->marked = 0;
    totalAllocated += size;
    totalStructs++;

    instance->next = gcObjects;
    gcObjects = instance;

    return instance;
  }

  FORCE_INLINE void freeStruct(StructInstance *s)
  {
    size_t size = sizeof(StructInstance);
    s->values.destroy();
    s->~StructInstance();
    totalStructs--;
    arena.Free(s, size);
    totalAllocated -= size;
  }
  FORCE_INLINE ArrayInstance *createArray()
  {
    checkGC();
    size_t size = sizeof(ArrayInstance);
    void *mem = (ArrayInstance *)arena.Allocate(size); // 32kb
    ArrayInstance *instance = new (mem) ArrayInstance();

    instance->next = gcObjects;
    gcObjects = instance;
    totalArrays++;

    instance->marked = 0;
    totalAllocated += size;

    return instance;
  }

  void markForFree(const Value &v);

  FORCE_INLINE void freeArray(ArrayInstance *a)
  {
    size_t size = sizeof(ArrayInstance);
    // size += a->values.capacity() * sizeof(Value);
    a->values.destroy();
    a->~ArrayInstance();
    arena.Free(a, size);
    totalAllocated -= size;
    totalArrays--;
  }

  BufferInstance *createBuffer(int count, int typeRaw);
  void freeBuffer(BufferInstance *b);

  FORCE_INLINE MapInstance *createMap()
  {
    checkGC();
    size_t size = sizeof(MapInstance);
    void *mem = (MapInstance *)arena.Allocate(size); // 40kb
    MapInstance *instance = new (mem) MapInstance();
    instance->marked = 0;

    instance->next = gcObjects;
    gcObjects = instance;
    totalMaps++;
    totalAllocated += size;

    return instance;
  }

  FORCE_INLINE void freeMap(MapInstance *m)
  {
    size_t size = sizeof(MapInstance);
    // size += m->table.capacity * sizeof(Value);
    totalAllocated -= size;
    totalMaps--;

    m->table.destroy();
    m->~MapInstance();
    arena.Free(m, size);
  }

  FORCE_INLINE NativeClassInstance *createNativeClass(bool persistent = false)
  {

    checkGC();
    size_t size = sizeof(NativeClassInstance);
    void *mem = (NativeClassInstance *)arena.Allocate(size); // 32kb
    NativeClassInstance *instance = new (mem) NativeClassInstance();
    instance->persistent = persistent;

    // Se não for persistent, adiciona ao GC
    if (!persistent)
    {
      instance->next = gcObjects;
      gcObjects = instance;
      totalNativeClasses++;
    }

    totalAllocated += size;

    return instance;
  }

  FORCE_INLINE void freeNativeClass(NativeClassInstance *n)
  {
    // Chama destrutor nativo para liberar userData APENAS se ownsUserData é true
    if (n->ownsUserData && n->klass && n->klass->destructor && n->userData)
    {
      n->klass->destructor(this, n->userData);
    }

    size_t size = sizeof(NativeClassInstance);
    totalAllocated -= size;
    n->~NativeClassInstance();
    arena.Free(n, size);
    totalNativeClasses--;
  }

  FORCE_INLINE NativeStructInstance *createNativeStruct(bool persistent = false)
  {
    checkGC();
    size_t size = sizeof(NativeStructInstance);
    void *mem = (NativeStructInstance *)arena.Allocate(size); // 32kb
    NativeStructInstance *instance = new (mem) NativeStructInstance();
    instance->persistent = persistent;
    totalAllocated += size;
    
    // Se não for persistent, adiciona ao GC
    if (!persistent)
    {
      instance->next = gcObjects;
      gcObjects = instance;
      totalNativeStructs++;
    }

    return instance;
  }

  FORCE_INLINE void freeNativeStruct(NativeStructInstance *n)
  {
    size_t size = sizeof(NativeStructInstance);
    totalAllocated -= size;
    n->~NativeStructInstance();
    arena.Free(n, size);
    totalNativeStructs--;
  }

  FORCE_INLINE void markRoots();
  FORCE_INLINE void markValue(const Value &v);
  void markObject(GCObject *obj);
  void sweep();
  void freeObject(GCObject *obj);

  FORCE_INLINE void markArray(ArrayInstance *a);
  FORCE_INLINE void markStruct(StructInstance *s);
  FORCE_INLINE void markClass(ClassInstance *c);
  FORCE_INLINE void markMap(MapInstance *m);
  FORCE_INLINE void markNativeClass(NativeClassInstance *n);
  FORCE_INLINE void markNativeStruct(NativeStructInstance *n);
  FORCE_INLINE void markBuffer(BufferInstance *b);

  // SWEEP
  void sweepArrays();
  void sweepStructs();
  void sweepClasses();
  void sweepMaps();
  void sweepNativeClasses();
  void sweepNativeStructs();
  void sweepBuffers();

public:
  Interpreter();
  ~Interpreter();
  void update(float deltaTime);

  void runGC();
  int getProcessPrivateIndex(const char *name);
 

  void dumpToFile(const char *filename);
  bool saveBytecode(const char *filename);
  bool loadBytecode(const char *filename);
  bool compileToBytecode(const char *source, const char *filename, bool dump = false);

  void setDebugMode(bool enabled) { debugMode_ = enabled; }
  bool isDebugMode() const { return debugMode_; }

  void setFileLoader(FileLoaderCallback loader, void *userdata = nullptr);

  NativeClassDef *registerNativeClass(const char *name, NativeConstructor ctor,
                                      NativeDestructor dtor, int argCount,
                                      bool persistent = false);
  void addNativeMethod(NativeClassDef *klass, const char *methodName,
                       NativeMethod method);
  void addNativeProperty(NativeClassDef *klass, const char *propName,
                         NativeGetter getter,
                         NativeSetter setter = nullptr // null = read-only
  );

  Value createNativeStruct(int structId, int argc, Value *args);
  NativeStructDef *registerNativeStruct(const char *name, size_t structSize,
                                        NativeStructCtor ctor = nullptr,
                                        NativeStructDtor dtor = nullptr,
                                        bool persistent = false);

  void addStructField(NativeStructDef *def, const char *fieldName,
                      size_t offset, FieldType type, bool readOnly = false);

  ProcessDef *addProcess(const char *name, Function *func);
  void destroyProcess(Process *proc);
  Process *spawnProcess(ProcessDef *proc);

  StructDef *addStruct(String *nam, int *id);

  StructDef *registerStruct(String *name);
  ClassDef *registerClass(String *nam);

  String *createString(const char *str, uint32 len);
  String *createString(const char *str);

  bool containsClassDefenition(String *name);
  bool getClassDefenition(String *name, ClassDef *result);
  bool tryGetClassDefenition(const char *name, ClassDef **result);
  bool tryGetNativeClassDef(const char *name, NativeClassDef **result);

  // Criar instâncias de classes script a partir do C++
  Value createClassInstance(const char *className, int argCount, Value *args);
  Value createClassInstance(ClassDef *klass, int argCount, Value *args);

  // Create instance WITHOUT calling init() - safe to call from native functions during runtime
  Value createClassInstanceRaw(const char *className);
  Value createClassInstanceRaw(ClassDef *klass);

  uint32 getTotalProcesses() const;
  uint32 getTotalAliveProcesses() const;
  Process *findProcessById(uint32 id);
  const Vector<Process *>& getAliveProcesses() const { return aliveProcesses; }

  void destroyFunction(Function *func);

  int registerNative(const char *name, NativeFunction func, int arity);
  int registerNativeProcess(const char *name, NativeFunctionProcess func, int arity);

  void print(Value value);

  // modules
  void registerBase();
  void registerArray();
  void registerMath();
  void registerOS();
  void registerPath();
  void registerFS();
  void registerTime();
  void registerFile();
  void registerJSON();
  void registerRegex();
  void registerZip();
  void registerSocket();
  void registerAll();

  Function *addFunction(const char *name, int arity = 0);
  Function *canRegisterFunction(const char *name, int arity, int *index);
  bool functionExists(const char *name);
  int registerFunction(const char *name, Function *func);

  void run_process_step(Process *proc);
  ProcessResult run_process(Process *process);

  float getCurrentTime() const;

  void runtimeError(const char *format, ...);
  void safetimeError(const char *format, ...);
  bool throwException(Value error);
  bool callFunction(Function *func, int argCount);
  bool callFunction(const char *name, int argCount);

  // Resolve function name with automatic __main__$ prefix fallback
  // Returns nullptr if function not found
  Function* getFunction(const char *name);

  // Call function with automatic name resolution (tries name, then __main__$name)
  bool callFunctionAuto(const char *name, int argCount);

  // Call a method on a class instance from C++
  // Pushes self + args, sets up the frame, and runs the method
  bool callMethod(Value instance, const char *methodName, int argCount, Value *args);

  Process *callProcess(ProcessDef *proc, int argCount);
  Process *callProcess(const char *name, int argCount);

  Function *compile(const char *source);
  Function *compileExpression(const char *source);
  bool run(const char *source, bool dump = false);
  bool compile(const char *source, bool dump);

  void reset();

  void setHooks(const VMHooks &h);

  void render();

  size_t getTotalAlocated() { return totalAllocated; }
  size_t getTotalClasses() { return totalClasses; }
  size_t getTotalStructs() { return totalStructs; }
  size_t getTotalArrays() { return totalArrays; }
  size_t getTotalMaps() { return totalMaps; }
  size_t getTotalNativeClasses() { return totalNativeClasses; }
  size_t getTotalNativeStructs() { return totalNativeStructs; }

  void killAliveProcess();

  // ProcessExec/Process context (for callbacks from external libraries like GTK)
  ProcessExec* getCurrentExec() { return currentExec(); }
  void setCurrentExec(Process* process) { currentProcess = process; }
  Process* getCurrentProcess() { return currentProcess; }
  void setCurrentProcess(Process* process) { currentProcess = process; }

  uint16 defineModule(const char *name);
  ModuleBuilder addModule(const char *name);
  ModuleDef *getModule(uint16 id);
  bool getModuleId(String *name, uint16 *outId);
  bool getModuleId(const char *name, uint16 *outId);
  bool containsModule(const char *name);

  // Plugin system
  bool loadPlugin(const char *path);                    // Load plugin from specific path
  bool loadPluginByName(const char *name);              // Search and load plugin by name
  void addPluginSearchPath(const char *path);           // Add search path for plugins
  void unloadAllPlugins();                              // Cleanup all loaded plugins
  const char *getLastPluginError() const;               // Get last plugin error message

  void printStack();
  void disassemble();

  int addGlobal(const char *name, Value value);
  bool setGlobal(const char *name, Value value);
  String *addGlobalEx(const char *name, Value value);
  Value getGlobal(const char *name);
  Value getGlobal(uint32 index);
  bool tryGetGlobal(const char *name, Value *value);

  // ===== ARRAY EXTRACTION HELPERS (for native bindings - no allocation) =====
  // Extract values from a BuLang array to a C buffer (stack-allocated by caller)
  // Returns number of elements extracted, or -1 if not an array
  int getFloats(Value v, float* out, int maxCount);
  int getInts(Value v, int* out, int maxCount);
  int getDoubles(Value v, double* out, int maxCount);

  // Convenience for common vector types (returns false if wrong size/type)
  bool getVec2(Value v, float* out);  // extracts 2 floats
  bool getVec3(Value v, float* out);  // extracts 3 floats
  bool getVec4(Value v, float* out);  // extracts 4 floats

  // Matrices (column-major, OpenGL style)
  bool getMat3(Value v, float* out);  // extracts 9 floats (3x3)
  bool getMat4(Value v, float* out);  // extracts 16 floats (4x4)

  // Get array length (returns -1 if not array)
  int getArrayLength(Value v);

  // ===== STACK API   =====
  const Value &peek(int index); // -1 = topo, 0 = base
  void push(Value value);
  Value pop();

  // ===== PUSH HELPERS =====

  void pushInt(int n);
  void pushFloat(float f);
  void pushPointer(void *p);
  void pushByte(uint8 b);
  void pushDouble(double d);
  void pushString(const char *s);
  void pushBool(bool b);
  void pushNil();

  int toInt(int index);
  double toDouble(int index);
  const char *toString(int index);
  bool toBool(int index);

  // ===== STACK INFO =====
  int getTop();
  void setTop(int index);
  bool checkStack(int extra);

  // ===== STACK MANIPULATION =====
  void insert(int index);      // Insere topo no index
  void remove(int index);      // Remove index
  void replace(int index);     // Substitui index pelo topo
  void copy(int from, int to); // Copia from → to
  void rotate(int idx, int n); // Roda n elementos

  // ===== TYPE CHECKING =====
  ValueType getType(int index);
  bool isInt(int index);
  bool isDouble(int index);
  bool isString(int index);
  bool isBool(int index);
  bool isFunction(int index);
  bool isNil(int index);

  // ====== VALUE ====
  Value makeClosure()
  {
    Value v;
    v.type = ValueType::CLOSURE;
    v.as.closure = createClosure();
    return v;
  }

  FORCE_INLINE Value makeClassInstance()
  {
    Value v;
    v.type = ValueType::CLASSINSTANCE;
    v.as.sClass = creatClass();
    return v;
  }

  FORCE_INLINE Value makeNativeClassInstance()
  {
    Value v;
    v.type = ValueType::NATIVECLASSINSTANCE;
    v.as.sClassInstance = createNativeClass(false);  // default: não persistent
    return v;
  }

  FORCE_INLINE Value makeNativeClassInstance(bool persistent)
  {
    Value v;
    v.type = ValueType::NATIVECLASSINSTANCE;
    v.as.sClassInstance = createNativeClass(persistent);
    return v;
  }

  FORCE_INLINE Value makeStructInstance()
  {
    Value v;
    v.type = ValueType::STRUCTINSTANCE;
    v.as.sInstance = createStruct();
    return v;
  }
  FORCE_INLINE Value makeBuffer(int count, int typeRaw)
  {
    Value v;
    v.type = ValueType::BUFFER;
    v.as.buffer = createBuffer(count, typeRaw);
    return v;
  }

  FORCE_INLINE Value makeMap()
  {
    Value v;
    v.type = ValueType::MAP;
    v.as.map = createMap();
    return v;
  }

  FORCE_INLINE Value makeArray()
  {
    Value v;
    v.type = ValueType::ARRAY;
    v.as.array = createArray();
    return v;
  }

  FORCE_INLINE Value makeNativeStructInstance()
  {
    Value v;
    v.type = ValueType::NATIVESTRUCTINSTANCE;
    v.as.sNativeStruct = createNativeStruct(false);  // default: não persistent
    return v;
  }

  FORCE_INLINE Value makeNativeStructInstance(bool persistent)
  {
    Value v;
    v.type = ValueType::NATIVESTRUCTINSTANCE;
    v.as.sNativeStruct = createNativeStruct(persistent);
    return v;
  }
  FORCE_INLINE Value makeString(const char *str)
  {
    Value v;
    v.type = ValueType::STRING;
    v.as.string = createString(str);
    return v;
  }
  FORCE_INLINE Value makeString(String *str)
  {
    Value v;
    v.type = ValueType::STRING;
    v.as.string = str;
    return v;
  }

  FORCE_INLINE Value makeNil()
  {
    Value v;
    v.type = ValueType::NIL;
    return v;
  }

  FORCE_INLINE Value makeInt(int i)
  {
    Value v;
    v.type = ValueType::INT;
    v.as.integer = i;
    return v;
  }

  FORCE_INLINE Value makeUInt(uint32 i)
  {
    Value v;
    v.type = ValueType::UINT;
    v.as.unsignedInteger = i;
    return v;
  }

  FORCE_INLINE Value makeDouble(double d)
  {
    Value v;
    v.type = ValueType::DOUBLE;
    v.as.number = d;
    return v;
  }

  FORCE_INLINE Value makeBool(bool b)
  {
    Value v;
    v.type = ValueType::BOOL;
    v.as.boolean = b;
    return v;
  }

  FORCE_INLINE Value makeFunction(int idx)
  {
    Value v;
    v.type = ValueType::FUNCTION;
    v.as.integer = idx;
    return v;
  }

  FORCE_INLINE Value makeNative(int idx)
  {
    Value v;
    v.type = ValueType::NATIVE;
    v.as.integer = idx;
    return v;
  }

  FORCE_INLINE Value makeNativeProcess(int idx)
  {
    Value v;
    v.type = ValueType::NATIVEPROCESS;
    v.as.integer = idx;
    return v;
  }


  FORCE_INLINE Value makeNativeClass(int idx)
  {
    Value v;
    v.type = ValueType::NATIVECLASS;
    v.as.integer = idx;
    return v;
  }

  FORCE_INLINE Value makeProcess(int idx)
  {
    Value v;
    v.type = ValueType::PROCESS;
    v.as.integer = idx;
    return v;
  }

  FORCE_INLINE Value makeProcessInstance(Process *proc)
  {
    Value v;
    v.type = ValueType::PROCESS_INSTANCE;
    v.as.process = proc;
    return v;
  }

  FORCE_INLINE Value makeStruct(int idx)
  {
    Value v;
    v.type = ValueType::STRUCT;
    v.as.integer = idx;
    return v;
  }

  FORCE_INLINE Value makeClass(int idx)
  {
    Value v;
    v.type = ValueType::CLASS;
    v.as.integer = idx;
    return v;
  }

  FORCE_INLINE Value makePointer(void *pointer)
  {
    Value v;
    v.type = ValueType::POINTER;
    v.as.pointer = pointer;
    return v;
  }

  FORCE_INLINE Value makeNativeStruct(int idx)
  {
    Value v;
    v.type = ValueType::NATIVESTRUCT;
    v.as.integer = idx;
    return v;
  }

  FORCE_INLINE Value makeByte(int idx)
  {
    Value v;
    v.type = ValueType::BYTE;
    v.as.byte = idx;
    return v;
  }

  FORCE_INLINE Value makeFloat(float idx)
  {
    Value v;
    v.type = ValueType::FLOAT;
    v.as.real = idx;
    return v;
  }
  FORCE_INLINE Value makeModuleRef(uint16 moduleId, uint16 funcId)
  {
    Value v;
    v.type = ValueType::MODULEREFERENCE;
    uint32 packed = 0;

    packed |= (moduleId & 0xFFFF) << 16; // 16 bits
    packed |= (funcId & 0xFFFF);         // 16 bits
    v.as.unsignedInteger = packed;
    return v;
  }
};

bool getBuiltinTypedArrayData(const Value &value, const void **outData);
