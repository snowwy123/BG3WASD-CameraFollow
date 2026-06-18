#pragma comment(lib, "version.lib")

#include "Settings.hpp"
#include "Hooks/LoadInputConfigHook.hpp"
#include "InputconfigPatcher.hpp"
#include "State.hpp"

using enum Command;

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

        config.Bind(camera_follow_offset, -110.0);
        config.Bind(camera_follow_left_turn_offset, -210.0);

        config.Bind(camera_follow_offset_transition_strength, 0.02);
        config.Bind(camera_follow_offset_transition_max_step, 1.0);

        config.Bind(camera_follow_offset_transition_out_strength, 0.015);
        config.Bind(camera_follow_offset_transition_out_max_step, 0.6);

        config.Bind(camera_follow_target_strength, 0.065);
        config.Bind(camera_follow_target_max_step, 2.0);

        config.Bind(camera_follow_strength, 0.13);
        config.Bind(camera_follow_max_step, 3.5);

        config.Bind(camera_follow_near_snap_max_step, 0.5);

        config.Bind(camera_follow_straight_move_drift_ms, 200);

        config.Bind(camera_follow_wa_multiplier, 0.30);
        config.Bind(camera_follow_wd_multiplier, 0.75);
        config.Bind(camera_follow_sa_multiplier, 0.50);
        config.Bind(camera_follow_sd_multiplier, 0.50);
    }

    config.Load();

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
