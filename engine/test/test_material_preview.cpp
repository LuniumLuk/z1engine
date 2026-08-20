#include "z1engine.h"

using namespace z1;

struct OurApp : Application {
	void init() override {};
};

static std::shared_ptr<StaticMesh> make_sphere() {
	constexpr int stacks = 24;
	constexpr int slices = 32;
	constexpr float pi = 3.14159265359f;

	std::vector<StaticMesh::VertexData> vertices;
	std::vector<uint32_t> indices;
	vertices.reserve((stacks + 1) * (slices + 1));
	indices.reserve(stacks * slices * 6);

	for (int i = 0; i <= stacks; ++i) {
		float phi = pi * i / stacks;
		float v = (float)i / stacks;
		for (int j = 0; j <= slices; ++j) {
			float theta = 2.0f * pi * j / slices;
			float u = (float)j / slices;

			glm::vec3 p = {
				std::sin(phi) * std::cos(theta),
				std::cos(phi),
				std::sin(phi) * std::sin(theta)
			};
			glm::vec3 tangent = glm::normalize(glm::vec3{ std::cos(theta), 0.0f, -std::sin(theta) });

			StaticMesh::VertexData vd;
			vd.position = p;
			vd.normal = p;
			vd.texcoord0 = { u, v };
			vd.tangent = glm::vec4{ tangent, 1.0f };
			vertices.push_back(vd);
		}
	}

	for (int i = 0; i < stacks; ++i) {
		for (int j = 0; j < slices; ++j) {
			uint32_t a = i * (slices + 1) + j;
			uint32_t b = a + slices + 1;
			indices.push_back(a);
			indices.push_back(a + 1);
			indices.push_back(b);
			indices.push_back(b);
			indices.push_back(a + 1);
			indices.push_back(b + 1);
		}
	}

	return std::make_shared<StaticMesh>(vertices, indices, PrimitiveType::Triangles);
}

int main() {
	OurApp app;
	app.init();

	int failures = 0;

	// default engine material instance (same one the renderers use as fallback)
	auto mi = g_runtime_context.m_asset_manager->get<MaterialInstance>(ENGINE_RESOURCE("material/MI_phone"));
	if (!mi) {
		std::cerr << "FAIL: MI_phone not found" << std::endl;
		return 1;
	}

	auto scene = std::make_shared<Scene>();

	auto cam = scene->create_transient_entity("camera");
	cam->add_component<CameraComponent>();
	cam->get_component<TransformComponent>().m_location = { 0.0f, 0.0f, 3.0f };
	scene->set_main_camera(cam);

	auto light = scene->create_entity("light");
	light->add_component<LightComponent>(LightType::Directional, glm::vec3(1.0f), 3.0f);
	light->get_component<TransformComponent>().m_rotation = { 45.0f, -30.0f, 0.0f };

	auto sphere = scene->create_entity("sphere");
	auto& mesh_comp = sphere->add_component<StaticMeshComponent>(make_sphere());
	mesh_comp.m_override_materials["slot0"] = mi;

	auto fb = Framebuffer::create(256, 256, {
		{ ImageFormat::RGBA8, SamplerMode::Nearest, WrapMode::ClampToBorder },
		{ ImageFormat::DepthStencil },
	});

	// dedicated renderer instance, exactly like the material editor preview
	if (g_runtime_context.m_global->render_mode == RenderMode::Deferred) {
		auto renderer = std::make_shared<RendererDeferred>();
		renderer->draw(scene, fb);
	}
	else {
		auto renderer = std::make_shared<RendererForward>();
		renderer->draw(scene, fb);
	}

	// center pixel must hit the lit sphere
	unsigned char px[4] = {};
	fb->read_pixels(0, 128, 128, 1, 1, px);
	int lum = (int)px[0] + (int)px[1] + (int)px[2];
	if (lum < 10) {
		std::cerr << "FAIL: preview sphere not visible (center lum " << lum << ")" << std::endl;
		++failures;
	}

	// corner pixel must stay background (no sky in the preview scene)
	unsigned char bg[4] = {};
	fb->read_pixels(0, 4, 4, 1, 1, bg);
	int bg_lum = (int)bg[0] + (int)bg[1] + (int)bg[2];
	if (bg_lum > lum / 4) {
		std::cerr << "FAIL: unexpected background brightness (corner lum " << bg_lum << ")" << std::endl;
		++failures;
	}

	if (failures == 0) {
		std::cout << "test_material_preview: all checks passed" << std::endl;
	}
	return failures;
}
