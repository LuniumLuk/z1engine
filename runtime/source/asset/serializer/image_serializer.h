#pragma once

#include "core/io.h"
#include "render/image.h"

namespace z1 {

	std::shared_ptr<Image2D> load_image2d_asset(Guid const& guid);

}
