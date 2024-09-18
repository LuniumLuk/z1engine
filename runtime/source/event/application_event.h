#pragma once

#include "event/event.h"

namespace z1 {

    struct API WindowResizeEvent : Event {
        WindowResizeEvent(uint32_t width, uint32_t height)
            : m_width(width)
            , m_height(height) {}

        uint32_t get_width() const { return m_width; }
        uint32_t get_height() const { return m_height; }

        std::string to_string() const override {
            std::stringstream ss;
            ss << "WindowResizeEvent: " << m_width << ", " << m_height;
            return ss.str();
        }

        EVENT_STRUCT_CATEGORY(EventCategoryApplication)
        EVENT_STRUCT_TYPE(WindowResize)

    private:
        uint32_t m_width, m_height;
    };

    struct API WindowCloseEvent : Event {
        WindowCloseEvent() {}

        EVENT_STRUCT_CATEGORY(EventCategoryApplication)
        EVENT_STRUCT_TYPE(WindowClose)
    };

    struct API WindowFocusEvent : Event {
        WindowFocusEvent() {}

        EVENT_STRUCT_CATEGORY(EventCategoryApplication)
        EVENT_STRUCT_TYPE(WindowFocus)
    };

    struct API WindowLostFocusEvent : Event {
        WindowLostFocusEvent() {}

        EVENT_STRUCT_CATEGORY(EventCategoryApplication)
        EVENT_STRUCT_TYPE(WindowLostFocus)
    };

}
