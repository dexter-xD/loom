-- Auto-formatter plugin
-- Uses external formatters only (clang-format, prettier, stylua, etc.)

local function debug_print(msg)
    if editor and editor.debug_log then
        editor.debug_log(msg)
    end
end

local function info_print(msg)
    print(msg)
end

autoformat = {
    name = "autoformat",
    version = "3.1",
    description = "Formats code using external formatters only",
    
    -- plugin state
    enabled = true,
    format_on_save = false,
    use_external_formatters = true,
    
    -- track autosave state to avoid formatting on autosave
    is_autosave_in_progress = false,
    
    -- track if we just formatted to prevent double formatting
    just_formatted = false,
    
    -- supported languages and their external formatters
    formatters = {
        cpp = {
            external = "clang-format",
            args = {"--style=Google"}
        },
        c = {
            external = "clang-format", 
            args = {"--style=Google"}
        },
        javascript = {
            external = "prettier",
            args = {"--parser", "babel", "--single-quote", "--trailing-comma", "es5"}
        },
        typescript = {
            external = "prettier",
            args = {"--parser", "typescript", "--single-quote", "--trailing-comma", "es5"}
        },
        json = {
            external = "prettier",
            args = {"--parser", "json"}
        },
        html = {
            external = "prettier",
            args = {"--parser", "html"}
        },
        css = {
            external = "prettier",
            args = {"--parser", "css"}
        },
        lua = {
            external = "stylua",
            args = {"--indent-type", "Spaces", "--indent-width", "4"}
        },
        python = {
            external = "black",
            args = {"--quiet", "-"}
        },
        rust = {
            external = "rustfmt",
            args = {"--emit", "stdout"}
        }
    }
}

-- initialize plugin
function autoformat.initialize()
    debug_print("Initializing auto-formatter plugin...")
    
    -- check configuration
    if config and config.plugins and config.plugins.autoformat then
        autoformat.enabled = config.plugins.autoformat.enabled ~= false
        autoformat.format_on_save = config.plugins.autoformat.format_on_save == true
        autoformat.use_external_formatters = config.plugins.autoformat.use_external_formatters == true
    end
    
    if not autoformat.enabled then
        debug_print("Auto-formatter plugin is disabled")
        return
    end
    
    -- check available external formatters
    autoformat.check_available_formatters()
    
    -- register for events
    if autoformat.format_on_save then
        events.connect("file_saved", "autoformat.on_file_saved")
        debug_print("Auto-format: Format on save enabled")
    end
    
    debug_print("Auto-formatter plugin initialized successfully")
end

-- check which external formatters are available
function autoformat.check_available_formatters()
    debug_print("Auto-format: Checking available external formatters...")
    
    for language, formatter_config in pairs(autoformat.formatters) do
        if formatter_config.external then
            local available = autoformat.is_command_available(formatter_config.external)
            formatter_config.available = available
            
            if available then
                debug_print(string.format("  ✓ %s: %s", language, formatter_config.external))
            else
                debug_print(string.format("  ✗ %s: %s (not found)", language, formatter_config.external))
            end
        end
    end
end

-- check if a command is available in PATH
function autoformat.is_command_available(command)
    local handle = io.popen(string.format("command -v %s 2>/dev/null", command))
    if handle then
        local result = handle:read("*a")
        handle:close()
        return result and string.len(result) > 0
    end
    return false
end

-- cleanup plugin
function autoformat.cleanup()
    debug_print("Cleaning up auto-formatter plugin...")
end

-- format current document
function autoformat.format_document()
    local text = editor.get_text()
    if not text or string.len(text) == 0 then
        debug_print("Auto-format: No content to format")
        return
    end
    
    -- Store current cursor position
    local cursor_line, cursor_column = editor.get_cursor_position()
    
    -- Try to detect language from file content
    local language = autoformat.detect_language(text)
    if not autoformat.formatters[language] then
        debug_print(string.format("Auto-format: Language '%s' not supported", language or "unknown"))
        return
    end
    
    local formatted_text = autoformat.format_text(text, language)
    if formatted_text and formatted_text ~= text then
        editor.set_text(formatted_text)
        
        -- Restore cursor position after formatting
        editor.set_cursor_position(cursor_line, cursor_column)
        
        -- Mark that we just formatted to prevent double formatting
        autoformat.just_formatted = true
        
        debug_print(string.format("Auto-format: Document formatted (%s)", language))
        editor.set_status_text(string.format("Formatted with %s", 
                              autoformat.formatters[language].external))
        return true
    else
        debug_print("Auto-format: No changes needed or formatting failed")
        return false
    end
end

-- format text using external formatter only
function autoformat.format_text(text, language)
    local formatter_config = autoformat.formatters[language]
    if not formatter_config then
        debug_print(string.format("Auto-format: Language '%s' not supported", language))
        return text
    end
    
    -- Only use external formatters
    if formatter_config.external and formatter_config.available then
        local formatted = autoformat.format_with_external(text, language, formatter_config)
        if formatted then
            return formatted
        else
            debug_print(string.format("Auto-format: %s failed", formatter_config.external))
        end
    else
        if not formatter_config.available then
            debug_print(string.format("Auto-format: %s not available for %s", formatter_config.external, language))
        end
    end
    
    return text
end

-- format using external formatter
function autoformat.format_with_external(text, language, formatter_config)
    local command = formatter_config.external
    local args = formatter_config.args or {}
    
    -- Create temporary file for input
    local temp_file = "/tmp/autoformat_input_" .. os.time()
    local file = io.open(temp_file, "w")
    if not file then
        debug_print("Auto-format: Failed to create temporary file")
        return nil
    end
    
    file:write(text)
    file:close()
    
    -- Build command
    local cmd_parts = {command}
    for _, arg in ipairs(args) do
        table.insert(cmd_parts, arg)
    end
    
    -- Handle different formatter input methods
    if command == "prettier" then
        table.insert(cmd_parts, temp_file)
    elseif command == "clang-format" then
        table.insert(cmd_parts, temp_file)
    elseif command == "stylua" then
        table.insert(cmd_parts, temp_file)
    elseif command == "black" then
        table.insert(cmd_parts, temp_file)
    elseif command == "rustfmt" then
        table.insert(cmd_parts, temp_file)
    else
        table.insert(cmd_parts, temp_file)
    end
    
    local full_command = table.concat(cmd_parts, " ")
    
    -- Execute formatter
    local handle = io.popen(full_command .. " 2>/dev/null")
    if not handle then
        os.remove(temp_file)
        return nil
    end
    
    local result = handle:read("*a")
    local success = handle:close()
    
    -- Clean up
    os.remove(temp_file)
    
    if success and result and string.len(result) > 0 then
        return result
    else
        return nil
    end
end

-- Enhanced language detection based on content patterns
function autoformat.detect_language(text)
    if not text then return "text" end
    
    local lower_text = string.lower(text)
    
    -- Lua detection
    if string.match(text, "function%s+%w+%s*%(") and
       (string.match(text, "%s+end%s*$") or string.match(text, "\nend")) or
       string.match(text, "local%s+%w+") or
       string.match(text, "require%s*%(") then
        return "lua"
    end
    
    -- Python detection
    if string.match(text, "def%s+%w+%s*%(") or
       string.match(text, "import%s+%w+") or
       string.match(text, "from%s+%w+%s+import") or
       string.match(text, "if%s+__name__%s*==%s*['\"]__main__['\"]") then
        return "python"
    end
    
    -- Rust detection
    if string.match(text, "fn%s+%w+%s*%(") or
       string.match(text, "use%s+std::") or
       string.match(text, "let%s+mut%s+") or
       string.match(text, "impl%s+") then
        return "rust"
    end
    
    -- C/C++ detection
    if string.match(text, "#include%s*[<\"]") or
       string.match(text, "int%s+main%s*%(") or
       string.match(text, "#define%s+") or
       string.match(text, "using%s+namespace") or
       string.match(text, "cout%s*<<") or
       string.match(text, "std::") then
        return "cpp"
    end
    
    -- TypeScript specific patterns (check before JavaScript)
    if string.match(text, "interface%s+%w+") or
       string.match(text, "type%s+%w+%s*=") or
       string.match(text, ":%s*%w+%s*=") then
        return "typescript"
    end
    
    -- JavaScript detection
    if string.match(text, "function%s+%w*%s*%(") or
       string.match(text, "var%s+%w+") or
       string.match(text, "let%s+%w+") or
       string.match(text, "const%s+%w+") or
       string.match(text, "=>%s*") or
       string.match(text, "console%.log") or
       string.match(text, "document%.") then
        return "javascript"
    end
    
    -- JSON detection (very specific patterns)
    if (string.match(text, "^%s*{") and string.match(text, "}%s*$")) or
       (string.match(text, "^%s*%[") and string.match(text, "%]%s*$")) then
        if string.match(text, "\"[^\"]*\"%s*:%s*") and
           not string.match(text, "#include") and
           not string.match(text, "function") then
            return "json"
        end
    end
    
    -- HTML/XML detection
    if string.match(text, "<!DOCTYPE%s+html") or
       string.match(lower_text, "<html") or
       (string.match(text, "<%w+") and string.match(text, "</%w+>")) then
        return "html"
    end
    
    -- CSS detection (check last as it can be ambiguous)
    if string.match(text, "%w+%s*{[^}]*}") and
       string.match(text, ":%s*[^;{]+;") and
       not string.match(text, "#include") and
       not string.match(text, "function") and
       not string.match(text, "def%s+") then
        return "css"
    end
    
    return "text"
end

-- event handlers
function autoformat.on_file_saved(event_name, file_path)
    -- Don't format if autosave is in progress
    if autoformat.is_autosave_in_progress then
        debug_print("Auto-format: Skipping format (autosave in progress)")
        return
    end
    
    -- Don't format if we just formatted before saving (prevent double formatting)
    if autoformat.just_formatted then
        debug_print("Auto-format: Skipping format (already formatted before save)")
        autoformat.just_formatted = false
        return
    end
    
    if autoformat.format_on_save and autoformat.enabled then
        debug_print("Auto-format: Formatting on manual save...")
        autoformat.format_document()
    end
end

-- hook into autosave timer to detect when autosave is happening
function autoformat.on_autosave_start()
    autoformat.is_autosave_in_progress = true
end

function autoformat.on_autosave_end()
    -- Use a small delay to ensure the save event has been processed
    autoformat.is_autosave_in_progress = false
end

-- make event handlers globally accessible
_G["autoformat.on_file_saved"] = autoformat.on_file_saved
_G["autoformat.on_autosave_start"] = autoformat.on_autosave_start
_G["autoformat.on_autosave_end"] = autoformat.on_autosave_end

-- utility functions
function autoformat.toggle()
    autoformat.enabled = not autoformat.enabled
    info_print(string.format("Auto-formatter %s", autoformat.enabled and "enabled" or "disabled"))
    editor.set_status_text(string.format("Auto-formatter %s", autoformat.enabled and "enabled" or "disabled"))
end

function autoformat.toggle_format_on_save()
    autoformat.format_on_save = not autoformat.format_on_save
    info_print(string.format("Format on save %s", autoformat.format_on_save and "enabled" or "disabled"))
    editor.set_status_text(string.format("Format on save %s", autoformat.format_on_save and "enabled" or "disabled"))
    
    -- Update event connection
    if autoformat.format_on_save then
        events.connect("file_saved", "autoformat.on_file_saved")
    else
        events.disconnect("file_saved", "autoformat.on_file_saved")
    end
end

function autoformat.show_status()
    info_print("=== Auto-formatter Status ===")
    info_print(string.format("Enabled: %s", autoformat.enabled and "Yes" or "No"))
    info_print(string.format("Format on save: %s", autoformat.format_on_save and "Yes" or "No"))
    info_print(string.format("External formatters: %s", autoformat.use_external_formatters and "Yes" or "No"))
    
    info_print("\nAvailable formatters:")
    for language, config in pairs(autoformat.formatters) do
        if config.external then
            local status = config.available and "✓" or "✗"
            info_print(string.format("  %s %s: %s", status, language, config.external))
        end
    end
    info_print("=============================")
end

debug_print("Auto-formatter plugin loaded successfully!")