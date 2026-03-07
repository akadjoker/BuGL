#include "compiler.hpp"
#include "code.hpp"
#include "interpreter.hpp"
#include "platform.hpp"
#include "opcode.hpp"
#include "pool.hpp"
#include "value.hpp"
#include <cstdio>
#include <cstdlib>
#include <stdarg.h>

// ============================================
// PARSE RULE TABLE - DEFINIÇÃO
// ============================================
constexpr ParseRule makeRule(ParseFn prefix, ParseFn infix, Precedence prec)
{
  return {prefix, infix, prec};
}

ParseRule Compiler::rules[TOKEN_COUNT];

// ============================================
// CONSTRUCTOR
// ============================================

Compiler::Compiler(Interpreter *vm)
    : vm_(vm), lexer(nullptr),
      function(nullptr), currentChunk(nullptr),
      currentProcess(nullptr), hadError(false),
      panicMode(false), scopeDepth(0), localCount_(0), loopDepth_(0),
      switchDepth_(0), isProcess_(false), tryDepth(0),
      expressionDepth(0), declarationDepth(0), callDepth(0),
      upvalueCount_(0)
{
  initRules();
  cursor = 0;
}

Compiler::~Compiler()
{
  delete lexer;
}

// ============================================
// GLOBAL VARIABLE INDEXING
// ============================================

uint16 Compiler::getOrCreateGlobalIndex(const std::string &name)
{
  auto it = globalIndices_.find(name);
  if (it != globalIndices_.end())
  {
    return it->second; // Already indexed
  }

  // Create new index
  uint16 index = nextGlobalIndex_++;
  globalIndices_[name] = index;

  // Ensure reverse mapping array is large enough
  if (index >= globalIndexToName_.size())
  {
    globalIndexToName_.resize(index + 1);
  }
  globalIndexToName_[index] = name;

  return index;
}

// ============================================
// INICIALIZAÇÃO DA TABELA
// ============================================

void Compiler::initRules()
{

  for (int i = 0; i < TOKEN_COUNT; i++)
  {
    rules[i] = {nullptr, nullptr, PREC_NONE};
  }

  // Agora define os que têm funções
  rules[TOKEN_LPAREN] = {&Compiler::grouping, &Compiler::call, PREC_CALL};
  rules[TOKEN_RPAREN] = {nullptr, nullptr, PREC_NONE};
  rules[TOKEN_LBRACE] = {nullptr, nullptr, PREC_NONE};
  rules[TOKEN_RBRACE] = {nullptr, nullptr, PREC_NONE};
  rules[TOKEN_COMMA] = {nullptr, nullptr, PREC_NONE};
  rules[TOKEN_SEMICOLON] = {nullptr, nullptr, PREC_NONE};

  rules[TOKEN_DOT] = {nullptr, &Compiler::dot, PREC_CALL};
  rules[TOKEN_SELF] = {&Compiler::self, nullptr, PREC_NONE};
  rules[TOKEN_SUPER] = {&Compiler::super, nullptr, PREC_NONE};

  // Arithmetic
  rules[TOKEN_PLUS] = {nullptr, &Compiler::binary, PREC_TERM};
  rules[TOKEN_MINUS] = {&Compiler::unary, &Compiler::binary, PREC_TERM};
  rules[TOKEN_STAR] = {nullptr, &Compiler::binary, PREC_FACTOR};
  rules[TOKEN_SLASH] = {nullptr, &Compiler::binary, PREC_FACTOR};
  rules[TOKEN_PERCENT] = {nullptr, &Compiler::binary, PREC_FACTOR};

  // Comparison
  rules[TOKEN_EQUAL] = {nullptr, nullptr, PREC_NONE};
  rules[TOKEN_EQUAL_EQUAL] = {nullptr, &Compiler::binary, PREC_EQUALITY};
  rules[TOKEN_BANG_EQUAL] = {nullptr, &Compiler::binary, PREC_EQUALITY};
  rules[TOKEN_LESS] = {nullptr, &Compiler::binary, PREC_COMPARISON};
  rules[TOKEN_LESS_EQUAL] = {nullptr, &Compiler::binary, PREC_COMPARISON};
  rules[TOKEN_GREATER] = {nullptr, &Compiler::binary, PREC_COMPARISON};
  rules[TOKEN_GREATER_EQUAL] = {nullptr, &Compiler::binary, PREC_COMPARISON};

  rules[TOKEN_PLUS_PLUS] = {&Compiler::prefixIncrement, nullptr, PREC_NONE};
  rules[TOKEN_MINUS_MINUS] = {&Compiler::prefixDecrement, nullptr, PREC_NONE};

  // Logical
  rules[TOKEN_QUESTION] = {nullptr, &Compiler::ternary, PREC_CONDITIONAL};
  rules[TOKEN_AND_AND] = {nullptr, &Compiler::and_, PREC_AND};
  rules[TOKEN_OR_OR] = {nullptr, &Compiler::or_, PREC_OR};
  rules[TOKEN_BANG] = {&Compiler::unary, nullptr, PREC_NONE};

  rules[TOKEN_PIPE] = {nullptr, &Compiler::binary, PREC_BITWISE_OR};
  rules[TOKEN_CARET] = {nullptr, &Compiler::binary, PREC_BITWISE_XOR};
  rules[TOKEN_AMPERSAND] = {nullptr, &Compiler::binary, PREC_BITWISE_AND};
  rules[TOKEN_TILDE] = {&Compiler::unary, nullptr, PREC_NONE};
  rules[TOKEN_LEFT_SHIFT] = {nullptr, &Compiler::binary, PREC_SHIFT};
  rules[TOKEN_RIGHT_SHIFT] = {nullptr, &Compiler::binary, PREC_SHIFT};

  // Literals
  rules[TOKEN_INT] = {&Compiler::number, nullptr, PREC_NONE};
  rules[TOKEN_FLOAT] = {&Compiler::number, nullptr, PREC_NONE};
  rules[TOKEN_STRING] = {&Compiler::string, nullptr, PREC_NONE};
  rules[TOKEN_IDENTIFIER] = {&Compiler::variable, nullptr, PREC_NONE};
  rules[TOKEN_TRUE] = {&Compiler::literal, nullptr, PREC_NONE};
  rules[TOKEN_FALSE] = {&Compiler::literal, nullptr, PREC_NONE};
  rules[TOKEN_NIL] = {&Compiler::literal, nullptr, PREC_NONE};

  // math
  // MATH UNARY (Funções de 1 argumento)
  rules[TOKEN_SIN] = {&Compiler::mathUnary, nullptr, PREC_NONE};
  rules[TOKEN_COS] = {&Compiler::mathUnary, nullptr, PREC_NONE};
  rules[TOKEN_TAN] = {&Compiler::mathUnary, nullptr, PREC_NONE};
  rules[TOKEN_ASIN] = {&Compiler::mathUnary, nullptr, PREC_NONE};
  rules[TOKEN_ACOS] = {&Compiler::mathUnary, nullptr, PREC_NONE};
  rules[TOKEN_ATAN] = {&Compiler::mathUnary, nullptr, PREC_NONE};
  rules[TOKEN_SQRT] = {&Compiler::mathUnary, nullptr, PREC_NONE};
  rules[TOKEN_ABS] = {&Compiler::mathUnary, nullptr, PREC_NONE};
  rules[TOKEN_FLOOR] = {&Compiler::mathUnary, nullptr, PREC_NONE};
  rules[TOKEN_CEIL] = {&Compiler::mathUnary, nullptr, PREC_NONE};
  rules[TOKEN_DEG] = {&Compiler::mathUnary, nullptr, PREC_NONE};
  rules[TOKEN_RAD] = {&Compiler::mathUnary, nullptr, PREC_NONE};
  rules[TOKEN_LOG] = {&Compiler::mathUnary, nullptr, PREC_NONE};
  rules[TOKEN_EXP] = {&Compiler::mathUnary, nullptr, PREC_NONE};

  // MATH BINARY (Funções de 2 argumentos)
  rules[TOKEN_ATAN2] = {&Compiler::mathBinary, nullptr, PREC_NONE};
  rules[TOKEN_POW] = {&Compiler::mathBinary, nullptr, PREC_NONE};

  rules[TOKEN_CLOCK] = {&Compiler::expressionClock, nullptr, PREC_NONE};
  rules[TOKEN_TYPE] = {&Compiler::typeExpression, nullptr, PREC_NONE};
  rules[TOKEN_PROC] = {&Compiler::procExpression, nullptr, PREC_NONE};
  rules[TOKEN_GET_ID] = {&Compiler::getIdExpression, nullptr, PREC_NONE};

  rules[TOKEN_FOREACH] = {nullptr, nullptr, PREC_NONE};

  rules[TOKEN_LBRACKET] = {
      &Compiler::arrayLiteral, //  PREFIX: [1, 2, 3]
      &Compiler::subscript,    //  INFIX: arr[i]
      PREC_CALL                //  Mesma precedência que . e ()
  };

  rules[TOKEN_AT] = {
      &Compiler::bufferLiteral, // PREFIX
      nullptr,                  // INFIX
      PREC_NONE};

  rules[TOKEN_FREE] = {
      &Compiler::freeExpression,
      nullptr,
      PREC_NONE};

  rules[TOKEN_LEN] = {
      &Compiler::lengthExpression, // PREFIX
      nullptr,                     // INFIX
      PREC_NONE};

  rules[TOKEN_LBRACE] = {&Compiler::mapLiteral, //  PREFIX: {key: value}
                         nullptr,               //  Sem INFIX
                         PREC_NONE};

  // Keywords (all nullptr já setados no loop)
  rules[TOKEN_EOF] = {nullptr, nullptr, PREC_NONE};
  rules[TOKEN_ERROR] = {nullptr, nullptr, PREC_NONE};
}

void Compiler::predeclareProcessGlobals()
{
  if (tokens.empty())
    return;

  for (size_t i = 0; i + 1 < tokens.size(); i++)
  {
    if (tokens[i].type != TOKEN_PROCESS)
      continue;

    const Token &nameTok = tokens[i + 1];
    if (nameTok.type != TOKEN_IDENTIFIER)
      continue;

    // Make forward process calls resolvable regardless of declaration order.
    declaredGlobals_.insert(nameTok.lexeme);
    getOrCreateGlobalIndex(nameTok.lexeme);
  }
}

bool Compiler::enterSwitchContext()
{
  if (switchDepth_ >= MAX_SWITCH_DEPTH)
  {
    error("Switch nested too deeply");
    return false;
  }

  switchLoopDepthStack_[switchDepth_] = loopDepth_;
  switchDepth_++;
  return true;
}

void Compiler::leaveSwitchContext()
{
  if (switchDepth_ > 0)
    switchDepth_--;
}

void Compiler::recoverToCurrentSwitchEnd()
{
  // Local recovery: consume everything until the matching '}' of current switch.
  int braceDepth = 1;
  while (!check(TOKEN_EOF) && braceDepth > 0)
  {
    if (check(TOKEN_LBRACE))
    {
      braceDepth++;
    }
    else if (check(TOKEN_RBRACE))
    {
      braceDepth--;
    }
    advance();
  }
}

// ============================================
// MAIN ENTRY POINT
// ============================================

void Compiler::setFileLoader(FileLoaderCallback loader, void *userdata)
{
  fileLoader = loader;
  fileLoaderUserdata = userdata;
}

ProcessDef *Compiler::compile(const std::string &source)
{
  delete lexer;
  lexer = new Lexer(source);
  stats.maxExpressionDepth = 0;
  stats.maxScopeDepth = 0;
  stats.totalErrors = 0;
  stats.totalWarnings = 0;
  enclosingStack_.clear();
  declaredGlobals_.clear();
  upvalueCount_ = 0;
  isProcess_ = true; // Top-level code IS a process
  switchDepth_ = 0;


  globalIndices_.clear();
  globalIndexToName_.clear();

  // Copy actual indices from VM's nativeGlobalIndices map
  // This ensures compiler uses the same indices as runtime
  vm_->nativeGlobalIndices.forEach([this](String *nameStr, uint16 index)
                                   {
    const std::string name = nameStr->chars();
    globalIndices_[name] = index;
    if (index >= globalIndexToName_.size())
    {
      globalIndexToName_.resize(index + 1);
    }
    globalIndexToName_[index] = name;
    declaredGlobals_.insert(name); });

  // Também importar globals registados via addGlobal()
  for (size_t i = 0; i < vm_->globalIndexToName_.size(); i++)
  {
    String *nameStr = vm_->globalIndexToName_[i];
    if (!nameStr)
      continue;
    const std::string name = nameStr->chars();
    if (globalIndices_.find(name) == globalIndices_.end())
    {
      globalIndices_[name] = (uint16)i;
      if (i >= globalIndexToName_.size())
      {
        globalIndexToName_.resize(i + 1);
      }
      globalIndexToName_[i] = name;
      declaredGlobals_.insert(name);
    }
  }


  nextGlobalIndex_ = static_cast<uint16>(vm_->globalsArray.size());

  compileStartTime = std::chrono::steady_clock::now();

  tokens = lexer->scanAll();

  if (tokens.empty())
  {
    error("Empty source");

    return nullptr;
  }

  predeclareProcessGlobals();

  function = vm_->addFunction("__main__", 0);
  if (!function)
  {
    error("Fail to create main function");
    return nullptr;
  }

  currentChunk = function->chunk;
  currentFunctionType = FunctionType::TYPE_SCRIPT;
  currentClass = nullptr;

  advance();

  while (!match(TOKEN_EOF) && !hadError)
  {
    declaration();
  }

  currentProcess = vm_->addProcess("__main_process__", function);

  if (!currentProcess)
  {
    error("Fail to create main process");
    return nullptr;
  }

  emitReturn();

  if (hadError)
  {
    return nullptr;
  }

  currentProcess->finalize();

  importedModules.clear();
  usingModules.clear();

  auto endTime = std::chrono::steady_clock::now();
  stats.compileTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - compileStartTime);

  // Info("Compilation Statistics:");
  // Info("  Max Expression Depth: %zu", stats.maxExpressionDepth);
  // Info("  Max Scope Depth: %zu", stats.maxScopeDepth);
  // Info("  Total Errors: %zu", stats.totalErrors);
  // Info("  Total Warnings: %zu", stats.totalWarnings);
  // Info("  Compile Time: %lld ms", stats.compileTime.count());

  return currentProcess;
}

ProcessDef *Compiler::compileExpression(const std::string &source)
{
  delete lexer;
  stats.maxExpressionDepth = 0;
  stats.maxScopeDepth = 0;
  stats.totalErrors = 0;
  stats.totalWarnings = 0;
  isProcess_ = true; // Expression compilation IS a process
  upvalueCount_ = 0;
  switchDepth_ = 0;
  lexer = new Lexer(source);

  compileStartTime = std::chrono::steady_clock::now();
  tokens = lexer->scanAll();

  function = vm_->addFunction("__expr__", 0);
  currentChunk = function->chunk;

  currentProcess = vm_->addProcess("__main__", function);
  currentFunctionType = FunctionType::TYPE_SCRIPT;
  currentClass = nullptr;
  enclosingStack_.clear();
  declaredGlobals_.clear();
  globalIndices_.clear();
  globalIndexToName_.clear();

  // Use actual indices from VM's nativeGlobalIndices map
  vm_->nativeGlobalIndices.forEach([this](String *nameStr, uint16 index)
                                   {
    const std::string name = nameStr->chars();
    globalIndices_[name] = index;
    if (index >= globalIndexToName_.size())
    {
      globalIndexToName_.resize(index + 1);
    }
    globalIndexToName_[index] = name;
    declaredGlobals_.insert(name); });

  // Também importar globals registados via addGlobal()
  for (size_t i = 0; i < vm_->globalIndexToName_.size(); i++)
  {
    String *nameStr = vm_->globalIndexToName_[i];
    if (!nameStr)
      continue;
    const std::string name = nameStr->chars();
    if (globalIndices_.find(name) == globalIndices_.end())
    {
      globalIndices_[name] = (uint16)i;
      if (i >= globalIndexToName_.size())
      {
        globalIndexToName_.resize(i + 1);
      }
      globalIndexToName_[i] = name;
      declaredGlobals_.insert(name);
    }
  }

  // Next global index starts after all registered natives
  nextGlobalIndex_ = static_cast<uint16>(vm_->globalsArray.size());

  advance();

  if (check(TOKEN_EOF))
  {
    error("Empty expression");
    function = nullptr;
    return nullptr;
  }

  expression();
  consume(TOKEN_EOF, "Expect end of expression");

  emitByte(OP_RETURN);

  if (hadError)
  {
    return nullptr;
  }


  currentProcess->finalize();

  importedModules.clear();
  usingModules.clear();
  auto endTime = std::chrono::steady_clock::now();
  stats.compileTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - compileStartTime);

  return currentProcess;
}

void Compiler::clear()
{

  tokens.clear();
  delete lexer;
  lexer = nullptr;
  function = nullptr;
  currentChunk = nullptr;

  currentProcess = nullptr;
  currentClass = nullptr;
  hadError = false;
  importedModules.clear();
  usingModules.clear();
  tryDepth = 0;
  panicMode = false;
  scopeDepth = 0;
  localCount_ = 0;
  loopDepth_ = 0;
  switchDepth_ = 0;
  cursor = 0;
  currentFunctionType = FunctionType::TYPE_SCRIPT;
}

void Compiler::warning(const char *message)
{
  warningAt(previous, message);
}

void Compiler::warningAt(Token &token, const char *message)
{
  OsPrintf("[line %d] Warning", token.line);

  if (token.type == TOKEN_EOF)
  {
    OsPrintf(" at end");
  }
  else if (token.type != TOKEN_ERROR)
  {
    OsPrintf(" at '%s'", token.lexeme.c_str());
  }

  OsPrintf(": %s\n", message);
  stats.totalWarnings++;
}

int Compiler::resolveUpvalue(Token &name)
{
  // Se não há função pai, não pode ser upvalue
  if (enclosingStack_.empty())
    return -1;

  // Procura em TODOS os níveis (de baixo para cima)
  for (int level = enclosingStack_.size() - 1; level >= 0; level--)
  {
    // Procura nos locals desse nível
    for (int i = enclosingStack_[level].locals.size() - 1; i >= 0; i--)
    {
      if (enclosingStack_[level].locals[i].name == name.lexeme)
      {
        // Marca como capturado
        enclosingStack_[level].locals[i].isCaptured = true;

        // Se é do pai imediato: isLocal=true
        // Se é de mais acima: isLocal=false
        bool isLocal = (level == (int)enclosingStack_.size() - 1);
        return addUpvalue(i, isLocal);
      }
    }
  }

  return -1;
}

int Compiler::addUpvalue(uint8 index, bool isLocal)
{
  for (int i = 0; i < upvalueCount_; i++)
  {
    if (upvalues_[i].index == index && upvalues_[i].isLocal == isLocal)
    {
      return i;
    }
  }
  if (upvalueCount_ >= MAX_LOCALS)
  {
    error("Too many closure variables in function");
    return 0;
  }

  upvalues_[upvalueCount_].isLocal = isLocal;
  upvalues_[upvalueCount_].index = index;

  return upvalueCount_++;
}

// ============================================
// TOKEN MANAGEMENT
// ============================================

void Compiler::advance()
{

  previous = current;
  if (cursor >= (int)tokens.size())
  {
    current.type = TOKEN_EOF;
    return;
  }

  current = tokens[cursor++];
}

Token Compiler::peek(int offset)
{
  if (tokens.empty())
  {
    Token eof;
    eof.type = TOKEN_EOF;
    eof.lexeme = "";
    eof.line = 1;
    return eof;
  }

  size_t index = cursor + offset;
  if (index >= tokens.size())
  {
    return tokens.back();
  }
  return tokens[index];
}

bool Compiler::checkNext(TokenType t) { return peek(0).type == t; }

bool Compiler::check(TokenType type) { return current.type == type; }

bool Compiler::match(TokenType type)
{
  if (!check(type))
    return false;
  advance();
  return true;
}

void Compiler::consume(TokenType type, const char *message)
{
  if (current.type == type)
  {
    advance();
    return;
  }

  errorAtCurrent(message);
}

bool Compiler::isKeywordToken(TokenType type)
{
  // Keywords que podem ser usadas como nomes de campos/propriedades
  switch (type)
  {
  // Control flow
  case TOKEN_VAR:
  case TOKEN_DEF:
  case TOKEN_IF:
  case TOKEN_ELIF:
  case TOKEN_ELSE:
  case TOKEN_WHILE:
  case TOKEN_FOR:
  case TOKEN_FOREACH:
  case TOKEN_IN:
  case TOKEN_RETURN:
  case TOKEN_BREAK:
  case TOKEN_CONTINUE:
  case TOKEN_DO:
  case TOKEN_LOOP:
  case TOKEN_SWITCH:
  case TOKEN_CASE:
  case TOKEN_DEFAULT:
  // Built-ins
  case TOKEN_PRINT:
  case TOKEN_PROCESS:
  case TOKEN_TYPE:
  case TOKEN_PROC:
  case TOKEN_GET_ID:
  case TOKEN_FRAME:
  case TOKEN_EXIT:
  case TOKEN_LEN:
  case TOKEN_FREE:
  // OOP
  case TOKEN_STRUCT:
  case TOKEN_ENUM:
  case TOKEN_CLASS:
  case TOKEN_SELF:
  case TOKEN_THIS:
  case TOKEN_SUPER:
  // Modules
  case TOKEN_INCLUDE:
  case TOKEN_IMPORT:
  case TOKEN_USING:
  case TOKEN_REQUIRE:
  // Exceptions
  case TOKEN_TRY:
  case TOKEN_CATCH:
  case TOKEN_FINALLY:
  case TOKEN_THROW:
  // Math
  case TOKEN_SIN:
  case TOKEN_COS:
  case TOKEN_SQRT:
  case TOKEN_ABS:
  case TOKEN_FLOOR:
  case TOKEN_CEIL:
  case TOKEN_DEG:
  case TOKEN_RAD:
  case TOKEN_TAN:
  case TOKEN_ATAN:
  case TOKEN_ATAN2:
  case TOKEN_POW:
  case TOKEN_LOG:
  case TOKEN_EXP:
  // Array
  case TOKEN_PUSH:
  // Timer
  case TOKEN_CLOCK:
  case TOKEN_TIME:
  // Labels
  case TOKEN_LABEL:
  case TOKEN_GOTO:
  case TOKEN_GOSUB:
  // Literals (podem ser usados como nomes)
  case TOKEN_TRUE:
  case TOKEN_FALSE:
  case TOKEN_NIL:
    return true;
  default:
    return false;
  }
}

void Compiler::consumeIdentifierLike(const char *message)
{
  // Aceita identifier OU keyword como nome de campo/propriedade
  if (current.type == TOKEN_IDENTIFIER || isKeywordToken(current.type))
  {
    advance();
    return;
  }
  errorAtCurrent(message);
}

// ============================================
// ERROR HANDLING
// ============================================

void Compiler::fail(const char *fmt, ...)
{
  char buffer[1024];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buffer, sizeof(buffer), fmt, args);
  va_end(args);

  errorAt(previous, buffer);
}

void Compiler::error(const char *message) { errorAt(previous, message); }

void Compiler::errorAt(Token &token, const char *message)
{
  if (panicMode)
    return;
  panicMode = true;

  OsPrintf("[line %d] Error", token.line);

  if (token.type == TOKEN_EOF)
  {
    OsPrintf(" at end");
  }
  else if (token.type == TOKEN_ERROR)
  {
    // Nothing
  }
  else
  {
    OsPrintf(" at '%s'", token.lexeme.c_str());
  }

  OsPrintf(": %s\n", message);
  hadError = true;
  stats.totalErrors++;
}

void Compiler::errorAtCurrent(const char *message)
{
  errorAt(current, message);
}

void Compiler::synchronize()
{
  panicMode = false;

  while (current.type != TOKEN_EOF)
  {
    if (previous.type == TOKEN_SEMICOLON)
      return;

    switch (current.type)
    {
    //  TOP-LEVEL DECLARATIONS
    case TOKEN_IMPORT:
    case TOKEN_USING:
    case TOKEN_INCLUDE:
    case TOKEN_DEF:
    case TOKEN_PROCESS:
    case TOKEN_CLASS:
    case TOKEN_STRUCT:
    case TOKEN_VAR:

    //  CONTROL FLOW STATEMENTS
    case TOKEN_IF:
    case TOKEN_WHILE:
    case TOKEN_DO:
    case TOKEN_LOOP:
    case TOKEN_FOR:
    case TOKEN_SWITCH:

    //  JUMP STATEMENTS
    case TOKEN_BREAK:
    case TOKEN_CONTINUE:
    case TOKEN_RETURN:
    case TOKEN_GOTO:
    case TOKEN_GOSUB:

    //  SPECIAL STATEMENTS
    case TOKEN_PRINT:
    case TOKEN_FRAME:
    case TOKEN_EXIT:
      return;

    default:; // Nothing
    }

    advance();
  }
}

// ============================================
// BYTECODE EMISSION
// ============================================

void Compiler::emitByte(uint8 byte)
{
  currentChunk->write(byte, previous.line);
}

void Compiler::emitBytes(uint8 byte1, uint8 byte2)
{
  emitByte(byte1);
  emitByte(byte2);
}

void Compiler::emitShort(uint16 value)
{
  emitByte((value >> 8) & 0xFF);
  emitByte(value & 0xFF);
}

void Compiler::emitDiscard(uint8 count)
{
  emitByte(OP_DISCARD);
  emitByte(count);
}

void Compiler::emitReturn()
{
  emitByte(OP_NIL);
  emitByte(OP_RETURN);
}

uint16 Compiler::makeConstant(Value value)
{
  if (hadError)
    return 0;

  int constant = currentChunk->addConstant(value);
  if (constant > UINT16_MAX)
  {
    error("Function too large (>65536 constants)");
    hadError = true;
    return 0;
  }

  return (uint16)constant;
}

void Compiler::emitConstant(Value value)
{
  uint16 constant = makeConstant(value);
  if (hadError)
    return;
  emitByte(OP_CONSTANT);
  emitShort(constant);
}

// ============================================
// JUMPS
// ============================================

int Compiler::emitJump(uint8 instruction)
{
  emitByte(instruction);
  emitByte(0xff);
  emitByte(0xff);
  return currentChunk->count - 2;
}

void Compiler::patchJump(int offset)
{
  int jump = currentChunk->count - offset - 2;

  if (jump > UINT16_MAX)
  {
    error("Too much code to jump over");
  }

  currentChunk->code[offset] = (jump >> 8) & 0xff;
  currentChunk->code[offset + 1] = jump & 0xff;
}

void Compiler::emitLoop(int loopStart)
{
  emitByte(OP_LOOP);

  int offset = currentChunk->count - loopStart + 2;
  if (offset > UINT16_MAX)
  {
    error("Loop body too large");
  }

  emitByte((offset >> 8) & 0xff);
  emitByte(offset & 0xff);
}

void Compiler::patchJumpTo(int operandOffset, int targetOffset)
{
  int jump = targetOffset - (operandOffset + 2); // target - after operand
  if (jump < 0)
  {
    error("Backward goto must use OP_LOOP");
    return;
  }
  if (jump > UINT16_MAX)
  {
    error("Jump distance too large");
    return;
  }

  currentChunk->code[operandOffset] = (jump >> 8) & 0xff;
  currentChunk->code[operandOffset + 1] = jump & 0xff;
}

void Compiler::emitGosubTo(int targetOffset)
{
  emitByte(OP_GOSUB);
  int from = (int)currentChunk->count + 2;
  int delta = targetOffset - from;
  if (delta < -32768 || delta > 32767)
  {
    error("gosub jump out of range");
  }

  emitByte((delta >> 8) & 0xff);
  emitByte(delta & 0xff);
}

// ============================================
// PRATT PARSER - CORE
// ============================================

void Compiler::parsePrecedence(Precedence precedence)
{
  // ===== PROTEÇÃO CONTRA STACK OVERFLOW =====
  if (++expressionDepth > MAX_EXPRESSION_DEPTH)
  {
    error("Expression nested too deeply");
    --expressionDepth;
    return;
  }

  // Atualiza estatísticas
  if (expressionDepth > stats.maxExpressionDepth)
  {
    stats.maxExpressionDepth = expressionDepth;
  }

  advance();

  ParseFn prefixRule = getRule(previous.type)->prefix;

  if (prefixRule == nullptr)
  {
    error("Expect expression");
    --expressionDepth;
    return;
  }

  bool canAssign = (precedence <= PREC_ASSIGNMENT);
  (this->*prefixRule)(canAssign);

  // Verifica se houve erro
  if (hadError)
  {
    --expressionDepth;
    return;
  }

  while (precedence <= getRule(current.type)->prec)
  {
    advance();
    ParseFn infixRule = getRule(previous.type)->infix;

    if (infixRule == nullptr)
    {
      break;
    }

    (this->*infixRule)(canAssign);

    // Verifica se houve erro
    if (hadError)
    {
      --expressionDepth;
      return;
    }
  }

  if (canAssign && match(TOKEN_EQUAL))
  {
    error("Invalid assignment target");
  }

  --expressionDepth;
}

ParseRule *Compiler::getRule(TokenType type) { return &rules[type]; }

void Compiler::resolveGotos()
{
  for (const GotoJump &jump : pendingGotos)
  {
    int targetOffset = -1;

    for (const Label &l : labels)
    {
      if (l.name == jump.target)
      {
        targetOffset = l.offset;
        break;
      }
    }

    if (targetOffset == -1)
    {
      error("Undefined label");
      continue;
    }

 
    int opcodePos = jump.jumpOffset - 1;

    if (targetOffset < jump.jumpOffset)
    {
      // Backward jump: usa OP_LOOP
      currentChunk->code[opcodePos] = OP_LOOP;

      // Offset backward = distância para trás
      int offset = jump.jumpOffset - targetOffset + 2;
      if (offset > UINT16_MAX)
      {
        error("Goto distance too large");
        continue;
      }

      currentChunk->code[jump.jumpOffset] = (offset >> 8) & 0xff;
      currentChunk->code[jump.jumpOffset + 1] = offset & 0xff;
    }
    else
    {
      // Forward jump
      patchJumpTo(jump.jumpOffset, targetOffset);
    }
  }

  pendingGotos.clear();
}

void Compiler::resolveGosubs()
{
  for (const auto &j : pendingGosubs)
  {
    int targetOffset = -1;
    for (const auto &l : labels)
      if (l.name == j.target)
      {
        targetOffset = l.offset;
        break;
      }

    if (targetOffset < 0)
    {
      error("Undefined label");
      continue;
    }

    patchJumpTo(j.jumpOffset, targetOffset); // signed int16
  }
  pendingGosubs.clear();
}

void Compiler::validateIdentifierName(const Token &nameToken)
{
  const std::string &name = nameToken.lexeme;

  if (!lexer)
    return;

  // 1. Verifica se é keyword
  if (lexer->isKeyword(name))
  {
    fail("Cannot use keyword '%s' as identifier name", name.c_str());
    return;
  }

  // 2. Verifica se começa com número
  if (!name.empty() && std::isdigit(name[0]))
  {
    fail("Identifier '%s' cannot start with a digit", name.c_str());
    return;
  }

  // 3. Verifica se contém caracteres inválidos
  for (char c : name)
  {
    if (!std::isalnum(c) && c != '_')
    {
      fail("Identifier '%s' contains invalid character '%c'",
           name.c_str(), c);
      return;
    }
  }

  // 4. Verifica se é muito longo
  if (name.length() >= MAX_IDENTIFIER_LENGTH)
  {
    fail("Identifier '%s' is too long (max %d characters)",
         name.c_str(), MAX_IDENTIFIER_LENGTH);
    return;
  }

  if (name.length() >= 2 && name[0] == '_' && name[1] == '_')
  {
    Warning("Identifier '%s' starts with '__' which is typically "
            "reserved for internal use",
            name.c_str());
  }
}
