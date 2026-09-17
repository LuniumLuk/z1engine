#pragma once

#include "core/core.h"
#include "core/io.h"
#include "core/guid.h"
#include "util/yaml.h"

namespace z1 {

	struct AssetMeta {
		Guid guid;
		std::string type;
		Filepath path;
		// used by the editor to tell apart "engine" assets
		// from normal project assets. Not saved to disk.
		std::string root;

		YAML::Node extra; // extra fields for each asset type

		std::string name() const {
			return path.filename().string();
		}
	};

	struct AssetBase {
		virtual ~AssetBase() = default;
		AssetMeta m_meta = {};
		mutable bool m_is_dirty = false;
		mutable bool m_is_saved = false;

		void mark_dirty() const {
			m_is_dirty = true;
			m_is_saved = false;
		}

		void mark_saved() const {
			m_is_dirty = false;
			m_is_saved = true;
		}
	};

	template <typename Derived>
	struct Asset;

}
