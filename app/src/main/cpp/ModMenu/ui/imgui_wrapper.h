#pragma once
#include <imgui.h>
#include <backends/imgui_impl_opengl3.h>

namespace ui {
class ImGuiWrapper {
public:
    ImGuiWrapper(ImVec2 display_size);
    virtual ~ImGuiWrapper();

    virtual bool init();
    virtual void render();

    // The viewport is re-read every frame so rotation and window resizes are
    // picked up without re-initializing the context.
    void set_display_size(ImVec2 display_size);

protected:
    virtual void draw() = 0;

    ImVec2 m_display_size;
    ImVec2 m_display_scale;

private:
    double m_last_frame_time;
};
} // ui
