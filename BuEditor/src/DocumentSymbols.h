#pragma once

#include <deque>
#include <string>

enum class DocumentSymbolKind
{
    Class,
    Struct,
    Def,
    Process
};

struct DocumentSymbol
{
    DocumentSymbolKind kind = DocumentSymbolKind::Def;
    std::string name;
    int line = 0;
    int end_line = 0;
    int indent = 0;
    std::deque<DocumentSymbol> children;
};

std::deque<DocumentSymbol> ScanDocumentSymbols(const std::string& text);
