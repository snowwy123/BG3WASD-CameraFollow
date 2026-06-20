BG3YawBridge_Ready = false

Ext.Events.SessionLoaded:Subscribe(function()
    BG3YawBridge_Ready = true
    Ext.Utils.Print("[BG3YawBridge] Session loaded")
end)

local lastWrite = 0

Ext.Events.Tick:Subscribe(function()
    -- Some setups fire Tick before Osiris query adapters are ready.
    -- Keep the original direct Osi.GetHostCharacter() logic, but wait for SessionLoaded first.
    if not BG3YawBridge_Ready then
        return
    end

    local now = Ext.Utils.MonotonicTime()

    if now - lastWrite < 250 then
        return
    end

    lastWrite = now

    local host = Osi.GetHostCharacter()

    if host == nil then
        return
    end

    local entity = Ext.Entity.Get(host)

    if entity == nil or entity.Steering == nil then
        return
    end

    local yawRad = entity.Steering.TargetRotation
    local yawDeg = math.deg(yawRad)

    Ext.IO.SaveFile(
        "YawBridge.txt",
        tostring(yawRad) .. "\n" ..
        tostring(yawDeg) .. "\n" ..
        tostring(host)
    )
end)

Ext.Require("Server/BG3WASD_Notifications.lua")
