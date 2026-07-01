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
    String toggle_mouse_steering_follow{ "ToggleMouseSteering", "ModHotkeys" };
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
    Boolean enable_mouse_steering_follow{ "EnableMouseSteering", "MouseSteering" };

    Double mouse_steering_sensitivity{ "MouseYawSensitivity", "MouseSteering" };

    // Pitch settings are kept here for config compatibility / future work.
    // The current camera hook does not yet provide reliable vertical pitch control.
    Boolean mouse_steering_enable_pitch{ "EnableMousePitch", "MouseSteering" };
    Double mouse_steering_pitch_sensitivity{ "MousePitchSensitivity", "MouseSteering" };
    Double mouse_steering_pitch_min{ "MousePitchMinDegrees", "MouseSteering" };
    Double mouse_steering_pitch_max{ "MousePitchMaxDegrees", "MouseSteering" };

    Boolean mouse_steering_forward_only{ "MouseSteeringForwardOnly", "MouseSteering" };

    Boolean mouse_steering_lock_cursor{ "LockCursorWhileMouseSteering", "MouseSteering" };

    Boolean camera_follow_suspend_on_manual_camera{
        "PauseOnManualCameraInput",
        "CameraFollow"
    };

    Boolean camera_follow_disable_in_combat{
        "PauseInCombat",
        "CameraFollow"
    };

    Boolean camera_follow_pause_during_movement_mode_override{
        "PauseDuringMovementModeOverride",
        "CameraFollow"
    };

    Double camera_follow_offset{ "DefaultShoulderOffsetDegrees", "CameraFollow" };
    Double camera_follow_left_turn_offset{ "LeftTurnShoulderOffsetDegrees", "CameraFollow" };

    Double camera_follow_offset_transition_strength{
        "ShoulderBlendInResponsiveness",
        "CameraFollow"
    };

    Double camera_follow_offset_transition_max_step{
        "ShoulderBlendInMaxStep",
        "CameraFollow"
    };

    Double camera_follow_offset_transition_out_strength{
        "ShoulderBlendOutResponsiveness",
        "CameraFollow"
    };

    Double camera_follow_offset_transition_out_max_step{
        "ShoulderBlendOutMaxStep",
        "CameraFollow"
    };

    Double camera_follow_target_strength{ "TargetDirectionResponsiveness", "CameraFollow" };
    Double camera_follow_target_max_step{ "TargetDirectionMaxTurnSpeed", "CameraFollow" };

    Double camera_follow_strength{ "CameraTurnResponsiveness", "CameraFollow" };
    Double camera_follow_max_step{ "CameraMaxTurnSpeed", "CameraFollow" };

    Double camera_follow_near_snap_max_step{ "CloseRangeMaxTurnSpeed", "CameraFollow" };

    Integer camera_follow_straight_move_drift_ms{
        "StraightMovementFollowWindowMs",
        "CameraFollow"
    };

    Double camera_follow_wa_multiplier{ "ForwardLeftTurnBias", "CameraFollowMovementBias" };
    Double camera_follow_wd_multiplier{ "ForwardRightTurnBias", "CameraFollowMovementBias" };
    Double camera_follow_sa_multiplier{ "BackwardLeftTurnBias", "CameraFollowMovementBias" };
    Double camera_follow_sd_multiplier{ "BackwardRightTurnBias", "CameraFollowMovementBias" };

    void Load() noexcept;
    std::vector<std::string> GetBoundKeycombos(std::string setting);

private:
    TomlConfig config = COMPILE_PROXY("NativeMods/BG3WASD.toml"sv);
    bool first_time_loaded = true;

    void InitState();
    void ApplyLegacyConfigFallbacks();
};