-- Theme Switcher Plugin for Loom
-- Provides functions to switch between available themes

-- Plugin metadata
theme_switcher = {
    name = "theme_switcher",
    version = "1.2",
    description = "Provides functions to switch between available themes"
}

-- Check if plugin is enabled
local function is_plugin_enabled()
    local enabled = get_config("plugins.theme_switcher.enabled", false)
    local auto_load = get_config("plugins.theme_switcher.auto_load", false)
    return enabled and auto_load
end

-- Available themes
local available_themes = {
    "gruvbox",
    "dracula", 
    "catppuccin-mocha"
}

-- Function to toggle/switch to next theme (main entry point)
function toggle_theme()
    if not is_plugin_enabled() then
        editor.set_status_text("Theme switcher plugin is disabled")
        return
    end
    
    switch_to_next_theme()
end

-- Function to switch to next theme
function switch_to_next_theme()
    if not is_plugin_enabled() then
        editor.set_status_text("Theme switcher plugin is disabled")
        return
    end
    
    local current_theme = editor.get_theme()
    editor.debug_log("Current theme: " .. current_theme)
    local current_index = 1
    
    -- Find current theme index
    for i, theme in ipairs(available_themes) do
        if theme == current_theme then
            current_index = i
            break
        end
    end
    
    editor.debug_log("Current theme index: " .. current_index)
    
    -- Get next theme (wrap around)
    local next_index = (current_index % #available_themes) + 1
    local next_theme = available_themes[next_index]
    
    editor.debug_log("Next theme: " .. next_theme .. " (index: " .. next_index .. ")")
    
    -- Switch to next theme
    editor.set_theme(next_theme)
    editor.set_status_text("Switched to theme: " .. next_theme)
    
    -- Verify the theme was actually changed
    local new_current_theme = editor.get_theme()
    editor.debug_log("Theme after switch: " .. new_current_theme)
end

-- Function to switch to a specific theme
function switch_to_theme(theme_name)
    if not is_plugin_enabled() then
        editor.set_status_text("Theme switcher plugin is disabled")
        return
    end
    
    -- Check if theme is available
    local theme_found = false
    for _, theme in ipairs(available_themes) do
        if theme == theme_name then
            theme_found = true
            break
        end
    end
    
    if theme_found then
        editor.set_theme(theme_name)
        editor.set_status_text("Switched to theme: " .. theme_name)
    else
        editor.set_status_text("Theme not found: " .. theme_name)
    end
end

-- Function to list available themes
function list_themes()
    if not is_plugin_enabled() then
        editor.set_status_text("Theme switcher plugin is disabled")
        return
    end
    
    local theme_list = "Available themes: " .. table.concat(available_themes, ", ")
    editor.set_status_text(theme_list)
end

-- Function to get current theme
function get_current_theme()
    if not is_plugin_enabled() then
        editor.set_status_text("Theme switcher plugin is disabled")
        return nil
    end
    
    local current_theme = editor.get_theme()
    editor.set_status_text("Current theme: " .. current_theme)
    return current_theme
end

-- Register event handlers
events.connect("theme_changed", "on_theme_changed")

-- Event handler for theme changes
function on_theme_changed(event_name, theme_name)
    editor.debug_log("Theme changed to: " .. theme_name)
end

-- Add functions to the plugin table
theme_switcher.toggle_theme = toggle_theme
theme_switcher.switch_to_next_theme = switch_to_next_theme
theme_switcher.switch_to_theme = switch_to_theme
theme_switcher.list_themes = list_themes
theme_switcher.get_current_theme = get_current_theme
theme_switcher.is_enabled = is_plugin_enabled

-- Plugin initialization
if is_plugin_enabled() then
    editor.debug_log("Theme Switcher plugin loaded and enabled")
    editor.set_status_text("Theme Switcher plugin ready - Current theme: " .. editor.get_theme())
else
    editor.debug_log("Theme Switcher plugin loaded but disabled")
end