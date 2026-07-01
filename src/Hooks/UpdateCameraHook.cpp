#include "UpdateCameraHook.hpp"
#include "Hooks/IsInControllerModeHook.hpp"
#include "InputHook.hpp"
#include "Settings.hpp"
#include "State.hpp"

#include <fstream>
#include <sstream>
#include <string>
#include <Windows.h>
#include <cmath>
#include <cstdlib>

void UpdateCameraHook::EnableSpecifically(uintptr_t address_incl_offset)
{
    OriginalFunc = dku::Hook::write_call<5>(address_incl_offset, OverrideFunc);
}

static std::string GetYawBridgePath()
{
    char* localAppData = nullptr;
    size_t len = 0;

    if (_dupenv_s(&localAppData, &len, "LOCALAPPDATA") != 0 || localAppData == nullptr)
    {
        return "";
    }

    std::string path = localAppData;
    free(localAppData);

    path += "\\Larian Studios\\Baldur's Gate 3\\Script Extender\\YawBridge.txt";

    return path;
}

static bool ReadYawBridge(float& yawRad, float& yawDeg, std::string& guid)
{
    const std::string path = GetYawBridgePath();

    if (path.empty())
    {
        return false;
    }

    std::ifstream file(path);

    if (!file.is_open())
    {
        return false;
    }

    std::string lineRad;
    std::string lineDeg;
    std::string lineGuid;

    if (!std::getline(file, lineRad)) return false;
    if (!std::getline(file, lineDeg)) return false;
    if (!std::getline(file, lineGuid)) return false;

    try
    {
        yawRad = std::stof(lineRad);
        yawDeg = std::stof(lineDeg);
        guid = lineGuid;
        return true;
    }
    catch (...)
    {
        return false;
    }
}


static bool TryGetForegroundGameWindowCenter(POINT& center)
{
    HWND hwnd = GetForegroundWindow();
    if (!hwnd)
    {
        return false;
    }

    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);
    if (pid != GetCurrentProcessId())
    {
        return false;
    }

    RECT clientRect{};
    if (!GetClientRect(hwnd, &clientRect))
    {
        return false;
    }

    center.x = (clientRect.right - clientRect.left) / 2;
    center.y = (clientRect.bottom - clientRect.top) / 2;

    return ClientToScreen(hwnd, &center) != 0;
}


static void SetMouseSteeringGameRotate(State* state, bool active)
{
    static bool wasActive = false;

    if (active == wasActive)
    {
        return;
    }

    // Use BG3's own CameraToggleMouseRotate command for the optional
    // vertical mouse-steering mode. This mimics holding middle mouse while
    // moving, so the game handles both yaw and pitch normally.
    state->SetIsRotating(active);
    InputHook::ResetMouseMoveTracking();
    wasActive = active;
}

static void SetMouseSteeringCursorLock(State* state, bool active)
{
    static bool wasActive = false;
    static POINT restorePoint{};
    static bool restorePointValid = false;

    if (active)
    {
        POINT center{};
        if (!TryGetForegroundGameWindowCenter(center))
        {
            if (wasActive)
            {
                state->HideCursor(false);
                InputHook::ResetMouseMoveTracking();
                wasActive = false;
                restorePointValid = false;
            }
            return;
        }

        if (!wasActive)
        {
            GetCursorPos(&restorePoint);
            restorePointValid = true;
            state->HideCursor(true);
            wasActive = true;
        }

        // Keep the cursor from hitting screen edges while steering.
        // Reset first so the recenter move doesn't count as camera input.
        InputHook::ResetMouseMoveTracking();
        SetCursorPos(center.x, center.y);
        return;
    }

    if (wasActive)
    {
        state->HideCursor(false);
        InputHook::ResetMouseMoveTracking();

        if (restorePointValid)
        {
            SetCursorPos(restorePoint.x, restorePoint.y);
        }

        restorePointValid = false;
        wasActive = false;
    }
}

// Called in GameThread, before HandleCameraInput.
int64_t UpdateCameraHook::OverrideFunc(uint64_t a1, uint64_t a2, uint64_t a3, int64_t a4)
{
    auto* settings = Settings::GetSingleton();
    auto* state = State::GetSingleton();

    if (IsInControllerModeHook::Get().Read())
    {
        InputHook::ConsumeMouseMoveX();
        InputHook::ConsumeMouseMoveY();
        SetMouseSteeringGameRotate(state, false);
        SetMouseSteeringCursorLock(state, false);
        return OriginalFunc(a1, a2, a3, a4);
    }

    int64_t originalResult = OriginalFunc(a1, a2, a3, a4);

    int64_t camera_object_ptr = *(int64_t*)(a4 + 48);

    bool cameraFollowBlockedByCombat = false;

    if (camera_object_ptr && *settings->camera_follow_disable_in_combat)
    {
        const bool cameraObjectCombat =
            (*reinterpret_cast<bool*>(camera_object_ptr + 168) & 1) != 0;

        cameraFollowBlockedByCombat =
            cameraObjectCombat || state->old_combat_state;
    }

    const bool wHeld = (GetAsyncKeyState('W') & 0x8000) != 0;
    const bool sHeld = (GetAsyncKeyState('S') & 0x8000) != 0;
    const bool aHeld = (GetAsyncKeyState('A') & 0x8000) != 0;
    const bool dHeld = (GetAsyncKeyState('D') & 0x8000) != 0;

    const bool qHeld = (GetAsyncKeyState('Q') & 0x8000) != 0;
    const bool eHeld = (GetAsyncKeyState('E') & 0x8000) != 0;
    const bool middleMouseHeld = (GetAsyncKeyState(VK_MBUTTON) & 0x8000) != 0;

    const bool forwardBackHeld = wHeld || sHeld;
    const bool strafeHeld = aHeld || dHeld;
    const bool straightMoveHeld = forwardBackHeld && !strafeHeld;
    const bool movementInput = wHeld || aHeld || sHeld || dHeld;
    const bool manualCameraInput = qHeld || eHeld || middleMouseHeld;

    const bool characterMovementMode = state->IsCharacterMovementMode();
    const bool cameraFollowBlockedByMovementMode =
        *settings->camera_follow_pause_during_movement_mode_override &&
        !characterMovementMode;

    const bool mouseSteeringInput =
        *settings->mouse_steering_forward_only
            ? (wHeld || (wHeld && aHeld) || (wHeld && dHeld))
            : movementInput;

    const bool cameraFollowActive =
        camera_object_ptr &&
        *settings->enable_camera_follow &&
        state->camera_follow_toggled &&
        !cameraFollowBlockedByMovementMode &&
        !cameraFollowBlockedByCombat;

    const bool mouseSteeringActive =
        camera_object_ptr &&
        *settings->enable_mouse_steering_follow &&
        state->mouse_steering_follow_toggled &&
        characterMovementMode &&
        !cameraFollowBlockedByMovementMode &&
        !cameraFollowBlockedByCombat;

    const bool shouldMouseSteerCamera =
        mouseSteeringActive &&
        mouseSteeringInput &&
        !middleMouseHeld;

    const bool useGameRotateMouseSteering =
        shouldMouseSteerCamera &&
        *settings->mouse_steering_allow_vertical_camera &&
        !state->rotate_keys.empty();

    const bool useDirectMouseSteering =
        shouldMouseSteerCamera &&
        !useGameRotateMouseSteering;

    SetMouseSteeringGameRotate(state, useGameRotateMouseSteering);

    const int mouseMoveX = useDirectMouseSteering
        ? InputHook::ConsumeMouseMoveX()
        : (InputHook::ConsumeMouseMoveX(), 0);

    const int mouseMoveY = useDirectMouseSteering
        ? InputHook::ConsumeMouseMoveY()
        : (InputHook::ConsumeMouseMoveY(), 0);

    if (*settings->mouse_steering_lock_cursor && useDirectMouseSteering)
    {
        SetMouseSteeringCursorLock(state, true);
    }
    else
    {
        SetMouseSteeringCursorLock(state, false);
    }

    if (cameraFollowActive || shouldMouseSteerCamera)
    {
        float* cameraAngle = reinterpret_cast<float*>(camera_object_ptr + 0xAC);
        float* cameraPitch = reinterpret_cast<float*>(camera_object_ptr + 0x164);

        if (useDirectMouseSteering && mouseMoveX != 0)
        {
            *cameraAngle +=
                static_cast<float>(mouseMoveX) *
                static_cast<float>(*settings->mouse_steering_sensitivity);

            while (*cameraAngle > 180.0f) *cameraAngle -= 360.0f;
            while (*cameraAngle < -180.0f) *cameraAngle += 360.0f;
        }

        // Legacy direct-pitch experiment. In the default vertical mode, BG3's
        // own CameraToggleMouseRotate input handles pitch instead.
        if (useDirectMouseSteering &&
            *settings->mouse_steering_enable_pitch &&
            mouseMoveY != 0)
        {
            float pitch =
                *cameraPitch +
                static_cast<float>(mouseMoveY) *
                static_cast<float>(*settings->mouse_steering_pitch_sensitivity);

            const float pitchMin =
                static_cast<float>(*settings->mouse_steering_pitch_min);
            const float pitchMax =
                static_cast<float>(*settings->mouse_steering_pitch_max);

            if (pitch < pitchMin) pitch = pitchMin;
            if (pitch > pitchMax) pitch = pitchMax;

            *cameraPitch = pitch;
        }

        if (cameraFollowActive)
        {
            float yawRad = 0.0f;
            float yawDeg = 0.0f;
            std::string guid;

            const bool yawRead = ReadYawBridge(yawRad, yawDeg, guid);

            if (yawRead)
            {
            const uint32_t nowMoveTime = SDL_GetTicks();

            float desiredCameraOffset =
                static_cast<float>(*settings->camera_follow_offset);

            // W+A needs a wider shoulder angle or it feels a bit cramped.
            if (wHeld && aHeld)
            {
                desiredCameraOffset =
                    static_cast<float>(*settings->camera_follow_left_turn_offset);
            }

            static bool offsetInit = false;
            static float smoothedCameraOffset = 0.0f;

            if (!offsetInit)
            {
                smoothedCameraOffset = desiredCameraOffset;
                offsetInit = true;
            }

            float offsetDelta = desiredCameraOffset - smoothedCameraOffset;

            const bool returningToDefaultOffset =
                std::abs(
                    desiredCameraOffset -
                    static_cast<float>(*settings->camera_follow_offset)) < 0.01f;

            float offsetStrength =
                returningToDefaultOffset
                    ? static_cast<float>(*settings->camera_follow_offset_transition_out_strength)
                    : static_cast<float>(*settings->camera_follow_offset_transition_strength);

            float offsetMaxStep =
                returningToDefaultOffset
                    ? static_cast<float>(*settings->camera_follow_offset_transition_out_max_step)
                    : static_cast<float>(*settings->camera_follow_offset_transition_max_step);

            float offsetMove = offsetDelta * offsetStrength;

            if (offsetMove > offsetMaxStep) offsetMove = offsetMaxStep;
            if (offsetMove < -offsetMaxStep) offsetMove = -offsetMaxStep;

            smoothedCameraOffset += offsetMove;

            float rawTargetYaw = yawDeg + smoothedCameraOffset;

            while (rawTargetYaw > 180.0f) rawTargetYaw -= 360.0f;
            while (rawTargetYaw < -180.0f) rawTargetYaw += 360.0f;

            static bool targetInit = false;
            static float virtualTargetYaw = 0.0f;
            static bool wasStraightMoveHeld = false;
            static uint32_t straightMoveStartTime = 0;
            static bool followSuspendedByManualCamera = false;

            if (!targetInit)
            {
                virtualTargetYaw = rawTargetYaw;
                targetInit = true;
            }

            float targetDelta = rawTargetYaw - virtualTargetYaw;

            while (targetDelta > 180.0f) targetDelta -= 360.0f;
            while (targetDelta < -180.0f) targetDelta += 360.0f;

            float targetMove =
                targetDelta *
                static_cast<float>(*settings->camera_follow_target_strength);

            float targetMaxStep =
                static_cast<float>(*settings->camera_follow_target_max_step);

            if (targetMove > targetMaxStep) targetMove = targetMaxStep;
            if (targetMove < -targetMaxStep) targetMove = -targetMaxStep;

            virtualTargetYaw += targetMove;

            while (virtualTargetYaw > 180.0f) virtualTargetYaw -= 360.0f;
            while (virtualTargetYaw < -180.0f) virtualTargetYaw += 360.0f;

            float delta = virtualTargetYaw - *cameraAngle;

            while (delta > 180.0f) delta -= 360.0f;
            while (delta < -180.0f) delta += 360.0f;

            float step = 0.0f;

            float absDelta = std::abs(delta);

            float followStrength = static_cast<float>(*settings->camera_follow_strength);

            float followMaxStep = static_cast<float>(*settings->camera_follow_max_step);

            const bool diagonalMoveHeld =
                (wHeld && aHeld) || (wHeld && dHeld) || (sHeld && aHeld) || (sHeld && dHeld);

            // Close-range snap protection.
            // Diagonals need proper rotation and big turns should not feel sluggish,
            // so only soften small close camera corrections.
            if (!diagonalMoveHeld && absDelta <= 55.0f)
            {
                float nearBlend = absDelta / 55.0f;

                // Very soft near the target, less soft as it gets farther out.
                nearBlend = nearBlend * nearBlend;

                float nearStrength = 0.04f;

                float blendedStrength =
                    nearStrength + ((followStrength - nearStrength) * nearBlend);

                step = delta * blendedStrength;

                float nearMaxStep = static_cast<float>(*settings->camera_follow_near_snap_max_step);

                if (step > nearMaxStep)
                    step = nearMaxStep;

                if (step < -nearMaxStep)
                    step = -nearMaxStep;
            }
            else
            {
                step = delta * followStrength;

                if (step > followMaxStep)
                    step = followMaxStep;

                if (step < -followMaxStep)
                    step = -followMaxStep;
            }

            if (straightMoveHeld && !wasStraightMoveHeld)
            {
                straightMoveStartTime = nowMoveTime;
            }

            wasStraightMoveHeld = straightMoveHeld;

            // Give manual camera input some breathing room instead of yanking back instantly.
            if (*settings->camera_follow_suspend_on_manual_camera)
            {
                if (manualCameraInput)
                {
                    followSuspendedByManualCamera = true;
                }
                else if (movementInput)
                {
                    // Movement resumes character-facing camera follow after a manual camera adjustment.
                    // Idle frames deliberately keep the suspension, so standing still does not yank
                    // the camera back to character facing after Q/E or middle-mouse rotation.
                    followSuspendedByManualCamera = false;
                }
            }
            else
            {
                followSuspendedByManualCamera = false;
            }

            bool combatActive =
                (*reinterpret_cast<bool*>(camera_object_ptr + 168) & 1) != 0;

            bool allowFollow = true;

            if (*settings->camera_follow_disable_in_combat &&
                combatActive)
            {
                allowFollow = false;
            }

            if (followSuspendedByManualCamera)
            {
                allowFollow = false;
            }


            // Straight W/S can feed back into camera-relative movement and spin, so limit it.
            if (straightMoveHeld)
            {
                allowFollow =
                    (nowMoveTime - straightMoveStartTime) <
                    static_cast<uint32_t>(
                        *settings->camera_follow_straight_move_drift_ms);
            }

            // F6 mouse steering owns yaw while active; don't stack smooth follow on top.
            if (shouldMouseSteerCamera)
            {
                allowFollow = false;
            }

            if (wHeld && aHeld)
            {
                step *= static_cast<float>(
                    *settings->camera_follow_wa_multiplier);
            }
            else if (wHeld && dHeld)
            {
                step *= static_cast<float>(
                    *settings->camera_follow_wd_multiplier);
            }
            else if (sHeld && aHeld)
            {
                step *= static_cast<float>(
                    *settings->camera_follow_sa_multiplier);
            }
            else if (sHeld && dHeld)
            {
                step *= static_cast<float>(
                    *settings->camera_follow_sd_multiplier);
            }

            if (allowFollow)
            {
                *cameraAngle += step;
            }
            }
        }
    }

    bool new_combat_state = false;

    if (camera_object_ptr)
    {
        new_combat_state = (*reinterpret_cast<bool*>(camera_object_ptr + 168) & 1) != 0;
    }

    if (!state->combat_state_initiliazed || new_combat_state != state->old_combat_state)
    {
        if (*settings->enable_auto_toggling_movement_mode)
        {
            state->SetMovementModeToggled(!new_combat_state);
        }

        if (new_combat_state == false && *settings->walk_after_combat)
        {
            state->walking_toggled = true;
        }

        state->old_combat_state = new_combat_state;
        state->combat_state_initiliazed = true;
        state->last_time_combat_state_changed = SDL_GetTicks();
    }

    return originalResult;
}