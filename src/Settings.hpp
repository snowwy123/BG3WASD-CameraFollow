#pragma once

#include "DKUtil/Config.hpp"

using namespace DKUtil::Alias;

class Settings : public DKUtil::model::Singleton<Settings>
{
public:
    String toggle_walkspeed{ "ToggleWalkspeed", "ModHotkeys" };
    String toggle_movement_mode{ "ToggleMovementMode", "ModHotkeys" };
    String hold_movement_mode{ "HoldMovementMode", "ModHotkeys" };
    String toggle_autoforward{ "ToggleAutoforward", "ModHotkeys" };
    String toggle_camera_follow{ "ToggleCameraFollow", "ModHotkeys" };
    String toggle_mouse_steering_follow{ "ToggleMouseSteeringFollowMode", "ModHotkeys" };
    String hold_walkspeed{ "HoldWalkspeed", "ModHotkeys" };
    String reload_config{ "ReloadConfig", "ModHotkeys" };

    Double walk_speed{ "WalkSpeed", "Core" };
    Boolean walking_is_default{ "WalkingIsDefault", "Core" };
    Boolean walk_after_combat{ "SwitchToWalkingAfterCombat", "Core" };

    Boolean enable_auto_toggling_movement_mode{
        "EnableAutoTogglingMovementMode",
        "AutoToggleMovementMode"
    };

    Boolean enable_improved_mouselook{ "EnableMouselook", "Mouselook" };
    Boolean enable_rotate_plus_lmb_is_forward{
        "EnableRotatePlusLeftclickMovesForward",
        "Mouselook"
    };
    Integer rotate_threshold{ "RotateThreshold", "Mouselook" };

    Boolean block_interact_move{ "BlockInteractMove", "InteractMoveBlocker" };

    Boolean enable_camera_follow{ "EnableCameraFollow", "CameraFollow" };
    Boolean enable_mouse_steering_follow{ "EnableMouseSteeringFollowMode", "CameraFollow" };

    Double mouse_steering_sensitivity{ "MouseSteeringSensitivity", "CameraFollow" };

    Boolean mouse_steering_enable_pitch{ "MouseSteeringEnablePitch", "CameraFollow" };
    Double mouse_steering_pitch_sensitivity{ "MouseSteeringPitchSensitivity", "CameraFollow" };
    Double mouse_steering_pitch_min{ "MouseSteeringPitchMin", "CameraFollow" };
    Double mouse_steering_pitch_max{ "MouseSteeringPitchMax", "CameraFollow" };

    Boolean mouse_steering_forward_only{ "MouseSteeringForwardOnly", "CameraFollow" };

    Boolean mouse_steering_lock_cursor{ "MouseSteeringLockCursor", "CameraFollow" };

    Boolean camera_follow_suspend_on_manual_camera{
        "SuspendFollowOnManualCamera",
        "CameraFollow"
    };

    Boolean camera_follow_disable_in_combat{
        "DisableFollowInCombat",
        "CameraFollow"
    };

    Double camera_follow_offset{ "CameraOffset", "CameraFollow" };
    Double camera_follow_left_turn_offset{ "LeftTurnOffset", "CameraFollow" };

    Double camera_follow_offset_transition_strength{
        "OffsetTransitionStrength",
        "CameraFollow"
    };

    Double camera_follow_offset_transition_max_step{
        "OffsetTransitionMaxStep",
        "CameraFollow"
    };

    Double camera_follow_offset_transition_out_strength{
        "OffsetTransitionOutStrength",
        "CameraFollow"
    };

    Double camera_follow_offset_transition_out_max_step{
        "OffsetTransitionOutMaxStep",
        "CameraFollow"
    };

    Double camera_follow_target_strength{ "TargetFollowStrength", "CameraFollow" };
    Double camera_follow_target_max_step{ "TargetFollowMaxStep", "CameraFollow" };

    Double camera_follow_strength{ "CameraFollowStrength", "CameraFollow" };
    Double camera_follow_max_step{ "CameraFollowMaxStep", "CameraFollow" };

    Double camera_follow_near_snap_max_step{ "NearSnapMaxStep", "CameraFollow" };

    Integer camera_follow_straight_move_drift_ms{
        "StraightMoveDriftMs",
        "CameraFollow"
    };

    Double camera_follow_wa_multiplier{ "WA_Multiplier", "CameraFollow" };
    Double camera_follow_wd_multiplier{ "WD_Multiplier", "CameraFollow" };
    Double camera_follow_sa_multiplier{ "SA_Multiplier", "CameraFollow" };
    Double camera_follow_sd_multiplier{ "SD_Multiplier", "CameraFollow" };

    void Load() noexcept;
    std::vector<std::string> GetBoundKeycombos(std::string setting);

private:
    TomlConfig config = COMPILE_PROXY("NativeMods/BG3WASD.toml"sv);
    bool first_time_loaded = true;

    void InitState();
};