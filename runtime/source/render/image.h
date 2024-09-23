#pragma once

#include "core/io.h"
#include "render/resource.h"
#include <vector>

namespace z1 {

    enum struct API ImageType {
        Image2D = 0,
        ImageCube,
        Image2DArray,
        ImageCubeArray,
    };

    enum struct API ImageFormat {
        None = 0,
        RGBA8,
        RGBA32F,
        Depth,
        DepthStencil,
    };

    enum struct API SamplerMode {
        Linear = 0,
        Nearest,
    };

    enum struct API WrapMode {
        Repeat = 0,
        MirroredRepeat,
        ClampToEdge,
        ClampToBorder,
    };

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

        virtual void write(void* data, size_t size) const = 0;

        virtual void* get_native_handle() const = 0;
        Description const& get_description() const { return m_description; }

    protected:
        Description m_description{};
    };

    struct API Image2D : Image {
        static std::shared_ptr<Image2D> create(
            Filepath const& path,
            SamplerMode sampler_mode = SamplerMode::Linear,
            WrapMode wrap_mode = WrapMode::Repeat);

        static std::shared_ptr<Image2D> create(
            void* data,
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
