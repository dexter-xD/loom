-- configuration file for Loom text editor
-- this file defines editor settings, keybindings, and plugins

config = {
    -- editor settings
    editor = {
        font_family = "JetBrains Mono",
        font_size = 12,
        tab_width = 8,
        show_line_numbers = true,
        word_wrap = true,
        auto_indent = true,
        highlight_current_line = true
    },
    
    -- theme settings
    theme = {
        name = "gruvbox", -- available: "gruvbox", "dracula", "catppuccin-mocha"
    },

    -- syntax highlighting colors (Tree-sitter)
    syntax = {
        keyword = "#fb4934",
        control = "#fb4934",
        type = "#fabd2f",
        ["function"] = "#83a598",
        constant = "#d3869b",
        builtin = "#458588",
        string = "#b8bb26",
        number = "#d3869b",
        comment = "#928374",
        operator = "#fe8019",
        punctuation = "#ebdbb2",
        preprocessor = "#8ec07c",
        annotation = "#fabd2f",
        escape = "#fe8019"
    },

    -- basic highlighter colors (HTML, CSS, JSON, and embedded content)
    basic_highlighter = {
        -- HTML-specific colors
        tag = "#fb4934",           -- HTML tags - Gruvbox bright red
        attribute = "#fabd2f",     -- HTML attributes - Gruvbox bright yellow
        attribute_value = "#b8bb26", -- HTML attribute values - Gruvbox bright green
        entity = "#fe8019",        -- HTML entities - Gruvbox bright orange

        -- CSS-specific colors (for both standalone CSS files and <style> blocks in HTML)
        css_property = "#83a598",  -- CSS properties - Gruvbox bright blue
        css_value = "#d3869b",     -- CSS values - Gruvbox bright purple
        css_selector = "#fabd2f",  -- CSS selectors - Gruvbox bright yellow

        -- JavaScript colors (for <script> blocks embedded in HTML)
        js_keyword = "#fb4934",    -- JavaScript keywords - Gruvbox bright red
        js_string = "#b8bb26",     -- JavaScript strings - Gruvbox bright green
        js_comment = "#928374",    -- JavaScript comments - Gruvbox gray
        js_function = "#83a598",   -- JavaScript functions - Gruvbox bright blue

        -- JSON-specific colors
        json_key = "#fabd2f",      -- JSON object keys - Gruvbox bright yellow
        json_string = "#b8bb26",   -- JSON string values - Gruvbox bright green
        json_number = "#d3869b",   -- JSON numbers - Gruvbox bright purple
        json_boolean = "#fb4934",  -- JSON true/false - Gruvbox bright red
        json_null = "#fe8019"      -- JSON null values - Gruvbox bright orange
    },

    -- markdown syntax highlighting colors (Gruvbox theme)
    markdown_syntax = {
        heading1 = "#fb4934",     -- H1 headings - Gruvbox bright red
        heading2 = "#fabd2f",     -- H2 headings - Gruvbox bright yellow
        heading3 = "#b8bb26",     -- H3 headings - Gruvbox bright green
        heading4 = "#83a598",     -- H4 headings - Gruvbox bright blue
        heading5 = "#d3869b",     -- H5 headings - Gruvbox bright purple
        heading6 = "#8ec07c",     -- H6 headings - Gruvbox bright aqua
        link = "#83a598",         -- links - Gruvbox bright blue
        code = "#fe8019",         -- inline code - Gruvbox bright orange
        code_block = "#fe8019",   -- code blocks - Gruvbox bright orange
        emphasis = "#d3869b",     -- italic text - Gruvbox bright purple
        strong = "#fb4934",       -- bold text - Gruvbox bright red
        list = "#b8bb26",         -- list markers - Gruvbox bright green
        quote = "#928374",        -- blockquotes - Gruvbox gray
        separator = "#665c54"     -- horizontal rules - Gruvbox dark gray
    },
    
    
    -- keybinding configuration
    keybindings = {
        ["Ctrl+S"] = "save_file",
        ["Ctrl+O"] = "open_file",
        ["Ctrl+N"] = "new_file",
        ["Ctrl+T"] = "new_tab",
        ["Ctrl+W"] = "close_file",
        ["Ctrl+Q"] = "quit_application",
        ["Ctrl+Z"] = "undo",
        ["Ctrl+Y"] = "redo",
        ["Ctrl+C"] = "copy",
        ["Ctrl+V"] = "paste",
        ["Ctrl+X"] = "cut",
        ["Ctrl+A"] = "select_all",
        ["Ctrl+F"] = "find",
        ["Ctrl+H"] = "replace",
        ["F11"] = "toggle_fullscreen",
        ["Ctrl+L"] = "set_language",
        ["Ctrl+Shift+L"] = "redetect_language",
        ["Ctrl+Shift+T"] = "toggle_theme",
        ["Ctrl+Shift+F"] = "format_document",
        ["F12"] = "toggle_file_tree"
    },
    
    -- window settings
    window = {
        width = 1224,
        height = 768,
        remember_size = true,
        remember_position = true
    },
    
    -- plugin configuration
    plugins = {
        -- global plugin settings
        enabled = true,
        auto_load = true,
        error_recovery = true,
        
        -- individual plugin settings
        autosave = {
            enabled = false,
            interval = 10, -- seconds
            save_on_focus_lost = false,
            backup_files = false
        },
        
        autoformat = {
            enabled = true,
            auto_load = true,
            format_on_save = true,  -- enable format on save
            use_external_formatters = true 
        },
        theme_switcher = {
            enabled = true,
            auto_load = true
        },
    }
}



-- function to get configuration value with default fallback
function get_config(key, default_value)
    local keys = {}
    for k in string.gmatch(key, "([^%.]+)") do
        table.insert(keys, k)
    end
    
    local value = config
    for _, k in ipairs(keys) do
        if type(value) == "table" and value[k] ~= nil then
            value = value[k]
        else
            return default_value
        end
    end
    
    return value
end

-- function to set configuration value
function set_config(key, value)
    local keys = {}
    for k in string.gmatch(key, "([^%.]+)") do
        table.insert(keys, k)
    end
    
    local current = config
    for i = 1, #keys - 1 do
        local k = keys[i]
        if type(current[k]) ~= "table" then
            current[k] = {}
        end
        current = current[k]
    end
    
    current[keys[#keys]] = value
end

-- configuration loaded message
print("Configuration loaded successfully")
