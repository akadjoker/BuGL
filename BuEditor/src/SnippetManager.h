#pragma once

#include <string>
#include <vector>

struct BuLangSnippet
{
    std::string name;
    std::string description;
    std::string body;          // Template text (use | for cursor position)
};

namespace SnippetManager
{

inline const std::vector<BuLangSnippet>& GetSnippets()
{
    static const std::vector<BuLangSnippet> snippets = {
        {
            "class",
            "Class definition with constructor",
            "class Name {\n"
            "    def init() {\n"
            "        |\n"
            "    }\n"
            "}\n"
        },
        {
            "def",
            "Function definition",
            "def name() {\n"
            "    |\n"
            "}\n"
        },
        {
            "defargs",
            "Function with parameters",
            "def name(a, b) {\n"
            "    |\n"
            "}\n"
        },
        {
            "for",
            "For loop (counting)",
            "for (var i = 0; i < count; i = i + 1) {\n"
            "    |\n"
            "}\n"
        },
        {
            "foreach",
            "Foreach loop",
            "foreach (var item in collection) {\n"
            "    |\n"
            "}\n"
        },
        {
            "if",
            "If statement",
            "if (condition) {\n"
            "    |\n"
            "}\n"
        },
        {
            "ifelse",
            "If/else statement",
            "if (condition) {\n"
            "    |\n"
            "} else {\n"
            "    \n"
            "}\n"
        },
        {
            "ifelif",
            "If/elif/else statement",
            "if (condition) {\n"
            "    |\n"
            "} elif (other) {\n"
            "    \n"
            "} else {\n"
            "    \n"
            "}\n"
        },
        {
            "while",
            "While loop",
            "while (condition) {\n"
            "    |\n"
            "}\n"
        },
        {
            "dowhile",
            "Do-while loop",
            "do {\n"
            "    |\n"
            "} while (condition);\n"
        },
        {
            "switch",
            "Switch statement",
            "switch (value) {\n"
            "    case 1:\n"
            "        |break;\n"
            "    default:\n"
            "        break;\n"
            "}\n"
        },
        {
            "struct",
            "Struct definition",
            "struct Name {\n"
            "    var x = 0;\n"
            "    var y = 0;|\n"
            "}\n"
        },
        {
            "try",
            "Try/catch block",
            "try {\n"
            "    |\n"
            "} catch (e) {\n"
            "    print(e);\n"
            "}\n"
        },
        {
            "process",
            "Process (coroutine)",
            "process name() {\n"
            "    |\n"
            "    frame;\n"
            "}\n"
        },
        {
            "print",
            "Print statement",
            "print(|);\n"
        },
        {
            "var",
            "Variable declaration",
            "var name = |;\n"
        },
        {
            "import",
            "Import module",
            "import \"|\";\n"
        },
        {
            "include",
            "Include file",
            "include \"|\";\n"
        },
        {
            "main",
            "Main script template",
            "// Main entry point\n"
            "\n"
            "def main() {\n"
            "    |\n"
            "}\n"
            "\n"
            "main();\n"
        },
    };
    return snippets;
}

// Insert a snippet into a TextEditor at the current cursor position.
// Returns the text that was inserted (with | removed).
inline std::string PrepareSnippetText(const BuLangSnippet& snippet, int& cursor_offset)
{
    std::string text = snippet.body;
    cursor_offset = -1;
    size_t marker = text.find('|');
    if (marker != std::string::npos)
    {
        cursor_offset = static_cast<int>(marker);
        text.erase(marker, 1);
    }
    return text;
}

} // namespace SnippetManager
