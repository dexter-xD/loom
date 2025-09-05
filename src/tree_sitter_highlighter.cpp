#include "tree_sitter_highlighter.h"
#include "lua_bridge.h"
#include <QDebug>

TreeSitterHighlighter::TreeSitterHighlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent)
    , m_currentLanguage("text")
    , m_parser(nullptr)
    , m_tree(nullptr)
    , m_language(nullptr)
    , m_luaBridge(nullptr)
{
    setupGruvboxColors();
    initializeParser();
}

TreeSitterHighlighter::~TreeSitterHighlighter()
{
    if (m_tree) {
        ts_tree_delete(m_tree);
    }
    if (m_parser) {
        ts_parser_delete(m_parser);
    }
}

void TreeSitterHighlighter::setLanguage(const QString &language)
{
    if (m_currentLanguage != language) {
        m_currentLanguage = language;
        initializeParser();
    }
}

QString TreeSitterHighlighter::currentLanguage() const
{
    return m_currentLanguage;
}

void TreeSitterHighlighter::setLuaBridge(LuaBridge *bridge)
{
    m_luaBridge = bridge;
    if (m_luaBridge) {
        setupGruvboxColors(); // Reload colors from config
    }
}

void TreeSitterHighlighter::setupGruvboxColors()
{
    // More accurate Gruvbox colors based on the actual theme
    QMap<QString, QColor> defaultColors;
    defaultColors["keyword"] = QColor("#d79921");        // Gruvbox yellow
    defaultColors["control"] = QColor("#fb4934");        // Gruvbox bright red
    defaultColors["type"] = QColor("#fabd2f");           // Gruvbox bright yellow
    defaultColors["function"] = QColor("#b8bb26");       // Gruvbox bright green
    defaultColors["constant"] = QColor("#d3869b");       // Gruvbox bright purple
    defaultColors["builtin"] = QColor("#83a598");        // Gruvbox bright blue
    defaultColors["string"] = QColor("#b8bb26");         // Gruvbox bright green
    defaultColors["number"] = QColor("#d3869b");         // Gruvbox bright purple
    defaultColors["comment"] = QColor("#928374");        // Gruvbox gray
    defaultColors["operator"] = QColor("#ebdbb2");       // Gruvbox fg
    defaultColors["punctuation"] = QColor("#ebdbb2");    // Gruvbox fg
    defaultColors["preprocessor"] = QColor("#8ec07c");   // Gruvbox bright aqua
    defaultColors["annotation"] = QColor("#fabd2f");     // Gruvbox bright yellow
    defaultColors["escape"] = QColor("#fe8019");         // Gruvbox bright orange
    defaultColors["method"] = QColor("#83a598");         // Gruvbox bright blue
    defaultColors["namespace"] = QColor("#fe8019");      // Gruvbox bright orange
    defaultColors["macro"] = QColor("#8ec07c");          // Gruvbox bright aqua

    // Try to get colors from config
    QMap<QString, QString> configColors;
    if (m_luaBridge) {
        configColors = m_luaBridge->getSyntaxColors();
    }

    // Helper function to get color from config or default
    auto getColor = [&](const QString &key) -> QColor {
        if (configColors.contains(key) && !configColors[key].isEmpty()) {
            return QColor(configColors[key]);
        }
        return defaultColors.value(key, QColor("#ebdbb2"));
    };

    // Set up formats with colors from config
    QTextCharFormat keywordFormat;
    keywordFormat.setForeground(getColor("keyword"));
    keywordFormat.setFontWeight(QFont::Bold);
    m_colorFormats["keyword"] = keywordFormat;

    QTextCharFormat controlFormat;
    controlFormat.setForeground(getColor("control"));
    controlFormat.setFontWeight(QFont::Bold);
    m_colorFormats["control"] = controlFormat;

    QTextCharFormat commentFormat;
    commentFormat.setForeground(getColor("comment"));
    commentFormat.setFontItalic(true);
    m_colorFormats["comment"] = commentFormat;

    QTextCharFormat stringFormat;
    stringFormat.setForeground(getColor("string"));
    m_colorFormats["string"] = stringFormat;

    QTextCharFormat numberFormat;
    numberFormat.setForeground(getColor("number"));
    m_colorFormats["number"] = numberFormat;

    QTextCharFormat preprocessorFormat;
    preprocessorFormat.setForeground(getColor("preprocessor"));
    preprocessorFormat.setFontWeight(QFont::Bold);
    m_colorFormats["preprocessor"] = preprocessorFormat;

    QTextCharFormat functionFormat;
    functionFormat.setForeground(getColor("function"));
    functionFormat.setFontWeight(QFont::Bold);
    m_colorFormats["function"] = functionFormat;

    QTextCharFormat typeFormat;
    typeFormat.setForeground(getColor("type"));
    typeFormat.setFontWeight(QFont::Bold);
    m_colorFormats["type"] = typeFormat;

    QTextCharFormat operatorFormat;
    operatorFormat.setForeground(getColor("operator"));
    m_colorFormats["operator"] = operatorFormat;

    QTextCharFormat constantFormat;
    constantFormat.setForeground(getColor("constant"));
    constantFormat.setFontWeight(QFont::Bold);
    m_colorFormats["constant"] = constantFormat;

    QTextCharFormat builtinFormat;
    builtinFormat.setForeground(getColor("builtin"));
    m_colorFormats["builtin"] = builtinFormat;

    QTextCharFormat annotationFormat;
    annotationFormat.setForeground(getColor("annotation"));
    m_colorFormats["annotation"] = annotationFormat;

    QTextCharFormat escapeFormat;
    escapeFormat.setForeground(getColor("escape"));
    escapeFormat.setFontWeight(QFont::Bold);
    m_colorFormats["escape"] = escapeFormat;

    QTextCharFormat punctuationFormat;
    punctuationFormat.setForeground(getColor("punctuation"));
    m_colorFormats["punctuation"] = punctuationFormat;

    QTextCharFormat methodFormat;
    methodFormat.setForeground(getColor("method"));
    methodFormat.setFontWeight(QFont::Bold);
    m_colorFormats["method"] = methodFormat;

    QTextCharFormat namespaceFormat;
    namespaceFormat.setForeground(getColor("namespace"));
    namespaceFormat.setFontWeight(QFont::Bold);
    m_colorFormats["namespace"] = namespaceFormat;

    QTextCharFormat macroFormat;
    macroFormat.setForeground(getColor("macro"));
    macroFormat.setFontWeight(QFont::Bold);
    m_colorFormats["macro"] = macroFormat;
}

QTextCharFormat TreeSitterHighlighter::getFormat(const QString &colorName) const
{
    return m_colorFormats.value(colorName, QTextCharFormat());
}

void TreeSitterHighlighter::highlightBlock(const QString &text)
{
    if (!m_parser || !m_language) {
        return;
    }

    // Get the full document text
    QString fullText = document()->toPlainText();

    // Update the tree with the full text
    updateTree(fullText);

    if (!m_tree) {
        return;
    }

    // Clear existing formats for this block
    setFormat(0, text.length(), QTextCharFormat());

    // Use the main parser for all languages
    TSNode root = ts_tree_root_node(m_tree);
    highlightNode(root, fullText);
}

void TreeSitterHighlighter::highlightNode(TSNode node, const QString &fullText)
{
    if (ts_node_is_null(node)) {
        return;
    }

    uint32_t start_byte = ts_node_start_byte(node);
    uint32_t end_byte = ts_node_end_byte(node);
    const char* node_type = ts_node_type(node);
    QString nodeTypeStr = QString(node_type);

    // Get current block boundaries
    int blockStart = currentBlock().position();
    int blockEnd = blockStart + currentBlock().length();

    // Skip if node is completely outside current block
    if ((int)end_byte <= blockStart || (int)start_byte >= blockEnd) {
        return;
    }

    // Get node text
    QString nodeText = fullText.mid(start_byte, end_byte - start_byte);

    // Calculate positions relative to current block
    int localStart = qMax(0, (int)start_byte - blockStart);
    int localEnd = qMin(currentBlock().length(), (int)end_byte - blockStart);
    int length = localEnd - localStart;

    // HTML highlighting is now handled by a separate HtmlHighlighter

    // Apply highlighting based on node type and content
    QString colorName = getColorForNode(nodeTypeStr, nodeText, node);

    if (!colorName.isEmpty() && length > 0) {
        QTextCharFormat format = getFormat(colorName);
        if (format != QTextCharFormat()) {
            setFormat(localStart, length, format);
        }
    }

    // Process children
    uint32_t child_count = ts_node_child_count(node);
    for (uint32_t i = 0; i < child_count; i++) {
        TSNode child = ts_node_child(node, i);
        highlightNode(child, fullText);
    }
}

QString TreeSitterHighlighter::getColorForNode(const QString &nodeType, const QString &nodeText, TSNode node)
{
    // Language-specific keyword sets
    static QMap<QString, QSet<QString>> languageKeywords;
    if (languageKeywords.isEmpty()) {
        // C/C++ keywords
        languageKeywords["cpp"] = {"auto", "break", "case", "char", "const", "continue", "default", "do",
                                   "double", "else", "enum", "extern", "float", "for", "goto", "if",
                                   "inline", "int", "long", "register", "restrict", "return", "short",
                                   "signed", "sizeof", "static", "struct", "switch", "typedef", "union",
                                   "unsigned", "void", "volatile", "while", "_Alignas", "_Alignof",
                                   "_Atomic", "_Bool", "_Complex", "_Generic", "_Imaginary", "_Noreturn",
                                   "_Static_assert", "_Thread_local", "class", "namespace", "template",
                                   "typename", "using", "public", "private", "protected", "virtual",
                                   "override", "final", "new", "delete", "this", "friend", "operator",
                                   "explicit", "constexpr", "noexcept", "nullptr", "true", "false"};

        // JavaScript keywords
        languageKeywords["javascript"] = {"async", "await", "break", "case", "catch", "class", "const",
                                         "continue", "debugger", "default", "delete", "do", "else", "export",
                                         "extends", "finally", "for", "function", "if", "import", "in",
                                         "instanceof", "let", "new", "return", "super", "switch", "this",
                                         "throw", "try", "typeof", "var", "void", "while", "with", "yield",
                                         "true", "false", "null", "undefined"};

        // Python keywords
        languageKeywords["python"] = {"and", "as", "assert", "async", "await", "break", "class", "continue",
                                     "def", "del", "elif", "else", "except", "finally", "for", "from",
                                     "global", "if", "import", "in", "is", "lambda", "nonlocal", "not",
                                     "or", "pass", "raise", "return", "try", "while", "with", "yield",
                                     "True", "False", "None"};

        // Rust keywords
        languageKeywords["rust"] = {"as", "async", "await", "break", "const", "continue", "crate", "dyn",
                                   "else", "enum", "extern", "false", "fn", "for", "if", "impl", "in",
                                   "let", "loop", "match", "mod", "move", "mut", "pub", "ref", "return",
                                   "self", "Self", "static", "struct", "super", "trait", "true", "type",
                                   "unsafe", "use", "where", "while"};

        // Java keywords
        languageKeywords["java"] = {"abstract", "assert", "boolean", "break", "byte", "case", "catch",
                                   "char", "class", "const", "continue", "default", "do", "double",
                                   "else", "enum", "extends", "final", "finally", "float", "for",
                                   "goto", "if", "implements", "import", "instanceof", "int", "interface",
                                   "long", "native", "new", "package", "private", "protected", "public",
                                   "return", "short", "static", "strictfp", "super", "switch", "synchronized",
                                   "this", "throw", "throws", "transient", "try", "void", "volatile", "while"};

        // Go keywords
        languageKeywords["go"] = {"break", "case", "chan", "const", "continue", "default", "defer",
                                 "else", "fallthrough", "for", "func", "go", "goto", "if", "import",
                                 "interface", "map", "package", "range", "return", "select", "struct",
                                 "switch", "type", "var", "true", "false", "nil"};

        // Lua keywords
        languageKeywords["lua"] = {"and", "break", "do", "else", "elseif", "end", "false", "for",
                                  "function", "goto", "if", "in", "local", "nil", "not", "or",
                                  "repeat", "return", "then", "true", "until", "while"};
    }

    // Comments - highest priority
    if (nodeType == "comment" || nodeType == "line_comment" || nodeType == "block_comment" ||
        nodeType == "multiline_comment" || nodeType == "single_line_comment") {
        return "comment";
    }

    // Strings and character literals
    if (nodeType == "string_literal" || nodeType == "char_literal" ||
        nodeType == "raw_string_literal" || nodeType == "string" ||
        nodeType == "system_lib_string" || nodeType == "header_name" ||
        nodeType == "string_fragment" || nodeType == "string_start" ||
        nodeType == "string_end" || nodeType == "escape_sequence") {
        return "string";
    }

    // Numbers
    if (nodeType == "number_literal" || nodeType == "integer_literal" ||
        nodeType == "float_literal" || nodeType == "decimal_literal" ||
        nodeType == "hex_literal" || nodeType == "octal_literal" ||
        nodeType == "binary_literal" || nodeType == "number") {
        return "number";
    }

    // Check for language-specific keywords
    if (languageKeywords.contains(m_currentLanguage) &&
        languageKeywords[m_currentLanguage].contains(nodeText)) {
        return "keyword";
    }

    // Preprocessor directives
    if (nodeType.startsWith("preproc") || nodeType == "preproc_directive" ||
        nodeType == "preproc_include" || nodeType == "preproc_define" ||
        nodeType == "preproc_ifdef" || nodeType == "preproc_ifndef" ||
        nodeType == "preproc_if" || nodeType == "preproc_else" ||
        nodeType == "preproc_endif" || nodeType == "preproc_call") {
        return "preprocessor";
    }

    // Function declarations and calls
    if (nodeType == "function_declarator" || nodeType == "function_definition" ||
        nodeType == "function_declaration" || nodeType == "method_definition" ||
        nodeType == "function" || nodeType == "method_declaration") {
        return "function";
    }

    if (nodeType == "call_expression" || nodeType == "method_call") {
        return "function";
    }

    // Type identifiers and declarations
    if (nodeType == "type_identifier" || nodeType == "primitive_type" ||
        nodeType == "type_descriptor" || nodeType == "sized_type_specifier" ||
        nodeType == "struct_specifier" || nodeType == "union_specifier" ||
        nodeType == "enum_specifier" || nodeType == "class_specifier" ||
        nodeType == "interface_declaration" || nodeType == "class_declaration") {
        return "type";
    }

    // Variable declarations and identifiers
    if (nodeType == "variable_declarator" || nodeType == "parameter_declaration" ||
        nodeType == "field_declaration" || nodeType == "property_declaration") {
        return "builtin";
    }

    // Constants and literals
    if (nodeText == "true" || nodeText == "false" || nodeText == "null" ||
        nodeText == "nullptr" || nodeText == "NULL" || nodeText == "nil" ||
        nodeText == "undefined" || nodeText == "None" ||
        (nodeText.toUpper() == nodeText && nodeText.length() > 2 &&
         nodeText.contains(QRegExp("^[A-Z_][A-Z0-9_]*$")))) {
        return "constant";
    }

    // Operators
    static QSet<QString> operators = {
        "+", "-", "*", "/", "%", "=", "==", "!=", "<", ">", "<=", ">=",
        "&&", "||", "!", "&", "|", "^", "~", "<<", ">>", "++", "--",
        "+=", "-=", "*=", "/=", "%=", "&=", "|=", "^=", "<<=", ">>=",
        "->", ".", "::", "?", ":", ".*", "->*", "<=>", "and", "or", "not",
        "and_eq", "or_eq", "xor", "xor_eq", "bitand", "bitor", "compl"
    };

    if (operators.contains(nodeText) || nodeType == "binary_expression" ||
        nodeType == "unary_expression" || nodeType == "assignment_expression" ||
        nodeType == "comparison_operator" || nodeType == "arithmetic_operator") {
        return "operator";
    }

    // Punctuation
    static QSet<QString> punctuation = {
        ";", ",", "(", ")", "[", "]", "{", "}", "<", ">"
    };

    if (punctuation.contains(nodeText) || nodeType == "punctuation") {
        return "punctuation";
    }

    // CSS highlighting is now handled by HtmlHighlighter



    // Generic identifiers - check context
    if (nodeType == "identifier") {
        if (isTypeIdentifier(nodeText, node)) {
            return "type";
        }
        if (isFunctionIdentifier(node)) {
            return "function";
        }
    }

    // Field access and member expressions
    if (nodeType == "field_expression" || nodeType == "field_identifier" ||
        nodeType == "member_expression" || nodeType == "property_identifier") {
        return "builtin";
    }

    // Namespace and scope resolution
    if (nodeType == "namespace_identifier" || nodeType == "scope_resolution" ||
        nodeType == "qualified_identifier") {
        return "namespace";
    }

    // Template/generics
    if (nodeType == "template_parameter_list" || nodeType == "template_argument_list" ||
        nodeType == "type_arguments" || nodeType == "type_parameters") {
        return "type";
    }

    // Macro definitions and calls
    if (nodeType == "preproc_function_def" ||
        (nodeType == "identifier" && isMacroCall(nodeText, node))) {
        return "macro";
    }

    return QString(); // No highlighting
}

bool TreeSitterHighlighter::isTypeIdentifier(const QString &text, TSNode node)
{
    // Qt types (more comprehensive)
    if ((text.startsWith("Q") && text.length() > 1 && text[1].isUpper()) ||
        text.startsWith("QML") || text.startsWith("Qt")) {
        return true;
    }
    
    // Standard library types and namespaces
    static QSet<QString> stdTypes = {
        "std", "string", "vector", "map", "set", "list", "deque", "stack", "queue",
        "unique_ptr", "shared_ptr", "weak_ptr", "function", "thread", "mutex",
        "condition_variable", "atomic", "exception", "runtime_error", "logic_error",
        "iostream", "istream", "ostream", "fstream", "ifstream", "ofstream",
        "stringstream", "istringstream", "ostringstream", "pair", "tuple",
        "array", "unordered_map", "unordered_set", "priority_queue",
        "optional", "variant", "any", "chrono", "regex", "random"
    };
    
    if (stdTypes.contains(text)) {
        return true;
    }
    
    // Check parent context for type usage
    TSNode parent = ts_node_parent(node);
    if (!ts_node_is_null(parent)) {
        const char* parent_type = ts_node_type(parent);
        QString parentTypeStr = QString(parent_type);
        
        // Type in various declaration contexts
        if (parentTypeStr == "declaration" || parentTypeStr == "parameter_declaration" ||
            parentTypeStr == "field_declaration" || parentTypeStr == "variable_declaration" ||
            parentTypeStr == "type_descriptor" || parentTypeStr == "template_argument" ||
            parentTypeStr == "template_parameter" || parentTypeStr == "cast_expression" ||
            parentTypeStr == "new_expression" || parentTypeStr == "delete_expression") {
            return true;
        }
    }
    
    // CamelCase heuristic for custom types
    if (text.length() > 1 && text[0].isUpper()) {
        bool hasCamelCase = false;
        for (int i = 1; i < text.length(); i++) {
            if (text[i].isUpper() && text[i-1].isLower()) {
                hasCamelCase = true;
                break;
            }
        }
        if (hasCamelCase) {
            return true;
        }
    }
    
    return false;
}

bool TreeSitterHighlighter::isFunctionIdentifier(TSNode node)
{
    TSNode parent = ts_node_parent(node);
    if (ts_node_is_null(parent)) {
        return false;
    }
    
    const char* parent_type = ts_node_type(parent);
    QString parentTypeStr = QString(parent_type);
    
    // Function calls
    if (parentTypeStr == "call_expression") {
        // Check if this identifier is the function name (first child)
        if (ts_node_child_count(parent) > 0) {
            TSNode first_child = ts_node_child(parent, 0);
            return ts_node_eq(node, first_child);
        }
    }
    
    // Function declarations/definitions
    if (parentTypeStr == "function_declarator" || parentTypeStr == "function_definition" ||
        parentTypeStr == "function_declaration" || parentTypeStr == "method_declaration") {
        return true;
    }
    
    // Constructor/destructor
    if (parentTypeStr == "constructor_declaration" || parentTypeStr == "destructor_declaration") {
        return true;
    }
    
    return false;
}

bool TreeSitterHighlighter::isMacroCall(const QString &text, TSNode node)
{
    // Common macro patterns
    if (text.toUpper() == text && text.length() > 2) {
        return true;
    }
    
    // Qt macros
    static QSet<QString> qtMacros = {
        "Q_OBJECT", "Q_PROPERTY", "Q_INVOKABLE", "Q_GADGET", "Q_ENUM",
        "Q_FLAG", "Q_DECLARE_METATYPE", "Q_REGISTER_METATYPE",
        "SIGNAL", "SLOT", "emit", "connect", "disconnect"
    };
    
    if (qtMacros.contains(text)) {
        return true;
    }
    
    return false;
}

void TreeSitterHighlighter::initializeParser()
{
    if (m_parser) {
        ts_parser_delete(m_parser);
    }
    if (m_tree) {
        ts_tree_delete(m_tree);
        m_tree = nullptr;
    }

    m_parser = ts_parser_new();

    if (m_currentLanguage == "c") {
        m_language = tree_sitter_c();
    } else if (m_currentLanguage == "cpp" || m_currentLanguage == "cxx" || m_currentLanguage == "cc") {
        m_language = tree_sitter_cpp();
    } else if (m_currentLanguage == "javascript" || m_currentLanguage == "js") {
        m_language = tree_sitter_javascript();
    } else if (m_currentLanguage == "python" || m_currentLanguage == "py") {
        m_language = tree_sitter_python();
    } else if (m_currentLanguage == "rust" || m_currentLanguage == "rs") {
        m_language = tree_sitter_rust();
    } else if (m_currentLanguage == "java") {
        m_language = tree_sitter_java();
    } else if (m_currentLanguage == "go") {
        m_language = tree_sitter_go();
    } else if (m_currentLanguage == "lua") {
        m_language = tree_sitter_lua();
    } else {
        m_language = nullptr;
        return;
    }

    ts_parser_set_language(m_parser, m_language);
}

void TreeSitterHighlighter::updateTree(const QString &text)
{
    if (!m_parser) {
        return;
    }

    if (m_tree) {
        ts_tree_delete(m_tree);
    }

    std::string stdText = text.toStdString();
    m_tree = ts_parser_parse_string(m_parser, nullptr, stdText.c_str(), stdText.length());
}

void TreeSitterHighlighter::walkTreeAndHighlight(TSNode node, const QString &text, int offset)
{
    // This method is deprecated in favor of highlightNode
    Q_UNUSED(node)
    Q_UNUSED(text)
    Q_UNUSED(offset)
}

void TreeSitterHighlighter::addRule(const QString &pattern, const QString &colorName)
{
    // Tree-sitter doesn't use regex rules, so this is a no-op
    Q_UNUSED(pattern)
    Q_UNUSED(colorName)
}

void TreeSitterHighlighter::clearRules()
{
    // Tree-sitter doesn't use regex rules, so this is a no-op
}

QColor TreeSitterHighlighter::gruvboxColor(const QString &colorName) const
{
    static QMap<QString, QColor> colors = {
        {"bg", QColor("#282828")},
        {"fg", QColor("#ebdbb2")},
        {"red", QColor("#cc241d")},
        {"green", QColor("#98971a")},
        {"yellow", QColor("#d79921")},
        {"blue", QColor("#458588")},
        {"purple", QColor("#b16286")},
        {"aqua", QColor("#689d6a")},
        {"orange", QColor("#d65d0e")},
        {"gray", QColor("#928374")},
        {"bright_red", QColor("#fb4934")},
        {"bright_green", QColor("#b8bb26")},
        {"bright_yellow", QColor("#fabd2f")},
        {"bright_blue", QColor("#83a598")},
        {"bright_purple", QColor("#d3869b")},
        {"bright_aqua", QColor("#8ec07c")},
        {"bright_orange", QColor("#fe8019")}
    };

    return colors.value(colorName, QColor("#ebdbb2"));
}

QString TreeSitterHighlighter::nodeTypeToColorName(const QString &nodeType) const
{
    // This method is deprecated in favor of getColorForNode
    if (nodeType == "comment") {
        return "comment";
    } else if (nodeType == "string_literal" || nodeType == "char_literal" || nodeType == "string") {
        return "string";
    } else if (nodeType == "number_literal" || nodeType == "integer_literal" || nodeType == "float_literal") {
        return "number";
    } else if (nodeType == "preproc_directive" || nodeType == "preproc_include" || nodeType == "preproc_define") {
        return "preprocessor";
    }

    return QString();
}
