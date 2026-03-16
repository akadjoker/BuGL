#include "DocumentSymbols.h"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <string_view>
#include <vector>

namespace
{
struct SymbolStackEntry
{
    int indent = 0;
    DocumentSymbol* symbol = nullptr;
};

std::string_view TrimLeft(std::string_view value)
{
    while (!value.empty() && std::isspace(static_cast<unsigned char>(value.front())))
    {
        value.remove_prefix(1);
    }
    return value;
}

std::string_view TrimRight(std::string_view value)
{
    while (!value.empty() && std::isspace(static_cast<unsigned char>(value.back())))
    {
        value.remove_suffix(1);
    }
    return value;
}

std::string_view Trim(std::string_view value)
{
    return TrimRight(TrimLeft(value));
}

int CountIndent(std::string_view value)
{
    int indent = 0;
    for (char ch : value)
    {
        if (ch == ' ')
        {
            ++indent;
        }
        else if (ch == '\t')
        {
            indent += 4;
        }
        else
        {
            break;
        }
    }
    return indent;
}

void StripSingleLineComment(std::string_view& value)
{
    const size_t comment_pos = value.find("//");
    if (comment_pos != std::string_view::npos)
    {
        value = value.substr(0, comment_pos);
    }
}

bool ParseSymbolDeclaration(std::string_view line, DocumentSymbolKind& out_kind, std::string& out_name)
{
    struct Keyword
    {
        const char* text;
        DocumentSymbolKind kind;
    };

    static const Keyword keywords[] = {
        {"class", DocumentSymbolKind::Class},
        {"struct", DocumentSymbolKind::Struct},
        {"def", DocumentSymbolKind::Def},
        {"process", DocumentSymbolKind::Process},
    };

    const std::string_view trimmed = Trim(line);
    for (const Keyword& keyword : keywords)
    {
        const std::string_view token(keyword.text);
        if (trimmed.rfind(token, 0) != 0)
        {
            continue;
        }

        if (trimmed.size() > token.size() &&
            !std::isspace(static_cast<unsigned char>(trimmed[token.size()])))
        {
            continue;
        }

        std::string_view tail = TrimLeft(trimmed.substr(token.size()));
        if (tail.empty())
        {
            return false;
        }

        size_t name_end = 0;
        while (name_end < tail.size())
        {
            const char ch = tail[name_end];
            if (std::isalnum(static_cast<unsigned char>(ch)) || ch == '_')
            {
                ++name_end;
                continue;
            }
            break;
        }

        if (name_end == 0)
        {
            return false;
        }

        out_kind = keyword.kind;
        out_name.assign(tail.substr(0, name_end));
        return true;
    }

    return false;
}

void FinalizeSymbolRanges(std::deque<DocumentSymbol>& symbols, int last_line)
{
    for (size_t i = 0; i < symbols.size(); ++i)
    {
        DocumentSymbol& symbol = symbols[i];
        const int next_line = i + 1 < symbols.size() ? symbols[i + 1].line - 1 : last_line;
        symbol.end_line = std::max(symbol.line, next_line);
        if (!symbol.children.empty())
        {
            FinalizeSymbolRanges(symbol.children, symbol.end_line);
            symbol.end_line = std::max(symbol.end_line, symbol.children.back().end_line);
        }
    }
}
}

std::deque<DocumentSymbol> ScanDocumentSymbols(const std::string& text)
{
    std::deque<DocumentSymbol> roots;
    std::vector<SymbolStackEntry> container_stack;

    std::istringstream stream(text);
    std::string line_text;
    int line_index = 0;
    while (std::getline(stream, line_text))
    {
        std::string_view view(line_text);
        StripSingleLineComment(view);
        view = TrimRight(view);
        if (Trim(view).empty())
        {
            ++line_index;
            continue;
        }

        DocumentSymbolKind kind = DocumentSymbolKind::Def;
        std::string name;
        if (!ParseSymbolDeclaration(view, kind, name))
        {
            ++line_index;
            continue;
        }

        const int indent = CountIndent(view);
        while (!container_stack.empty() && indent <= container_stack.back().indent)
        {
            container_stack.pop_back();
        }

        if (kind == DocumentSymbolKind::Process)
        {
            container_stack.clear();
        }

        DocumentSymbol symbol;
        symbol.kind = kind;
        symbol.name = std::move(name);
        symbol.line = line_index;
        symbol.end_line = line_index;
        symbol.indent = indent;

        DocumentSymbol* inserted = nullptr;
        if (kind == DocumentSymbolKind::Def && !container_stack.empty())
        {
            container_stack.back().symbol->children.push_back(std::move(symbol));
            inserted = &container_stack.back().symbol->children.back();
        }
        else
        {
            roots.push_back(std::move(symbol));
            inserted = &roots.back();
        }

        if (kind == DocumentSymbolKind::Class || kind == DocumentSymbolKind::Struct)
        {
            container_stack.push_back({indent, inserted});
        }

        ++line_index;
    }

    FinalizeSymbolRanges(roots, std::max(0, line_index - 1));
    return roots;
}
