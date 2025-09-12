#pragma once

#include "core/io.h"
#include "scene/scene.h"
#include "asset/serializer/serializer.h"

namespace z1::io {

	struct SceneSerializer : Serializer<SceneSerializer, Scene> {

		static bool serialize(Filepath const& path, std::shared_ptr<Scene> const& asset);

		static std::shared_ptr<Scene> deserialize(Filepath const& path);

	};

}
