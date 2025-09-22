#include "z1engine.h"

using namespace z1;

struct OurApp : Application {
	void init() override {};
};

int main() {
	// override content root and shader root for test environment
	FileSystem::s_content_root = "../../content";
	FileSystem::s_engine_root = "../../engine";

	OurApp app;
	app.init();

	Filepath cwd = std::filesystem::current_path();
	std::cout << "current working directory: " << cwd.generic_string() << std::endl;

	// generate and serialize sample scene
	auto scene = std::make_shared<Scene>();
	auto persp_cam = scene->create_entity("Persp Camera");
	persp_cam->add_component<CameraComponent>();

	auto ortho_cam = scene->create_entity("Ortho Camera");
	auto& ortho_cc = ortho_cam->add_component<CameraComponent>();
	ortho_cc.m_is_perspective = false;
	ortho_cc.m_near = -20.0f;
	ortho_cc.m_far = 20.0f;
	ortho_cc.m_intrinsic.size = 4.0f; // frustum size
	scene->set_main_camera(ortho_cam);

	auto tex0 = g_runtime_context.m_asset_manager->get<Texture2D>("texture/T_awesomeface");
	auto tex1 = g_runtime_context.m_asset_manager->get<Texture2D>("texture/T_tira-checker");
	auto tex2 = g_runtime_context.m_asset_manager->get<Texture2D>("texture/T_roguelikeSheet");

	{
		auto ent = scene->create_entity("Square_0");
		ent->add_component<SpriteComponent>(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f), tex0);
	}

	{
		auto ent = scene->create_entity("Square_1");
		ent->add_component<SpriteComponent>(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f), tex1);
		ent->get_component<TransformComponent>().m_location = glm::vec3(0.0f, 0.0f, -9.5f);
		ent->get_component<TransformComponent>().m_scale = glm::vec3(10.0f, 10.0f, 1.0f);
	}

	{
		auto ent = scene->create_entity("Mesh_0");
		ent->add_component<StaticMeshComponent>(
			g_runtime_context.m_asset_manager->get<StaticMesh>("SM_bunny")
		);
		ent->get_component<TransformComponent>().m_location = glm::vec3(0.5f, 0.5f, -5.0f);
	}

	{
		auto ent = scene->create_entity("Mesh_1");
		ent->add_component<StaticMeshComponent>(
			g_runtime_context.m_asset_manager->get<StaticMesh>("SM_fireplace_room")
		);
		ent->get_component<TransformComponent>().m_location = glm::vec3(1.0f, -2.0f, -4.0f);
	}

	{
		auto ent = scene->create_entity("Mesh_2");
		ent->add_component<StaticMeshComponent>(
			g_runtime_context.m_asset_manager->get<StaticMesh>("DamagedHelmet/mesh_helmet_LP_13930damagedHelmet")
		);
		ent->get_component<TransformComponent>().m_location = glm::vec3(-1.5f, 0.0f, -1.0f);
		ent->get_component<TransformComponent>().m_rotation = glm::vec3(90.0f, 0.0f, 0.0f);
	}

	uint32_t quad_rows = 4;
	uint32_t quad_cols = 4;
	float quad_stride = 0.15f;
	float quad_size = 0.1f;

	uint32_t tile_size = 16;
	uint32_t stride = 1;
	for (uint32_t i = 0; i < quad_rows; ++i) {
		for (uint32_t j = 0; j < quad_cols; ++j) {
			auto tile = SubTexture2D::create(tex2, i * (tile_size + stride), j * (tile_size + stride), tile_size, tile_size, true);
			auto ent = scene->create_entity("SubImage2D_" + std::to_string(i * quad_rows + j));
			ent->add_component<SpriteComponent>(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f), tile);
			ent->get_component<TransformComponent>().m_location = glm::vec3(-(float)i * quad_stride - 0.2f, -(float)j * quad_stride - 0.2f, 1.0f);
			ent->get_component<TransformComponent>().m_scale = glm::vec3(quad_size, quad_size, 1.0f);
		}
	}

	for (uint32_t i = 0; i < quad_rows; ++i) {
		for (uint32_t j = 0; j < quad_cols; ++j) {
			auto ent = scene->create_entity("Quad_" + std::to_string(i * quad_rows + j));
			ent->add_component<SpriteComponent>(glm::vec4((float)(i % quad_rows) / quad_rows, (float)(j % quad_cols) / quad_cols, 1.0f, 1.0f));
			ent->get_component<TransformComponent>().m_location = glm::vec3(i * quad_stride, j * quad_stride, 0.1f);
			ent->get_component<TransformComponent>().m_scale = glm::vec3(quad_size, quad_size, 1.0f);
		}
	}

	Scene::serialize("scene/sample", scene);

	return 0;
}
