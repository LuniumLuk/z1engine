#include "z1engine.h"

using namespace z1;

struct OurApp : Application {
	void init() override {};
};

int main() {
	OurApp app;
	app.init();

	Filepath cwd = std::filesystem::current_path();
	std::cout << "current working directory: " << cwd.generic_string() << std::endl;

	auto rg = RenderGraph();
	auto const& fb = g_runtime_context.m_graphics_context->m_swapchain_framebuffer;

	rg.add_pass("shadow-gen")
		.set_resolution(1024, 1024)
		.add_output("shadow-map", ImageFormat::Depth)
		.execute([](RenderGraphNode& node, GraphicsContext& ctx)
			{
				// ... do draw shadow map
			});

	rg.add_pass("gbuffer-gen")
		.set_resolution_as(fb)
		.add_output("gbuffer-a", ImageFormat::RGBA8)
		.add_output("gbuffer-b", ImageFormat::RGBA8)
		.add_output("gbuffer-c", ImageFormat::RGBA32F)
		.execute([](RenderGraphNode& node, GraphicsContext& ctx)
			{
				// ... do draw gbuffer
			});

	rg.add_pass("deferred-lighting")
		.set_resolution_as(fb)
		.add_input("gbuffer-a")
		.add_input("gbuffer-b")
		.add_input("gbuffer-c")
		.add_input("shadow-map")
		.add_output("lit-scene", ImageFormat::RGBA8)
		.execute([](RenderGraphNode& node, GraphicsContext& ctx)
			{
				// ... do deferred lighting

				uint32_t binding0 = node.bind_input_name("gbuffer-a");
				uint32_t binding1 = node.bind_input_index(1);

				// use the gbuffer-a image ...

				node.unbind_input_name("gbuffer-a");
				node.unbind_input_index(1);
			});

	rg.add_pass("post-processing")
		.add_input("lit-scene")
		.set_output(fb)
		.execute([](RenderGraphNode& node, GraphicsContext& ctx)
			{
				// ... do post processing
			});

	rg.compile();
	rg.execute();

	uint8_t data[100 * 100 * 4];
	fb->read_pixels(0, 0, 0, 100, 100, (void*)data);

	return 0;
}
