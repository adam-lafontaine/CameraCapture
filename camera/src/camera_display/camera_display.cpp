#pragma once

#include "camera_display.hpp"
#include "../../../libs/imgui/imgui.h"

#include <thread>
#include <array>


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


    class CameraCommand
    {
    public:
        b8 toggle_stream = 0;
        b8 grab = 0;
        int camera_id = -1;
    };
}


/* ui */

namespace camera_display
{
    constexpr auto im_gray(f32 value)
    {
        return ImVec4(value, value, value, 1.0f);
    }

    
    template <size_t LEN>
    class StrLabel
    {
    public:
        char data[LEN] = { 0 };
    };


    template <size_t N, size_t LEN>
    static constexpr std::array<StrLabel<LEN>, N> label_array(cstr base)
    {
        std::array<StrLabel<LEN>, N> labels;
        auto len = span::strlen(base);

        for (u32 i = 0; i < N; i++)
        {
            char* c = labels[i].data;

            for (u32 b = 0; b < len; b++)
            {
                *c = *(base + b);
                ++c;
            }

            *c = '#';
            ++c;
            *c = '#';
            ++c;

            for (u32 b = 0; b < len; b++)
            {
                *c = *(base + b);
                ++c;
            }

            *c = 'A' + i;
            ++c;
            *c = 0; // just in case
        }

        return labels;
    }


    static void camera_properties_table(cam::CameraList& cameras, CameraCommand& cmd)
    {
        enum class columns : int
        {
            camera = 0,
            width,
            height,
            fps,
            format,
            vendor,
            product,
            serial,
            status,
            grab,
            stream,            
            count
        };

        int table_flags = ImGuiTableFlags_BordersInnerV;
        auto table_dims = ImVec2(0.0f, 0.0f);

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
            ImGui::TableSetupColumn("Status", ImGuiTableColumnFlags_WidthFixed, 50.0f);
            ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 60.0f); // grab
            ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 60.0f); // stream
            
            ImGui::TableHeadersRow();
        };

        constexpr auto grab_btn_labels = label_array<16, 32>("Grab");
        constexpr auto stream_on_labels = label_array<16, 32>("On");
        constexpr auto stream_off_labels = label_array<16, 32>("Off");

        auto const table_row = [&](cam::Camera& camera)
        {
            ImGui::TableSetColumnIndex((int)columns::camera);
            ImGui::Text("%s", camera.label.begin);

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

            ImGui::TableSetColumnIndex((int)columns::grab);
            if (!camera.busy)
            {
                auto btn_label = (cstr)grab_btn_labels[camera.id].data;
                if (ImGui::Button(btn_label, ImVec2(50.0f, 0.0f)))
                {
                    cmd.grab = 1;
                    cmd.camera_id = camera.id;
                    printf("%d", camera.id);
                }
            }

            ImGui::TableSetColumnIndex((int)columns::stream);
            auto is_streaming = camera.status == cam::CameraStatus::Streaming;
            if (!camera.busy || is_streaming)
            {
                auto label_on = stream_on_labels[camera.id].data;
                auto label_off = stream_off_labels[camera.id].data;
                auto btn_label = (cstr)(is_streaming ? label_off : label_on);
                if (ImGui::Button(btn_label, ImVec2(50.0f, 0.0f)))
                {
                    cmd.toggle_stream = 1;
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


/* camera controls */

namespace camera_display
{
    static void init_cameras(CameraState& state)
    {
        state.cameras = camera_usb::enumerate_cameras();

        for (u32 i = 0; i < state.cameras.count; i++)
        {
            auto& camera = state.cameras.list[i];
            open_camera(camera);
        }
    }


    static void grab_image(CameraState& state, cam::Camera& camera)
    {
        cam::grab_image(camera, state.display);
    }


    static void stream_camera(CameraState& state, cam::Camera& camera)
    {
        auto const is_on = [&](){ return state.is_streaming; };

        auto const on_grab = [&](img::ImageView const& frame){ img::copy(frame, state.display); };

        state.is_streaming = true;

        for (u32 i = 0; i < state.cameras.count; i++)
        {
            auto& c = state.cameras.list[i];
            c.busy = 1;
        }

        cam::stream_camera(camera, on_grab, is_on);
    }


    static void grab_image_async(CameraState& state, cam::Camera& camera)
    {
        if (camera.busy)
        {
            return;
        }

        std::thread th([&](){ grab_image(state, camera); });

        th.detach();
    }


    static void stream_camera_async(CameraState& state, cam::Camera& camera)
    {
        if (camera.busy)
        {
            return;
        }

        std::thread th([&](){ stream_camera(state, camera); });

        th.detach();
    }
    
    
    static void toggle_stream_async(CameraState& state, cam::Camera& camera)
    {
        if (state.is_streaming)
        {
            state.is_streaming = false;
            for (u32 i = 0; i < state.cameras.count; i++)
            {
                auto& c = state.cameras.list[i];
                c.busy = 0;
            }
        }
        else
        {
            stream_camera_async(state, camera);
        }
    }
}


/* api */

namespace camera_display
{
    void init_async(CameraState& state)
    {
        std::thread th([&](){ init_cameras(state); });

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

        if (cmd.grab)
        {
            grab_image_async(state, state.cameras.list[cmd.camera_id]);
        }
        else if (cmd.toggle_stream)
        {
            toggle_stream_async(state, state.cameras.list[cmd.camera_id]);
        }
        
    }
}