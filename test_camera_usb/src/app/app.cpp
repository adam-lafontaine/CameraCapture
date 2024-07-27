#include "app.hpp"
#include "../../../libs/usb/camera_usb.hpp"
#include "../../../libs/alloc_type/alloc_type.hpp"

#include <cassert>

namespace cam = camera_usb;

namespace app
{
    class CameraId
    {
    public:
        int value = -1;
    };
}


/* state */

namespace app
{
    class StateData
    {
    public:

        cam::CameraList camera_list;
        
        CameraId camera_id;
    };


    static inline StateData& get_data(AppState const& state)
    {
        return *state.data_;        
    }


    static inline cam::Camera& get_camera(AppState const& state)
    {
        auto& data = get_data(state);

        assert(data.camera_id.value >= 0);

        return data.camera_list.list[data.camera_id.value];
    }


    static inline bool set_camera(AppState const& state, u32 id)
    {  
        auto& data = get_data(state);
        auto& cameras = data.camera_list;

        if (id >= cameras.count)
        {
            return false;
        }

        auto& camera = data.camera_list.list[id];

        if (!camera.is_open() && !cam::open_camera(camera))
        {
            return false;
        }

        data.camera_id = { (int)id };

        return true;
    }


    static void destroy_state_data(AppState& state)
    {
        if (!state.data_)
        {
            return;
        }

        auto& data = get_data(state);

        cam::close(data.camera_list);

        mem::free(state.data_);
    }


    static bool create_state_data(AppState& state)
    {
        auto data = mem::malloc<StateData>("StateData");
        if (!data)
        {
            return false;
        }

        state.data_ = data;

        state.data_->camera_id.value = 0;

        return true;
    }
}


namespace app
{
    AppResult init(AppState& state)
    {
        AppResult result{};
        result.success = false;

        if (!create_state_data(state))
        {
            return result;
        }

        auto& data = get_data(state);

        data.camera_list = cam::enumerate_cameras();

        if (!data.camera_list.count)
        {
            return result;
        }
        
        for (i32 i = data.camera_list.count - 1; i >= 0; i--)
        {
            set_camera(state, (u32)i);
        }

        if(data.camera_id.value < 0)
        {
            return result;
        }

        auto& camera = get_camera(state);

        result.screen_dimensions = {
            camera.frame_width,
            camera.frame_height
        };

        result.success = true;

        return result;
    }


    bool set_screen_memory(AppState& state, image::ImageView screen)
    {
        if (!screen.width || !screen.height || !screen.matrix_data_)
        {
            return false;
        }

        state.screen = screen;

        return true;
    }


    void update(AppState& state, input::Input const& input)
    {
        if (input.keyboard.kbd_1.pressed)
        {
            if (set_camera(state, 0))
            {
                cam::grab_image(get_camera(state), state.screen);
            }
            
        }
        else if (input.keyboard.kbd_2.pressed)
        {
            if (set_camera(state, 1))
            {
                cam::grab_image(get_camera(state), state.screen);
            }
        }
        else if (input.keyboard.kbd_3.pressed)
        {
            
        }
    }


    void close(AppState& state)
    {
        destroy_state_data(state);
    }
}

#include "../../../libs/usb/camera_uvc.cpp"
#include "../../../libs/alloc_type/alloc_type.cpp"
#include "../../../libs/image/image.cpp"
#include "../../../libs/span/span.cpp"