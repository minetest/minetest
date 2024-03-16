#ifndef SERVER

#include "clientdynamicinfo.h"

#include "settings.h"
#include "client/renderingengine.h"
#include "gui/touchscreengui.h"

ClientDynamicInfo ClientDynamicInfo::getCurrent()
{
    v2u32 screen_size = RenderingEngine::getWindowSize();
    f32 density = RenderingEngine::getDisplayDensity();
    f32 gui_scaling = g_settings->getFloat("gui_scaling", 0.5f, 20.0f);
    f32 hud_scaling = g_settings->getFloat("hud_scaling", 0.5f, 20.0f);
    f32 real_gui_scaling = gui_scaling * density;
    f32 real_hud_scaling = hud_scaling * density;
    bool touch_controls = g_touchscreengui;

    return {
        screen_size, real_gui_scaling, real_hud_scaling,
        ClientDynamicInfo::calculateMaxFSSize(screen_size, gui_scaling),
        touch_controls
    };
}

v2f32 ClientDynamicInfo::calculateMaxFSSize(v2u32 render_target_size, f32 gui_scaling)
{
    f32 factor = (g_settings->getBool("enable_touch") ? 10 : 15) / gui_scaling;
    f32 ratio = (f32)render_target_size.X / (f32)render_target_size.Y;
    if (ratio < 1)
        return { factor, factor / ratio };
    else
        return { factor * ratio, factor };
}

#endif
