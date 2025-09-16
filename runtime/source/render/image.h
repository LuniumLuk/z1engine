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

	struct API Image : Resource {
		struct Description {
			uint32_t m_width = 0;
			uint32_t m_height = 0;
			uint32_t m_depth = 0;
			ImageFormat m_format = ImageFormat::None;
			SamplerMode m_sampler_mode = SamplerMode::Linear;
			WrapMode m_wrap_mode = WrapMode::Repeat;
			bool m_mipmap = true;
		};

		Image() : Resource(ResourceType::Image) {}

		virtual ~Image() = default;

		virtual void write(void const* data, size_t size) const = 0;

		virtual void* get_native_handle() const = 0;
		Description const& get_description() const { return m_description; }

	protected:
		Description m_description{};
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

	struct API SubImage2D {
		SubImage2D(std::shared_ptr<Image2D> image, glm::vec2 const& min, glm::vec2 const& max)
			: m_image(image) {
			m_texcoords[0] = min;
			m_texcoords[1] = glm::vec2(max.x, min.y);
			m_texcoords[2] = max;
			m_texcoords[3] = glm::vec2(min.x, max.y);
		}

		std::array<glm::vec2, 4> get_texcoords() const { return m_texcoords; }

		static std::shared_ptr<SubImage2D> create(
			std::shared_ptr<Image2D> image,
			uint32_t x, uint32_t y, uint32_t width, uint32_t height, bool top_left_origion = false) {
			if (top_left_origion) {
				y = image->get_description().m_height - y - height;
			}
			return std::make_shared<SubImage2D>(image,
				glm::vec2(
					(float)x / image->get_description().m_width,
					(float)y / image->get_description().m_height
				),
				glm::vec2(
					(float)(x + width) / image->get_description().m_width,
					(float)(y + height) / image->get_description().m_height
				));
		}

		std::array<glm::vec2, 4> m_texcoords;
		std::shared_ptr<Image2D> m_image;
	};

}
