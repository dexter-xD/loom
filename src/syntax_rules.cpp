// syntax rules implementation for different programming languages
// provides clean separation of language-specific highlighting rules
// supports multiple programming languages with organized rule sets

#include "syntax_rules.h"
#include "syntax_highlighter.h"
#include "debug_log.h"

// ═══════════════════════════════════════════════════════════════
//                         C++ RULES
// ═══════════════════════════════════════════════════════════════

QVector<SyntaxRules::Rule> SyntaxRules::getCppRules()
{
    QVector<Rule> rules;
    
    // Control flow keywords
    rules.append(Rule("\\b(if|else|for|while|do|switch|case|default|break|continue|return|goto)\\b", 
                     "control", "Control flow keywords"));
    
    // Type keywords
    rules.append(Rule("\\b(void|bool|char|short|int|long|float|double|signed|unsigned|const|volatile|static|extern|inline|virtual|explicit|mutable|constexpr|decltype|auto)\\b", 
                     "keyword", "Type and storage keywords"));
    
    // Class/struct keywords
    rules.append(Rule("\\b(class|struct|union|enum|namespace|template|typename|public|private|protected|friend|using|typedef)\\b", 
                     "keyword", "Class and namespace keywords"));
    
    // Memory management
    rules.append(Rule("\\b(new|delete|malloc|free|sizeof|alignof)\\b", 
                     "keyword", "Memory management keywords"));
    
    // Exception handling
    rules.append(Rule("\\b(try|catch|throw|noexcept)\\b", 
                     "keyword", "Exception handling keywords"));
    
    // Constants and literals
    rules.append(Rule("\\b(true|false|nullptr|NULL)\\b", 
                     "constant", "Boolean and null constants"));
    
    // Operators
    rules.append(Rule("(\\+\\+|--|\\+=|-=|\\*=|/=|%=|&=|\\|=|\\^=|<<=|>>=|==|!=|<=|>=|&&|\\|\\||<<|>>|->|\\.|::|\\?:|\\[|\\])", 
                     "operator", "C++ operators"));
    
    // Function definitions
    rules.append(Rule("\\b[a-zA-Z_][a-zA-Z0-9_]*(?=\\s*\\()", 
                     "function", "Function names"));
    
    // Standard library types
    rules.append(Rule("\\b(std::|string|vector|map|set|list|deque|stack|queue|pair|tuple|shared_ptr|unique_ptr|weak_ptr|array|bitset|complex|valarray)\\w*", 
                     "type", "Standard library types"));
    
    // Preprocessor directives
    rules.append(Rule("^\\s*#\\s*(include|define|undef|ifdef|ifndef|if|elif|else|endif|pragma|error|warning|line)\\b", 
                     "preprocessor", "Preprocessor directives"));
    
    // Include statements
    rules.append(Rule("#include\\s*[<\"][^>\"]*[>\"]", 
                     "preprocessor", "Include statements"));
    
    // Single line comments
    rules.append(Rule("//[^\n]*", 
                     "comment", "Single line comments"));
    
    // String literals with escape sequences
    rules.append(Rule("\"([^\"\\\\]|\\\\.)*\"", 
                     "string", "Double-quoted strings"));
    
    // Character literals
    rules.append(Rule("'([^'\\\\]|\\\\.)*'", 
                     "string", "Character literals"));
    
    // Raw string literals (C++11)
    rules.append(Rule("R\"[^(]*\\([^)]*\\)[^\"]*\"", 
                     "string", "Raw string literals"));
    
    // Add common number rules
    rules.append(getCommonNumberRules());
    
    return rules;
}

// ═══════════════════════════════════════════════════════════════
//                        PYTHON RULES
// ═══════════════════════════════════════════════════════════════

QVector<SyntaxRules::Rule> SyntaxRules::getPythonRules()
{
    QVector<Rule> rules;
    
    // Control flow
    rules.append(Rule("\\b(if|elif|else|for|while|break|continue|return|yield|pass)\\b", 
                     "control", "Control flow keywords"));
    
    // Python keywords
    rules.append(Rule("\\b(def|class|import|from|as|global|nonlocal|lambda|with|try|except|finally|raise|assert|del|async|await)\\b", 
                     "keyword", "Python keywords"));
    
    // Constants
    rules.append(Rule("\\b(True|False|None|NotImplemented|Ellipsis)\\b", 
                     "constant", "Python constants"));
    
    // Operators
    rules.append(Rule("\\b(and|or|not|in|is)\\b", 
                     "operator", "Python logical operators"));
    rules.append(Rule("(\\+|\\-|\\*|/|//|%|\\*\\*|==|!=|<|>|<=|>=|=|\\+=|\\-=|\\*=|/=|//=|%=|\\*\\*=|:=)", 
                     "operator", "Python arithmetic operators"));
    
    // Function definitions
    rules.append(Rule("\\bdef\\s+([a-zA-Z_][a-zA-Z0-9_]*)(?=\\s*\\()", 
                     "function", "Function definitions"));
    rules.append(Rule("\\basync\\s+def\\s+([a-zA-Z_][a-zA-Z0-9_]*)(?=\\s*\\()", 
                     "function", "Async function definitions"));
    
    // Class definitions
    rules.append(Rule("\\bclass\\s+([a-zA-Z_][a-zA-Z0-9_]*)(?=\\s*[\\(:])", 
                     "type", "Class definitions"));
    
    // Built-in functions
    rules.append(Rule("\\b(print|len|range|enumerate|zip|map|filter|sorted|reversed|sum|min|max|abs|round|int|float|str|bool|list|tuple|dict|set|type|isinstance|hasattr|getattr|setattr|delattr|open|input|iter|next|all|any|bin|hex|oct|ord|chr|eval|exec|compile|globals|locals|vars|dir|help|id|hash|repr|format|divmod|pow|callable|classmethod|staticmethod|property|super|slice)\\b", 
                     "builtin", "Built-in functions"));
    
    // Decorators
    rules.append(Rule("@[a-zA-Z_][a-zA-Z0-9_.]*", 
                     "annotation", "Decorators"));
    
    // Magic methods
    rules.append(Rule("\\b__[a-zA-Z_][a-zA-Z0-9_]*__\\b", 
                     "builtin", "Magic methods"));
    
    // Single line comments
    rules.append(Rule("#[^\n]*", 
                     "comment", "Single line comments"));
    
    // F-strings
    rules.append(Rule("f\"([^\"\\\\]|\\\\.|\\{[^}]*\\})*\"", 
                     "string", "F-strings with double quotes"));
    rules.append(Rule("f'([^'\\\\]|\\\\.|\\{[^}]*\\})*'", 
                     "string", "F-strings with single quotes"));
    rules.append(Rule("rf\"([^\"\\\\]|\\\\.|\\{[^}]*\\})*\"", 
                     "string", "Raw f-strings"));
    rules.append(Rule("fr\"([^\"\\\\]|\\\\.|\\{[^}]*\\})*\"", 
                     "string", "Raw f-strings (alternative)"));
    
    // Raw strings
    rules.append(Rule("r\"([^\"]|\"\")*\"", 
                     "string", "Raw strings with double quotes"));
    rules.append(Rule("r'([^']|'')*'", 
                     "string", "Raw strings with single quotes"));
    
    // Regular strings (single line only)
    rules.append(Rule("\"([^\"\\\\\\n]|\\\\.)*\"", 
                     "string", "Regular strings with double quotes"));
    rules.append(Rule("'([^'\\\\\\n]|\\\\.)*'", 
                     "string", "Regular strings with single quotes"));
    
    // Python numbers with complex number support
    rules.append(Rule("\\b\\d+\\.\\d*([eE][+-]?\\d+)?[jJ]?\\b", 
                     "number", "Floating point numbers"));
    rules.append(Rule("\\b\\.\\d+([eE][+-]?\\d+)?[jJ]?\\b", 
                     "number", "Decimal numbers starting with dot"));
    rules.append(Rule("\\b\\d+[jJ]?\\b", 
                     "number", "Integer numbers"));
    rules.append(Rule("\\b0[xX][0-9a-fA-F]+[jJ]?\\b", 
                     "number", "Hexadecimal numbers"));
    rules.append(Rule("\\b0[oO][0-7]+[jJ]?\\b", 
                     "number", "Octal numbers"));
    rules.append(Rule("\\b0[bB][01]+[jJ]?\\b", 
                     "number", "Binary numbers"));
    
    return rules;
}

// ═══════════════════════════════════════════════════════════════
//                         LUA RULES
// ═══════════════════════════════════════════════════════════════

QVector<SyntaxRules::Rule> SyntaxRules::getLuaRules()
{
    QVector<Rule> rules;
    
    // Lua keywords
    rules.append(Rule("\\b(and|break|do|else|elseif|end|false|for|function|goto|if|in|local|nil|not|or|repeat|return|then|true|until|while)\\b", 
                     "keyword", "Lua keywords"));
    
    // Built-in functions
    rules.append(Rule("\\b(assert|collectgarbage|dofile|error|getmetatable|ipairs|load|loadfile|next|pairs|pcall|print|rawequal|rawget|rawlen|rawset|require|select|setmetatable|tonumber|tostring|type|xpcall)\\b", 
                     "builtin", "Lua built-in functions"));
    
    // Standard library
    rules.append(Rule("\\b(coroutine|debug|io|math|os|package|string|table|utf8)\\b", 
                     "type", "Lua standard library"));
    
    // Function definitions
    rules.append(Rule("\\bfunction\\s+([a-zA-Z_][a-zA-Z0-9_.]*)\\s*\\(", 
                     "function", "Function definitions"));
    rules.append(Rule("\\blocal\\s+function\\s+([a-zA-Z_][a-zA-Z0-9_]*)\\s*\\(", 
                     "function", "Local function definitions"));
    
    // Single line comments (but not multi-line comment start)
    rules.append(Rule("--(?!\\[\\[)[^\n]*", 
                     "comment", "Single line comments"));
    
    // String literals (single line only)
    rules.append(Rule("\"([^\"\\\\\\n]|\\\\.)*\"", 
                     "string", "Double-quoted strings"));
    rules.append(Rule("'([^'\\\\\\n]|\\\\.)*'", 
                     "string", "Single-quoted strings"));
    
    // Numbers
    rules.append(Rule("\\b\\d+\\.\\d*([eE][+-]?\\d+)?\\b", 
                     "number", "Floating point numbers"));
    rules.append(Rule("\\b\\.\\d+([eE][+-]?\\d+)?\\b", 
                     "number", "Decimal numbers starting with dot"));
    rules.append(Rule("\\b\\d+([eE][+-]?\\d+)?\\b", 
                     "number", "Integer numbers"));
    rules.append(Rule("\\b0[xX][0-9a-fA-F]+(\\.p[+-]?\\d+)?\\b", 
                     "number", "Hexadecimal numbers"));
    
    // Operators
    rules.append(Rule("(\\+|\\-|\\*|/|%|\\^|==|~=|<=|>=|<|>|=|\\.\\.|\\.\\.\\.)", 
                     "operator", "Lua operators"));
    
    return rules;
}

// ═══════════════════════════════════════════════════════════════
//                         JAVA RULES
// ═══════════════════════════════════════════════════════════════

QVector<SyntaxRules::Rule> SyntaxRules::getJavaRules()
{
    QVector<Rule> rules;
    
    // Java keywords
    rules.append(Rule("\\b(abstract|assert|boolean|break|byte|case|catch|char|class|const|continue|default|do|double|else|enum|extends|final|finally|float|for|goto|if|implements|import|instanceof|int|interface|long|native|new|package|private|protected|public|return|short|static|strictfp|super|switch|synchronized|this|throw|throws|transient|try|void|volatile|while)\\b", 
                     "keyword", "Java keywords"));
    
    // Constants
    rules.append(Rule("\\b(true|false|null)\\b", 
                     "constant", "Java constants"));
    
    // Single line comments
    rules.append(Rule("//[^\n]*", 
                     "comment", "Single line comments"));
    
    // String literals
    rules.append(Rule("\"([^\"\\\\]|\\\\.)*\"", 
                     "string", "Double-quoted strings"));
    rules.append(Rule("'([^'\\\\]|\\\\.)*'", 
                     "string", "Character literals"));
    
    // Add common number rules
    rules.append(getCommonNumberRules());
    
    return rules;
}

// ═══════════════════════════════════════════════════════════════
//                      TYPESCRIPT RULES
// ═══════════════════════════════════════════════════════════════

QVector<SyntaxRules::Rule> SyntaxRules::getTypeScriptRules()
{
    // TypeScript is similar to JavaScript with additional type annotations
    QVector<Rule> rules = getJavaScriptRules();
    
    // TypeScript-specific keywords
    rules.append(Rule("\\b(interface|type|enum|namespace|module|declare|abstract|readonly|keyof|infer|never|unknown|any|object)\\b", 
                     "keyword", "TypeScript keywords"));
    
    // Type annotations
    rules.append(Rule(":\\s*[a-zA-Z_$][a-zA-Z0-9_$<>\\[\\]]*", 
                     "type", "Type annotations"));
    
    return rules;
}

// ═══════════════════════════════════════════════════════════════
//                         RUST RULES
// ═══════════════════════════════════════════════════════════════

QVector<SyntaxRules::Rule> SyntaxRules::getRustRules()
{
    QVector<Rule> rules;
    
    // Rust keywords
    rules.append(Rule("\\b(as|break|const|continue|crate|else|enum|extern|false|fn|for|if|impl|in|let|loop|match|mod|move|mut|pub|ref|return|self|Self|static|struct|super|trait|true|type|unsafe|use|where|while|async|await|dyn)\\b", 
                     "keyword", "Rust keywords"));
    
    // Single line comments
    rules.append(Rule("//[^\n]*", 
                     "comment", "Single line comments"));
    
    // String literals
    rules.append(Rule("\"([^\"\\\\]|\\\\.)*\"", 
                     "string", "Double-quoted strings"));
    
    // Add common number rules
    rules.append(getCommonNumberRules());
    
    return rules;
}

// ═══════════════════════════════════════════════════════════════
//                          GO RULES
// ═══════════════════════════════════════════════════════════════

QVector<SyntaxRules::Rule> SyntaxRules::getGoRules()
{
    QVector<Rule> rules;
    
    // Go keywords
    rules.append(Rule("\\b(break|case|chan|const|continue|default|defer|else|fallthrough|for|func|go|goto|if|import|interface|map|package|range|return|select|struct|switch|type|var)\\b", 
                     "keyword", "Go keywords"));
    
    // Constants
    rules.append(Rule("\\b(true|false|nil|iota)\\b", 
                     "constant", "Go constants"));
    
    // Single line comments
    rules.append(Rule("//[^\n]*", 
                     "comment", "Single line comments"));
    
    // String literals
    rules.append(Rule("\"([^\"\\\\]|\\\\.)*\"", 
                     "string", "Double-quoted strings"));
    rules.append(Rule("`[^`]*`", 
                     "string", "Raw string literals"));
    
    // Add common number rules
    rules.append(getCommonNumberRules());
    
    return rules;
}

// ═══════════════════════════════════════════════════════════════
//                         JSON RULES
// ═══════════════════════════════════════════════════════════════

QVector<SyntaxRules::Rule> SyntaxRules::getJsonRules()
{
    QVector<Rule> rules;
    
    // JSON values
    rules.append(Rule("\\b(true|false|null)\\b", 
                     "constant", "JSON constants"));
    
    // String literals (keys and values)
    rules.append(Rule("\"([^\"\\\\]|\\\\.)*\"", 
                     "string", "JSON strings"));
    
    // Numbers
    rules.append(Rule("-?\\b\\d+(\\.\\d+)?([eE][+-]?\\d+)?\\b", 
                     "number", "JSON numbers"));
    
    return rules;
}

// ═══════════════════════════════════════════════════════════════
//                       XML/HTML RULES
// ═══════════════════════════════════════════════════════════════

QVector<SyntaxRules::Rule> SyntaxRules::getXmlHtmlRules()
{
    QVector<Rule> rules;
    
    // XML/HTML tags
    rules.append(Rule("</?[a-zA-Z][a-zA-Z0-9]*[^>]*>", 
                     "keyword", "XML/HTML tags"));
    
    // Attributes
    rules.append(Rule("\\b[a-zA-Z-]+(?=\\s*=)", 
                     "type", "XML/HTML attributes"));
    
    // Attribute values
    rules.append(Rule("\"([^\"\\\\]|\\\\.)*\"", 
                     "string", "Attribute values"));
    rules.append(Rule("'([^'\\\\]|\\\\.)*'", 
                     "string", "Attribute values"));
    
    // Comments
    rules.append(Rule("<!--[^>]*-->", 
                     "comment", "XML/HTML comments"));
    
    return rules;
}

// ═══════════════════════════════════════════════════════════════
//                         CSS RULES
// ═══════════════════════════════════════════════════════════════

QVector<SyntaxRules::Rule> SyntaxRules::getCssRules()
{
    QVector<Rule> rules;
    
    // CSS selectors
    rules.append(Rule("\\.[a-zA-Z][a-zA-Z0-9_-]*", 
                     "type", "CSS class selectors"));
    rules.append(Rule("#[a-zA-Z][a-zA-Z0-9_-]*", 
                     "type", "CSS ID selectors"));
    
    // CSS properties
    rules.append(Rule("\\b[a-zA-Z-]+(?=\\s*:)", 
                     "keyword", "CSS properties"));
    
    // CSS values
    rules.append(Rule("\"([^\"\\\\]|\\\\.)*\"", 
                     "string", "CSS string values"));
    rules.append(Rule("'([^'\\\\]|\\\\.)*'", 
                     "string", "CSS string values"));
    
    // CSS colors
    rules.append(Rule("#[0-9a-fA-F]{3,6}\\b", 
                     "number", "CSS hex colors"));
    
    // CSS comments
    rules.append(Rule("/\\*[^*]*\\*+(?:[^/*][^*]*\\*+)*/", 
                     "comment", "CSS comments"));
    
    // Add common number rules for CSS units
    rules.append(Rule("\\b\\d+(\\.\\d+)?(px|em|rem|%|vh|vw|pt|pc|in|cm|mm|ex|ch|vmin|vmax|fr)\\b", 
                     "number", "CSS units"));
    
    return rules;
}

// ═══════════════════════════════════════════════════════════════
//                      JAVASCRIPT RULES
// ═══════════════════════════════════════════════════════════════

QVector<SyntaxRules::Rule> SyntaxRules::getJavaScriptRules()
{
    QVector<Rule> rules;
    
    // Control flow
    rules.append(Rule("\\b(if|else|for|while|do|switch|case|default|break|continue|return)\\b", 
                     "control", "Control flow keywords"));
    
    // JavaScript keywords
    rules.append(Rule("\\b(var|let|const|function|class|extends|import|export|from|as|default|new|delete|typeof|instanceof|in|of|with|debugger|try|catch|finally|throw|async|await|yield)\\b", 
                     "keyword", "JavaScript keywords"));
    
    // Constants
    rules.append(Rule("\\b(true|false|null|undefined|NaN|Infinity)\\b", 
                     "constant", "JavaScript constants"));
    
    // Operators
    rules.append(Rule("(===|!==|==|!=|<=|>=|<|>|&&|\\|\\||\\+\\+|--|\\+=|-=|\\*=|/=|%=|&=|\\|=|\\^=|<<=|>>=|>>>|=>|\\?\\.|\\?\\?)", 
                     "operator", "JavaScript operators"));
    
    // Function definitions
    rules.append(Rule("\\b(function\\s+[a-zA-Z_$][a-zA-Z0-9_$]*|[a-zA-Z_$][a-zA-Z0-9_$]*\\s*(?=\\s*[=:]\\s*(?:function|\\([^)]*\\)\\s*=>)))", 
                     "function", "Function definitions"));
    
    // Class definitions
    rules.append(Rule("\\bclass\\s+[a-zA-Z_$][a-zA-Z0-9_$]*", 
                     "type", "Class definitions"));
    
    // Built-in objects
    rules.append(Rule("\\b(console|window|document|Array|Object|String|Number|Boolean|Date|RegExp|Math|JSON|Promise|Set|Map|WeakSet|WeakMap|Symbol|Proxy|Reflect)\\b", 
                     "builtin", "Built-in objects"));
    
    // Built-in methods
    rules.append(Rule("\\b(log|error|warn|info|push|pop|shift|unshift|slice|splice|indexOf|includes|forEach|map|filter|reduce|find|some|every|sort|reverse|join|split|replace|match|search|test|exec|toString|valueOf|hasOwnProperty|isPrototypeOf|propertyIsEnumerable)\\b", 
                     "builtin", "Built-in methods"));
    
    // Single line comments
    rules.append(Rule("//[^\n]*", 
                     "comment", "Single line comments"));
    
    // Template literals
    rules.append(Rule("`([^`\\\\$]|\\\\.|\\$(?!\\{)|\\$\\{[^}]*\\})*`", 
                     "string", "Template literals"));
    
    // Regular strings
    rules.append(Rule("\"([^\"\\\\]|\\\\.)*\"", 
                     "string", "Double-quoted strings"));
    rules.append(Rule("'([^'\\\\]|\\\\.)*'", 
                     "string", "Single-quoted strings"));
    
    // Regular expressions
    rules.append(Rule("/(?![*/])([^/\\\\\\n]|\\\\.)+/[gimuy]*", 
                     "string", "Regular expressions"));
    
    // Add common number rules
    rules.append(getCommonNumberRules());
    
    return rules;
}

// ═══════════════════════════════════════════════════════════════
//                       HELPER METHODS
// ═══════════════════════════════════════════════════════════════

QVector<SyntaxRules::Rule> SyntaxRules::getCommonNumberRules()
{
    QVector<Rule> rules;
    
    // Floating point numbers
    rules.append(Rule("\\b\\d+\\.\\d*([eE][+-]?\\d+)?[fFlL]?\\b", 
                     "number", "Floating point numbers"));
    rules.append(Rule("\\b\\.\\d+([eE][+-]?\\d+)?[fFlL]?\\b", 
                     "number", "Decimal numbers starting with dot"));
    rules.append(Rule("\\b\\d+[eE][+-]?\\d+[fFlL]?\\b", 
                     "number", "Scientific notation"));
    
    // Integer numbers
    rules.append(Rule("\\b\\d+[uUlL]*\\b", 
                     "number", "Integer numbers"));
    
    // Hexadecimal numbers
    rules.append(Rule("\\b0[xX][0-9a-fA-F]+[uUlL]*\\b", 
                     "number", "Hexadecimal numbers"));
    
    // Octal numbers
    rules.append(Rule("\\b0[0-7]+[uUlL]*\\b", 
                     "number", "Octal numbers"));
    
    // Binary numbers
    rules.append(Rule("\\b0[bB][01]+[uUlL]*\\b", 
                     "number", "Binary numbers"));
    
    return rules;
}

QVector<SyntaxRules::Rule> SyntaxRules::getGenericRules()
{
    QVector<Rule> rules;
    
    // Generic text highlighting - minimal rules
    rules.append(Rule("\"([^\"\\\\]|\\\\.)*\"", 
                     "string", "Double-quoted strings"));
    rules.append(Rule("'([^'\\\\]|\\\\.)*'", 
                     "string", "Single-quoted strings"));
    rules.append(Rule("\\b\\d+(\\.\\d+)?\\b", 
                     "number", "Numbers"));
    
    return rules;
}

QVector<SyntaxRules::Rule> SyntaxRules::getCommonCStyleRules()
{
    QVector<Rule> rules;
    
    // Common C-style language patterns
    rules.append(Rule("//[^\n]*", 
                     "comment", "Single line comments"));
    rules.append(Rule("\"([^\"\\\\]|\\\\.)*\"", 
                     "string", "Double-quoted strings"));
    rules.append(Rule("'([^'\\\\]|\\\\.)*'", 
                     "string", "Character literals"));
    
    return rules;
}

QVector<SyntaxRules::Rule> SyntaxRules::getCommonStringRules()
{
    QVector<Rule> rules;
    
    // Common string patterns
    rules.append(Rule("\"([^\"\\\\]|\\\\.)*\"", 
                     "string", "Double-quoted strings"));
    rules.append(Rule("'([^'\\\\]|\\\\.)*'", 
                     "string", "Single-quoted strings"));
    
    return rules;
}

// ═══════════════════════════════════════════════════════════════
//                      RULE APPLICATION
// ═══════════════════════════════════════════════════════════════

void SyntaxRules::applyRules(SyntaxHighlighter *highlighter, const QString &language)
{
    if (!highlighter) return;
    
    QVector<Rule> rules;
    
    if (language == "cpp" || language == "c") {
        rules = getCppRules();
    } else if (language == "python") {
        rules = getPythonRules();
    } else if (language == "lua") {
        rules = getLuaRules();
    } else if (language == "javascript" || language == "js") {
        rules = getJavaScriptRules();
    } else {
        rules = getGenericRules();
    }
    
    // Apply all rules to the highlighter
    DEBUG_LOG_SYNTAX("SyntaxRules: Applying" << rules.size() << "rules for language:" << language);
    for (const auto &rule : rules) {
        DEBUG_LOG_SYNTAX("  Adding rule:" << rule.pattern << "with color:" << rule.colorName);
        highlighter->addRule(rule.pattern, rule.colorName);
    }
    DEBUG_LOG_SYNTAX("SyntaxRules: All rules applied successfully");
}