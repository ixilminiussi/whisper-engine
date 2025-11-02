#ifndef FROST_HPP
#define FROST_HPP

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>

#include <IconsMaterialSymbols.h>

#include <wsp_custom_imgui.hpp>
#include <wsp_custom_types.hpp>
#include <wsp_inputs.hpp>

#include <spdlog/spdlog.h>
#include <tuple>
#include <type_traits>

namespace frost
{

enum WhispUsage
{
    eClass,
    eEnum,
    eStruct
};

enum Edit
{
    eDrag,
    eSlider,
    eInput
};

template <typename T> wsp::dictionary<std::string, T> const &EnumDictionary();

template <typename T> struct Meta;
template <typename T> struct Meta<T *> : Meta<T>
{
};

template <typename Owner, typename T> struct Field
{
    char const *name;
    T Owner::*member;
    Edit edit;
    float min, max, step;
    char const *format;
};

template <typename Enum> inline bool RenderEnum(char const *label, Enum *address)
{
    wsp::dictionary<std::string, Enum> const &dict = EnumDictionary<Enum>();

    std::string currentSelection = dict.from(*address);

    bool modified = false;

    if (ImGui::BeginCombo(label, currentSelection.c_str()))
    {
        for (auto const &[name, val] : dict)
        {
            bool isSelected = *address == val;
            if (ImGui::Selectable(name.c_str(), isSelected))
            {
                currentSelection = name;
                *address = val;
                isSelected = true;
                modified = true;
            }
            if (isSelected)
            {
                ImGui::SetItemDefaultFocus();
            }
        }

        ImGui::EndCombo();
    }

    return modified;
}

template <typename T> inline bool RenderEditor(Meta<T> meta, T *owner)
{
    if constexpr (std::is_pointer_v<T>)
    {
        return RenderEditor<std::remove_pointer_t<T>>(Meta<std::remove_pointer_t<T>>{}, *owner);
    }

    if constexpr (meta.usage == WhispUsage::eClass)
    {
        bool modified = false;

        int i = 0;
        std::apply(
            [owner, &modified, &i](auto &&...args) {
                ((ImGui::PushID(i++), modified |= RenderField(args, owner), ImGui::PopID()), ...);
            },
            meta.fields);

        if (modified)
        {
            if constexpr (meta.hasRefresh)
            {
                (owner->*meta.refreshFunc)();
            }
        }

        return modified;
    }

    return false;
}

template <typename T> inline bool RenderNode(char const *label, T *address, Edit, float, float, float, char const *)
{
    std::string stringified = std::string(label);
    if constexpr (Meta<T>{}.usage == frost::WhispUsage::eEnum)
    {
        return RenderEnum<T>(label, address);
    }
    if (stringified.size() == 0)
    {
        return RenderEditor(Meta<T>{}, address);
    }
    if (stringified[0] == '#')
    {
        return RenderEditor(Meta<T>{}, address);
    }

    bool modified = false;
    if (wsp::TreeNode(label))
    {
        modified = RenderEditor(Meta<T>{}, address);
        ImGui::TreePop();
    }

    return modified;
}

template <>
inline bool RenderNode<float>(char const *label, float *address, Edit edit, float min, float max, float step,
                              char const *format)
{
    switch (edit)
    {
    case eInput:
        return ImGui::InputFloat(label, address, step, max, format);
        break;
    case eSlider:
        return ImGui::SliderFloat(label, address, min, max, format);
        break;
    case eDrag:
        return ImGui::DragFloat(label, address, step, min, max, format);
        break;
    }

    return false;
}

template <>
inline bool RenderNode<int>(char const *label, int *address, Edit edit, float min, float max, float step,
                            char const *format)
{
    switch (edit)
    {
    case eInput:
        return ImGui::InputInt(label, address, step, max);
        break;
    case eSlider:
        return ImGui::SliderInt(label, address, min, max);
        break;
    case eDrag:
        return ImGui::DragInt(label, address, step, min, max);
        break;
    }

    return false;
}

template <>
inline bool RenderNode<glm::vec3>(char const *label, glm::vec3 *address, Edit edit, float min, float max, float step,
                                  char const *format)
{
    switch (edit)
    {
    case eInput:
        return ImGui::InputFloat3(label, (float *)address, format);
        break;
    case eSlider:
        return ImGui::SliderFloat3(label, (float *)address, min, max, format);
        break;
    case eDrag:
        return ImGui::DragFloat3(label, (float *)address, step, min, max, format);
        break;
    }

    return false;
}

template <>
inline bool RenderNode<glm::vec2>(char const *label, glm::vec2 *address, Edit edit, float min, float max, float step,
                                  char const *format)
{
    switch (edit)
    {
    case eInput:
        return ImGui::InputFloat2(label, (float *)address, format);
        break;
    case eSlider:
        return ImGui::SliderFloat2(label, (float *)address, min, max, format);
        break;
    case eDrag:
        return ImGui::DragFloat2(label, (float *)address, step, min, max, format);
        break;
    }

    return false;
}

template <>
inline bool RenderNode<std::string>(char const *label, std::string *address, Edit edit, float, float, float,
                                    char const *)
{
    return ImGui::InputText(label, address);
}

template <>
inline bool RenderNode<char const *>(char const *label, char const **address, Edit edit, float, float, float,
                                     char const *)
{
    return ImGui::InputText(label, (std::string *)address);
}

template <>
inline bool RenderNode<wsp::Axis>(char const *label, wsp::Axis *address, Edit edit, float, float, float, char const *)
{
    auto findSelected = [&]() {
        int i = 0;

        for (auto &pair : wsp::WSP_AXIS_DICTIONARY)
        {
            if (*address == *pair.second)
            {
                return i;
            }
            i++;
        }

        return 0;
    };

    int const currentItem = findSelected();
    bool modified = false;

    ImGui::SetNextItemWidth(200);
    if (ImGui::BeginCombo(label, wsp::WSP_AXIS_DICTIONARY[currentItem].first))
    {
        for (std::pair<char const *, wsp::Axis *> const &pair : wsp::WSP_AXIS_DICTIONARY)
        {
            bool const isSelected = (*pair.second == *address);

            if (ImGui::Selectable(pair.first, isSelected))
            {
                address->code = pair.second->code;
                address->source = pair.second->source;
                address->id = pair.second->id;
                modified = true;
            }

            if (isSelected)
            {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }

    return modified;
}

template <>
inline bool RenderNode<wsp::Button>(char const *label, wsp::Button *address, Edit edit, float, float, float,
                                    char const *)
{
    auto findSelected = [&]() {
        int i = 0;

        for (std::pair<char const *, wsp::Button *> const &pair : wsp::WSP_BUTTON_DICTIONARY)
        {
            if (*address == *pair.second)
            {
                return i;
            }
            i++;
        }

        return 0;
    };

    bool modified = false;

    ImGui::SetNextItemWidth(200);
    if (ImGui::BeginCombo(label, wsp::WSP_BUTTON_DICTIONARY[findSelected()].first))
    {
        for (std::pair<char const *, wsp::Button *> const &pair : wsp::WSP_BUTTON_DICTIONARY)
        {
            bool const isSelected = (*pair.second == *address);

            if (ImGui::Selectable(pair.first, isSelected))
            {
                address->code = pair.second->code;
                address->source = pair.second->source;
                address->id = pair.second->id;
                modified = true;
            }

            if (isSelected)
            {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }

    return modified;
}

template <typename First, typename Second>
inline bool RenderNode(char const *label, std::pair<First, Second> *address, Edit edit, float min, float max,
                       float step, char const *format)
{
    bool modified = false;

    if (ImGui::BeginTable(label, 3, ImGuiTableFlags_SizingFixedFit))
    {
        ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_None);
        ImGui::TableSetupColumn("First", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Second", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::LabelText(label, "");
        ImGui::TableSetColumnIndex(1);
        modified |= RenderNode("##First", &(address->first), edit, min, max, step, format);
        ImGui::TableSetColumnIndex(2);
        modified |= RenderNode("##Second", &(address->second), edit, min, max, step, format);

        ImGui::EndTable();
    }

    return modified;
}

template <typename Element>
inline bool RenderNode(char const *label, std::vector<Element> *address, Edit edit, float min, float max, float step,
                       char const *format)
{
    int i = 0;

    bool modified = false;

    ImGui::Text(label);

    std::vector<Element> &list = *address;

    if (ImGui::BeginTable(label, 3, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_Borders))
    {
        ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_None);
        ImGui::TableSetupColumn("Item", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("   ", ImGuiTableColumnFlags_None);
        ImGui::TableHeadersRow();

        auto it = list.begin();

        while (it != list.end())
        {
            ImGui::PushID(i);

            ImGui::TableNextRow();

            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%i", i);

            ImGui::TableSetColumnIndex(1);
            RenderNode(fmt::format("##{}", i).c_str(), &*it, edit, min, max, step, format);

            ImGui::TableSetColumnIndex(2);
            if (wsp::RedButton(ICON_MS_DELETE))
            {
                it = list.erase(it);
                modified = true;
            }
            else
            {
                it++;
            }

            i++;

            ImGui::PopID();
        }

        ImGui::EndTable();
    }

    if (wsp::GreenButton(ICON_MS_ADD))
    {
        check(std::is_default_constructible_v<Element> &&
              "Frost: FPROPERTY map elements MUST be default constructible");
        list.emplace_back();
        modified = true;
    }

    return modified;
}

template <typename T, size_t N>
inline bool RenderNode(char const *label, std::array<T, N> *address, Edit edit, float min, float max, float step,
                       char const *format)
{
    bool modified = false;

    ImGui::Text(label);

    int i = 0;
    for (T &element : *address)
    {
        modified |= RenderNode(std::to_string(i).c_str(), &element, edit, min, max, step, format);
        i++;
    }

    return modified;
}

template <typename T, typename N>
inline bool RenderNode(char const *label, wsp::dictionary<T, N> *address, Edit edit, float min, float max, float step,
                       char const *format)
{
    return RenderNode(label, &address->data, edit, min, max, step, format);
}

template <typename Owner, typename T> inline bool RenderField(Field<Owner, T> field, Owner *owner)
{
    T &var = owner->*(field.member);
    return RenderNode(field.name, &var, field.edit, field.min, field.max, field.step, field.format);
}

template <typename Owner, typename T> bool RenderField(Field<Owner, T> field, Owner **owner)
{
    return RenderField(field, *owner);
}

} // namespace frost

#endif
