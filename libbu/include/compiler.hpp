#pragma once

#include "config.hpp"
#include "lexer.hpp"
#include "token.hpp"
#include "types.hpp"
#include "vector.hpp"
#include "set.hpp"
#include <cstring>
#include <set>
#include <string>
#include <vector>
#include <memory>
#include <chrono>

class Code;
struct Value;
class Compiler;
struct Function;
struct CallFrame;
struct ProcessExec;
struct Process;
struct String;
struct ProcessDef;
struct ClassDef;
class Interpreter;

typedef void (Compiler::*ParseFn)(bool canAssign);

enum Precedence
{
  PREC_NONE,
  PREC_ASSIGNMENT,
  PREC_CONDITIONAL, // ?:
  PREC_OR,          // ||
  PREC_AND,         // &&
  PREC_BITWISE_OR,  // |
  PREC_BITWISE_XOR, // ^
  PREC_BITWISE_AND, // &
  PREC_EQUALITY,    // == !=
  PREC_COMPARISON,  // < > <= >=
  PREC_SHIFT,       // << >>
  PREC_TERM,        // + -
  PREC_FACTOR,      // * / %
  PREC_UNARY,       // ! - ~ ++ --
  PREC_CALL,        // ()
  PREC_PRIMARY
};

struct ParseRule
{
  ParseFn prefix;
  ParseFn infix;
  Precedence prec;
};

// ============================================
// LIMITES DE SEGURANÇA UNIFICADOS
// ============================================

#define MAX_IDENTIFIER_LENGTH 255 // Alinhado com lexer

#define MAX_EXPRESSION_DEPTH 200  // Prevenir stack overflow em expressões
#define MAX_DECLARATION_DEPTH 100 // Prevenir recursão infinita
#define MAX_CALL_DEPTH 100        // Limitar calls aninhados
#define MAX_SCOPE_DEPTH 256       // Limitar scopes aninhados
#define MAX_TRY_DEPTH 64          // Limitar try/catch aninhados

#define MAX_LABELS 32
#define MAX_GOTOS 32
#define MAX_GOSUBS 32

#define MAX_LOCALS 1024
#define MAX_LOOP_DEPTH 32
#define MAX_BREAKS_PER_LOOP 256
#define MAX_SWITCH_DEPTH 64

struct Local
{
  std::string name;
  int depth;
  bool usedInitLocal;
  bool isCaptured;

  Local() : depth(-1), usedInitLocal(false), isCaptured(false) {}

  bool equals(const std::string &str) const
  {
    return name == str;
  }

  bool equals(const char *str, size_t len) const
  {
    return name.length() == len && std::memcmp(name.c_str(), str, len) == 0;
  }
};

struct UpvalueInfo
{
  uint8 index;
  bool isLocal;
};

struct LoopContext
{
  int loopStart;
  int breakJumps[MAX_BREAKS_PER_LOOP];
  int breakCount;
  int scopeDepth;
  bool isForeach;

  LoopContext() : loopStart(0), breakCount(0), scopeDepth(0), isForeach(false) {}

  bool addBreak(int jump)
  {
    if (breakCount >= MAX_BREAKS_PER_LOOP)
    {
      return false;
    }
    breakJumps[breakCount++] = jump;
    return true;
  }
};

struct Label
{
  std::string name;
  int offset;
};

struct GotoJump
{
  std::string target;
  int jumpOffset;
};

// ============================================
// OPÇÕES DE COMPILAÇÃO
// ============================================

struct CompilerOptions
{
  bool strictMode = true;
  bool allowUnsafeCode = false;

  // Limites de tamanho
  size_t maxSourceSize = 1024 * 1024; // 1MB
  size_t maxTokens = 100000;
  size_t maxFunctions = 10000;
  size_t maxConstants = 65535;

  // Timeouts
  std::chrono::milliseconds compileTimeout{5000};

  // Validação
  bool validateUnicode = true;
  bool checkIntegerOverflow = true;
};

// ============================================
// COMPILER CLASS (HARDENED)
// ============================================

class Compiler
{
public:
  Compiler(Interpreter *vm);
  ~Compiler();

  void setFileLoader(FileLoaderCallback loader, void *userdata = nullptr);
  void setOptions(const CompilerOptions &opts) { options = opts; }

  ProcessDef *compile(const std::string &source);
  ProcessDef *compileExpression(const std::string &source);
  
  const std::vector<std::string>& getGlobalIndexToName() const { return globalIndexToName_; }

  void clear();

  // Estatísticas para debugging
  struct Stats
  {
    size_t maxExpressionDepth = 0;
    size_t maxScopeDepth = 0;
    size_t totalErrors = 0;
    size_t totalWarnings = 0;
    std::chrono::milliseconds compileTime{0};
  };

  Stats getStats() const { return stats; }

private:
  Interpreter *vm_;
  Lexer *lexer;
  Token current;
  Token previous;
  Token next;

  int cursor;

  FunctionType currentFunctionType;
  Function *function;
  Code *currentChunk;

  ClassDef *currentClass;
  ProcessDef *currentProcess;
  Vector<String *> argNames;
  std::vector<Token> tokens;

  bool hadError;
  bool panicMode;

  int expressionDepth;
  int declarationDepth;
  int callDepth;
  int scopeDepth;
  int tryDepth;
  int loopDepth_;
  int switchDepth_;
  int switchLoopDepthStack_[MAX_SWITCH_DEPTH];

  Local locals_[MAX_LOCALS];
  int localCount_;

  LoopContext loopContexts_[MAX_LOOP_DEPTH];
  bool isProcess_;

  struct EnclosingContext
  {
    Function *function;
    std::vector<Local> locals;
  };

  std::vector<EnclosingContext> enclosingStack_;
  int upvalueCount_;
  UpvalueInfo upvalues_[MAX_LOCALS];

  int resolveUpvalue(Token &name);
  int addUpvalue(uint8 index, bool isLocal);

  std::vector<Label> labels;
  std::vector<GotoJump> pendingGotos;
  std::vector<GotoJump> pendingGosubs;

  CompilerOptions options;
  Stats stats;
  std::chrono::steady_clock::time_point compileStartTime;

  std::vector<std::string> errors;
  std::vector<std::string> warnings;
  std::set<std::string> declaredGlobals_;  // Track declared global variable names

  // Global variable indexing for optimization
  std::unordered_map<std::string, uint16> globalIndices_;  // Map global name -> index
  std::vector<std::string> globalIndexToName_;  // Map index -> name (for debug messages)
  uint16 nextGlobalIndex_ = 0;  // Next available global index
  
  uint16 getOrCreateGlobalIndex(const std::string& name);  // Get or assign global index

  // Token management
  void advance();
  Token peek(int offset = 0);

  bool checkNext(TokenType t);

  bool check(TokenType type);
  bool match(TokenType type);
  void consume(TokenType type, const char *message);

  // Helper para aceitar keywords como identificadores (campos, propriedades)
  bool isKeywordToken(TokenType type);
  void consumeIdentifierLike(const char *message);

  void beginLoop(int loopStart, bool isForeach = false);
  void endLoop();
  void emitBreak();
  void pushScope();
  void popScope();
  void emitContinue();
  int discardLocals(int depth);
  void breakStatement();
  void continueStatement();

  void error(const char *message);
  void errorAt(Token &token, const char *message);
  void errorAtCurrent(const char *message);
  void fail(const char *format, ...);
  void synchronize();

  void warning(const char *message);
  void warningAt(Token &token, const char *message);

  bool checkExpressionDepth();
  bool checkDeclarationDepth();
  bool checkCallDepth();
  bool checkScopeDepth();
  bool checkTryDepth();
  bool checkLabelCount();
  bool checkGotoCount();
  bool checkCompileTimeout();

  // Bytecode emission
  void emitByte(uint8 byte);
  void emitBytes(uint8 byte1, uint8 byte2);
  void emitShort(uint16 value);
  void emitDiscard(uint8 count);
  void emitReturn();
  void emitConstant(Value value);
  uint16 makeConstant(Value value);

  int emitJump(uint8 instruction);
  void patchJump(int offset);

  void emitLoop(int loopStart);

  // Pratt parser
  void expression();
  void parsePrecedence(Precedence precedence);
  ParseRule *getRule(TokenType type);

  void validateIdentifierName(const Token &nameToken);

  // Parse functions (prefix)
  void number(bool canAssign);
  void string(bool canAssign);
  void literal(bool canAssign);
  void grouping(bool canAssign);
  void unary(bool canAssign);
  void variable(bool canAssign);
  void lengthExpression(bool canAssign);
  void freeExpression(bool canAssign);
  void mathUnary(bool canAssign);
  void mathBinary(bool canAssign);
  void expressionClock(bool canAssign);
  void typeExpression(bool canAssign);
  void procExpression(bool canAssign);
  void getIdExpression(bool canAssign);

  // Parse functions (infix)
  void binary(bool canAssign);
  void ternary(bool canAssign);
  void and_(bool canAssign);
  void or_(bool canAssign);
  void call(bool canAssign);

  // Statements
  void declaration();
  void statement();
  void varDeclaration();
  void funDeclaration();
  void processDeclaration();
  void expressionStatement();
  void printStatement();
  void ifStatement();
  void whileStatement();
  void doWhileStatement();
  void loopStatement();
  void switchStatement();
  void forStatement();
  void foreachStatement();
  void returnStatement();
  void block();

  void tryStatement();
  void throwStatement();

  void dot(bool canAssign);
  void self(bool canAssign);
  void super(bool canAssign);

  void labelStatement();
  void gotoStatement();
  void gosubStatement();
  void resolveGotos();
  void resolveGosubs();
  void emitGosubTo(int targetOffset);
  void patchJumpTo(int operandOffset, int targetOffset);

  void emitVarOp(uint8 op, int arg);
  void handle_assignment(uint8 getOp, uint8 setOp, int arg, bool canAssign);

  void prefixIncrement(bool canAssign);
  void prefixDecrement(bool canAssign);

  // Variables
  uint16 identifierConstant(Token &name);
  void namedVariable(Token &name, bool canAssign);
  void defineVariable(uint16 global);
  void declareVariable();
  void addLocal(Token &name);
  int resolveLocal(Token &name);
  void markInitialized();

  uint8 argumentList();

  void compileFunction(Function *func, bool isProcess);
  void compileProcess(const std::string &name);

  bool isProcessFunction(const char *name) const;

  void structDeclaration();
  void arrayLiteral(bool canAssign);
  void subscript(bool canAssign);
  void mapLiteral(bool canAssign);
  void bufferLiteral(bool canAssign);

  void classDeclaration();
  void method(ClassDef *classDef);

  // Scope
  void beginScope();
  void endScope();

  bool inProcessFunction() const;

  void initRules();
  void predeclareProcessGlobals();
  bool enterSwitchContext();
  void leaveSwitchContext();
  void recoverToCurrentSwitchEnd();

  void frameStatement();
  void exitStatement();

  void includeStatement();
  void parseImport();
  void parseUsing();
  void parseRequire();

  FileLoaderCallback fileLoader = nullptr;
  void *fileLoaderUserdata = nullptr;
  std::set<std::string> includedFiles;
  std::set<std::string> importedModules;
  std::set<std::string> usingModules;

  static ParseRule rules[TOKEN_COUNT];
};

inline bool Compiler::checkExpressionDepth()
{
  if (expressionDepth >= MAX_EXPRESSION_DEPTH)
  {
    error("Expression nested too deeply");
    return false;
  }
  return true;
}

inline bool Compiler::checkDeclarationDepth()
{
  if (declarationDepth >= MAX_DECLARATION_DEPTH)
  {
    error("Declarations nested too deeply");
    return false;
  }
  return true;
}

inline bool Compiler::checkCallDepth()
{
  if (callDepth >= MAX_CALL_DEPTH)
  {
    error("Function calls nested too deeply");
    return false;
  }
  return true;
}

inline bool Compiler::checkScopeDepth()
{
  if (scopeDepth >= MAX_SCOPE_DEPTH)
  {
    error("Scopes nested too deeply");
    return false;
  }
  return true;
}

inline bool Compiler::checkTryDepth()
{
  if (tryDepth >= MAX_TRY_DEPTH)
  {
    error("Try blocks nested too deeply");
    return false;
  }
  return true;
}

inline bool Compiler::checkLabelCount()
{
  if (labels.size() >= MAX_LABELS)
  {
    error("Too many labels in function");
    return false;
  }
  return true;
}

inline bool Compiler::checkGotoCount()
{
  if (pendingGotos.size() >= MAX_GOTOS)
  {
    error("Too many goto statements");
    return false;
  }
  return true;
}

inline bool Compiler::checkCompileTimeout()
{
  auto now = std::chrono::steady_clock::now();
  auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - compileStartTime);

  if (elapsed > options.compileTimeout)
  {
    error("Compilation timeout exceeded");
    return false;
  }
  return true;
}
