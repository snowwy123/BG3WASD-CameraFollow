#include "UpdateCameraHook.hpp"
#include "Hooks/IsInControllerModeHook.hpp"
#include "Settings.hpp"
#include "State.hpp"

#include <fstream>
#include <sstream>
#include <string>
#include <Windows.h>
#include <cmath>

void UpdateCameraHook::EnableSpecifically(uintptr_t address_incl_offset)
{
    OriginalFunc = dku::Hook::write_call<5>(address_incl_offset, OverrideFunc);
}

static bool ReadYawBridge(float& yawRad, float& yawDeg, std::string& guid)
{
    const char* path =
        "C:\\Users\\Cam\\AppData\\Local\\Larian Studios\\Baldur's Gate 3\\Script Extender\\YawBridge.txt";

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

// Called in GameThread, before HandleCameraInput.
int64_t UpdateCameraHook::OverrideFunc(uint64_t a1, uint64_t a2, uint64_t a3, int64_t a4)
{
    auto* settings = Settings::GetSingleton();
    auto* state = State::GetSingleton();

    if (IsInControllerModeHook::Get().Read())
    {
        return OriginalFunc(a1, a2, a3, a4);
    }

    int64_t camera_object_ptr = *(int64_t*)(a4 + 48);

    if (camera_object_ptr && *settings->enable_camera_follow)
    {
        float* cameraAngle = reinterpret_cast<float*>(camera_object_ptr + 0xAC);

        float yawRad = 0.0f;
        float yawDeg = 0.0f;
        std::string guid;

        const bool yawRead = ReadYawBridge(yawRad, yawDeg, guid);

        if (yawRead)
        {
            const bool wHeld = (GetAsyncKeyState('W') & 0x8000) != 0;
            const bool sHeld = (GetAsyncKeyState('S') & 0x8000) != 0;
            const bool aHeld = (GetAsyncKeyState('A') & 0x8000) != 0;
            const bool dHeld = (GetAsyncKeyState('D') & 0x8000) != 0;

            const bool qHeld = (GetAsyncKeyState('Q') & 0x8000) != 0;
            const bool eHeld = (GetAsyncKeyState('E') & 0x8000) != 0;
            const bool middleMouseHeld = (GetAsyncKeyState(VK_MBUTTON) & 0x8000) != 0;

            float desiredCameraOffset =
                static_cast<float>(*settings->camera_follow_offset);

            // W+A felt better with a wider shoulder angle during testing.
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
            float offsetMove = offsetDelta * 0.12f;

            if (offsetMove > 4.0f) offsetMove = 4.0f;
            if (offsetMove < -4.0f) offsetMove = -4.0f;

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

            float step =
                delta *
                static_cast<float>(*settings->camera_follow_strength);

            float maxStep =
                static_cast<float>(*settings->camera_follow_max_step);

            if (step > maxStep) step = maxStep;
            if (step < -maxStep) step = -maxStep;

            const bool forwardBackHeld = wHeld || sHeld;
            const bool strafeHeld = aHeld || dHeld;
            const bool straightMoveHeld = forwardBackHeld && !strafeHeld;
            const bool movementInput = wHeld || aHeld || sHeld || dHeld;
            const bool manualCameraInput = qHeld || eHeld || middleMouseHeld;

            const uint32_t nowMoveTime = SDL_GetTicks();

            if (straightMoveHeld && !wasStraightMoveHeld)
            {
                straightMoveStartTime = nowMoveTime;
            }

            wasStraightMoveHeld = straightMoveHeld;

            // Let the player look around without the camera snapping back instantly.
            if (manualCameraInput)
            {
                followSuspendedByManualCamera = true;
            }

            if (movementInput)
            {
                followSuspendedByManualCamera = false;
            }

            bool combatActive =
                (*reinterpret_cast<bool*>(camera_object_ptr + 168) & 1) != 0;

            bool allowFollow = true;

            if (combatActive)
            {
                allowFollow = false;
            }

            if (followSuspendedByManualCamera)
            {
                allowFollow = false;
            }

            // W-only/S-only can spiral because BG3WASD movement is camera-relative.
            if (straightMoveHeld)
            {
                allowFollow =
                    (nowMoveTime - straightMoveStartTime) <
                    static_cast<uint32_t>(
                        *settings->camera_follow_straight_move_drift_ms);
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

    return OriginalFunc(a1, a2, a3, a4);
}