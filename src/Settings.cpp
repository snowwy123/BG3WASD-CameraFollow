#pragma comment(lib, "version.lib")

#include "Settings.hpp"
#include "Hooks/LoadInputConfigHook.hpp"
#include "InputconfigPatcher.hpp"
#include "State.hpp"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>

using enum Command;

namespace
{
    std::string Trim(std::string value)
    {
        auto notSpace = [](unsigned char ch) { return !std::isspace(ch); };
        value.erase(value.begin(), std::find_if(value.begin(), value.end(), notSpace));
        value.erase(std::find_if(value.rbegin(), value.rend(), notSpace).base(), value.end());
        return value;
    }

    std::string StripInlineComment(const std::string& value)
    {
        bool inSingleQuote = false;
        bool inDoubleQuote = false;

        for (std::size_t i = 0; i < value.size(); ++i)
        {
            const char ch = value[i];
            if (ch == '\'' && !inDoubleQuote)
            {
                inSingleQuote = !inSingleQuote;
            }
            else if (ch == '"' && !inSingleQuote)
            {
                inDoubleQuote = !inDoubleQuote;
            }
            else if (ch == '#' && !inSingleQuote && !inDoubleQuote)
            {
                return value.substr(0, i);
            }
        }

        return value;
    }

    std::optional<std::string> FindTomlValue(
        const std::string& content,
        std::string_view sectionName,
        std::string_view keyName)
    {
        std::istringstream stream(content);
        std::string line;
        std::string currentSection;

        while (std::getline(stream, line))
        {
            line = Trim(StripInlineComment(line));
            if (line.empty())
            {
                continue;
            }

            if (line.front() == '[' && line.back() == ']')
            {
                currentSection = Trim(line.substr(1, line.size() - 2));
                continue;
            }

            if (currentSection != sectionName)
            {
                continue;
            }

            const auto equals = line.find('=');
            if (equals == std::string::npos)
            {
                continue;
            }

            const auto key = Trim(line.substr(0, equals));
            if (key != keyName)
            {
                continue;
            }

            return Trim(line.substr(equals + 1));
        }

        return std::nullopt;
    }

    std::optional<bool> ParseLegacyBool(const std::string& raw)
    {
        auto value = Trim(raw);
        std::ranges::transform(value, value.begin(), [](unsigned char ch) {
            return static_cast<char>(std::tolower(ch));
        });

        if (value == "true")
        {
            return true;
        }
        if (value == "false")
        {
            return false;
        }

        return std::nullopt;
    }

    std::optional<double> ParseLegacyDouble(const std::string& raw)
    {
        try
        {
            std::size_t consumed = 0;
            const auto value = std::stod(raw, &consumed);
            if (consumed == 0)
            {
                return std::nullopt;
            }
            return value;
        }
        catch (...)
        {
            return std::nullopt;
        }
    }

    std::optional<std::int64_t> ParseLegacyInteger(const std::string& raw)
    {
        try
        {
            std::size_t consumed = 0;
            const auto value = std::stoll(raw, &consumed);
            if (consumed == 0)
            {
                return std::nullopt;
            }
            return static_cast<std::int64_t>(value);
        }
        catch (...)
        {
            return std::nullopt;
        }
    }

    std::optional<std::string> ParseLegacyString(const std::string& raw)
    {
        auto value = Trim(raw);
        if (value.size() >= 2 &&
            ((value.front() == '"' && value.back() == '"') ||
             (value.front() == '\'' && value.back() == '\'')))
        {
            value = value.substr(1, value.size() - 2);
        }
        return value;
    }
}

void Settings::Load() noexcept
{
    if (first_time_loaded)
    {
        config.Bind(toggle_walkspeed, "key:insert");
        config.Bind(toggle_movement_mode, "key:capslock");
        config.Bind(hold_movement_mode, "");
        config.Bind(toggle_autoforward, "shift+key:w");
        config.Bind(toggle_camera_follow, "key:f7");
        config.Bind(toggle_mouse_steering_follow, "key:f6");
        config.Bind(hold_walkspeed, "");
        config.Bind(reload_config, "key:f11");

        config.Bind<0.0, 1.0>(walk_speed, 0.3);
        config.Bind(walking_is_default, FALSE);
        config.Bind(walk_after_combat, FALSE);

        config.Bind(enable_auto_toggling_movement_mode, TRUE);

        config.Bind(enable_improved_mouselook, TRUE);
        config.Bind(enable_rotate_plus_lmb_is_forward, TRUE);
        config.Bind(rotate_threshold, 200);

        config.Bind(block_interact_move, FALSE);

        config.Bind(enable_camera_follow, TRUE);
        config.Bind(enable_mouse_steering_follow, TRUE);

        config.Bind(mouse_steering_sensitivity, 0.12);
        config.Bind(mouse_steering_enable_pitch, TRUE);
        config.Bind(mouse_steering_pitch_sensitivity, -0.05);
        config.Bind(mouse_steering_pitch_min, -85.0);
        config.Bind(mouse_steering_pitch_max, 85.0);
        config.Bind(mouse_steering_forward_only, TRUE);
        config.Bind(mouse_steering_lock_cursor, TRUE);

        config.Bind(camera_follow_suspend_on_manual_camera, TRUE);
        config.Bind(camera_follow_disable_in_combat, TRUE);
        config.Bind(camera_follow_pause_during_movement_mode_override, TRUE);

        config.Bind(camera_follow_offset, -110.0);
        config.Bind(camera_follow_left_turn_offset, -210.0);

        config.Bind(camera_follow_offset_transition_strength, 0.025);
        config.Bind(camera_follow_offset_transition_max_step, 1.25);

        config.Bind(camera_follow_offset_transition_out_strength, 0.04);
        config.Bind(camera_follow_offset_transition_out_max_step, 1.5);

        config.Bind(camera_follow_target_strength, 0.08);
        config.Bind(camera_follow_target_max_step, 2.4);

        config.Bind(camera_follow_strength, 0.12);
        config.Bind(camera_follow_max_step, 3.0);

        config.Bind(camera_follow_near_snap_max_step, 0.5);

        config.Bind(camera_follow_straight_move_drift_ms, 50);

        config.Bind(camera_follow_wa_multiplier, 0.50);
        config.Bind(camera_follow_wd_multiplier, 0.75);
        config.Bind(camera_follow_sa_multiplier, 0.50);
        config.Bind(camera_follow_sd_multiplier, 0.50);
    }

    config.Load();
    ApplyLegacyConfigFallbacks();

    if (!*enable_improved_mouselook)
    {
        // TODO ToggleMouselook
        // *toggle_mouselook = "";
        // *toggle_movement_toggles_mouselook = false;
    }

    auto* state = State::GetSingleton();

    if (first_time_loaded)
    {
        InitState();
    }
    else
    {
        // During first load, this is called in AfterInitialLoadInputConfigHook.
        InputconfigPatcher::Patch();
    }

    state->EnableInteractMoveBlocker(state->IsCharacterMovementMode());

    first_time_loaded = false;
    INFO("Config loaded successfully."sv)
}

void Settings::InitState()
{
    auto* state = State::GetSingleton();

    state->walking_toggled = walking_is_default;
    // Master switch only. F6 still starts off until the player turns it on.
    state->mouse_steering_follow_toggled = false;

    // Flag invalid to react later.
    state->cursor_position_to_restore.x = -1;
}

std::vector<std::string> Settings::GetBoundKeycombos(std::string setting)
{
    std::vector<std::string> result = dku::string::split(setting, ","sv);
    return result;
}


void Settings::ApplyLegacyConfigFallbacks()
{
    const auto configPath = DKUtil::Config::GetPath("NativeMods/BG3WASD.toml"sv);
    std::ifstream file(configPath);
    if (!file.is_open())
    {
        return;
    }

    const std::string content(
        (std::istreambuf_iterator<char>(file)),
        std::istreambuf_iterator<char>());

    auto hasNewKey = [&](std::string_view section, std::string_view key) {
        return FindTomlValue(content, section, key).has_value();
    };

    auto applyBool = [&](auto& setting,
                         std::string_view newSection,
                         std::string_view newKey,
                         std::string_view oldSection,
                         std::string_view oldKey) {
        if (hasNewKey(newSection, newKey))
        {
            return;
        }

        if (const auto raw = FindTomlValue(content, oldSection, oldKey))
        {
            if (const auto parsed = ParseLegacyBool(*raw))
            {
                *setting = *parsed;
            }
        }
    };

    auto applyDouble = [&](auto& setting,
                           std::string_view newSection,
                           std::string_view newKey,
                           std::string_view oldSection,
                           std::string_view oldKey) {
        if (hasNewKey(newSection, newKey))
        {
            return;
        }

        if (const auto raw = FindTomlValue(content, oldSection, oldKey))
        {
            if (const auto parsed = ParseLegacyDouble(*raw))
            {
                *setting = *parsed;
            }
        }
    };

    auto applyInteger = [&](auto& setting,
                            std::string_view newSection,
                            std::string_view newKey,
                            std::string_view oldSection,
                            std::string_view oldKey) {
        if (hasNewKey(newSection, newKey))
        {
            return;
        }

        if (const auto raw = FindTomlValue(content, oldSection, oldKey))
        {
            if (const auto parsed = ParseLegacyInteger(*raw))
            {
                *setting = *parsed;
            }
        }
    };

    auto applyString = [&](auto& setting,
                           std::string_view newSection,
                           std::string_view newKey,
                           std::string_view oldSection,
                           std::string_view oldKey) {
        if (hasNewKey(newSection, newKey))
        {
            return;
        }

        if (const auto raw = FindTomlValue(content, oldSection, oldKey))
        {
            if (const auto parsed = ParseLegacyString(*raw))
            {
                *setting = *parsed;
            }
        }
    };

    applyString(toggle_mouse_steering_follow,
                "ModHotkeys", "ToggleMouseSteering",
                "ModHotkeys", "ToggleMouseSteeringFollowMode");

    applyBool(enable_mouse_steering_follow,
              "MouseSteering", "EnableMouseSteering",
              "CameraFollow", "EnableMouseSteeringFollowMode");

    applyDouble(mouse_steering_sensitivity,
                "MouseSteering", "MouseYawSensitivity",
                "CameraFollow", "MouseSteeringSensitivity");
    applyBool(mouse_steering_enable_pitch,
              "MouseSteering", "EnableMousePitch",
              "CameraFollow", "MouseSteeringEnablePitch");
    applyDouble(mouse_steering_pitch_sensitivity,
                "MouseSteering", "MousePitchSensitivity",
                "CameraFollow", "MouseSteeringPitchSensitivity");
    applyDouble(mouse_steering_pitch_min,
                "MouseSteering", "MousePitchMinDegrees",
                "CameraFollow", "MouseSteeringPitchMin");
    applyDouble(mouse_steering_pitch_max,
                "MouseSteering", "MousePitchMaxDegrees",
                "CameraFollow", "MouseSteeringPitchMax");
    applyBool(mouse_steering_forward_only,
              "MouseSteering", "MouseSteeringForwardOnly",
              "CameraFollow", "MouseSteeringForwardOnly");
    applyBool(mouse_steering_lock_cursor,
              "MouseSteering", "LockCursorWhileMouseSteering",
              "CameraFollow", "MouseSteeringLockCursor");

    applyBool(camera_follow_suspend_on_manual_camera,
              "CameraFollow", "PauseOnManualCameraInput",
              "CameraFollow", "SuspendFollowOnManualCamera");
    applyBool(camera_follow_disable_in_combat,
              "CameraFollow", "PauseInCombat",
              "CameraFollow", "DisableFollowInCombat");

    applyDouble(camera_follow_offset,
                "CameraFollow", "DefaultShoulderOffsetDegrees",
                "CameraFollow", "CameraOffset");
    applyDouble(camera_follow_left_turn_offset,
                "CameraFollow", "LeftTurnShoulderOffsetDegrees",
                "CameraFollow", "LeftTurnOffset");

    applyDouble(camera_follow_offset_transition_strength,
                "CameraFollow", "ShoulderBlendInResponsiveness",
                "CameraFollow", "OffsetTransitionStrength");
    applyDouble(camera_follow_offset_transition_max_step,
                "CameraFollow", "ShoulderBlendInMaxStep",
                "CameraFollow", "OffsetTransitionMaxStep");
    applyDouble(camera_follow_offset_transition_out_strength,
                "CameraFollow", "ShoulderBlendOutResponsiveness",
                "CameraFollow", "OffsetTransitionOutStrength");
    applyDouble(camera_follow_offset_transition_out_max_step,
                "CameraFollow", "ShoulderBlendOutMaxStep",
                "CameraFollow", "OffsetTransitionOutMaxStep");

    applyDouble(camera_follow_target_strength,
                "CameraFollow", "TargetDirectionResponsiveness",
                "CameraFollow", "TargetFollowStrength");
    applyDouble(camera_follow_target_max_step,
                "CameraFollow", "TargetDirectionMaxTurnSpeed",
                "CameraFollow", "TargetFollowMaxStep");

    applyDouble(camera_follow_strength,
                "CameraFollow", "CameraTurnResponsiveness",
                "CameraFollow", "CameraFollowStrength");
    applyDouble(camera_follow_max_step,
                "CameraFollow", "CameraMaxTurnSpeed",
                "CameraFollow", "CameraFollowMaxStep");
    applyDouble(camera_follow_near_snap_max_step,
                "CameraFollow", "CloseRangeMaxTurnSpeed",
                "CameraFollow", "NearSnapMaxStep");

    applyInteger(camera_follow_straight_move_drift_ms,
                 "CameraFollow", "StraightMovementFollowWindowMs",
                 "CameraFollow", "StraightMoveDriftMs");

    applyDouble(camera_follow_wa_multiplier,
                "CameraFollowMovementBias", "ForwardLeftTurnBias",
                "CameraFollow", "WA_Multiplier");
    applyDouble(camera_follow_wd_multiplier,
                "CameraFollowMovementBias", "ForwardRightTurnBias",
                "CameraFollow", "WD_Multiplier");
    applyDouble(camera_follow_sa_multiplier,
                "CameraFollowMovementBias", "BackwardLeftTurnBias",
                "CameraFollow", "SA_Multiplier");
    applyDouble(camera_follow_sd_multiplier,
                "CameraFollowMovementBias", "BackwardRightTurnBias",
                "CameraFollow", "SD_Multiplier");
}
