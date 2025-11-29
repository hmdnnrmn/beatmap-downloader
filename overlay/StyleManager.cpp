#include "StyleManager.h"

StyleManager& StyleManager::Instance() {
    static StyleManager instance;
    return instance;
}

void StyleManager::ApplyTheme() {
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    // Base Colors
    ImVec4 color_text = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    ImVec4 color_text_disabled = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    ImVec4 color_window_bg = ImVec4(0.10f, 0.10f, 0.12f, 0.95f); // Dark Blue-Grey
    ImVec4 color_child_bg = ImVec4(0.15f, 0.15f, 0.18f, 1.00f);
    ImVec4 color_popup_bg = ImVec4(0.10f, 0.10f, 0.12f, 0.95f);
    ImVec4 color_border = ImVec4(0.25f, 0.25f, 0.30f, 0.50f);
    ImVec4 color_border_shadow = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    ImVec4 color_frame_bg = ImVec4(0.20f, 0.20f, 0.24f, 1.00f);
    ImVec4 color_frame_bg_hovered = ImVec4(0.25f, 0.25f, 0.30f, 1.00f);
    ImVec4 color_frame_bg_active = ImVec4(0.30f, 0.30f, 0.36f, 1.00f);
    ImVec4 color_title_bg = ImVec4(0.10f, 0.10f, 0.12f, 1.00f);
    ImVec4 color_title_bg_active = ImVec4(0.10f, 0.10f, 0.12f, 1.00f);
    ImVec4 color_title_bg_collapsed = ImVec4(0.10f, 0.10f, 0.12f, 0.51f);
    ImVec4 color_menubar_bg = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
    ImVec4 color_scrollbar_bg = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
    ImVec4 color_scrollbar_grab = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
    ImVec4 color_scrollbar_grab_hovered = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
    ImVec4 color_scrollbar_grab_active = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
    ImVec4 color_check_mark = ImVec4(1.00f, 0.40f, 0.67f, 1.00f); // osu! Pink
    ImVec4 color_slider_grab = ImVec4(1.00f, 0.40f, 0.67f, 1.00f); // osu! Pink
    ImVec4 color_slider_grab_active = ImVec4(1.00f, 0.60f, 0.80f, 1.00f);
    ImVec4 color_button = ImVec4(0.20f, 0.20f, 0.24f, 1.00f);
    ImVec4 color_button_hovered = ImVec4(1.00f, 0.40f, 0.67f, 0.50f); // Pink tint
    ImVec4 color_button_active = ImVec4(1.00f, 0.40f, 0.67f, 0.80f);
    ImVec4 color_header = ImVec4(1.00f, 0.40f, 0.67f, 0.31f);
    ImVec4 color_header_hovered = ImVec4(1.00f, 0.40f, 0.67f, 0.60f);
    ImVec4 color_header_active = ImVec4(1.00f, 0.40f, 0.67f, 0.80f);
    ImVec4 color_separator = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
    ImVec4 color_separator_hovered = ImVec4(1.00f, 0.40f, 0.67f, 0.78f);
    ImVec4 color_separator_active = ImVec4(1.00f, 0.40f, 0.67f, 1.00f);
    ImVec4 color_resize_grip = ImVec4(1.00f, 0.40f, 0.67f, 0.20f);
    ImVec4 color_resize_grip_hovered = ImVec4(1.00f, 0.40f, 0.67f, 0.67f);
    ImVec4 color_resize_grip_active = ImVec4(1.00f, 0.40f, 0.67f, 0.95f);
    ImVec4 color_tab = ImVec4(0.15f, 0.15f, 0.18f, 0.86f);
    ImVec4 color_tab_hovered = ImVec4(1.00f, 0.40f, 0.67f, 0.60f);
    ImVec4 color_tab_active = ImVec4(1.00f, 0.40f, 0.67f, 0.80f);
    ImVec4 color_tab_unfocused = ImVec4(0.10f, 0.10f, 0.12f, 0.97f);
    ImVec4 color_tab_unfocused_active = ImVec4(0.15f, 0.15f, 0.18f, 1.00f);
    ImVec4 color_plot_lines = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
    ImVec4 color_plot_lines_hovered = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
    ImVec4 color_plot_histogram = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    ImVec4 color_plot_histogram_hovered = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
    ImVec4 color_text_selected_bg = ImVec4(1.00f, 0.40f, 0.67f, 0.35f);
    ImVec4 color_drag_drop_target = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
    ImVec4 color_nav_highlight = ImVec4(1.00f, 0.40f, 0.67f, 1.00f);
    ImVec4 color_nav_windowing_highlight = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    ImVec4 color_nav_windowing_dim_bg = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
    ImVec4 color_modal_window_dim_bg = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);

    colors[ImGuiCol_Text] = color_text;
    colors[ImGuiCol_TextDisabled] = color_text_disabled;
    colors[ImGuiCol_WindowBg] = color_window_bg;
    colors[ImGuiCol_ChildBg] = color_child_bg;
    colors[ImGuiCol_PopupBg] = color_popup_bg;
    colors[ImGuiCol_Border] = color_border;
    colors[ImGuiCol_BorderShadow] = color_border_shadow;
    colors[ImGuiCol_FrameBg] = color_frame_bg;
    colors[ImGuiCol_FrameBgHovered] = color_frame_bg_hovered;
    colors[ImGuiCol_FrameBgActive] = color_frame_bg_active;
    colors[ImGuiCol_TitleBg] = color_title_bg;
    colors[ImGuiCol_TitleBgActive] = color_title_bg_active;
    colors[ImGuiCol_TitleBgCollapsed] = color_title_bg_collapsed;
    colors[ImGuiCol_MenuBarBg] = color_menubar_bg;
    colors[ImGuiCol_ScrollbarBg] = color_scrollbar_bg;
    colors[ImGuiCol_ScrollbarGrab] = color_scrollbar_grab;
    colors[ImGuiCol_ScrollbarGrabHovered] = color_scrollbar_grab_hovered;
    colors[ImGuiCol_ScrollbarGrabActive] = color_scrollbar_grab_active;
    colors[ImGuiCol_CheckMark] = color_check_mark;
    colors[ImGuiCol_SliderGrab] = color_slider_grab;
    colors[ImGuiCol_SliderGrabActive] = color_slider_grab_active;
    colors[ImGuiCol_Button] = color_button;
    colors[ImGuiCol_ButtonHovered] = color_button_hovered;
    colors[ImGuiCol_ButtonActive] = color_button_active;
    colors[ImGuiCol_Header] = color_header;
    colors[ImGuiCol_HeaderHovered] = color_header_hovered;
    colors[ImGuiCol_HeaderActive] = color_header_active;
    colors[ImGuiCol_Separator] = color_separator;
    colors[ImGuiCol_SeparatorHovered] = color_separator_hovered;
    colors[ImGuiCol_SeparatorActive] = color_separator_active;
    colors[ImGuiCol_ResizeGrip] = color_resize_grip;
    colors[ImGuiCol_ResizeGripHovered] = color_resize_grip_hovered;
    colors[ImGuiCol_ResizeGripActive] = color_resize_grip_active;
    colors[ImGuiCol_Tab] = color_tab;
    colors[ImGuiCol_TabHovered] = color_tab_hovered;
    colors[ImGuiCol_TabActive] = color_tab_active;
    colors[ImGuiCol_TabUnfocused] = color_tab_unfocused;
    colors[ImGuiCol_TabUnfocusedActive] = color_tab_unfocused_active;
    colors[ImGuiCol_PlotLines] = color_plot_lines;
    colors[ImGuiCol_PlotLinesHovered] = color_plot_lines_hovered;
    colors[ImGuiCol_PlotHistogram] = color_plot_histogram;
    colors[ImGuiCol_PlotHistogramHovered] = color_plot_histogram_hovered;
    colors[ImGuiCol_TextSelectedBg] = color_text_selected_bg;
    colors[ImGuiCol_DragDropTarget] = color_drag_drop_target;
    colors[ImGuiCol_NavHighlight] = color_nav_highlight;
    colors[ImGuiCol_NavWindowingHighlight] = color_nav_windowing_highlight;
    colors[ImGuiCol_NavWindowingDimBg] = color_nav_windowing_dim_bg;
    colors[ImGuiCol_ModalWindowDimBg] = color_modal_window_dim_bg;

    // Style Vars
    style.WindowRounding = 8.0f;
    style.ChildRounding = 6.0f;
    style.FrameRounding = 4.0f;
    style.PopupRounding = 6.0f;
    style.ScrollbarRounding = 4.0f;
    style.GrabRounding = 4.0f;
    style.TabRounding = 6.0f;
    
    style.WindowPadding = ImVec2(12.0f, 12.0f);
    style.FramePadding = ImVec2(8.0f, 4.0f);
    style.ItemSpacing = ImVec2(8.0f, 6.0f);
    style.ItemInnerSpacing = ImVec2(6.0f, 6.0f);
    style.IndentSpacing = 20.0f;
    style.ScrollbarSize = 14.0f;
    style.GrabMinSize = 12.0f;
}

#include "fonts/UbuntuMono_R_ttf.h"

void StyleManager::LoadFonts() {
    ImGuiIO& io = ImGui::GetIO();
    ImFontConfig config;
    config.FontDataOwnedByAtlas = false;
    io.Fonts->AddFontFromMemoryTTF(UbuntuMono_R_ttf, sizeof(UbuntuMono_R_ttf), 18.0f, &config, io.Fonts->GetGlyphRangesChineseFull());
}




