local BG3WASD_NotificationPath = "BG3WASD_Notification.txt"
local BG3WASD_LastNotificationContents = nil
local BG3WASD_LastNotificationPoll = 0

local function BG3WASD_LoadNotificationFile()
    local ok, contents = pcall(function()
        return Ext.IO.LoadFile(BG3WASD_NotificationPath)
    end)

    if not ok then
        return nil
    end

    return contents
end

local function BG3WASD_ExtractNotificationMessage(contents)
    if contents == nil or contents == "" then
        return nil
    end

    -- Native side writes two lines:
    -- line 1: unique id / tick
    -- line 2: message text
    local message = contents:match("^[^\r\n]*[\r\n]+(.+)$") or contents

    if message == nil then
        return nil
    end

    message = message:gsub("^[\r\n%s]+", "")
    message = message:gsub("[\r\n%s]+$", "")

    if message == "" then
        return nil
    end

    return message
end

-- Do not replay an old toggle message from a previous game session.
BG3WASD_LastNotificationContents = BG3WASD_LoadNotificationFile()

Ext.Events.Tick:Subscribe(function()
    local now = Ext.Utils.MonotonicTime()

    if now - BG3WASD_LastNotificationPoll < 150 then
        return
    end

    BG3WASD_LastNotificationPoll = now

    local contents = BG3WASD_LoadNotificationFile()

    if contents == nil or contents == "" then
        return
    end

    if contents == BG3WASD_LastNotificationContents then
        return
    end

    BG3WASD_LastNotificationContents = contents

    local message = BG3WASD_ExtractNotificationMessage(contents)

    if message == nil then
        return
    end

    local host = Osi.GetHostCharacter()

    if host == nil then
        return
    end

    Osi.ShowNotification(host, message)
end)
