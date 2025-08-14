#pragma once

// --- precompiled header ----------------
#include "pch.h"
// ---------------------------------------

#include "core/core.h"
#include "core/io.h"
#include "core/maths.h"
#include "core/application.h"
#include "core/layer.h"
#include "core/layer_stack.h"
#include "core/window.h"
#include "core/input.h"
#include "core/timer.h"
#include "core/keycodes.h"

#include "event/event.h"
#include "event/key_event.h"
#include "event/mouse_event.h"
#include "event/application_event.h"

#include "scene/scene.h"
#include "scene/entity.h"
#include "scene/component/base.h"
#include "scene/component/camera.h"
#include "scene/component/mesh.h"
#include "scene/component/sprite.h"

#include "render/graphics_context.h"
#include "render/mesh.h"
#include "render/material.h"
#include "render/render_pass.h"
#include "render/buffer.h"
#include "render/shader.h"
#include "render/image.h"
#include "render/vertex_array.h"
#include "render/framebuffer.h"
#include "render/renderer/renderer_2d.h"
#include "render/renderer/renderer_forward.h"

#include "utils/instrumentor.h"
#include "utils/random_utils.h"
#include "utils/string_utils.h"
#include "utils/thread_pool.h"

#include "io/asset_manager.h"
#include "io/image_loader.h"
#include "io/mesh_loader.h"
