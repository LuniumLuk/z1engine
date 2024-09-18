#include "pch.h"
#include "render/camera.h"
#include "core/maths.h"
#include "glm/ext/matrix_transform.hpp"
#include "glm/ext/matrix_clip_space.hpp"

namespace z1 {

    Camera::Camera()
        : m_eye({ 0.0f, 0.0f, 0.0f })
        , m_at({ 0.0f, 0.0f, 1.0f })
        , m_up({ 0.0f, 1.0f, 0.0f }) {}

    Camera::Camera(glm::vec3 const& eye, glm::vec3 const& at, glm::vec3 const& up)
        : m_eye(eye)
        , m_at(at)
        , m_up(up) {}

    glm::mat3 Camera::get_basis() const {
        auto forward = glm::normalize(m_at - m_eye);
        auto right = glm::normalize(glm::cross(forward, m_up));
        auto up = glm::normalize(glm::cross(right, forward));
        return glm::mat3(right, up, forward);
    }

    void Camera::move(glm::vec3 const& vec, bool absolute) {
        if (absolute) {
            m_eye += vec;
            m_at += vec;
        }
        else {
            auto delta = get_basis() * vec;
            m_eye += delta;
            m_at += delta;
        }
    }

    void Camera::rotate(float pitch, float yaw, float roll) {
        auto basis = get_basis();

        glm::mat4 transform(1.0f);
        transform = glm::rotate(transform, pitch, basis[0]);
        transform = glm::rotate(transform, yaw, basis[1]);
        transform = glm::rotate(transform, roll, basis[2]);

        auto forward = glm::vec3(transform * glm::vec4(basis[2], 1.0f));

        auto cos = glm::dot(forward, m_up);
        if (1.0f - std::abs(cos) < 0.0001f) {
            return;
        }

        m_at = m_eye + forward;
    }

    void Camera::rotate(float yaw, float pitch, float roll, glm::vec3 const& center) {
        auto forward = center - m_eye;
        auto dist = glm::length(forward);
        auto right = glm::cross(forward, m_up);
        auto up = glm::cross(right, forward);


        glm::mat4 transform(1.0f);
        transform = glm::rotate(transform, pitch, right);
        transform = glm::rotate(transform, yaw, up);
        transform = glm::rotate(transform, roll, forward);

        forward = glm::normalize(glm::vec3(transform * glm::vec4(forward, 1.0f)));

        auto cos = glm::dot(forward, m_up);
        if (1.0f - std::abs(cos) < 0.0001f) {
            return;
        }

        m_eye = center - forward * dist;
        m_at = m_eye + forward;
    }

    glm::mat4 Camera::get_projview() const {
        PROFILE_FUNCTION();
        return get_proj() * get_view();
    }

    glm::mat4 Camera::get_view() const {
        PROFILE_FUNCTION();
        return glm::lookAt(m_eye, m_at, m_up);
    }

    std::shared_ptr<Camera> Camera::create_ortho(float cx, float cy, float w, float aspect, float zNear, float zFar) {
        return std::shared_ptr<Camera>(new OrthoCamera(cx, cy, w, aspect, zNear, zFar));
    }

    std::shared_ptr<Camera> Camera::create_persp(glm::vec3 const& eye, glm::vec3 const& at, glm::vec3 const& up, float fov) {
        return std::shared_ptr<Camera>(new PerspCamera(eye, at, up, fov));
    }

    OrthoCamera::OrthoCamera(float cx, float cy, float w, float aspect, float zNear, float zFar)
        : m_center_x(cx)
        , m_center_y(cy)
        , m_half_width(w / 2) {
        m_aspect = aspect;
        m_near = zNear;
        m_far = zFar;
    }

    glm::mat4 OrthoCamera::get_proj() const {
        PROFILE_FUNCTION();
        float left = m_center_x - m_half_width;
        float right = m_center_x + m_half_width;
        float bottom = m_center_y - m_half_width * m_aspect;
        float top = m_center_y + m_half_width * m_aspect;
        return glm::ortho(left, right, bottom, top, m_near, m_far);
    }

    void OrthoCamera::zoom(float zoom) {
        CORE_ASSERT(zoom > 0.0f, "zoom factor must be positive");
        m_half_width /= zoom;
    }

    PerspCamera::PerspCamera(glm::vec3 const& eye, glm::vec3 const& at, glm::vec3 const& up, float fov)
        : Camera(eye, at, up)
        , m_fov(fov) {}

    glm::mat4 PerspCamera::get_proj() const {
        PROFILE_FUNCTION();
        return glm::perspective(m_fov, m_aspect, m_near, m_far);
    }

    void PerspCamera::zoom(float zoom) {
        CORE_ASSERT(zoom > 0.0f, "zoom factor must be positive");
        m_fov /= zoom;
        m_fov = std::clamp(m_fov, 0.0f, pi);
    }

}
