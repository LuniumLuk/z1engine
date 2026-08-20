#pragma once

#include "asset/asset.h"

namespace z1 {

	struct API ScriptAsset : Asset<ScriptAsset> {
		// --- begin asset interface ---
		static std::shared_ptr<ScriptAsset> load(Guid const& guid, AssetMeta const& meta, Filepath const& file) {
			auto asset = std::make_shared<ScriptAsset>();
			asset->m_meta = meta;
			return asset;
		}
		// --- end asset interface ---
	};

}
