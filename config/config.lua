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
        ["Ctrl+Shift+L"] = "redetect_language"
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
            format_on_save = true,  -- enable format on save
            use_external_formatters = true 
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

-- print configuration loaded message
print("Configuration loaded successfully")