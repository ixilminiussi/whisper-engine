#ifndef WFROST_GENERATED_wsp_node
#define WFROST_GENERATED_wsp_node

#include <frost.hpp>
#include <wsp_devkit.hpp>

 
#define WCLASS_BODY$Node() friend struct frost::Meta<wsp::Node>;

#undef WGENERATED_META_DATA
#define WGENERATED_META_DATA() \
using namespace wsp;template <> struct frost::Meta<wsp::Node> { static constexpr char const * name = "Node"; static constexpr frost::WhispUsage usage =frost::WhispUsage::eClass; using Type =wsp::Node;static constexpr auto fields = std::make_tuple(frost::Field<Type,Transform>{"Transform", &Type::_transform,Edit::eInput,0.f,0.f,.01,"%.3f"},frost::Field<Type,std::vector<class Node *>>{"Children", &Type::_children,Edit::eInput,0.f,0.f,.01,"%.3f"});static constexpr bool hasRefresh = false;};

#endif
