#ifndef FROST_HPP
#define FROST_HPP

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include <imgui.h>

#include <spdlog/spdlog.h>
#include <stdexcept>
#include <tuple>
#include <vector>

namespace frost
{

enum Edit
{
    eDrag,
    eSlider,
    eInput
};

template <typename T> struct Meta;

template <typename Owner, typename T> struct Field
{
    char const *name;
    T Owner::*member;
    Edit edit;
    float min, max, step;
    char const *format;
};

template <typename T> inline bool RenderEditor(Meta<T> meta, T *owner)
{
    bool modified{false};

    std::apply([owner, &modified](auto &&...args) { ((modified |= (RenderField(args, owner))), ...); }, meta.fields);

    if (modified && meta.hasRefresh)
    {
        (owner->*meta.refreshFunc)();
    }

    return modified;
}

template <typename Owner, typename T> inline bool RenderField(Field<Owner, T> field, Owner *owner)
{
    bool modified = false;
    if (ImGui::TreeNode(field.name))
    {
        T &var = owner->*(field.member);
        modified = RenderEditor(Meta<T>{}, &var);
        ImGui::TreePop();
    }

    return modified;
}

template <typename Owner> inline bool RenderField(Field<Owner, float> field, Owner *owner)
{
    float &var = owner->*(field.member);

    switch (field.edit)
    {
    case eInput:
        return ImGui::InputFloat(field.name, &var, field.step, field.max, field.format);
        break;
    case eSlider:
        return ImGui::SliderFloat(field.name, &var, field.min, field.max, field.format);
        break;
    case eDrag:
        return ImGui::DragFloat(field.name, &var, field.step, field.min, field.max, field.format);
        break;
    }

    return false;
}

template <typename Owner> inline bool RenderField(Field<Owner, int> field, Owner *owner)
{
    int &var = owner->*(field.member);

    switch (field.edit)
    {
    case eInput:
        return ImGui::InputInt(field.name, &var, field.step, field.max);
        break;
    case eSlider:
        return ImGui::SliderInt(field.name, &var, field.min, field.max);
        break;
    case eDrag:
        return ImGui::DragInt(field.name, &var, field.step, field.min, field.max);
        break;
    }

    return false;
}

template <typename Owner> inline bool RenderField(Field<Owner, glm::vec3> field, Owner *owner)
{
    glm::vec3 &var = owner->*(field.member);

    switch (field.edit)
    {
    case eInput:
        return ImGui::InputFloat3(field.name, (float *)&var, field.format);
        break;
    case eSlider:
        return ImGui::SliderFloat3(field.name, (float *)&var, field.min, field.max, field.format);
        break;
    case eDrag:
        return ImGui::DragFloat3(field.name, (float *)&var, field.step, field.min, field.max, field.format);
        break;
    }

    return false;
}

template <typename Owner> inline bool RenderField(Field<Owner, glm::vec2> field, Owner *owner)
{
    glm::vec2 &var = owner->*(field.member);

    switch (field.edit)
    {
    case eInput:
        return ImGui::InputFloat2(field.name, (float *)&var, field.format);
        break;
    case eSlider:
        return ImGui::SliderFloat2(field.name, (float *)&var, field.min, field.max, field.format);
        break;
    case eDrag:
        return ImGui::DragFloat2(field.name, (float *)&var, field.step, field.min, field.max, field.format);
        break;
    }

    return false;
}

} // namespace frost

#endif
