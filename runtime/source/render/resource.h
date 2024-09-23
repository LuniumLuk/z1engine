#pragma once

#include "core/core.h"
#include "core/io.h"
#include <stack>

namespace z1 {

#define RESOURCE_TYPES \
    X(Image) \
    X(UniformBuffer) \

#define X(name) name,
    enum struct API ResourceType {
        RESOURCE_TYPES
    };
#undef X

#define X(name) case ResourceType::name: return #name;
    inline std::string get_resource_name(ResourceType type) {
        switch (type) {
            RESOURCE_TYPES
        }
        return "unknown resource type";
    }
#undef X

    struct API Resource {
        friend struct Renderer;
        friend struct ResourceManager;

        Resource(ResourceType type);
        virtual ~Resource();

        /*
        * Bind this resource to a binding point, but directly call this method is deprecate
        * because the binding point is managed by the renderer, thus might be overwritten
        */
        virtual void bind(uint32_t binding) const = 0;
        virtual void unbind(uint32_t binding) const = 0;

        ResourceType get_resource_type() const { return m_type; }
        uint32_t get_resource_id() const { return m_id; }
        uint32_t get_binding() const { return m_binding; }
        uint32_t get_ref_count() const { return m_ref_count; }
        bool is_bound() const { return m_binding != INVALID_BINDING; }

    private:
        ResourceType m_type;
        uint32_t m_id;
        uint32_t m_binding = INVALID_BINDING;
        uint32_t m_ref_count = 0;
    };

    struct ResourceManager {

        ResourceManager();
        ~ResourceManager();

        Resource* get(uint32_t id) { return m_resources[id]; }

        template<typename T>
        T* get(uint32_t id) { return static_cast<T*>(m_resources[id]); }

        /*
        * Request for a resource to be bind
        * if success, a valid binding point will be returned
        */
        uint32_t bind_resource(uint32_t id);
        /*
        * unbind a resource
        * if texture already unbind, nothing will happen
        * else the texture's reference count will be decreased, texture will be unbind when reference count is 0
        */
        void unbind_resource(uint32_t id);

        uint32_t register_resource(Resource* resource) {
            m_resources.push_back(resource);
            return static_cast<uint32_t>(m_resources.size()) - 1;
        }

        void unregister_resource(uint32_t id) {
            m_resources[id] = nullptr;
        }

        std::vector<Resource*> m_resources;
        std::stack<uint32_t> m_valid_image_bindings;
        std::stack<uint32_t> m_valid_uniform_buffer_bindings;

    };

}
