#pragma once

#include "camera_display.hpp"
#include "../../../libs/imgui/imgui.h"

#include <thread>


namespace cam = camera_usb;


namespace camera_display
{
    inline constexpr cstr decode_status(cam::ConnectionStatus s)
    {
        using S = cam::ConnectionStatus;

        switch (s)
        {
        case S::Disconnected: return "No cameras";
        case S::Connecting:   return "Connecting...";
        case S::Connected:    return "Cameras OK";

        default: return "UNKN";
        }
    }


    inline constexpr cstr decode_status(cam::CameraStatus s)
    {
        using S = cam::CameraStatus;

        switch (s)
        {
        case S::Inactive:  return "I";
        case S::Active:    return "A";
        case S::Open:      return "O";
        case S::Streaming: return "S";

        default: return "UNKN";
        }
    }
}


namespace camera_display
{
    static void open_camera(CameraState& state, cam::Camera& camera)
    {
        if (!cam::open_camera(camera))
        {
            assert(false && "*** did not open camera ***");
            return;
        }

        auto const is_on = [&](){ return state.is_streaming; };

        auto const on_grab = [&](img::ImageView const& frame){ img::copy(frame, state.display); };

        state.is_streaming = true;

        cam::stream_camera(camera, on_grab, is_on);
    }


    static void open_camera_async(CameraState& state, cam::Camera& camera)
    {
        if (camera.busy)
        {
            return;
        }

        std::thread th([&](){ open_camera(state, camera); });

        th.detach();
    }


    static void close_camera_async(CameraState& state, cam::Camera& camera)
    {
        if (camera.busy)
        {
            return;
        }

        std::thread th([&]()
        { 
            state.is_streaming = false;
            cam::close_camera(camera); 
        });

        th.detach();
    }
    
    
    static void toggle_activate_async(CameraState& state, cam::Camera& camera)
    {
        b8 busy = 0;

        using S = cam::CameraStatus;

        switch (camera.status)
        {
        case S::Inactive:
            
            break;
        
        case S::Active:
            open_camera_async(state, camera);
            busy = 1;
            break;

        default:
            close_camera_async(state, camera);
            busy = 0;
            break;
        }

        for (u32 i = 0; i < state.cameras.count; i++)
        {
            auto& c = state.cameras.list[i];
            if (c.id != camera.id)
            {
                c.busy = busy;
            }
        }
    }


    class CameraCommand
    {
    public:
        b8 toggle = 0;
        int camera_id = -1;
    };
}


namespace camera_display
{
    constexpr auto im_gray(f32 value)
    {
        return ImVec4(value, value, value, 1.0f);
    }


    static void camera_properties_table(cam::CameraList& cameras, CameraCommand& cmd)
    {
        enum class columns : int
        {
            radio = 0,
            width,
            height,
            fps,
            format,
            vendor,
            product,
            serial,
            activate,
            status,
            count
        };

        int table_flags = ImGuiTableFlags_BordersInnerV;
        auto table_dims = ImVec2(0.0f, 0.0f);

        static int camera_id = 0;

        //ImU32 cell_bg_color = ImGui::GetColorU32(im_gray(0.1f));

        auto const setup_columns = []()
        {
            ImGui::TableSetupColumn("Camera", ImGuiTableColumnFlags_WidthFixed, 40.0f);
            ImGui::TableSetupColumn("W", ImGuiTableColumnFlags_WidthFixed, 30.0f);
            ImGui::TableSetupColumn("H", ImGuiTableColumnFlags_WidthFixed, 30.0f);
            ImGui::TableSetupColumn("FPS", ImGuiTableColumnFlags_WidthFixed, 30.0f);
            ImGui::TableSetupColumn("Format", ImGuiTableColumnFlags_WidthFixed, 50.0f);
            ImGui::TableSetupColumn("Vendor", ImGuiTableColumnFlags_WidthFixed, 50.0f);
            ImGui::TableSetupColumn("Product", ImGuiTableColumnFlags_WidthFixed, 50.0f);
            ImGui::TableSetupColumn("SN", ImGuiTableColumnFlags_WidthFixed, 50.0f);
            ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 80.0f);
            ImGui::TableSetupColumn("Status", ImGuiTableColumnFlags_WidthFixed, 50.0f);
            ImGui::TableHeadersRow();
        };

        auto const table_row = [&](cam::Camera& camera)
        {
            ImGui::TableSetColumnIndex((int)columns::radio);
            ImGui::RadioButton(camera.label.begin, &camera_id, camera.id);

            ImGui::TableSetColumnIndex((int)columns::width);
            ImGui::Text("%u", camera.frame_width);

            ImGui::TableSetColumnIndex((int)columns::height);
            ImGui::Text("%u", camera.frame_height);

            ImGui::TableSetColumnIndex((int)columns::fps);
            ImGui::Text("%u", camera.fps);

            ImGui::TableSetColumnIndex((int)columns::format);
            ImGui::Text("%s", camera.format.begin);

            ImGui::TableSetColumnIndex((int)columns::vendor);
            ImGui::Text("%s", camera.vendor.begin);

            ImGui::TableSetColumnIndex((int)columns::product);
            ImGui::Text("%s", camera.product.begin);

            ImGui::TableSetColumnIndex((int)columns::serial);
            ImGui::Text("%s", camera.serial_number.begin);

            ImGui::TableSetColumnIndex((int)columns::status);
            ImGui::Text("%s", decode_status(camera.status));

            ImGui::TableSetColumnIndex((int)columns::activate);
            if (!camera.busy)
            {
                auto label = camera.status == cam::CameraStatus::Active ? "Turn On" : "Turn Off";
                if (ImGui::Button(label))
                {
                    cmd.toggle = 1;
                    cmd.camera_id = camera.id;
                }
            }
            
        };

        if (!ImGui::BeginTable("CameraPropertiesTable", (int)columns::count, table_flags, table_dims)) 
        { 
            return; 
        }

        setup_columns();

        for (u32 i = 0; i < cameras.count; i++)
        {
            ImGui::TableNextRow();
            table_row(cameras.list[i]);
        }

        ImGui::EndTable();
    }
}


/* api */

namespace camera_display
{
    void init_async(CameraState& state)
    {
        std::thread th([&]()
        {
            state.cameras = camera_usb::enumerate_cameras();
        });

        th.detach();
    }


    void close(CameraState& state)
    {
        camera_usb::close(state.cameras);
    }


    void show_cameras(CameraState& state)
    { 
        CameraCommand cmd{};
        camera_properties_table(state.cameras, cmd);
        
        ImGui::Text("%s", decode_status(state.cameras.status));

        if (cmd.toggle)
        {
            toggle_activate_async(state, state.cameras.list[cmd.camera_id]);
        }
        
    }
}