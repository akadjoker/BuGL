#include "TextEditor.h"

const TextEditor::LanguageDefinition& TextEditor::LanguageDefinition::BuLang()
{
	static bool inited = false;
	static LanguageDefinition langDef;
	if (!inited)
	{
		static const char* const buKeywords[] = {
			"break", "case", "foreach","class", "include", "continue", "elif","else", "self","parent","try", "catch",
			"false", "for", "if", "import", "nil", "require", "def", "default","process", "switch",
			"return", "struct", "true", "using", "var", "while","do","frame",
		};

		static const char* const buBuiltins[] = {
			"format", "int", "len", "print", "real",  "str", "typeid", "write","sin","cos","tan","sqrt","abs","floor","ceil","round",
			"Vector2", "Vector3", "Vector4", "Quaternion", "Matrix3", "Matrix4",
		};

		langDef.mKeywords.clear();
		langDef.mIdentifiers.clear();
		langDef.mTokenRegexStrings.clear();

		for (const char* keyword : buKeywords)
		{
			langDef.mKeywords.insert(keyword);
		}

		for (const char* builtin : buBuiltins)
		{
			Identifier id;
			id.mDeclaration = "Built-in symbol";
			langDef.mIdentifiers.insert(std::make_pair(std::string(builtin), id));
		}

		langDef.mTokenRegexStrings.push_back(std::make_pair<std::string, PaletteIndex>(
			R"##(@\"([^"]|\"\")*\")##", PaletteIndex::String));
		langDef.mTokenRegexStrings.push_back(std::make_pair<std::string, PaletteIndex>(
			R"##(\"(\\.|[^\"])*\")##", PaletteIndex::String));
		langDef.mTokenRegexStrings.push_back(std::make_pair<std::string, PaletteIndex>(
			R"##(\'(\\.|[^\'])*\')##", PaletteIndex::CharLiteral));
		langDef.mTokenRegexStrings.push_back(std::make_pair<std::string, PaletteIndex>(
			R"##([+-]?([0-9]+([.][0-9]*)?|[.][0-9]+)([eE][+-]?[0-9]+)?)##", PaletteIndex::Number));
		langDef.mTokenRegexStrings.push_back(std::make_pair<std::string, PaletteIndex>(
			R"##(0[xX][0-9a-fA-F]+)##", PaletteIndex::Number));
		langDef.mTokenRegexStrings.push_back(std::make_pair<std::string, PaletteIndex>(
			R"##([a-zA-Z_][a-zA-Z0-9_]*)##", PaletteIndex::Identifier));
		langDef.mTokenRegexStrings.push_back(std::make_pair<std::string, PaletteIndex>(
			R"##([\[\]\{\}\!\%\^\&\*\(\)\-\+\=\~\|\<\>\?\/\;\,\.\:])##", PaletteIndex::Punctuation));

		langDef.mCommentStart = "/*";
		langDef.mCommentEnd = "*/";
		langDef.mSingleLineComment = "//";
		langDef.mCaseSensitive = true;
		langDef.mName = "BuLang";

		inited = true;
	}
	return langDef;
}
