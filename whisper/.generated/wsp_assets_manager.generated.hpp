#ifndef WFROST_GENERATED_wsp_assets_manager
#define WFROST_GENERATED_wsp_assets_manager

#include <frost.hpp>
#include <wsp_devkit.hpp>

 
#define WCLASS_BODY$AssetsManager() friend struct frost::Meta<wsp::AssetsManager>;

#undef WGENERATED_META_DATA
#define WGENERATED_META_DATA() \
using namespace wsp;template <> struct frost::Meta<wsp::AssetsManager> { static constexpr char const * name = "Assets Manager"; static constexpr frost::WhispUsage usage =frost::WhispUsage::eClass; using Type =wsp::AssetsManager;static constexpr auto fields = std::make_tuple();static constexpr bool hasRefresh = false;};

#endif
