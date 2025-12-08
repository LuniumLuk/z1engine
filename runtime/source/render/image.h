#pragma once

#include "core/io.h"
#include "core/guid.h"
#include "render/resource.h"
#include "glm/glm.hpp"
#include <vector>

namespace z1 {

#define DATA_TYPE_LIST  \
	X(Image2D)          \
	X(ImageCube)        \
	X(Image2DArray)     \
	X(ImageCubeArray)   \

#define X(a) a,
	enum struct API ImageType {
		DATA_TYPE_LIST
	};
#undef X

#define X(a) case ImageType::a: return #a;
	inline std::string get_image_type_name(ImageType type) {
		switch (type) {
			DATA_TYPE_LIST
		default: return "unknown type";
		}
	}
#undef X
#undef DATA_TYPE_LIST

#define DATA_TYPE_LIST  \
	X(None)             \
	X(RGBA8)            \
	X(RGBA32F)          \
	X(Depth)            \
	X(DepthStencil)     \

#define X(a) a,
	enum struct API ImageFormat {
		DATA_TYPE_LIST
	};
#undef X

#define X(a) case ImageFormat::a: return #a;
	inline std::string get_image_format_name(ImageFormat type) {
		switch (type) {
			DATA_TYPE_LIST
		default: return "unknown format type";
		}
	}
#undef X
#undef DATA_TYPE_LIST

#define DATA_TYPE_LIST  \
	X(Linear)           \
	X(Nearest)          \

#define X(a) a,
	enum struct API SamplerMode {
		DATA_TYPE_LIST
	};
#undef X

#define X(a) case SamplerMode::a: return #a;
	inline std::string get_sampler_mode_name(SamplerMode type) {
		switch (type) {
			DATA_TYPE_LIST
		default: return "unknown sampler mode";
		}
	}
#undef X
#undef DATA_TYPE_LIST

#define DATA_TYPE_LIST  \
	X(Repeat)           \
	X(MirroredRepeat)   \
	X(ClampToEdge)      \
	X(ClampToBorder)    \

#define X(a) a,
	enum struct API WrapMode {
		DATA_TYPE_LIST
	};
#undef X

#define X(a) case WrapMode::a: return #a;
	inline std::string get_wrap_mode_name(WrapMode type) {
		switch (type) {
			DATA_TYPE_LIST
		default: return "unknown wrap mode";
		}
	}
#undef X
#undef DATA_TYPE_LIST

	struct Shader;

	struct API Image : RenderResource {
		struct Description {
			uint32_t m_width = 0;
			uint32_t m_height = 0;
			uint32_t m_depth = 0;
			ImageFormat m_format = ImageFormat::None;
			SamplerMode m_sampler_mode = SamplerMode::Linear;
			WrapMode m_wrap_mode = WrapMode::Repeat;
			bool m_mipmap = true;
		};

		Image() : RenderResource(ResourceType::Image) {}

		virtual ~Image() = default;

		virtual void write(void const* data, size_t size) const = 0;

		virtual void* get_native_handle() const = 0;
		Description const& get_description() const { return m_description; }

		// Automatically bind to a binding point managed globally
		void bind() const;
		// Helper function to bind to a specific shader uniform
		void bind(std::shared_ptr<Shader> const& shader, std::string const& name) const;
		// Automatically unbind from the binding point managed globally
		void unbind() const;

		bool is_bound() const { return m_binding != INVALID_BINDING; }
		// Get the current binding point
		uint32_t get_binding() const { return m_binding; }


	protected:
		virtual void bind(uint32_t binding) const = 0;
		virtual void unbind(uint32_t binding) const = 0;

		Description m_description{};
		mutable uint32_t m_binding = INVALID_BINDING;
	};

	struct API Image2D : Image {
		static std::shared_ptr<Image2D> create(
			void const* data,
			size_t size,
			uint32_t width,
			uint32_t height,
			ImageFormat format = ImageFormat::RGBA8,
			SamplerMode sampler_mode = SamplerMode::Linear,
			WrapMode wrap_mode = WrapMode::Repeat);
	};

	struct API ImageCube : Image {
		struct Faces {
			void* m_right;
			void* m_left;
			void* m_top;
			void* m_bottom;
			void* m_front;
			void* m_back;
		};

		static std::shared_ptr<ImageCube> create(
			Faces data,
			size_t size,
			uint32_t width,
			uint32_t height,
			ImageFormat format = ImageFormat::RGBA8,
			SamplerMode sampler_mode = SamplerMode::Linear,
			WrapMode wrap_mode = WrapMode::Repeat);
	};

}
