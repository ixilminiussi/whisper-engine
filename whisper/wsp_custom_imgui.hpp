#ifndef WSP_CUSTOM_IMGUI
#define WSP_CUSTOM_IMGUI
#ifndef NDEBUG

#include <imgui.h>
#include <wsp_static_utils.hpp>

namespace wsp
{

namespace dracula
{
/*
ImVec4 const background{30.f / 255.f, 30.f / 255.f, 40.f / 255.f, 1.f};
ImVec4 const currentLine{rgb(108, 112, 134)};
ImVec4 const foreground{rgb(205, 214, 244)};
ImVec4 const comment{0.384f, 0.447f, 0.643f, 1.f};
ImVec4 const cyan{0.545f, 0.914f, 0.992f, 1.f};
ImVec4 const green{0.314f, 0.98f, 0.482f, 1.f};
ImVec4 const orange{1.f, 0.722f, 0.424f, 1.f};
ImVec4 const pink{1.f, 0.475f, 0.776f, 1.f};
ImVec4 const purple{0.741f, 0.576f, 0.976f, 1.f};
ImVec4 const red{1.f, 0.333f, 0.333f, 1.f};
ImVec4 const yellow{0.945f, 0.98f, 0.549f, 1.f};
ImVec4 const transparent{0.f, 0.f, 0.f, 0.f};
ImVec4 const comment_highlight{0.53f, 0.58f, 0.74f, 1.f};
*/
}

namespace catppuccin
{
static ImVec4 const rosewater{245.f / 255.f, 224.f / 255.f, 220 / 255.f, 1.f};
static ImVec4 const flamingo{242.f / 255.f, 205.f / 255.f, 205 / 255.f, 1.f};
static ImVec4 const pink{245.f / 255.f, 194.f / 255.f, 231 / 255.f, 1.f};
static ImVec4 const mauve{203.f / 255.f, 166.f / 255.f, 247 / 255.f, 1.f};
static ImVec4 const red{243.f / 255.f, 139.f / 255.f, 168 / 255.f, 1.f};
static ImVec4 const maroon{235.f / 255.f, 160.f / 255.f, 172 / 255.f, 1.f};
static ImVec4 const peach{250.f / 255.f, 179.f / 255.f, 135 / 255.f, 1.f};
static ImVec4 const yellow{249.f / 255.f, 226.f / 255.f, 175 / 255.f, 1.f};
static ImVec4 const green{166.f / 255.f, 227.f / 255.f, 161 / 255.f, 1.f};
static ImVec4 const teal{148.f / 255.f, 226.f / 255.f, 213 / 255.f, 1.f};
static ImVec4 const sky{137.f / 255.f, 220.f / 255.f, 235 / 255.f, 1.f};
static ImVec4 const sapphire{116.f / 255.f, 199.f / 255.f, 236 / 255.f, 1.f};
static ImVec4 const blue{137.f / 255.f, 180.f / 255.f, 250 / 255.f, 1.f};
static ImVec4 const lavender{180.f / 255.f, 190.f / 255.f, 254 / 255.f, 1.f};
static ImVec4 const text{205.f / 255.f, 214.f / 255.f, 244 / 255.f, 1.f};
static ImVec4 const subtext1{186.f / 255.f, 194.f / 255.f, 222 / 255.f, 1.f};
static ImVec4 const subtext0{166.f / 255.f, 173.f / 255.f, 200 / 255.f, 1.f};
static ImVec4 const overlay2{147.f / 255.f, 153.f / 255.f, 178 / 255.f, 1.f};
static ImVec4 const overlay1{127.f / 255.f, 132.f / 255.f, 156 / 255.f, 1.f};
static ImVec4 const overlay0{108.f / 255.f, 112.f / 255.f, 134 / 255.f, 1.f};
static ImVec4 const surface0{88.f / 255.f, 91 / 255.f, 112 / 255.f, 1.f};
static ImVec4 const surface1{69.f / 255.f, 71 / 255.f, 90 / 255.f, 1.f};
static ImVec4 const surface2{49.f / 255.f, 50 / 255.f, 68 / 255.f, 1.f};
static ImVec4 const base{30.f / 255.f, 30 / 255.f, 46 / 255.f, 1.f};
static ImVec4 const mantle{24.f / 255.f, 24 / 255.f, 37 / 255.f, 1.f};
static ImVec4 const crust{17.f / 255.f, 17 / 255.f, 27 / 255.f, 1.f};
static ImVec4 const transparent{0.f, 0.f, 0.f, 0.f};
} // namespace catppuccin

inline void ApplyImGuiTheme()
{
    ImGui::StyleColorsDark();

    ImGuiStyle &style = ImGui::GetStyle();
    ImVec4 *colors = style.Colors;
    style.Alpha = 1.0f;

    style.FrameRounding = 2;

    using namespace catppuccin;

    colors[ImGuiCol_Text] = text;
    colors[ImGuiCol_TextDisabled] = subtext0;
    colors[ImGuiCol_WindowBg] = base;
    colors[ImGuiCol_ChildBg] = mantle;
    colors[ImGuiCol_PopupBg] = mantle;
    colors[ImGuiCol_Border] = crust;
    colors[ImGuiCol_BorderShadow] = transparent;
    colors[ImGuiCol_FrameBg] = surface2;
    colors[ImGuiCol_FrameBgHovered] = surface1;
    colors[ImGuiCol_FrameBgActive] = surface0;
    colors[ImGuiCol_TitleBg] = crust;
    colors[ImGuiCol_TitleBgActive] = crust;
    colors[ImGuiCol_TitleBgCollapsed] = crust;
    colors[ImGuiCol_MenuBarBg] = mantle;
    colors[ImGuiCol_ScrollbarBg] = surface2;
    colors[ImGuiCol_ScrollbarGrab] = teal;
    colors[ImGuiCol_ScrollbarGrabActive] = sky;
    colors[ImGuiCol_ScrollbarGrabHovered] = sky;
    colors[ImGuiCol_CheckMark] = green;
    colors[ImGuiCol_SliderGrab] = red;
    colors[ImGuiCol_SliderGrabActive] = maroon;
    colors[ImGuiCol_Button] = overlay0;
    colors[ImGuiCol_ButtonHovered] = overlay1;
    colors[ImGuiCol_ButtonActive] = overlay2;
    colors[ImGuiCol_Header] = overlay0;
    colors[ImGuiCol_HeaderHovered] = overlay1;
    colors[ImGuiCol_HeaderActive] = overlay2;
    colors[ImGuiCol_Separator] = overlay0;
    colors[ImGuiCol_SeparatorHovered] = overlay1;
    colors[ImGuiCol_SeparatorActive] = overlay2;
    colors[ImGuiCol_ResizeGrip] = lavender;
    colors[ImGuiCol_ResizeGripActive] = lavender;
    colors[ImGuiCol_ResizeGripHovered] = lavender;
    colors[ImGuiCol_PlotLines] = yellow;
    colors[ImGuiCol_PlotLinesHovered] = yellow;
    colors[ImGuiCol_PlotHistogram] = yellow;
    colors[ImGuiCol_PlotHistogramHovered] = yellow;
    colors[ImGuiCol_TableBorderLight] = overlay0;
    colors[ImGuiCol_TableBorderStrong] = overlay1;
    colors[ImGuiCol_TableHeaderBg] = overlay0;
    colors[ImGuiCol_TableRowBg] = surface0;
    colors[ImGuiCol_TableRowBgAlt] = surface1;
    colors[ImGuiCol_TextLink] = teal;
    colors[ImGuiCol_TextSelectedBg] = overlay0;
    colors[ImGuiCol_DragDropTarget] = yellow;
    colors[ImGuiCol_NavCursor] = yellow;
    colors[ImGuiCol_DockingEmptyBg] = flamingo;
    colors[ImGuiCol_DockingPreview] = pink;
    colors[ImGuiCol_ModalWindowDimBg] = yellow;
    colors[ImGuiCol_TabActive] = base;
    colors[ImGuiCol_TabUnfocused] = mantle;
    colors[ImGuiCol_TabUnfocusedActive] = base;
    colors[ImGuiCol_Tab] = base;
    colors[ImGuiCol_TabHovered] = surface0;
    colors[ImGuiCol_TabSelected] = base;
    colors[ImGuiCol_TabSelectedOverline] = base;
    colors[ImGuiCol_TabDimmed] = mantle;
    colors[ImGuiCol_TabDimmedSelected] = base;
    colors[ImGuiCol_TabDimmedSelectedOverline] = base;

    colors[ImGuiCol_NavHighlight] = yellow;
    colors[ImGuiCol_NavWindowingDimBg] = yellow;
    colors[ImGuiCol_NavWindowingHighlight] = yellow;

    for (int i = 0; i < ImGuiCol_COUNT; ++i)
    {
        style.Colors[i] = decodeSRGB(style.Colors[i]);
    }
}

inline bool TreeNode(char const *label)
{
    ImGui::PushStyleColor(ImGuiCol_Text, decodeSRGB(catppuccin::mauve));
    bool const r = ImGui::TreeNode(label);
    ImGui::PopStyleColor(1);

    return r;
}

inline bool ThumbnailButton(char const *label)
{
    static float const padding = 16.f;
    static float const thumbnailSize = 128.f;

    static ImGuiIO &io = ImGui::GetIO();

    static ImFont *Font = io.Fonts->AddFontFromFileTTF(
        (std::string(WSP_EDITOR_ASSETS) + std::string("MaterialIcons-Regular.ttf")).c_str(),
        thumbnailSize - padding * 2.);

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(padding, padding));
    ImGui::PushFont(Font);

    ImGui::PushStyleColor(ImGuiCol_Text, decodeSRGB(catppuccin::base));
    ImGui::PushStyleColor(ImGuiCol_Button, decodeSRGB(catppuccin::blue));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, decodeSRGB(catppuccin::lavender));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, decodeSRGB(catppuccin::lavender));
    bool const r = ImGui::Button(label, ImVec2{thumbnailSize, thumbnailSize});
    ImGui::PopStyleColor(4);
    ImGui::PopStyleVar(1);
    ImGui::PopFont();

    return r;
}

inline bool ThumbnailButton(ImTextureID textureID)
{
    static float const padding = 16.f;
    static float const thumbnailSize = 128.f;

    ImGui::PushStyleColor(ImGuiCol_Button, decodeSRGB(catppuccin::blue));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, decodeSRGB(catppuccin::lavender));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, decodeSRGB(catppuccin::lavender));
    bool const r = ImGui::ImageButton("", textureID, ImVec2{thumbnailSize, thumbnailSize});
    ImGui::PopStyleColor(3);

    return r;
}

inline bool RedButton(char const *label, ImVec2 const &size = ImVec2{0.f, 0.f})
{
    ImGui::PushStyleColor(ImGuiCol_Text, decodeSRGB(catppuccin::base));
    ImGui::PushStyleColor(ImGuiCol_Button, decodeSRGB(catppuccin::red));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, decodeSRGB(catppuccin::maroon));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, decodeSRGB(catppuccin::maroon));
    bool const r = ImGui::Button(label, size);
    ImGui::PopStyleColor(4);
    return r;
}

inline bool GreenButton(char const *label, ImVec2 const &size = ImVec2{0.f, 0.f})
{
    ImGui::PushStyleColor(ImGuiCol_Text, decodeSRGB(catppuccin::base));
    ImGui::PushStyleColor(ImGuiCol_Button, decodeSRGB(catppuccin::green));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, decodeSRGB(catppuccin::teal));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, decodeSRGB(catppuccin::teal));
    bool const r = ImGui::Button(label, size);
    ImGui::PopStyleColor(4);
    return r;
}

inline bool VanillaButton(char const *label, ImVec2 const &size = ImVec2{0.f, 0.f})
{
    ImGui::PushStyleColor(ImGuiCol_Text, decodeSRGB(catppuccin::base));
    ImGui::PushStyleColor(ImGuiCol_Button, decodeSRGB(catppuccin::blue));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, decodeSRGB(catppuccin::lavender));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, decodeSRGB(catppuccin::lavender));
    bool const r = ImGui::Button(label, size);
    ImGui::PopStyleColor(4);
    return r;
}

} // namespace wsp

#endif
#endif
