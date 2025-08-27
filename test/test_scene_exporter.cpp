#include "z1engine.h"

using namespace z1;

struct OurLayer : Layer {
	OurLayer() {
		m_scene = std::make_shared<Scene>();

		//{
		//	io::GltfImporterSettings settings{};
		//	settings.file = "../../editor/asset/mesh/DamagedHelmet.glb";
		//	settings.path = "DamagedHelmet";
		//	settings.root = "content";
		//	io::GltfImporter().import(settings);
		//}

		//{
		//	io::ImageImporterSettings settings{};
		//	settings.file = "../../runtime/asset/texture/awesomeface.png";
		//	settings.path = "T_awesomeface";
		//	settings.root = "content";
		//	io::ImageImporter().import(settings);
		//}

		auto cam = m_scene->create_entity("Camera");
		cam->add_component<CameraComponent>();
		cam->get_component<TransformComponent>().m_location.z = 5.0f;
		m_scene->set_main_camera(cam);

		{
			auto tex = g_runtime_context.m_asset_manager->get<Image2D>("T_awesomeface");
			auto ent = m_scene->create_entity("Square_0");
			ent->add_component<SpriteComponent>(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f), tex);
			ent->get_component<TransformComponent>().m_location.x = -1.0f;
		}

		{
			auto mesh = g_runtime_context.m_asset_manager->get<StaticMesh>("DamagedHelmet/mesh_helmet_LP_13930damagedHelmet");
			auto ent = m_scene->create_entity("Mesh_0");
			ent->add_component<StaticMeshComponent>(mesh);
			ent->get_component<TransformComponent>().m_location.x = 1.0f;
		}

		io::SceneExporter::export_scene(AssetManager::get_content_root() / "scene.meta.yaml", m_scene);
	}

	void on_update(float delta_time) override {
		m_scene->on_update(delta_time);
	}

	std::shared_ptr<Scene> m_scene;
};

struct OurApp : Application {
	void init() override {
		push_layer(std::make_shared<OurLayer>());
	};
};

int main() {

	AssetManager::add_shader_root("../../engine/shader");

	OurApp app;
	app.init();
	app.run();

	return 0;
}
