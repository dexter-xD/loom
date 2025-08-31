-- autosave plugin
-- automatically saves files every 30 seconds

autosave = {
    name = "autosave",
    version = "1.0",
    description = "Automatically saves files every 30 seconds",
    
    -- plugin state
    timer_id = nil,
    interval = 30000, -- 30 seconds in milliseconds
    enabled = true,
    last_save_time = 0,
    save_count = 0
}

-- initialize plugin
function autosave.initialize()
    print("Initializing autosave plugin...")
    
    -- check if autosave is enabled in configuration
    if config and config.plugins and config.plugins.autosave then
        autosave.enabled = config.plugins.autosave.enabled or true
        autosave.interval = (config.plugins.autosave.interval or 30) * 1000 -- convert to ms
    end
    
    if not autosave.enabled then
        print("Autosave plugin is disabled in configuration")
        return
    end
    
    -- create timer for autosave
    autosave.timer_id = timer.create(autosave.interval, "autosave_timer_callback", true)
    
    if autosave.timer_id then
        print(string.format("Autosave timer created with ID %d, interval: %d seconds", 
              autosave.timer_id, autosave.interval / 1000))
        editor.set_status_text(string.format("Autosave enabled (%ds interval)", autosave.interval / 1000))
    else
        print("Failed to create autosave timer")
    end
    
    -- register for file events to track when files are opened/saved manually
    events.connect("file_opened", "autosave.on_file_opened")
    events.connect("file_saved", "autosave.on_file_saved")
    events.connect("text_changed", "autosave.on_text_changed")
end

-- cleanup plugin
function autosave.cleanup()
    print("Cleaning up autosave plugin...")
    
    if autosave.timer_id then
        timer.stop(autosave.timer_id)
        autosave.timer_id = nil
        print("Autosave timer stopped")
    end
    
    -- cleanup global callback function
    _G["autosave_timer_callback"] = nil
end

-- timer callback - this is called every interval
function autosave.on_timer()
    if not autosave.enabled then
        return
    end
    
    -- get current text to check if there are changes
    local current_text = editor.get_text()
    
    -- only save if there's content and it's not empty
    if current_text and string.len(current_text) > 0 then
        -- notify autoformat that autosave is starting
        if _G["autoformat.on_autosave_start"] then
            _G["autoformat.on_autosave_start"]()
        end
        
        -- save the current file
        editor.save_file()
        
        autosave.save_count = autosave.save_count + 1
        autosave.last_save_time = os.time()
        
        print(string.format("Autosave: File saved automatically (count: %d)", autosave.save_count))
        editor.set_status_text(string.format("Autosaved at %s (count: %d)", 
                               os.date("%H:%M:%S"), autosave.save_count))
        
        -- notify autoformat that autosave is done
        if _G["autoformat.on_autosave_end"] then
            _G["autoformat.on_autosave_end"]()
        end
    else
        print("Autosave: No content to save")
    end
end

-- make the timer callback globally accessible
_G["autosave_timer_callback"] = autosave.on_timer

-- event handlers
function autosave.on_file_opened(event_name, file_path)
    print(string.format("Autosave: File opened - %s", file_path or "unknown"))
    -- reset save count for new file
    autosave.save_count = 0
end

function autosave.on_file_saved(event_name, file_path)
    print(string.format("Autosave: File saved manually - %s", file_path or "unknown"))
    -- update last save time when user saves manually
    autosave.last_save_time = os.time()
end

function autosave.on_text_changed(event_name, content, file_path)
    -- we could implement smart autosave here that only saves after a certain
    -- amount of changes or time since last change, but for now we rely on the timer
end

-- utility functions
function autosave.get_status()
    if not autosave.enabled then
        return "Autosave: Disabled"
    end
    
    local status = string.format("Autosave: Enabled (%ds interval)", autosave.interval / 1000)
    
    if autosave.save_count > 0 then
        status = status .. string.format(", %d saves", autosave.save_count)
        
        if autosave.last_save_time > 0 then
            local time_since_save = os.time() - autosave.last_save_time
            status = status .. string.format(", last save %ds ago", time_since_save)
        end
    end
    
    return status
end

function autosave.toggle()
    autosave.enabled = not autosave.enabled
    
    if autosave.enabled then
        if not autosave.timer_id then
            autosave.timer_id = timer.create(autosave.interval, "autosave_timer_callback", true)
        end
        print("Autosave enabled")
        editor.set_status_text("Autosave enabled")
    else
        if autosave.timer_id then
            timer.stop(autosave.timer_id)
            autosave.timer_id = nil
        end
        print("Autosave disabled")
        editor.set_status_text("Autosave disabled")
    end
end

print("Autosave plugin loaded successfully!")