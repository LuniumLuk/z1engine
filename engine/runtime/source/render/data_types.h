#pragma once

#include "core/core.h"

namespace z1 {

	// define enum by X Macro.
	// reference: https://digitalmars.com/articles/b51.html

	//        Name,           Size,      ElementCount
#define DATA_TYPE_LIST                                 \
			X(None,           0,         0           ) \
			X(Float,          4,         1           ) \
			X(Float2,         4 * 2,     2           ) \
			X(Float3,         4 * 3,     3           ) \
			X(Float4,         4 * 4,     4           ) \
			X(Int,            4,         1           ) \
			X(Int2,           4 * 2,     2           ) \
			X(Int3,           4 * 3,     3           ) \
			X(Int4,           4 * 4,     4           ) \
			X(Mat3,           4 * 3 * 3, 3           ) \
			X(Mat4,           4 * 4 * 4, 4           ) \
			X(Bool,           1,         1           ) \
			X(Sampler2D,      4,         1           ) \
			X(Sampler2DArray, 4,         1           ) \
			X(SamplerCube,    4,         1           )

	enum struct API DataType {
#define X(name, size, count) name,
		DATA_TYPE_LIST
#undef X
	};

	inline std::string get_data_type_name(DataType type) {
		switch (type) {
#define X(name, size, count) case DataType::name: return #name;
			DATA_TYPE_LIST
#undef X
		default:
			return "unknown data type";
		}
	}

	inline size_t API get_data_type_size(DataType type) {
		switch (type) {
#define X(name, size, count) case DataType::name: return size;
			DATA_TYPE_LIST
#undef X
		}

		CORE_ASSERT(false, "unknown data type!");
		return 0;
	}

	inline uint32_t API get_data_type_element_count(DataType type) {
		switch (type) {
#define X(name, size, count) case DataType::name: return count;
			DATA_TYPE_LIST
#undef X
		}

		CORE_ASSERT(false, "unknown data type!");
		return 0;
	}

#undef DATA_TYPE_LIST

}
