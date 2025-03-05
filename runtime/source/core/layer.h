#pragma once

#include "core/core.h"
#include "event/event.h"

namespace z1 {

    struct Application;

    struct API Layer {
        Layer(std::string const& name = "unnamed layer");
        virtual ~Layer();

        virtual void on_attach() {}
        virtual void on_detach() {}
        virtual void on_update(double delta_time) {}
        virtual void on_fixed_update() {}
        virtual void on_imgui_render() {}
        virtual void on_event(Event& event) {}

        std::string const& get_name() const { return m_name; }

    private:
        std::string m_name;

        Application* m_attached_application = nullptr;

        friend struct Application;
    };

}
