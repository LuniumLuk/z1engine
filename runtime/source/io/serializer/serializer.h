#pragma once

#include "core/core.h"
#include "core/io.h"

namespace z1::io {

	template <typename Derived, typename AssetType>
	struct Serializer {

		static bool serialize(Filepath const& path, std::shared_ptr<AssetType> const& asset) {
			return Derived::serialize(path, asset);
		}

		static std::shared_ptr<AssetType> deserialize(Filepath const& path) {
			return Derived::deserialize(path);
		}

	};

}
