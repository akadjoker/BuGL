#include "compiler.hpp"
#include "interpreter.hpp"
#include "value.hpp"
#include "opcode.hpp"
#include "pool.hpp"
#include "debug.hpp"

// ============================================
// STATEMENTS
// ============================================

void Compiler::declaration()
{
    if (++declarationDepth > MAX_DECLARATION_DEPTH)
    {
        error("Declarations nested too deeply");
        --declarationDepth;
        synchronize();
        return;
    }

    if (match(TOKEN_DEF))
    {
        funDeclaration();
    }
    else if (match(TOKEN_PROCESS))
    {
        processDeclaration();
    }
    else if (match(TOKEN_VAR))
    {
        varDeclaration();
    }
    else if (match(TOKEN_IMPORT))
    {
        parseImport();
    }
    else if (match(TOKEN_INCLUDE))
    {
        includeStatement();
    }
    else if (match(TOKEN_USING))
    {
        parseUsing();
    }
    else if (match(TOKEN_REQUIRE))
    {
        parseRequire();
    }
    else
    {
        statement();
    }

    if (panicMode)
    {
        synchronize();
    }

    --declarationDepth;
}

void Compiler::statement()
{
    // Verifica timeout periodicamente
    if (!checkCompileTimeout())
    {
        return;
    }

    if (check(TOKEN_IDENTIFIER) && peek(0).type == TOKEN_COLON)
    {
        labelStatement();
    }
    else if (match(TOKEN_FRAME))
    {
        frameStatement();
    }
    else if (match(TOKEN_EXIT))
    {
        exitStatement();
    }
    else if (match(TOKEN_PRINT))
    {
        printStatement();
    }
    else if (match(TOKEN_IF))
    {
        ifStatement();
    }
    else if (match(TOKEN_WHILE))
    {
        whileStatement();
    }
    else if (match(TOKEN_GOTO))
    {
        gotoStatement();
    }
    else if (match(TOKEN_GOSUB))
    {
        gosubStatement();
    }
    else if (match(TOKEN_DO))
    {
        doWhileStatement();
    }
    else if (match(TOKEN_LOOP))
    {
        loopStatement();
    }
    else if (match(TOKEN_FOR))
    {
        forStatement();
    }
    else if (match(TOKEN_FOREACH))
    {
        foreachStatement();
    }
    else if (match(TOKEN_BREAK))
    {
        breakStatement();
    }
    else if (match(TOKEN_SWITCH))
    {
        switchStatement();
    }
    else if (match(TOKEN_CONTINUE))
    {
        continueStatement();
    }
    else if (match(TOKEN_STRUCT))
    {
        structDeclaration();
    }
    else if (match(TOKEN_CLASS))
    {
        classDeclaration();
    }
    else if (match(TOKEN_TRY))
    {
        tryStatement();
    }
    else if (match(TOKEN_THROW))
    {
        throwStatement();
    }
    else if (match(TOKEN_LBRACE))
    {
        // ===== VALIDAÇÃO ANTES DE CRIAR SCOPE =====
        if (!checkScopeDepth())
        {
            // Consome até } mas não cria scope
            int depth = 1;
            while (depth > 0 && !check(TOKEN_EOF))
            {
                if (match(TOKEN_LBRACE))
                    depth++;
                else if (match(TOKEN_RBRACE))
                    depth--;
                else
                    advance();
            }
            return;
        }

        beginScope();
        block();
        endScope();
    }
    else if (match(TOKEN_RETURN))
    {
        returnStatement();
    }
    else
    {
        expressionStatement();
    }
}

void Compiler::printStatement()
{
    uint8_t argCount = 0;

    consume(TOKEN_LPAREN, "Expect '('");

    if (!check(TOKEN_RPAREN))
    {
        do
        {
            expression();
            if (hadError)
                return;
            argCount++;

            if (argCount > 255)
            {
                error("Cannot have more than 255 arguments");
            }
        } while (match(TOKEN_COMMA));
    }

    consume(TOKEN_RPAREN, "Expect ')' after arguments");

    consume(TOKEN_SEMICOLON, "Expect ';'");

    emitBytes(OP_PRINT, argCount);
}

void Compiler::expressionStatement()
{

    // Expressão normal
    expression();
    if (hadError)
        return;
    consume(TOKEN_SEMICOLON, "Expresion statemnt Expect ';'");
    emitByte(OP_POP);
}

// ============================================
// VARIABLES
// ============================================

void Compiler::varDeclaration()
{
    // =========================================
    // MÚLTIPLOS RETORNOS: var (a, b, c) = expr()
    // =========================================
    if (match(TOKEN_LPAREN))
    {
        std::vector<Token> names;
        std::vector<uint16_t> globals;

        // Colectar nomes das variáveis
        do
        {
            consume(TOKEN_IDENTIFIER, "Expect variable name in multi-assignment");
            names.push_back(previous);

            // OPTIMIZATION: Use global index instead of constant pool
            uint16_t global = (scopeDepth == 0) ? getOrCreateGlobalIndex(previous.lexeme) : identifierConstant(previous);
            globals.push_back(global);

            if (scopeDepth > 0)
            {
                declareVariable();
                validateIdentifierName(previous);
                if (hadError)
                    return;
            }
        } while (match(TOKEN_COMMA));

        consume(TOKEN_RPAREN, "Expect ')' after variable list");
        consume(TOKEN_EQUAL, "Expect '=' in multi-assignment");

        // Compilar a expressão (deixa N valores na stack)
        expression();
        if (hadError)
            return;

        if (scopeDepth > 0)
        {
            // Locais: marcar todas as variáveis como inicializadas
            // (os valores já estão na stack, um por variável).
            int count = (int)names.size();
            int first = localCount_ - count;
            if (first < 0)
                first = 0;
            for (int i = first; i < localCount_; i++)
            {
                locals_[i].depth = scopeDepth;
                locals_[i].usedInitLocal = true;
            }
        }
        else
        {
            // Globais: define e consome valores da stack (LIFO)
            for (int i = (int)names.size() - 1; i >= 0; i--)
            {
                int privateIdx = vm_->getProcessPrivateIndex(names[i].lexeme.c_str());
                if (privateIdx != -1)
                {
                    Warning("Global variable '%s' shadows process private variable.",
                            names[i].lexeme.c_str());
                }
                declaredGlobals_.insert(names[i].lexeme);

                defineVariable(globals[i]);
            }
        }

        consume(TOKEN_SEMICOLON, "Expect ';' after multi-assignment");
        return;
    }

    // =========================================
    // DECLARAÇÃO NORMAL: var a, var a = x, etc.
    // =========================================
    do
    {
        consume(TOKEN_IDENTIFIER, "Expect variable name");
        Token nameToken = previous;

        // OPTIMIZATION: Use global index instead of constant pool for globals
        uint16_t global = (scopeDepth == 0) ? getOrCreateGlobalIndex(nameToken.lexeme) : identifierConstant(nameToken);

        if (scopeDepth > 0)
        {
            declareVariable();
            validateIdentifierName(nameToken);
            if (hadError)
            {
                return;
            }

            if (currentClass != nullptr && loopDepth_ > 1 && scopeDepth > 1)
            {
                Warning("Variable '%s' is declared inside loops in class methods.", nameToken.lexeme.c_str());
            }
        }

        // Se tem '=' atribui o valor, senão atribui nil
        if (match(TOKEN_EQUAL))
        {
            expression();
            if (hadError)
                return;
        }
        else
        {
            emitByte(OP_NIL);
        }

        // Track global variable names for proper resolution vs PRIVATE
        if (scopeDepth == 0)
        {
            // Avisa se a variável global tem o mesmo nome de uma private de processo
            int privateIdx = vm_->getProcessPrivateIndex(nameToken.lexeme.c_str());
            if (privateIdx != -1)
            {
                Warning("Global variable '%s' shadows process private variable. "
                        "Inside processes, use a different name or the global will be used instead of the private.",
                        nameToken.lexeme.c_str());
            }
            declaredGlobals_.insert(nameToken.lexeme);
        }

        defineVariable(global);

    } while (match(TOKEN_COMMA)); //  Continua se tem vírgula

    consume(TOKEN_SEMICOLON, "Expect ';' after variable declaration");
}

void Compiler::variable(bool canAssign)
{
    Token name = previous;
    std::string nameStr = name.lexeme;

    // =====================================================
    // PASSO 1: Procura em módulos USING (flat access)
    // Com detecção de conflitos
    // =====================================================

    struct UsingMatch
    {
        uint16 moduleId;
        uint16 id;
        std::string moduleName;
        bool isFunction;
    };
    std::vector<UsingMatch> matches;

    for (const auto &modName : usingModules)
    {
        uint16 moduleId;
        if (!vm_->getModuleId(modName.c_str(), &moduleId))
        {
            continue;
        }

        ModuleDef *mod = vm_->getModule(moduleId);
        if (!mod)
        {
            continue;
        }

        // Tenta como função
        uint16 funcId;
        if (mod->getFunctionId(nameStr.c_str(), &funcId))
        {
            matches.push_back({moduleId, funcId, modName, true});
        }

        // Tenta como constante
        uint16 constId;
        if (mod->getConstantId(nameStr.c_str(), &constId))
        {
            matches.push_back({moduleId, constId, modName, false});
        }
    }

    // Verifica conflitos
    if (matches.size() > 1)
    {
        std::string modules = matches[0].moduleName;
        for (size_t i = 1; i < matches.size(); i++)
        {
            modules += ", " + matches[i].moduleName;
        }
        fail("Ambiguous: '%s' found in multiple modules: %s. Use qualified name (module.%s)",
             nameStr.c_str(), modules.c_str(), nameStr.c_str());
        return;
    }

    // Único match encontrado
    if (matches.size() == 1)
    {
        const UsingMatch &m = matches[0];

        if (m.isFunction)
        {
            // É função! Deve ser chamada
            if (!match(TOKEN_LPAREN))
            {
                error("Module functions must be called");
                return;
            }

            // Emite ModuleRef
            Value ref = vm_->makeModuleRef(m.moduleId, m.id);
            emitConstant(ref);

            // Compila argumentos e CALL
            call(false);
            return;
        }
        else
        {
            // É constante! Emite valor direto
            ModuleDef *mod = vm_->getModule(m.moduleId);
            Value *value = mod->getConstant(m.id);
            if (value)
            {
                emitConstant(*value);
                return;
            }
        }
    }

    // =====================================================
    // PASSO 2: Verifica se é module.member (DOT access)
    // =====================================================
    if (check(TOKEN_DOT))
    {
        // Verifica se é módulo importado
        if (importedModules.find(nameStr) != importedModules.end())
        {
            // É módulo! Processa module.member
            advance(); // Consome DOT
            consume(TOKEN_IDENTIFIER, "Expect member name");
            Token member = previous;

            uint16 moduleId;
            if (!vm_->getModuleId(nameStr.c_str(), &moduleId))
            {
                fail("Module '%s' not found", nameStr.c_str());
                return;
            }

            ModuleDef *mod = vm_->getModule(moduleId);
            if (!mod)
            {
                fail("Module '%s' not found", nameStr.c_str());
                return;
            }
            // Tenta como função
            uint16 funcId;
            if (mod->getFunctionId(member.lexeme.c_str(), &funcId))
            {
                // É função! Deve ser chamada
                if (!match(TOKEN_LPAREN))
                {
                    error("Module functions must be called");
                    return;
                }

                // Emite ModuleRef
                Value ref = vm_->makeModuleRef(moduleId, funcId);
                emitConstant(ref);

                // Compila argumentos e CALL
                call(false);
                return; //  Sucesso!
            }

            // Tenta como constante
            uint16 constId;
            if (mod->getConstantId(member.lexeme.c_str(), &constId))
            {
                // É constante! Emite valor direto
                Value *value = mod->getConstant(constId);
                if (value)
                {
                    emitConstant(*value);
                    return; //  Sucesso!
                }
            }

            // Não encontrou
            fail("'%s' not found in module '%s'",
                 member.lexeme.c_str(),
                 nameStr.c_str());
            return;
        }
    }

    // =====================================================
    // PASSO 3: Variável normal (local ou global)
    // =====================================================
    namedVariable(name, canAssign);
}

void Compiler::and_(bool canAssign)
{
    (void)canAssign;
    int endJump = emitJump(OP_JUMP_IF_FALSE);
    emitByte(OP_POP);
    parsePrecedence(PREC_AND);
    patchJump(endJump);
}

void Compiler::or_(bool canAssign)
{
    (void)canAssign;
    int elseJump = emitJump(OP_JUMP_IF_FALSE);
    int endJump = emitJump(OP_JUMP);

    patchJump(elseJump);
    emitByte(OP_POP);

    parsePrecedence(PREC_OR);

    patchJump(endJump);
}

uint16 Compiler::identifierConstant(Token &name)
{

    return makeConstant(vm_->makeString(name.lexeme.c_str()));
}

// Helper para emitir opcode de variável - usa emitShort para globais (índice de constante)
void Compiler::emitVarOp(uint8 op, int arg)
{
    bool isGlobal = (op == OP_GET_GLOBAL || op == OP_SET_GLOBAL);
    emitByte(op);
    if (isGlobal)
        emitShort((uint16)arg);
    else
        emitByte((uint8)arg);
}

void Compiler::handle_assignment(uint8 getOp, uint8 setOp, int arg, bool canAssign)
{

    if (match(TOKEN_PLUS_PLUS))
    {
        // i++ (postfix) - retorna valor ANTIGO
        emitVarOp(getOp, arg);         // [old_value]
        emitByte(OP_DUP);              // [old_value, old_value]
        emitConstant(vm_->makeInt(1)); // [old_value, old_value, 1]
        emitByte(OP_ADD);              // [old_value, new_value]
        emitVarOp(setOp, arg);         // [old_value, new_value] (SET usa PEEK, não remove!)
        emitByte(OP_POP);              // [old_value] - remove o new_value
    }
    else if (match(TOKEN_MINUS_MINUS))
    {
        // i-- (postfix) - retorna valor ANTIGO
        emitVarOp(getOp, arg);         // [old_value]
        emitByte(OP_DUP);              // [old_value, old_value]
        emitConstant(vm_->makeInt(1)); // [old_value, old_value, 1]
        emitByte(OP_SUBTRACT);         // [old_value, new_value]
        emitVarOp(setOp, arg);         // [old_value, new_value] (SET usa PEEK)
        emitByte(OP_POP);              // [old_value] - remove o new_value
    }
    else if (canAssign && match(TOKEN_EQUAL))
    {
        expression();
        emitVarOp(setOp, arg);
    }
    else if (canAssign && match(TOKEN_PLUS_EQUAL))
    {
        emitVarOp(getOp, arg);
        expression();
        emitByte(OP_ADD);
        emitVarOp(setOp, arg);
    }
    else if (canAssign && match(TOKEN_MINUS_EQUAL))
    {
        emitVarOp(getOp, arg);
        expression();
        emitByte(OP_SUBTRACT);
        emitVarOp(setOp, arg);
    }
    else if (canAssign && match(TOKEN_STAR_EQUAL))
    {
        emitVarOp(getOp, arg);
        expression();
        emitByte(OP_MULTIPLY);
        emitVarOp(setOp, arg);
    }
    else if (canAssign && match(TOKEN_SLASH_EQUAL))
    {
        emitVarOp(getOp, arg);
        expression();
        emitByte(OP_DIVIDE);
        emitVarOp(setOp, arg);
    }
    else if (canAssign && match(TOKEN_PERCENT_EQUAL))
    {
        emitVarOp(getOp, arg);
        expression();
        emitByte(OP_MODULO);
        emitVarOp(setOp, arg);
    }
    else
    {
        emitVarOp(getOp, arg);
    }
}

void Compiler::namedVariable(Token &name, bool canAssign)
{
    uint8 getOp, setOp;
    int arg;

    // === 1. Tenta LOCAL (prioridade máxima - declaração explícita) ===
    arg = resolveLocal(name);
    if (arg != -1)
    {
        getOp = OP_GET_LOCAL;
        setOp = OP_SET_LOCAL;
        handle_assignment(getOp, setOp, arg, canAssign);
        return;
    }

    // === 2. Tenta UPVALUE ===
    arg = resolveUpvalue(name);
    if (arg != -1)
    {
        getOp = OP_GET_UPVALUE;
        setOp = OP_SET_UPVALUE;
        handle_assignment(getOp, setOp, arg, canAssign);
        return;
    }

    // === 3. Tenta GLOBAL (declaração explícita) ===
    // Verifica se foi declarado como global antes de usar PRIVATE
    if (declaredGlobals_.count(name.lexeme) > 0)
    {
        // OPTIMIZATION: Use direct index instead of hash lookup
        arg = getOrCreateGlobalIndex(name.lexeme);
        getOp = OP_GET_GLOBAL;
        setOp = OP_SET_GLOBAL;
        handle_assignment(getOp, setOp, arg, canAssign);
        return;
    }

    // === 4. PRIVATE (fallback para variáveis de processo) ===
    if (isProcess_)
    {
        arg = (int)vm_->getProcessPrivateIndex(name.lexeme.c_str());
        if (arg != -1)
        {
            getOp = OP_GET_PRIVATE;
            setOp = OP_SET_PRIVATE;
            handle_assignment(getOp, setOp, arg, canAssign);
            return;
        }
    }

    // === 5. Fallback final: ERRO - variável não declarada ===
    // Se chegou aqui, a variável não foi declarada com 'var'

    String *nameStr = vm_->createString(name.lexeme.c_str());
    if (vm_->globals.exist(nameStr))
    {
        // É uma classe/struct nativo - permite acesso
        arg = identifierConstant(name);
        getOp = OP_GET_GLOBAL;
        setOp = OP_SET_GLOBAL;
        handle_assignment(getOp, setOp, arg, canAssign);
        return;
    }

    // Variável não foi declarada com 'var' - ERRO!
    // Mesmo em scope local, não permitimos criar globais implicitamente
    fail("Undefined variable '%s'!",
          name.lexeme.c_str(), name.lexeme.c_str());
    
    // Emite código dummy para continuar compilação
    emitByte(OP_NIL);
}

void Compiler::defineVariable(uint16 global)
{
    if (scopeDepth > 0)
    {

        markInitialized();
        return;
    }

    emitByte(OP_DEFINE_GLOBAL);
    emitShort(global);
}

void Compiler::declareVariable()
{
    if (scopeDepth == 0)
        return;

    Token &name = previous;

    for (int i = localCount_ - 1; i >= 0; i--)
    {
        Local &local = locals_[i];

        if (local.depth != -1 && local.depth < scopeDepth)
        {
            break;
        }

        if (local.name == name.lexeme)
        {
            fail("Variable '%s' already declared in this scope", name.lexeme.c_str());
            return;
        }
    }

    addLocal(previous);
}

void Compiler::addLocal(Token &name)
{
    if (localCount_ >= MAX_LOCALS)
    {
        error("Too many local variables in function");
        return;
    }

    size_t len = name.lexeme.length();
    if (len >= MAX_IDENTIFIER_LENGTH)
    {
        fail("Identifier name too long (max %d characters)", MAX_IDENTIFIER_LENGTH - 1);
        return;
    }

    locals_[localCount_].name = name.lexeme;
    locals_[localCount_].depth = -1;
    locals_[localCount_].usedInitLocal = false;
    locals_[localCount_].isCaptured = false;

    localCount_++;
}

void Compiler::markInitialized()
{
    if (scopeDepth == 0)
        return;

    if (localCount_ > 0)
    {
        locals_[localCount_ - 1].depth = scopeDepth;
        locals_[localCount_ - 1].usedInitLocal = true;
    }
}

void Compiler::beginScope()
{
    if (scopeDepth >= MAX_SCOPE_DEPTH)
    {
        error("Scopes nested too deeply");
        return;
    }
    scopeDepth++;

    if (scopeDepth > (int)stats.maxScopeDepth)
    {
        stats.maxScopeDepth = scopeDepth;
    }
}

void Compiler::endScope()
{

    int popped = discardLocals(scopeDepth);
    localCount_ -= popped;
    scopeDepth--;
}

int Compiler::resolveLocal(Token &name)
{
    for (int i = localCount_ - 1; i >= 0; i--)
    {
        if (locals_[i].name == name.lexeme)
        {
            if (locals_[i].depth == -1)
            {
                error("Cannot read local variable in its own initializer");
            }
            return i;
        }
    }
    return -1;
}

void Compiler::block()
{
    while (!check(TOKEN_RBRACE) && !check(TOKEN_EOF))
    {
        if (hadError)
        {
            // Recovery local do bloco atual para evitar erros em cascata:
            // consome até ao '}' correspondente e sai silenciosamente.
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
            return;
        }
        declaration();
    }
    consume(TOKEN_RBRACE, "Expect '}' after block");
}

void Compiler::ifStatement()
{
    // if (condition)
    consume(TOKEN_LPAREN, "Expect '(' after 'if'");
    expression();
    if (hadError)
        return;
    consume(TOKEN_RPAREN, "Expect ')' after condition");

    // Jump para próximo bloco se condição for falsa
    int thenJump = emitJump(OP_JUMP_IF_FALSE);
    emitByte(OP_POP); // Pop da condição se for true

    // Then branch
    statement();
    if (hadError)
        return;

    // Lista de jumps para o final (depois de cada then/elif executar)
    std::vector<int> endJumps;
    endJumps.push_back(emitJump(OP_JUMP)); // Jump do if

    // Patch do thenJump (aponta para o próximo elif/else/end)
    patchJump(thenJump);
    emitByte(OP_POP); // Pop da condição se for false

    // Elif branches (pode ter vários)
    while (match(TOKEN_ELIF))
    {
        // elif (condition)
        consume(TOKEN_LPAREN, "Expect '(' after 'elif'");
        expression();
        if (hadError)
            return;
        consume(TOKEN_RPAREN, "Expect ')' after elif condition");

        // Jump para próximo bloco se condição for falsa
        int elifJump = emitJump(OP_JUMP_IF_FALSE);
        emitByte(OP_POP); // Pop se elif for true

        // Elif body
        statement();
        if (hadError)
            return;
        // Jump para o final após executar elif
        endJumps.push_back(emitJump(OP_JUMP));

        // Patch do elifJump (aponta para próximo elif/else/end)
        patchJump(elifJump);
        emitByte(OP_POP); // Pop se elif for false
    }

    // Else branch (opcional)
    if (match(TOKEN_ELSE))
    {
        statement();
        if (hadError)
            return;
    }

    // Patch todos os jumps para apontarem para o final
    for (int jump : endJumps)
    {
        patchJump(jump);
    }
}

void Compiler::beginLoop(int loopStart, bool isForeach)
{
    if (loopDepth_ >= MAX_LOOP_DEPTH)
    {
        error("Too many nested loops");
        return;
    }

    loopContexts_[loopDepth_].loopStart = loopStart;
    loopContexts_[loopDepth_].scopeDepth = scopeDepth;
    loopContexts_[loopDepth_].breakCount = 0;
    loopContexts_[loopDepth_].isForeach = isForeach;
    loopDepth_++;
}

void Compiler::endLoop()
{
    if (loopDepth_ == 0)
    {
        error("Internal error: endLoop without beginLoop");
        return;
    }
    loopDepth_--;
    LoopContext &ctx = loopContexts_[loopDepth_];
    for (int i = 0; i < ctx.breakCount; i++)
    {
        patchJump(ctx.breakJumps[i]);
    }
}

int Compiler::discardLocals(int depth)
{
    int local = localCount_ - 1;
    while (local >= 0 && locals_[local].depth >= depth)
    {
        if (locals_[local].isCaptured)
        {
            emitByte(OP_CLOSE_UPVALUE); // Move para heap
        }
        else
        {
            emitByte(OP_POP); // Só remove da stack
        }
        local--;
    }
    return localCount_ - local - 1;
}

void Compiler::emitBreak()
{
    if (loopDepth_ == 0)
    {
        error("Cannot use 'break' outside of a loop");
        return;
    }

    if (tryDepth > 0)
    {
        error("Cannot use 'break' inside try-catch-finally block");
        return;
    }

    LoopContext &ctx = loopContexts_[loopDepth_ - 1];

    discardLocals(ctx.scopeDepth + 1);

    if (ctx.isForeach)
    {
        emitDiscard(2);
    }

    if (!ctx.addBreak(emitJump(OP_JUMP)))
    {
        error("Too many breaks");
    }
}

void Compiler::emitContinue()
{
    if (loopDepth_ == 0)
    {
        error("Cannot use 'continue' outside of a loop");
        return;
    }
    if (tryDepth > 0)
    {
        error("Cannot use 'continue' inside try-catch-finally block");
        return;
    }
    LoopContext &ctx = loopContexts_[loopDepth_ - 1];

    discardLocals(ctx.scopeDepth + 1);

    emitLoop(ctx.loopStart);
}

void Compiler::whileStatement()
{
    int loopStart = currentChunk->count;

    consume(TOKEN_LPAREN, "Expect '(' after 'while'");
    expression();
    if (hadError)
        return;
    consume(TOKEN_RPAREN, "Expect ')' after condition");

    // 1. Se for falso, salta para 'exitJump'
    int exitJump = emitJump(OP_JUMP_IF_FALSE);

    // 2. Se for verdadeiro, faz POP do 'true' e entra no corpo
    emitByte(OP_POP);

    beginLoop(loopStart);
    statement();
    if (hadError)
    {
        endLoop();
        return;
    }
    emitLoop(loopStart); // Volta ao início
    // Passo A: Resolver a saída natural (quando o loop termina porque a condição é falsa)
    // O OP_JUMP_IF_FALSE aterra AQUI.
    patchJump(exitJump);

    // Passo B: Limpar o 'false' que ficou na stack da saída natural
    emitByte(OP_POP);
    // Passo C: Resolver os breaks.
    // Os breaks vêm de dentro do loop (onde a stack já está limpa).
    // Por isso, eles devem aterrar AQUI (depois do POP), e não antes.
    endLoop();
}

void Compiler::doWhileStatement()
{
    consume(TOKEN_LBRACE, "Expect '{' after 'do'");

    int loopStart = currentChunk->count; // Início do corpo (para onde o loop volta se for TRUE)

    beginLoop(loopStart); // Regista o loop

    // BODY
    beginScope();
    block();
    endScope();

    // O 'continue' num do-while deve saltar para a verificação da condição,
    // loopStart no contexto atual para apontar para AQUI.
    loopContexts_[loopDepth_ - 1].loopStart = currentChunk->count;

    consume(TOKEN_WHILE, "Expect 'while' after do body");
    consume(TOKEN_LPAREN, "Expect '(' after 'while'");
    expression();
    if (hadError)
        return;
    consume(TOKEN_RPAREN, "Expect ')' after condition");
    consume(TOKEN_SEMICOLON, "Expect ';' after do-while");

    // Lógica de Salto (Invertida em relação ao while normal)

    // 1. Se for FALSO, queremos sair. Mas OP_JUMP_IF_FALSE não faz pop.
    int exitJump = emitJump(OP_JUMP_IF_FALSE);

    // 2. Se for VERDADEIRO (não saltou), fazemos POP do 'true' da stack...
    emitByte(OP_POP);
    // 3. ... e voltamos ao início do bloco (loopStart original)
    emitLoop(loopStart);

    // 4. Se foi FALSO, aterra aqui.
    patchJump(exitJump);
    emitByte(OP_POP); // Remove o 'false' da stack

    endLoop();
}

void Compiler::loopStatement()
{
    // loop { ... }

    int loopStart = currentChunk->count;

    beginLoop(loopStart);

    // Body
    consume(TOKEN_LBRACE, "Expect '{' after 'loop'");
    beginScope();
    block();
    endScope();

    // Volta sempre ao início (loop infinito)
    emitLoop(loopStart);

    // Patch dos breaks (única forma de sair!)
    endLoop();
}

void Compiler::switchStatement()
{
    if (!enterSwitchContext())
    {
        return;
    }

    consume(TOKEN_LPAREN, "Expect '(' after 'switch'");
    expression(); // [value]
    if (hadError)
    {
        leaveSwitchContext();
        return;
    }
    consume(TOKEN_RPAREN, "Expect ')' after switch expression");
    consume(TOKEN_LBRACE, "Expect '{' before switch body");

    std::vector<int> endJumps;
    std::vector<int> caseFailJumps;

    // Parse cases
    while (match(TOKEN_CASE))
    {
        emitByte(OP_DUP); // [value, value]
        expression();     // [value, value, case_val]
        if (hadError)
        {
            recoverToCurrentSwitchEnd();
            leaveSwitchContext();
            return;
        }
        consume(TOKEN_COLON, "Expect ':' after case value");
        emitByte(OP_EQUAL); // [value, bool]

        int caseJump = emitJump(OP_JUMP_IF_FALSE);
        emitByte(OP_POP); // [value] - Pop comparison result

        emitByte(OP_POP); // []

        // Case body
        while (!check(TOKEN_CASE) && !check(TOKEN_DEFAULT) &&
               !check(TOKEN_RBRACE) && !check(TOKEN_EOF))
        {
            statement();
            if (hadError)
            {
                recoverToCurrentSwitchEnd();
                leaveSwitchContext();
                return;
            }
        }

        endJumps.push_back(emitJump(OP_JUMP));

        patchJump(caseJump);
        emitByte(OP_POP); // [value] - Pop comparison result
    }

    // Default (opcional)
    if (match(TOKEN_DEFAULT))
    {
        consume(TOKEN_COLON, "Expect ':' after 'default'");

        emitByte(OP_POP); // []

        while (!check(TOKEN_CASE) && !check(TOKEN_RBRACE) && !check(TOKEN_EOF))
        {
            statement();
            if (hadError)
            {
                recoverToCurrentSwitchEnd();
                leaveSwitchContext();
                return;
            }
        }
    }
    else
    {

        emitByte(OP_POP); // []
    }

    consume(TOKEN_RBRACE, "Expect '}' after switch body");

    for (int jump : endJumps)
    {
        patchJump(jump);
    }

    leaveSwitchContext();
}

void Compiler::breakStatement()
{
    if (switchDepth_ > 0)
    {
        int switchLoopDepth = switchLoopDepthStack_[switchDepth_ - 1];
        if (loopDepth_ <= switchLoopDepth)
        {
            error("Switch cases auto-exit; 'break' here would break an outer loop");
            consume(TOKEN_SEMICOLON, "Expect ';' after 'break'");
            return;
        }
    }

    emitBreak();
    consume(TOKEN_SEMICOLON, "Expect ';' after 'break'");
}

void Compiler::continueStatement()
{
    emitContinue();
    consume(TOKEN_SEMICOLON, "Expect ';' after 'continue'");
}

void Compiler::forStatement()
{
    // for cria um scope próprio para o initializer
    beginScope();

    consume(TOKEN_LPAREN, "Expect '(' after 'for'");

    // INITIALIZER (opcional)
    // Pode ser: var i = 0; ou i = 0; ou vazio
    if (match(TOKEN_SEMICOLON))
    {
        // Sem initializer
    }
    else if (match(TOKEN_VAR))
    {
        varDeclaration(); // var i = 0;
        if (hadError)
            return;
    }
    else
    {
        expressionStatement(); // i = 0;
        if (hadError)
            return;
    }

    // Marca onde o loop começa (para continue e para o loop)
    int loopStart = currentChunk->count;

    // CONDITION (opcional)
    int exitJump = -1;
    if (!check(TOKEN_SEMICOLON))
    {
        expression(); // i < 10
        if (hadError)
            return;
        consume(TOKEN_SEMICOLON, "Expect ';' after loop condition");

        // salta para fora se condição for falsa
        exitJump = emitJump(OP_JUMP_IF_FALSE);
        emitByte(OP_POP); // Pop da condição
    }
    else
    {
        consume(TOKEN_SEMICOLON, "Expect ';' after loop condition");
    }

    // INCREMENT (opcional)
    // Problema: increment vem ANTES do body no código, mas executa DEPOIS
    // Solução: saltar o increment, executar body, depois voltar pro increment
    if (!check(TOKEN_RPAREN))
    {
        // salta sobre o código do increment
        int bodyJump = emitJump(OP_JUMP);

        int incrementStart = currentChunk->count;
        expression();     // i = i + 1
        emitByte(OP_POP); // Pop do resultado
        consume(TOKEN_RPAREN, "Expect ')' after for clauses");

        // Volta para o início do loop (condition)
        emitLoop(loopStart);

        // Agora loopStart aponta para o increment (para continue)
        loopStart = incrementStart;

        // Patch do bodyJump para saltar o increment
        patchJump(bodyJump);
    }
    else
    {
        consume(TOKEN_RPAREN, "Expect ')' after for clauses");
    }

    // Registra o loop para break/continue
    beginLoop(loopStart);

    // BODY
    statement();

    // Volta para o increment (ou condition se não houver increment)
    emitLoop(loopStart);

    // Patch do exitJump (se houver condition)
    if (exitJump != -1)
    {
        patchJump(exitJump);
        emitByte(OP_POP); // Pop da condição falsa
    }

    endLoop();  // Patch dos breaks
    endScope(); // Limpa variáveis do initializer
}

void Compiler::foreachStatement()
{
    consume(TOKEN_LPAREN, "Expect '(' after 'foreach'");
    consume(TOKEN_IDENTIFIER, "Expect variable name");
    Token itemName = previous;
    consume(TOKEN_IN, "Expect 'in'");

    expression();
    consume(TOKEN_RPAREN, "Expect ')'");

    Token tmp;
    tmp.lexeme = "__seq___";
    tmp.type = TOKEN_IDENTIFIER;
    tmp.column = previous.column;
    addLocal(tmp);
    markInitialized();
    emitByte(OP_NIL);
    tmp.lexeme = "__iter__";
    addLocal(tmp);
    markInitialized();
    int loopStart = currentChunk->count;
    beginLoop(loopStart, true);

    emitByte(OP_COPY2);
    emitByte(OP_ITER_NEXT);

    int exitJump = emitJump(OP_JUMP_IF_FALSE);
    emitByte(OP_POP);

    emitByte(OP_SWAP);
    emitByte(OP_POP);

    emitByte(OP_COPY2);
    emitByte(OP_ITER_VALUE);

    beginScope();
    addLocal(itemName);
    markInitialized();
    statement();

    endScope(); // Remove item (slot 2), faz POP

    emitLoop(loopStart);

    patchJump(exitJump);
    emitDiscard(4);

    localCount_ -= 2; // Remove seq e iter

    endLoop();
}

void Compiler::returnStatement()
{

    if (isProcess_)
    {
        consume(TOKEN_SEMICOLON, "Expect ';'"); // quando usamo gosub nos processos
        emitByte(OP_RETURN_SUB);
        return;
    }

    if (function == nullptr)
    {
        error("Can't return from top-level code");
        return;
    }

    if (match(TOKEN_SEMICOLON))
    {
        emitReturn();
    }
    else if (match(TOKEN_LPAREN))
    {
        // Multi-return: return (a, b, c);
        if (currentFunctionType == FunctionType::TYPE_INITIALIZER)
        {
            error("Cannot return values from an initializer");
            return;
        }

        uint8_t count = 0;
        if (!check(TOKEN_RPAREN))
        {
            do
            {
                expression();
                if (hadError)
                    return;
                count++;
                if (count > 255)
                {
                    error("Cannot return more than 255 values");
                    return;
                }
            } while (match(TOKEN_COMMA));
        }

        consume(TOKEN_RPAREN, "Expect ')' after return values");
        consume(TOKEN_SEMICOLON, "Expect ';' after return statement");

        if (count == 0)
        {
            emitReturn(); // return () is same as return;
        }
        else if (count == 1)
        {
            emitByte(OP_RETURN); // return (a) is same as return a;
        }
        else
        {
            emitBytes(OP_RETURN_N, count);
        }
    }
    else
    {
        if (currentFunctionType == FunctionType::TYPE_INITIALIZER)
        {
            error("Cannot return a value from an initializer");
            return;
        }
        // return <expr>;
        expression();
        if (hadError)
            return;
        consume(TOKEN_SEMICOLON, "Expect ';' after return value");
        emitByte(OP_RETURN);
    }

    function->hasReturn = true;
}

uint8 Compiler::argumentList()
{
    uint8 argCount = 0;

    if (!check(TOKEN_RPAREN))
    {
        do
        {
            if (hadError)
                break;
            expression();

            if (argCount == 255)
            {
                error("Can't have more than 255 arguments");
            }
            argCount++;
        } while (match(TOKEN_COMMA));
    }

    consume(TOKEN_RPAREN, "Expect ')' after arguments");
    return argCount;
}

void Compiler::call(bool canAssign)
{
    (void)canAssign;

    if (callDepth >= MAX_CALL_DEPTH)
    {
        error("Function calls nested too deeply");

        uint8 argCount = 0;
        if (!check(TOKEN_RPAREN))
        {
            do
            {
                expression();
                argCount++;
                if (argCount > 255)
                {
                    error("Can't have more than 255 arguments");
                    break;
                }
            } while (match(TOKEN_COMMA));
        }
        consume(TOKEN_RPAREN, "Expect ')' after arguments");
        return;
    }

    callDepth++;
    uint8 argCount = argumentList();
    emitBytes(OP_CALL, argCount);
    callDepth--;
}

void Compiler::funDeclaration()
{
    consume(TOKEN_IDENTIFIER, "Expect function name");
    Token nameToken = previous;
    validateIdentifierName(nameToken);
    if (hadError)
    {
        return;
    }

    std::string actualName;

    if (function != nullptr)
    {
        // Nested function: adiciona prefixo da função pai
        actualName = function->name->chars();

        actualName += "$";
        actualName += nameToken.lexeme;
    }
    else
    {
        // Top-level function: nome normal
        actualName = nameToken.lexeme;
    }

    Function *func = vm_->addFunction(actualName.c_str(), 0);

    if (!func)
    {
        error("Function already exists");
        return;
    }

    if (scopeDepth > 0)
    {
        declareVariable(); // Adiciona 'inner' como local de 'outer'
    }
    else
    {
        // Register global name BEFORE compiling body so recursion works
        declaredGlobals_.insert(nameToken.lexeme);
    }

    // Compila função
    compileFunction(func, false); // false = não é process

    // Verifica se tem upvalues
    if (func->upvalueCount > 0)
    {
        // É uma CLOSURE (captura variáveis)
        uint16 constant = makeConstant(vm_->makeFunction(func->index));
        emitByte(OP_CLOSURE);
        emitShort(constant);
        // Emite info de cada upvalue (isLocal, index)
        for (int i = 0; i < func->upvalueCount; i++)
        {
            emitByte(upvalues_[i].isLocal ? 1 : 0);
            emitByte(upvalues_[i].index);
        }
    }
    else
    {
        // É função NORMAL (sem upvalues)
        emitConstant(vm_->makeFunction(func->index));
    }

    // Define como global ou local
    if (scopeDepth > 0)
    {
        defineVariable(0); // Local (já foi declarado acima)
    }
    else
    {
        // OPTIMIZATION: Use global index instead of constant pool
        uint16 globalIndex = getOrCreateGlobalIndex(nameToken.lexeme);
        defineVariable(globalIndex); // Global
    }
}

void Compiler::processDeclaration()
{
    consume(TOKEN_IDENTIFIER, "Expect process name");
    Token nameToken = previous;
    isProcess_ = true;
    argNames.clear();

    validateIdentifierName(nameToken);
    if (hadError)
    {
        return;
    }

    // Cria função para o process

    Function *func = vm_->addFunction(nameToken.lexeme.c_str(), 0);

    if (!func)
    {
        error("Function already exists");
        return;
    }

    // Compila processo
    compileFunction(func, true); // true = É PROCESS!

    // Cria blueprint (process não vai para globals como callable)
    ProcessDef *proc = vm_->addProcess(nameToken.lexeme.c_str(), func);
    currentProcess = proc;

    for (uint32 i = 0; i < argNames.size(); i++)
    {
        int privateIndex = vm_->getProcessPrivateIndex(argNames[i]->chars());

        if (privateIndex >= 0)
        {
            if (privateIndex == (int)PrivateIndex::ID)
            {
                Warning("Property 'ID' is readonly!");
            }
            else if (privateIndex == (int)PrivateIndex::FATHER)
            {
                Warning("Property 'FATHER' is readonly!");
            }
            else
            {
                proc->argsNames.push((uint8)privateIndex);
            }
        }
        else
        {

            proc->argsNames.push(255); // Marcador "sem private"
        }
    }
    argNames.clear();

    emitConstant(vm_->makeProcess(proc->index));
    // OPTIMIZATION: Use global index instead of constant pool
    declaredGlobals_.insert(nameToken.lexeme);
    uint16 globalIndex = getOrCreateGlobalIndex(nameToken.lexeme);
    defineVariable(globalIndex);

    proc->finalize();

    isProcess_ = false;
}

void Compiler::compileFunction(Function *func, bool isProcess)
{
    // ========================================
    // SALVA ESTADO
    // ========================================
    Function *enclosing = this->function;
    Code *enclosingChunk = this->currentChunk;
    int enclosingScopeDepth = this->scopeDepth;
    int enclosingLocalCount = this->localCount_;
    bool wasInProcess = this->isProcess_;
    int savedUpvalueCount = this->upvalueCount_;

    // Guarda o tamanho da stack (para restaurar depois)
    size_t savedStackSize = enclosingStack_.size();

    // ========================================
    // PUSH na stack de enclosing (se há pai)
    // ========================================
    if (enclosingLocalCount > 0)
    {
        EnclosingContext ctx;
        ctx.function = enclosing;

        // Copia locals atuais para o context
        ctx.locals.reserve(enclosingLocalCount);
        for (int i = 0; i < enclosingLocalCount; i++)
        {
            ctx.locals.push_back(this->locals_[i]);
        }

        enclosingStack_.push_back(ctx);  
    }

    // ========================================
    // TROCA CONTEXTO
    // ========================================
    this->function = func;
    this->currentChunk = func->chunk;
    this->scopeDepth = 0;
    this->localCount_ = 0;
    this->upvalueCount_ = 0;
    this->isProcess_ = isProcess;
    labels.clear();
    pendingGotos.clear();
    pendingGosubs.clear();

    if (!func || !func->chunk)
    {
        Error("Error in function");
        return;
    }

    // ========================================
    // PARSE PARÂMETROS
    // ========================================
    beginScope();
    consume(TOKEN_LPAREN, "Expect '(' after name");

    if (!isProcess)
    {
        Token dummyToken;
        dummyToken.lexeme = func->name->chars();
        addLocal(dummyToken);
        markInitialized();
    }

    if (!check(TOKEN_RPAREN))
    {
        do
        {
            func->arity++;
            if (func->arity > 255)
            {
                error("Can't have more than 255 parameters");
                break;
            }

            consume(TOKEN_IDENTIFIER, "Expect parameter name");
            if (isProcess)
            {
                argNames.push(vm_->createString(previous.lexeme.c_str()));

                int privateIndex = vm_->getProcessPrivateIndex(previous.lexeme.c_str());
                if (privateIndex >= 0 &&
                    privateIndex != (int)PrivateIndex::ID &&
                    privateIndex != (int)PrivateIndex::FATHER)
                {
                    // Param matches a process private (x, y, etc).
                    // Do NOT create a local to avoid shadowing the private.
                }
                else
                {
                    addLocal(previous);
                    markInitialized();
                }
            }
            else
            {
                addLocal(previous);
                markInitialized();
            }

        } while (match(TOKEN_COMMA));
    }

    consume(TOKEN_RPAREN, "Expect ')' after parameters");

    // ========================================
    // PARSE CORPO
    // ========================================
    consume(TOKEN_LBRACE, "Expect '{' before body");
    block();
    endScope();

    resolveGotos();
    resolveGosubs();

    labels.clear();
    pendingGotos.clear();
    pendingGosubs.clear();

    // ALWAYS emit implicit return to handle early returns in conditionals
    // If function already returned via explicit return, this code is unreachable (dead code)
    // but ensures all execution paths end with OP_RETURN
    emitReturn();
    function->hasReturn = true;

    // ========================================
    // GUARDA UPVALUE COUNT
    // ========================================
    func->upvalueCount = this->upvalueCount_;

    // ========================================
    // RESTAURA ESTADO (POP da stack)
    // ========================================
    this->function = enclosing;
    this->currentChunk = enclosingChunk;
    this->scopeDepth = enclosingScopeDepth;
    this->localCount_ = enclosingLocalCount;
    this->isProcess_ = wasInProcess;
    this->upvalueCount_ = savedUpvalueCount;

    // Restore locals_ array from enclosingStack_ before popping
    while (enclosingStack_.size() > savedStackSize)
    {
        EnclosingContext &ctx = enclosingStack_.back();
        // Restore locals_ array content
        for (size_t i = 0; i < ctx.locals.size(); i++)
        {
            this->locals_[i] = ctx.locals[i];
        }
        enclosingStack_.pop_back();
    }
}

void Compiler::prefixIncrement(bool canAssign)
{
    (void)canAssign;

    if (!check(TOKEN_IDENTIFIER))
    {
        error("Expect variable name after '++'");
        return;
    }

    advance();
    Token name = previous; // O nome da variável

    // -----------------------------------------------------------
    // CENÁRIO A: É uma PROPRIEDADE (ex: ++player.hp)
    // (Mantém-se igual ao que já tinhas e funcionava)
    // -----------------------------------------------------------
    if (match(TOKEN_DOT))
    {
        consume(TOKEN_IDENTIFIER, "Expect property name after '.'.");
        uint16_t nameIdx = identifierConstant(previous);

        // Resolver o objeto (usando a nova lógica completa!)
        // Nota: Aqui podíamos duplicar a lógica abaixo ou criar um helper,
        // mas para simplificar, vamos assumir que o objeto é Local ou Global
        // (Se quiseres ser perfecionista, o objeto 'player' também podia ser um Upvalue!)

        // ... (Para não complicar, mantemos a lógica simples para o objeto base
        // ou podes copiar o bloco 'Resolução Completa' abaixo para aqui também)
        int arg = resolveLocal(name);
        if (arg != -1)
        {
            emitBytes(OP_GET_LOCAL, (uint8)arg);
        }
        else
        {
            // OPTIMIZATION: Use global index instead of constant pool
            arg = getOrCreateGlobalIndex(name.lexeme);
            emitByte(OP_GET_GLOBAL);
            emitShort((uint16)arg);
        }

        emitByte(OP_DUP);
        emitByte(OP_GET_PROPERTY);
        emitShort(nameIdx);
        emitConstant(vm_->makeInt(1));
        emitByte(OP_ADD);
        emitByte(OP_SET_PROPERTY);
        emitShort(nameIdx);
    }
    // -----------------------------------------------------------
    // CENÁRIO B: É uma VARIÁVEL (++i, ++upvalue, ++private)
    // -----------------------------------------------------------
    else
    {
        uint8 getOp = OP_GET_GLOBAL, setOp = OP_SET_GLOBAL;
        int arg = -1;

        // 1. Tenta LOCAL (prioridade máxima)
        arg = resolveLocal(name);
        if (arg != -1)
        {
            getOp = OP_GET_LOCAL;
            setOp = OP_SET_LOCAL;
        }

        // 2. Tenta UPVALUE
        if (arg == -1)
        {
            arg = resolveUpvalue(name);
            if (arg != -1)
            {
                getOp = OP_GET_UPVALUE;
                setOp = OP_SET_UPVALUE;
            }
        }

        // 3. Tenta GLOBAL (se foi declarado como global)
        if (arg == -1 && declaredGlobals_.count(name.lexeme) > 0)
        {
            // OPTIMIZATION: Use global index instead of constant pool
            arg = getOrCreateGlobalIndex(name.lexeme);
            getOp = OP_GET_GLOBAL;
            setOp = OP_SET_GLOBAL;
        }

        // 4. Tenta PRIVATE (só se for Process e não achou global)
        if (arg == -1 && isProcess_)
        {
            int index = (int)vm_->getProcessPrivateIndex(name.lexeme.c_str());
            if (index != -1)
            {
                arg = index;
                getOp = OP_GET_PRIVATE;
                setOp = OP_SET_PRIVATE;
            }
        }

        // 5. Fallback para GLOBAL (se não achou nada)
        if (arg == -1)
        {
            // OPTIMIZATION: Use global index instead of constant pool
            arg = getOrCreateGlobalIndex(name.lexeme);
            getOp = OP_GET_GLOBAL;
            setOp = OP_SET_GLOBAL;
        }

        // Agora sim, emite o código correto para QUALQUER tipo de variável
        // ++i retorna o valor NOVO
        emitVarOp(getOp, arg);         // [old_value]
        emitConstant(vm_->makeInt(1)); // [old_value, 1]
        emitByte(OP_ADD);              // [new_value]
        emitVarOp(setOp, arg);         // [new_value] (SET usa PEEK, não remove!)
        // SET já deixa o new_value na stack, não precisa de DUP
    }
}

void Compiler::prefixDecrement(bool canAssign)
{
    (void)canAssign;

    // 1. Validar nome
    if (!check(TOKEN_IDENTIFIER))
    {
        error("Expect variable name after '--'");
        return;
    }

    advance();
    Token name = previous;

    // -----------------------------------------------------------
    // CENÁRIO A: É uma PROPRIEDADE (ex: --player.hp)
    // -----------------------------------------------------------
    if (match(TOKEN_DOT))
    {
        consume(TOKEN_IDENTIFIER, "Expect property name after '.'.");
        uint16_t nameIdx = identifierConstant(previous);

        // Resolver objeto (Local ou Global)
        int arg = resolveLocal(name);
        if (arg != -1)
        {
            emitBytes(OP_GET_LOCAL, (uint8)arg);
        }
        else
        {
            // OPTIMIZATION: Use global index instead of constant pool
            arg = getOrCreateGlobalIndex(name.lexeme);
            emitByte(OP_GET_GLOBAL);
            emitShort((uint16)arg);
        }

        emitByte(OP_DUP); // [obj, obj]
        emitByte(OP_GET_PROPERTY);
        emitShort(nameIdx);            // [obj, val_antigo]
        emitConstant(vm_->makeInt(1)); // [obj, val_antigo, 1]
        emitByte(OP_SUBTRACT);         // [obj, val_novo]
        emitByte(OP_SET_PROPERTY);
        emitShort(nameIdx); // [val_novo]
    }
    // -----------------------------------------------------------
    // CENÁRIO B: É uma VARIÁVEL (Locais, Upvalues, Globais, Privates)
    // -----------------------------------------------------------
    else
    {
        uint8 getOp = OP_GET_GLOBAL, setOp = OP_SET_GLOBAL;
        int arg = -1;

        // 1. Tenta LOCAL
        arg = resolveLocal(name);
        if (arg != -1)
        {
            getOp = OP_GET_LOCAL;
            setOp = OP_SET_LOCAL;
        }

        // 2. Tenta UPVALUE
        if (arg == -1)
        {
            arg = resolveUpvalue(name);
            if (arg != -1)
            {
                getOp = OP_GET_UPVALUE;
                setOp = OP_SET_UPVALUE;
            }
        }

        // 3. Tenta GLOBAL (se foi declarado como global)
        if (arg == -1 && declaredGlobals_.count(name.lexeme) > 0)
        {
            // OPTIMIZATION: Use global index instead of constant pool
            arg = getOrCreateGlobalIndex(name.lexeme);
            getOp = OP_GET_GLOBAL;
            setOp = OP_SET_GLOBAL;
        }

        // 4. Tenta PRIVATE (só se for Process e não achou global)
        if (arg == -1 && isProcess_)
        {
            int index = (int)vm_->getProcessPrivateIndex(name.lexeme.c_str());
            if (index != -1)
            {
                arg = index;
                getOp = OP_GET_PRIVATE;
                setOp = OP_SET_PRIVATE;
            }
        }

        // 5. Fallback GLOBAL
        if (arg == -1)
        {
            // OPTIMIZATION: Use global index instead of constant pool
            arg = getOrCreateGlobalIndex(name.lexeme);
            getOp = OP_GET_GLOBAL;
            setOp = OP_SET_GLOBAL;
        }

        // --i retorna o valor NOVO
        emitVarOp(getOp, arg);         // [old_value]
        emitConstant(vm_->makeInt(1)); // [old_value, 1]
        emitByte(OP_SUBTRACT);         // [new_value]
        emitVarOp(setOp, arg);         // [new_value] (SET usa PEEK, não remove!)
        // SET já deixa o new_value na stack
    }
}

void Compiler::frameStatement()
{
    // if (!isProcess_)
    // {
    //     error("'frame' can only be used in process body");
    //     return;
    // }

    if (match(TOKEN_LPAREN))
    {
        // frame(expression)
        expression(); // Percentagem vai para stack
        consume(TOKEN_RPAREN, "Expect ')' after percentage");
    }
    else
    {
        // frame; = frame(100);
        emitConstant(vm_->makeInt(100));
    }

    consume(TOKEN_SEMICOLON, "Expect ';' after frame");
    emitByte(OP_FRAME);
}

void Compiler::exitStatement()
{
    if (!isProcess_)
    {
        error("'exit' can only be used in process body");
        return;
    }

    if (match(TOKEN_LPAREN))
    {
        // exit(expression)
        expression();
        consume(TOKEN_RPAREN, "Expect ')' after exit code");
    }
    else
    {
        // exit;
        emitConstant(vm_->makeInt(0));
    }

    consume(TOKEN_SEMICOLON, "Expect ';' after exit");
    emitByte(OP_EXIT);
}

void Compiler::includeStatement()
{
    consume(TOKEN_STRING, "Expect filename after include");

    std::string filename = previous.lexeme.c_str();

    // std::set para proteção circular
    if (includedFiles.find(filename) != includedFiles.end())
    {
        fail("Circular include: %s", filename.c_str());
        return;
    }

    if (!fileLoader)
    {
        fail("No file loader set");
        return;
    }

    // CALLBACK retorna C-style
    size_t sourceSize = 0;
    const char *source = fileLoader(filename.c_str(), &sourceSize, fileLoaderUserdata);

    if (!source || sourceSize == 0)
    {
        fail("Cannot load %s %d", filename.c_str(), sourceSize);
        return;
    }

    // Adiciona ao set
    includedFiles.insert(filename);

    // SALVA estado
    Lexer *oldLexer = this->lexer;
    std::vector<Token> oldTokens = this->tokens;
    Token oldCurrent = this->current;
    Token oldPrevious = this->previous;
    int oldCursor = this->cursor;

    // COMPILA inline
    this->lexer = new Lexer(source, sourceSize);
    this->tokens = lexer->scanAll();
    predeclareProcessGlobals();
    this->cursor = 0;
    advance();

    while (!check(TOKEN_EOF) && !hadError)
    {
        declaration();
    }

    // RESTAURA
    delete this->lexer;
    this->lexer = oldLexer;
    this->tokens = oldTokens;
    this->current = oldCurrent;
    this->previous = oldPrevious;
    this->cursor = oldCursor;

    // Remove do set
    includedFiles.erase(filename);

    consume(TOKEN_SEMICOLON, "Expect ';' after include");
}

void Compiler::parseUsing()
{
    // using raylib, math;

    do
    {
        consume(TOKEN_IDENTIFIER, "Expect module name");
        Token moduleName = previous;
        std::string modName = moduleName.lexeme;

        if (importedModules.find(modName) == importedModules.end())
        {
            fail("Module '%s' not imported. Use 'import %s;' first",
                 moduleName.lexeme.c_str(),
                 moduleName.lexeme.c_str());
            return;
        }

        if (usingModules.find(modName) != usingModules.end())
        {
            Warning("Module '%s' already using", moduleName.lexeme.c_str());
        }
        else
        {
            usingModules.insert(modName);
        }

    } while (match(TOKEN_COMMA));

    consume(TOKEN_SEMICOLON, "Expect ';'");
}

void Compiler::parseImport()
{
    // import *;
    if (match(TOKEN_STAR))
    {
        // Importa TODOS os módulos definidos
        for (size_t i = 0; i < vm_->modules.size(); i++)
        {
            ModuleDef *mod = vm_->modules[i];
            importedModules.insert(mod->getName()->chars());
        }
        consume(TOKEN_SEMICOLON, "Expect ';'");
        return;
    }

    // import timer, net, fs;
    do
    {
        consume(TOKEN_IDENTIFIER, "Expect module name");
        Token moduleName = previous;
        std::string modName = moduleName.lexeme;

        // Verifica se módulo existe
        if (!vm_->containsModule(modName.c_str()))
        {
            fail("Module '%s' not defined", moduleName.lexeme.c_str());
            return;
        }

        //  Adiciona a importedModules, não usingModules!
        if (importedModules.find(modName) == importedModules.end())
        {
            importedModules.insert(modName);
        }
        else
        {
            Warning("Module '%s' already imported", moduleName.lexeme.c_str());
        }

    } while (match(TOKEN_COMMA));

    consume(TOKEN_SEMICOLON, "Expect ';' after import");
}

void Compiler::parseRequire()
{
    // require "SDL";
    // require "glfw,rlgl";      // múltiplos separados por vírgula
    // require "glfw;rlgl;gtk";  // múltiplos separados por ponto e vírgula

    consume(TOKEN_STRING, "Expect plugin name as string after 'require'");
    std::string pluginList = previous.lexeme;

    // Remove quotes from string literal
    if (pluginList.size() >= 2 && pluginList.front() == '"' && pluginList.back() == '"')
    {
        pluginList = pluginList.substr(1, pluginList.size() - 2);
    }

    // Parse múltiplos plugins separados por ',' ou ';'
    size_t start = 0;
    size_t end = 0;

    while (start < pluginList.size())
    {
        // Encontrar o próximo separador (',' ou ';')
        end = pluginList.find_first_of(",;", start);
        if (end == std::string::npos)
        {
            end = pluginList.size();
        }

        // Extrair nome do plugin e remover espaços
        std::string pluginName = pluginList.substr(start, end - start);

        // Trim whitespace
        size_t first = pluginName.find_first_not_of(" \t");
        size_t last = pluginName.find_last_not_of(" \t");
        if (first != std::string::npos && last != std::string::npos)
        {
            pluginName = pluginName.substr(first, last - first + 1);
        }

        // Ignorar strings vazias
        if (!pluginName.empty())
        {
            // Check if module is already loaded
            if (!vm_->containsModule(pluginName.c_str()))
            {
                // Try to load the plugin
                if (!vm_->loadPluginByName(pluginName.c_str()))
                {
                    fail("Failed to load plugin '%s': %s",
                         pluginName.c_str(),
                         vm_->getLastPluginError());
                    return;
                }

                // IMPORTANT: Resync global indices after loading plugin
                // The plugin may have registered new native functions, structs, and classes
                // that need to be available to the compiler
                vm_->nativeGlobalIndices.forEach([this](String *nameStr, uint16 index)
                                                 {
                    const std::string name = nameStr->chars();
                    if (globalIndices_.find(name) == globalIndices_.end())
                    {
                        // New native from the plugin - add to compiler indices
                        globalIndices_[name] = index;
                        if (index >= globalIndexToName_.size())
                        {
                            globalIndexToName_.resize(index + 1);
                        }
                        globalIndexToName_[index] = name;
                        declaredGlobals_.insert(name);
                    } });
                // Update nextGlobalIndex to be after all registered natives
                nextGlobalIndex_ = static_cast<uint16>(vm_->globalsArray.size());
            }
        }

        start = end + 1;
    }

    consume(TOKEN_SEMICOLON, "Expect ';' after require");
}

void Compiler::dot(bool canAssign)
{
    consumeIdentifierLike("Expect property name after '.'");
    Token propName = previous;

    uint16_t nameIdx = identifierConstant(propName);

    //  METHOD CALL
    if (match(TOKEN_LPAREN))
    {
        uint8_t argCount = argumentList();
        if (propName.lexeme == "push" && argCount == 1)
        {
            emitByte(OP_ARRAY_PUSH);
            emitByte(argCount);
        }
        else
        {
            emitByte(OP_INVOKE);
            emitShort(nameIdx);
            emitByte(argCount);
        }
    }
    // SIMPLE ASSIGNMENT
    else if (canAssign && match(TOKEN_EQUAL))
    {
        expression();
        emitByte(OP_SET_PROPERTY);
        emitShort(nameIdx);
    }
    //  COMPOUND ASSIGNMENTS
    else if (canAssign && match(TOKEN_PLUS_EQUAL))
    {
        // self.x += value
        // Stack antes: [self]
        emitByte(OP_DUP); // [self, self]
        emitByte(OP_GET_PROPERTY);
        emitShort(nameIdx); // [self, old_x]
        expression();       // [self, old_x, value]
        emitByte(OP_ADD);   // [self, new_x]
        emitByte(OP_SET_PROPERTY);
        emitShort(nameIdx); // []
    }
    else if (canAssign && match(TOKEN_MINUS_EQUAL))
    {
        emitByte(OP_DUP);
        emitByte(OP_GET_PROPERTY);
        emitShort(nameIdx);
        expression();
        emitByte(OP_SUBTRACT);
        emitByte(OP_SET_PROPERTY);
        emitShort(nameIdx);
    }
    else if (canAssign && match(TOKEN_STAR_EQUAL))
    {
        emitByte(OP_DUP);
        emitByte(OP_GET_PROPERTY);
        emitShort(nameIdx);
        expression();
        emitByte(OP_MULTIPLY);
        emitByte(OP_SET_PROPERTY);
        emitShort(nameIdx);
    }
    else if (canAssign && match(TOKEN_SLASH_EQUAL))
    {
        emitByte(OP_DUP);
        emitByte(OP_GET_PROPERTY);
        emitShort(nameIdx);
        expression();
        emitByte(OP_DIVIDE);
        emitByte(OP_SET_PROPERTY);
        emitShort(nameIdx);
    }
    else if (canAssign && match(TOKEN_PERCENT_EQUAL))
    {
        emitByte(OP_DUP);
        emitByte(OP_GET_PROPERTY);
        emitShort(nameIdx);
        expression();
        emitByte(OP_MODULO);
        emitByte(OP_SET_PROPERTY);
        emitShort(nameIdx);
    }
    //  INCREMENT/DECREMENT
    else if (canAssign && match(TOKEN_PLUS_PLUS))
    {
        // self.x++ (postfix) - retorna valor ANTIGO
        // Stack: [self]
        emitByte(OP_DUP); // [self, self]
        emitByte(OP_GET_PROPERTY);
        emitShort(nameIdx); // [self, old_x]
        emitByte(OP_SWAP);  // [old_x, self]
        emitByte(OP_DUP);   // [old_x, self, self]
        emitByte(OP_GET_PROPERTY);
        emitShort(nameIdx);            // [old_x, self, old_x]
        emitConstant(vm_->makeInt(1)); // [old_x, self, old_x, 1]
        emitByte(OP_ADD);              // [old_x, self, new_x]
        emitByte(OP_SET_PROPERTY);
        emitShort(nameIdx); // [old_x, new_x]
        emitByte(OP_POP);   // [old_x] ← resultado correto!
    }
    else if (canAssign && match(TOKEN_MINUS_MINUS))
    {
        // self.x-- (postfix) - retorna valor ANTIGO
        // Stack: [self]
        emitByte(OP_DUP); // [self, self]
        emitByte(OP_GET_PROPERTY);
        emitShort(nameIdx); // [self, old_x]
        emitByte(OP_SWAP);  // [old_x, self]
        emitByte(OP_DUP);   // [old_x, self, self]
        emitByte(OP_GET_PROPERTY);
        emitShort(nameIdx);            // [old_x, self, old_x]
        emitConstant(vm_->makeInt(1)); // [old_x, self, old_x, 1]
        emitByte(OP_SUBTRACT);         // [old_x, self, new_x]
        emitByte(OP_SET_PROPERTY);
        emitShort(nameIdx); // [old_x, new_x]
        emitByte(OP_POP);   // [old_x] ← resultado correto!
    }
    //  GET ONLY
    else
    {
        emitByte(OP_GET_PROPERTY);
        emitShort(nameIdx);
    }
}

void Compiler::subscript(bool canAssign)
{
    // arr[index], arr[index] = value ou arr[index] op= value

    expression(); // Index expression
    consume(TOKEN_RBRACKET, "Expect ']' after subscript");

    if (canAssign && match(TOKEN_EQUAL))
    {
        // arr[i] = value
        expression(); // Value
        emitByte(OP_SET_INDEX);
    }
    else if (canAssign && match(TOKEN_PLUS_EQUAL))
    {
        // Stack: [container, index]
        emitByte(OP_COPY2);    // [container, index, container, index]
        emitByte(OP_GET_INDEX); // [container, index, old_value]
        expression();          // [container, index, old_value, value]
        emitByte(OP_ADD);      // [container, index, new_value]
        emitByte(OP_SET_INDEX); // [new_value]
    }
    else if (canAssign && match(TOKEN_MINUS_EQUAL))
    {
        emitByte(OP_COPY2);
        emitByte(OP_GET_INDEX);
        expression();
        emitByte(OP_SUBTRACT);
        emitByte(OP_SET_INDEX);
    }
    else if (canAssign && match(TOKEN_STAR_EQUAL))
    {
        emitByte(OP_COPY2);
        emitByte(OP_GET_INDEX);
        expression();
        emitByte(OP_MULTIPLY);
        emitByte(OP_SET_INDEX);
    }
    else if (canAssign && match(TOKEN_SLASH_EQUAL))
    {
        emitByte(OP_COPY2);
        emitByte(OP_GET_INDEX);
        expression();
        emitByte(OP_DIVIDE);
        emitByte(OP_SET_INDEX);
    }
    else if (canAssign && match(TOKEN_PERCENT_EQUAL))
    {
        emitByte(OP_COPY2);
        emitByte(OP_GET_INDEX);
        expression();
        emitByte(OP_MODULO);
        emitByte(OP_SET_INDEX);
    }
    else
    {
        // arr[i]
        emitByte(OP_GET_INDEX);
    }
}

void Compiler::labelStatement()
{
    if (labels.size() >= MAX_LABELS)
    {
        error("Too many labels in function");
        advance();
        consume(TOKEN_COLON, "Expect ':'");
        return;
    }

    Token labelName = current;
    advance();
    consume(TOKEN_COLON, "Expect ':'");

    for (const Label &l : labels)
    {
        if (l.name == labelName.lexeme)
        {
            fail("Label '%s' already defined", labelName.lexeme.c_str());
            return;
        }
    }

    Label newLabel;
    newLabel.name = labelName.lexeme;
    newLabel.offset = currentChunk->count;

    labels.push_back(newLabel);
}

void Compiler::gotoStatement()
{
    if (pendingGotos.size() >= MAX_GOTOS)
    {
        error("Too many goto statements");
        consume(TOKEN_IDENTIFIER, "Expect label name");
        consume(TOKEN_SEMICOLON, "Expect ';'");
        return;
    }

    consume(TOKEN_IDENTIFIER, "Expect label name");
    Token target = previous;
    consume(TOKEN_SEMICOLON, "Expect ';'");

    emitByte(OP_JUMP);

    GotoJump jump;
    jump.target = target.lexeme;
    jump.jumpOffset = currentChunk->count;

    emitByte(0xFF);
    emitByte(0xFF);

    pendingGotos.push_back(jump);
}

void Compiler::gosubStatement()
{
    if (pendingGosubs.size() >= MAX_GOSUBS)
    {
        error("Too many gosub statements");
        consume(TOKEN_IDENTIFIER, "Expect label name");
        consume(TOKEN_SEMICOLON, "Expect ';'");
        return;
    }

    consume(TOKEN_IDENTIFIER, "Expect label name");
    Token target = previous;
    consume(TOKEN_SEMICOLON, "Expect ';'");

    emitByte(OP_GOSUB);

    GotoJump jump;
    jump.target = target.lexeme;
    jump.jumpOffset = currentChunk->count;

    emitByte(0xFF);
    emitByte(0xFF);

    pendingGosubs.push_back(jump);
}

void Compiler::structDeclaration()
{
    isProcess_ = false;
    consume(TOKEN_IDENTIFIER, "Expect struct name");
    Token structName = previous;
    uint16_t nameConstant = identifierConstant(structName);

    validateIdentifierName(structName);
    if (hadError)
    {
        return;
    }
    consume(TOKEN_LBRACE, "Expect '{' before struct body");

    StructDef *structDef = vm_->registerStruct(vm_->createString(structName.lexeme.c_str()));

    if (!structDef)
    {
        fail("Struct with name '%s' already exists", structName.lexeme.c_str());
        return;
    }

    structDef->argCount = 0;

    //  Loop externo: múltiplas linhas de fields
    while (!check(TOKEN_RBRACE) && !check(TOKEN_EOF))
    {

        if (check(TOKEN_SEMICOLON))
        {
            // Se encontrarmos um ; perdido, ignoramos e continuamos
            advance();
            continue;
        }
        // 1. Opcional: pode ter 'var' ou não
        bool hasVar = match(TOKEN_VAR);

        // 2. Loop interno: múltiplos fields separados por vírgula
        do
        {
            consumeIdentifierLike("Expect field name");

            String *fieldName = vm_->createString(previous.lexeme.c_str());
            // Não validar keywords - campos podem ter nomes como "loop", "break", etc.
            bool wasReplaced = structDef->names.set(fieldName, structDef->argCount);
            if (!wasReplaced)
            {
                Warning("Field '%s' redefined in struct '%s' (previous value replaced)",
                        fieldName->chars(), structName.lexeme.c_str());
            }
            structDef->argCount++;

        } while (match(TOKEN_COMMA));

        // === CORREÇÃO AQUI ===
        // Se tinha 'var', o ';' é obrigatório.
        if (hasVar)
        {
            consume(TOKEN_SEMICOLON, "Expect ';' after field declaration");
        }
        else
        {
            // Se não tinha 'var', o ';' é opcional, mas se estiver lá, TEMOS de o comer.
            // Caso contrário, ele bloqueia o loop seguinte.
            match(TOKEN_SEMICOLON);
        }
    }

    consume(TOKEN_RBRACE, "Expect '}' after struct body");

    // Opcional: permitir ou exigir ; no final da struct
    match(TOKEN_SEMICOLON);

    emitConstant(vm_->makeStruct(structDef->index));

    // OPTIMIZATION: Use global index for struct name instead of constant pool
    if (scopeDepth == 0)
    {
        uint16_t global = getOrCreateGlobalIndex(structName.lexeme);
        declaredGlobals_.insert(structName.lexeme);
        defineVariable(global);
    }
    else
    {
        defineVariable(nameConstant);
    }
}

void Compiler::self(bool canAssign)
{
    (void)canAssign;
    isProcess_ = false;
    if (currentClass == nullptr)
    {
        error("Cannot use 'self' outside of a class");
        return;
    }
    Token selfToken;
    selfToken.lexeme = "self";
    selfToken.type = TOKEN_IDENTIFIER;
    namedVariable(selfToken, canAssign);
}

void Compiler::super(bool canAssign)
{
    (void)canAssign;
    isProcess_ = false;
    if (currentClass == nullptr)
    {
        error("Cannot use 'super' outside of a class");
        return;
    }

    if (currentClass->superclass == nullptr)
    {
        error("Cannot use 'super' in a class with no superclass");
        return;
    }

    consume(TOKEN_DOT, "Expect '.' after 'super'");
    consume(TOKEN_IDENTIFIER, "Expect superclass method name");
    Token methodName = previous;
    uint16_t nameIdx = identifierConstant(methodName);

    consume(TOKEN_LPAREN, "Expect '(' after method name");

    // SELF PRIMEIRO!
    emitBytes(OP_GET_LOCAL, 0);

    // DEPOIS ARGUMENTOS!
    uint8_t argCount = argumentList();

    emitByte(OP_SUPER_INVOKE);
    emitByte(currentClass->index);
    emitShort(nameIdx);
    emitByte(argCount);
}

void Compiler::classDeclaration()
{
    isProcess_ = false;
    consume(TOKEN_IDENTIFIER, "Expect class name");
    Token className = previous;

    validateIdentifierName(className);
    if (hadError)
    {
        return;
    }
    //  Regista class blueprint na VM

    ClassDef *classDef = vm_->registerClass(
        vm_->createString(className.lexeme.c_str()));

    if (!classDef)
    {
        fail("Class with name '%s' already exists", className.lexeme.c_str());
        return;
    }

    // Emite class ID como constante
    emitConstant(vm_->makeClass(classDef->index));
    // OPTIMIZATION: Use global index instead of constant pool
    declaredGlobals_.insert(className.lexeme);
    uint16_t globalIndex = getOrCreateGlobalIndex(className.lexeme);
    defineVariable(globalIndex);

    // Herança?
    if (match(TOKEN_COLON))
    {
        consume(TOKEN_IDENTIFIER, "Expect superclass name");
        Token superName = previous;

        const char *name = superName.lexeme.c_str();

        // Primeiro tenta ClassDef (script class)
        ClassDef *classSuper = nullptr;
        if (vm_->tryGetClassDefenition(name, &classSuper))
        {
            if (classSuper == classDef)
            {
                fail("A class cannot inherit from itself");
                return;
            }
            classDef->inherited = true;
            classDef->parent = classSuper->name;
            classDef->superclass = classSuper;
            // Copia fieldCount também (herdar fields)
            classSuper->fieldNames.forEach([&](String *fieldName, uint8_t index)
                                           {
                classDef->fieldNames.set(fieldName, classDef->fieldCount);
                classDef->fieldCount++; });
        }
        else
        {
            // Tenta NativeClassDef (C++ class)
            NativeClassDef *nativeSuper = nullptr;
            if (vm_->tryGetNativeClassDef(name, &nativeSuper))
            {
                classDef->inherited = true;
                classDef->parent = nativeSuper->name;
                classDef->nativeSuperclass = nativeSuper;
                // NativeClass não tem fields para copiar, só properties
            }
            else
            {
                fail("Undefined superclass '%s'", superName.lexeme.c_str());
                return;
            }
        }
    }
    consume(TOKEN_LBRACE, "Expect '{'");

    while (check(TOKEN_VAR))
    {
        advance(); // Consome 'var'
        do
        {
            consumeIdentifierLike("Expect field name");
            Token fieldName = previous;
            // Não validar keywords - campos podem ter nomes como "loop", "break", etc.
            String *name = vm_->createString(fieldName.lexeme.c_str());
            // classDef->fieldNames.set(name, classDef->fieldCount);

            bool wasReplaced = classDef->fieldNames.set(name, classDef->fieldCount);
            if (!wasReplaced)
            {
                Warning("Field '%s' redefined in class '%s' (previous value replaced)",
                        fieldName.lexeme.c_str(), className.lexeme.c_str());
            }

            classDef->fieldCount++;

            // Parse default value if present
            if (match(TOKEN_EQUAL))
            {
                // Check for literal default values
                if (match(TOKEN_INT))
                {
                    // Parse integer literal
                    int64_t value = std::strtoll(previous.lexeme.c_str(), nullptr, 10);
                    classDef->fieldDefaults.push(vm_->makeInt(value));
                }
                else if (match(TOKEN_FLOAT))
                {
                    // Parse float literal
                    double value = std::strtod(previous.lexeme.c_str(), nullptr);
                    classDef->fieldDefaults.push(vm_->makeDouble(value));
                }
                else if (match(TOKEN_MINUS))
                {
                    // Support negative numeric literals: -123 / -3.14
                    if (match(TOKEN_INT))
                    {
                        int64_t value = -std::strtoll(previous.lexeme.c_str(), nullptr, 10);
                        classDef->fieldDefaults.push(vm_->makeInt(value));
                    }
                    else if (match(TOKEN_FLOAT))
                    {
                        double value = -std::strtod(previous.lexeme.c_str(), nullptr);
                        classDef->fieldDefaults.push(vm_->makeDouble(value));
                    }
                    else
                    {
                        // Non-literal expression - compile and discard, use nil
                        Warning("Field '%s' in class '%s': complex default not supported, using nil (set it in init())",
                                fieldName.lexeme.c_str(), className.lexeme.c_str());
                        expression();
                        emitByte(OP_POP);
                        classDef->fieldDefaults.push(vm_->makeNil());
                    }
                }
                else if (match(TOKEN_PLUS))
                {
                    // Support positive numeric literals: +123 / +3.14
                    if (match(TOKEN_INT))
                    {
                        int64_t value = std::strtoll(previous.lexeme.c_str(), nullptr, 10);
                        classDef->fieldDefaults.push(vm_->makeInt(value));
                    }
                    else if (match(TOKEN_FLOAT))
                    {
                        double value = std::strtod(previous.lexeme.c_str(), nullptr);
                        classDef->fieldDefaults.push(vm_->makeDouble(value));
                    }
                    else
                    {
                        // Non-literal expression - compile and discard, use nil
                        Warning("Field '%s' in class '%s': complex default not supported, using nil (set it in init())",
                                fieldName.lexeme.c_str(), className.lexeme.c_str());
                        expression();
                        emitByte(OP_POP);
                        classDef->fieldDefaults.push(vm_->makeNil());
                    }
                }
                else if (match(TOKEN_STRING))
                {
                    // Parse string literal
                    String *str = vm_->createString(previous.lexeme.c_str());
                    classDef->fieldDefaults.push(vm_->makeString(str));
                }
                else if (match(TOKEN_TRUE))
                {
                    classDef->fieldDefaults.push(vm_->makeBool(true));
                }
                else if (match(TOKEN_FALSE))
                {
                    classDef->fieldDefaults.push(vm_->makeBool(false));
                }
                else if (match(TOKEN_NIL))
                {
                    classDef->fieldDefaults.push(vm_->makeNil());
                }
                else
                {
                    // Non-literal expression - compile and discard, use nil
                    Warning("Field '%s' in class '%s': complex default not supported, using nil (set it in init())",
                            fieldName.lexeme.c_str(), className.lexeme.c_str());
                    expression();
                    emitByte(OP_POP);
                    classDef->fieldDefaults.push(vm_->makeNil());
                }
            }
            else
            {
                // No default - use nil
                classDef->fieldDefaults.push(vm_->makeNil());
            }

        } while (match(TOKEN_COMMA)); //  Continua se tem vírgula

        consume(TOKEN_SEMICOLON, "Expect ';'");
    }
    // Parse métodos
    while (match(TOKEN_DEF))
    {
        method(classDef); // Passa ClassDef!
    }

    consume(TOKEN_RBRACE, "Expect '}'");

    if (classDef->constructor == nullptr)
    {
        Warning("Class '%s' has no init() method - fields will be uninitialized (nil)",
                className.lexeme.c_str());
    }
}

void Compiler::method(ClassDef *classDef)
{
    isProcess_ = false;
    consume(TOKEN_IDENTIFIER, "Expect method name");
    Token methodName = previous;

    // Tipo de função
    this->currentFunctionType = FunctionType::TYPE_METHOD;
    // Registra função
    //    std::string funcName = classDef->name->chars() +std::string("::") + methodName.lexeme;
    std::string funcName = methodName.lexeme;
    Function *func = classDef->canRegisterFunction(vm_->createString(funcName.c_str()));
    if (!func)
    {
        fail("Function '%s' already exists in class '%s' ", funcName.c_str(), classDef->name->chars());
        return;
    }

    if (funcName == "init")
    {
        classDef->constructor = func;
        this->currentFunctionType = FunctionType::TYPE_INITIALIZER;
    }

    vm_->addFunctionsClasses(func);

    // ===== SALVA ESTADO =====
    Function *enclosing = this->function;
    Code *enclosingChunk = this->currentChunk;
    int enclosingScopeDepth = this->scopeDepth;
    int enclosingLocalCount = this->localCount_;
    ClassDef *enclosingClass = this->currentClass;

    // ===== TROCA CONTEXTO =====
    this->function = func;
    this->currentChunk = func->chunk;
    this->scopeDepth = 0;
    this->localCount_ = 0;
    this->currentClass = classDef;

    // ===== SCOPE DA FUNÇÃO =====
    beginScope(); // scopeDepth = 1

    // ===== SELF = LOCAL[0] =====
    Token selfToken;
    selfToken.lexeme = "self";
    selfToken.type = TOKEN_IDENTIFIER;

    addLocal(selfToken);
    markInitialized();

    // ===== PARAMS =====
    consume(TOKEN_LPAREN, "Expect '('");

    if (!check(TOKEN_RPAREN))
    {
        do
        {
            func->arity++;
            if (func->arity > 255)
            {
                error("Can't have more than 255 parameters");
                break;
            }

            consume(TOKEN_IDENTIFIER, "Expect parameter name");
            addLocal(previous);
            markInitialized();

        } while (match(TOKEN_COMMA));
    }

    consume(TOKEN_RPAREN, "Expect ')'");

    // ===== CORPO =====
    consume(TOKEN_LBRACE, "Expect '{'");
    block(); // block() chama beginScope/endScope internamente

    // Os parâmetros (self + args) ficam até o return

    // ===== RETURN =====
    if (currentFunctionType == FunctionType::TYPE_INITIALIZER)
    {
        // init() retorna self automaticamente
        emitBytes(OP_GET_LOCAL, 0); // self
        emitByte(OP_RETURN);
        function->hasReturn = true;
    }
    else
    {
        // ALWAYS emit implicit return for methods to handle early returns in conditionals
        // If method already returned via explicit return, this code is unreachable (dead code)
        // but ensures all execution paths end with OP_RETURN
        emitBytes(OP_GET_LOCAL, 0); // self
        emitByte(OP_RETURN);
        function->hasReturn = true;
    }

    endScope();

    // ===== RESTAURA ESTADO =====
    this->function = enclosing;
    this->currentChunk = enclosingChunk;
    this->scopeDepth = enclosingScopeDepth;
    this->localCount_ = enclosingLocalCount;
    this->currentClass = enclosingClass;
    this->currentFunctionType = FunctionType::TYPE_SCRIPT;
}

void Compiler::tryStatement()
{
    isProcess_ = false;
    consume(TOKEN_LBRACE, "Expect '{' after 'try'");

    if (tryDepth >= MAX_TRY_DEPTH)
    {
        error("Try blocks nested too deeply");
        int depth = 1;
        while (depth > 0 && !check(TOKEN_EOF))
        {
            if (match(TOKEN_LBRACE))
                depth++;
            else if (match(TOKEN_RBRACE))
                depth--;
            else
                advance();
        }
        return;
    }

    tryDepth++;

    emitByte(OP_TRY);

    int catchAddrOffset = currentChunk->count;
    emitByte(0xFF);
    emitByte(0xFF);

    int finallyAddrOffset = currentChunk->count;
    emitByte(0xFF);
    emitByte(0xFF);

    beginScope();
    block();
    endScope();

    emitByte(OP_POP_TRY);
    int tryExitJump = emitJump(OP_JUMP);

    int catchStart = -1;
    int catchExitJump = -1;

    if (match(TOKEN_CATCH))
    {
        catchStart = currentChunk->count;

        consume(TOKEN_LPAREN, "Expect '(' after 'catch'");
        consume(TOKEN_IDENTIFIER, "Expect exception variable");
        Token errorVar = previous;
        consume(TOKEN_RPAREN, "Expect ')'");
        consume(TOKEN_LBRACE, "Expect '{'");

        emitByte(OP_ENTER_CATCH);

        beginScope();
        addLocal(errorVar);
        markInitialized();

        block();
        endScope();

        emitByte(OP_POP_TRY);
        catchExitJump = emitJump(OP_JUMP);
    }

    int finallyStart = -1;
    if (match(TOKEN_FINALLY))
    {
        finallyStart = currentChunk->count;

        patchJump(tryExitJump);

        if (catchExitJump != -1)
        {
            patchJump(catchExitJump);
        }

        consume(TOKEN_LBRACE, "Expect '{'");

        emitByte(OP_ENTER_FINALLY);

        beginScope();
        block();
        endScope();

        emitByte(OP_EXIT_FINALLY);
    }
    else
    {
        patchJump(tryExitJump);
        if (catchExitJump != -1)
        {
            patchJump(catchExitJump);
        }
    }

    if (catchStart == -1 && finallyStart == -1)
    {
        error("Try must have catch or finally block");
    }

    if (catchStart != -1)
    {
        currentChunk->code[catchAddrOffset] = (catchStart >> 8) & 0xFF;
        currentChunk->code[catchAddrOffset + 1] = catchStart & 0xFF;
    }

    if (finallyStart != -1)
    {
        currentChunk->code[finallyAddrOffset] = (finallyStart >> 8) & 0xFF;
        currentChunk->code[finallyAddrOffset + 1] = finallyStart & 0xFF;
    }

    tryDepth--;
}

void Compiler::throwStatement()
{
    expression();
    if (hadError)
        return;
    consume(TOKEN_SEMICOLON, "Expect ';' after throw");
    emitByte(OP_THROW);
}
