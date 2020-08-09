#pragma once
#include "utils.h"

#include "imgui.h"

#include <DirectXMath.h>

struct ImGuiState
{
    bool show_demo_window = false;
    bool show = false;

    // Lighting.
    float light_power = 5.f;
    ImVec4 light_color = ImVec4(1.f, 1.f, 1.f, 1.f);

    // Model.
    float model_scale = 1.f;
    // Pitch, Yaw, Roll.
    // http://hugin.sourceforge.net/docs/manual/Image_positioning_model.html#:~:text=Positive%20Roll%20values%20mean%20the,%2B90%C2%B0%20(Zenith).
    ImVec4 model_rotation = ImVec4(0.f, 180.f, 0.f, 0.f);

    // Render config.
    bool wireframe = false;
    bool need_change_wireframe = false;
    bool show_model = false;
    bool show_zero_world_space = false;
    bool show_cube = false;
    bool show_cube_normals = true;
    bool enable_mouse = false;

    bool check_wireframe_change()
    {
        if (need_change_wireframe)
        {
            need_change_wireframe = false;
            return true;
        }
        return false;
    }
    DirectX::XMVECTOR get_light_color() const
    {
        return DirectX::XMVectorSet(
              light_power * light_color.x
            , light_power * light_color.y
            , light_power * light_color.z
            , 1.f);
    }
    DirectX::XMMATRIX get_model_scale() const
    {
        return DirectX::XMMatrixScaling(model_scale, model_scale, model_scale);
    }
    DirectX::XMMATRIX get_model_rotation() const
    {
        const float pitch = DegreesToRadians(model_rotation.x);
        const float yaw = DegreesToRadians(model_rotation.y);
        const float roll = DegreesToRadians(model_rotation.z);
        return DirectX::XMMatrixRotationRollPitchYaw(pitch, yaw, roll);
    }
};