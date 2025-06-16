#include <wsp_engine.h>
#include <wsp_render_graph.h>
#include <wsp_renderer.h>

// lib
#include <spdlog/spdlog.h>

int main()
{
    // wsp::RenderGraph renderGraph{};

    // renderGraph.DeclareResource("albedo", {});
    // renderGraph.DeclareResource("normal", {});
    // renderGraph.DeclareResource("depth", {});
    // renderGraph.DeclareResource("shadow", {});
    // renderGraph.DeclareResource("composite", {});
    // renderGraph.DeclareResource("editor", {});
    // renderGraph.DeclareResource("framebuffer", {});

    // renderGraph.AddPass("gbuffer", [](wsp::RGBuilder &b) {
    //     auto albedo = b.Write("albedo");
    //     auto normal = b.Write("normal");
    //     auto depth = b.Write("depth");
    //     auto shadow = b.Write("shadow");

    //     b.SetExecute([=](wsp::RGContext &ctx) {
    //         spdlog::info("GBuffer writes to {}", ctx.GetImageView(albedo)->debugName);
    //         spdlog::info("GBuffer writes to {}", ctx.GetImageView(normal)->debugName);
    //         spdlog::info("GBuffer writes to {}", ctx.GetImageView(depth)->debugName);
    //         spdlog::info("GBuffer writes to {}", ctx.GetImageView(shadow)->debugName);
    //     });
    // });

    // renderGraph.AddPass("composite", [](wsp::RGBuilder &b) {
    //     auto albedo = b.Read("albedo");
    //     auto normal = b.Read("normal");
    //     auto depth = b.Read("depth");
    //     auto shadow = b.Read("shadow");
    //     auto composite = b.Write("composite");

    //     b.SetExecute([=](wsp::RGContext &ctx) {
    //         spdlog::info("GBuffer reads from {}", ctx.GetImageView(albedo)->debugName);
    //         spdlog::info("GBuffer reads from {}", ctx.GetImageView(normal)->debugName);
    //         spdlog::info("GBuffer reads from {}", ctx.GetImageView(depth)->debugName);
    //         spdlog::info("GBuffer reads from {}", ctx.GetImageView(shadow)->debugName);
    //         spdlog::info("GBuffer writes to {}", ctx.GetImageView(composite)->debugName);
    //     });
    // });

    // renderGraph.AddPass("editor", [](wsp::RGBuilder &b) {
    //     auto albedo = b.Read("albedo");
    //     auto normal = b.Read("normal");
    //     auto depth = b.Read("depth");
    //     auto shadow = b.Read("shadow");
    //     auto composite = b.Read("composite");
    //     auto editor = b.Write("editor");

    //     b.SetExecute([=](wsp::RGContext &ctx) {
    //         spdlog::info("GBuffer reads from {}", ctx.GetImageView(albedo)->debugName);
    //         spdlog::info("GBuffer reads from {}", ctx.GetImageView(normal)->debugName);
    //         spdlog::info("GBuffer reads from {}", ctx.GetImageView(depth)->debugName);
    //         spdlog::info("GBuffer reads from {}", ctx.GetImageView(shadow)->debugName);
    //         spdlog::info("GBuffer reads from {}", ctx.GetImageView(composite)->debugName);
    //         spdlog::info("GBuffer writes to {}", ctx.GetImageView(editor)->debugName);
    //     });
    // });

    // renderGraph.AddPass("framebuffer", [](wsp::RGBuilder &b) {
    //     auto editor = b.Read("editor");
    //     auto framebuffer = b.Write("framebuffer");

    //     b.SetExecute([=](wsp::RGContext &ctx) {
    //         spdlog::info("GBuffer reads from {}", ctx.GetImageView(editor)->debugName);
    //         spdlog::info("GBuffer writes to {}", ctx.GetImageView(framebuffer)->debugName);
    //     });
    // });

    // renderGraph.Compile();
    // renderGraph.Execute();
    wsp::Engine::Kickstart();

    wsp::Engine::Get()->Run();

    wsp::Engine::Shutdown();
}
