#pragma once

#include "asset/asset.h"

namespace z1 {

	struct API ScriptAsset : Asset<ScriptAsset> {
		static std::shared_ptr<ScriptAsset> load(Guid const& guid) {
			auto asset = std::make_shared<ScriptAsset>();
			asset->m_meta = g_runtime_context.m_asset_manager->get_meta(guid);
			return asset;
		}
	};

}
