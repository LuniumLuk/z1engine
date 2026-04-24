#include "z1engine.h"
#include <fstream>
#include <vector>
#include <string>
#include "python/python_layer.h"
#include "python/python_script.h"

using namespace z1;

struct OurApp : Application {
	void init() override {};
};

int main() {
	OurApp app;
	app.init();

	Filepath scene_path = "sandbox-tests/scene_serialize_case";
	Filepath scene_file = (FileSystem::s_content_root / scene_path).concat(".yaml");
	Filepath sandbox_script_dir = FileSystem::s_content_root / "sandbox_tests";
	Filepath sandbox_script_file = sandbox_script_dir / "test_mover.py";
	Filepath sandbox_init_file = sandbox_script_dir / "__init__.py";
	auto cleanup_scene = [&]() {
		std::error_code ec;
		std::filesystem::remove(scene_file, ec);
		std::filesystem::remove(sandbox_script_file, ec);
		std::filesystem::remove(sandbox_init_file, ec);
		std::filesystem::remove_all(sandbox_script_dir, ec);
		std::filesystem::remove_all(FileSystem::s_content_root / "sandbox-tests", ec);
	};
	cleanup_scene();
	std::filesystem::create_directories(sandbox_script_dir);
	{
		std::ofstream init_file(sandbox_init_file.string(), std::ios::out | std::ios::trunc);
		if (!init_file.is_open()) {
			cleanup_scene();
			return 1;
		}
	}
	{
		std::ofstream script_file(sandbox_script_file.string(), std::ios::out | std::ios::trunc);
		if (!script_file.is_open()) {
			cleanup_scene();
			return 1;
		}
		script_file << "class TestMover:\n";
		script_file << "\tdef on_update(self, delta_time):\n";
		script_file << "\t\tt = self.entity.transform\n";
		script_file << "\t\tif t is not None:\n";
		script_file << "\t\t\tloc = t.location\n";
		script_file << "\t\t\tloc.x = loc.x + delta_time\n";
		script_file << "\t\t\tt.location = loc\n";
	}

	Filepath cwd = std::filesystem::current_path();
	std::cout << "current working directory: " << cwd.generic_string() << std::endl;

	// generate and serialize sample scene
	auto scene = Scene::create(scene_path);

#if 0
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
#endif

	{
		auto ent = scene->create_entity("Mesh_0");
		ent->add_component<StaticMeshComponent>(
			g_runtime_context.m_asset_manager->get<StaticMesh>("mesh/SM_Cube")
		);
		ent->get_component<TransformComponent>().m_location = glm::vec3(0.5f, 0.5f, -5.0f);
	}

	{
		auto ent = scene->create_entity("Mesh_1");
		ent->add_component<StaticMeshComponent>(
			g_runtime_context.m_asset_manager->get<StaticMesh>("mesh/SM_Sphere")
		);
		ent->get_component<TransformComponent>().m_location = glm::vec3(1.0f, -2.0f, -4.0f);
	}

	{
		auto ent = scene->create_entity("Mesh_2");
		ent->add_component<StaticMeshComponent>(
			g_runtime_context.m_asset_manager->get<StaticMesh>("mesh/SM_Cone")
		);
		ent->get_component<TransformComponent>().m_location = glm::vec3(-1.5f, 0.0f, -1.0f);
		ent->get_component<TransformComponent>().m_rotation = glm::vec3(90.0f, 0.0f, 0.0f);
	}

	// Add a set of engine meshes from engine/mesh for a more fruitful test scene
	{
		// create a wide ground plane first
		{
			auto ground = scene->create_entity("Ground");
			ground->add_component<StaticMeshComponent>(
				g_runtime_context.m_asset_manager->get<StaticMesh>(std::string("mesh/SM_Cube"))
			);
			auto& gtc = ground->get_component<TransformComponent>();
			gtc.m_location = glm::vec3(0.0f, -3.4f, -4.5f);
			gtc.m_scale = glm::vec3(30.0f, 0.2f, 30.0f);
		}

		std::vector<std::string> engine_meshes = { "SM_Cube", "SM_Sphere", "SM_Cylinder", "SM_Cone", "SM_Suzanne" };
		const uint32_t cols = 5;
		const float spacing = 5.0f;
		const int duplicates = 2; // number of copies for each mesh

		for (size_t i = 0; i < engine_meshes.size(); ++i) {
			const auto& mname = engine_meshes[i];
			// cluster center for this mesh type
			float cx = float((int)i % cols) * spacing - (float(cols - 1) * spacing * 0.5f);
			float cy = float((int)i / cols) * spacing - 0.5f;

			for (int d = 0; d < duplicates; ++d) {
				std::string ename = std::string("EngineMesh_") + mname + "_" + std::to_string(d);
				auto ent = scene->create_entity(ename);
				ent->add_component<StaticMeshComponent>(
					g_runtime_context.m_asset_manager->get<StaticMesh>(std::string("mesh/") + mname)
				);

				auto& tc = ent->get_component<TransformComponent>();

				// spread duplicates in a small pattern around the cluster center
				float ox = ((d % 2 == 0) ? -0.5f : 0.5f) * (1.0f + d * 0.15f);
				float oy = ((d / 2 == 0) ? -0.35f : 0.35f) * (1.0f + d * 0.12f);
				float oz = -4.0f + float((d % 3) - 1) * 0.35f;
				tc.m_location = glm::vec3(cx + ox, cy + oy, oz);

				// vary scales for visual interest
				float base_s = 0.7f + float((i + d) % 3) * 0.25f;
				float s = base_s * (1.0f + d * 0.08f);
				tc.m_scale = glm::vec3(s, s, s);

				// add dynamic rotations (degrees) so each copy is distinct
				tc.m_rotation = glm::vec3(10.0f * float(d) + 5.0f * float(i), 30.0f * float(i) + 22.0f * float(d), 8.0f * float(d));
			}
		}
	}

#if 0
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
#endif

	// Python Script Test
	if (g_runtime_context.m_python_layer) {
		std::cout << "Starting Python Script Test..." << std::endl;
		g_runtime_context.m_global->script_enabled = true;
		g_runtime_context.m_python_layer->on_attach();

		auto ent = scene->create_entity("PythonTestEntity");
		ent->attach_script<PythonScript>("sandbox_tests.test_mover", "TestMover");

		// Run update loop (Attach and Update)
		scene->on_update(0.1f);

		auto pos = ent->get_component<TransformComponent>().m_location;
		std::cout << "PythonTestEntity Pos X: " << pos.x << std::endl;

		if (pos.x > 0.05f) {
			std::cout << "[SUCCESS] Python Script moved entity!" << std::endl;
		}
		else {
			std::cout << "[FAILURE] Python Script did not move entity." << std::endl;
			g_runtime_context.m_python_layer->on_detach();
			cleanup_scene();
			return 1;
		}

		g_runtime_context.m_python_layer->on_detach();
	}

	scene->save();
	cleanup_scene();

	return 0;
}
