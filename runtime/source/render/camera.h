#pragma once

#include "core/core.h"
#include "glm/glm.hpp"

namespace z1 {

	struct API Camera {
		Camera();
		Camera(glm::vec3 const& eye, glm::vec3 const& at, glm::vec3 const& up = glm::vec3(0.0f, 1.0f, 0.0f));

		glm::mat4 get_projview() const;
		glm::mat4 get_view() const;
		virtual glm::mat4 get_proj() const = 0;

		glm::mat3 get_basis() const;

		/*
		* move the camera, if absolute is true, the move is w.r.t. the world
		* if absolute is false, the move is w.r.t. camera itself
		*/
		void move(glm::vec3 const& vec, bool absolute = false);
		/*
		* rotate the camera, if a center is provided, the rotation will be w.r.t. the center,
		* otherwise, it will be w.r.t. itself
		*/
		void rotate(float pitch, float yaw, float roll);
		void rotate(float pitch, float yaw, float roll, glm::vec3 const& center);
		/*
		* zoom the camera, for perspective camera, this will adjust its fov,
		* for orthogonal camera, this will adjust its frsutum size
		*/
		virtual void zoom(float zoom) = 0;

		glm::vec3 const& get_eye() const { return m_eye; }
		glm::vec3 const& get_at() const { return m_at; }
		glm::vec3 const& get_up() const { return m_up; }
		void set_eye(glm::vec3 const& eye) { m_eye = eye; }
		void set_at(glm::vec3 const& at) { m_at = at; }
		void set_up(glm::vec3 const& up) { m_up = up; }

		float get_aspect() const { return m_aspect; }
		void set_aspect(float aspect) { m_aspect = aspect; }

		float get_near() const { return m_near; }
		float get_far() const { return m_far; }
		void set_near(float znear) { m_near = znear; }
		void set_far(float zfar) { m_far = zfar; }

		static std::shared_ptr<Camera> create_ortho(float cx, float cy, float w, float aspect, float znear = -1.0f, float zfar = 1.0f);
		static std::shared_ptr<Camera> create_persp(glm::vec3 const& eye, glm::vec3 const& at, glm::vec3 const& up, float fov);

	protected:
		glm::vec3 m_eye;
		glm::vec3 m_at;
		glm::vec3 m_up;

		float m_near = 0.01f;
		float m_far = 1000.0f;
		float m_aspect = 1.0f;
	};

	struct API OrthoCamera : Camera {
		OrthoCamera(float cx, float cy, float w, float aspect, float znear = -1.0f, float zfar = 1.0f);

		void zoom(float zoom) override;

		glm::mat4 get_proj() const override;

	private:
		float m_center_x, m_center_y;
		float m_half_width;
	};

	struct API PerspCamera : Camera {
		PerspCamera(glm::vec3 const& eye, glm::vec3 const& at, glm::vec3 const& up, float fov);

		void zoom(float zoom) override;

		glm::mat4 get_proj() const override;

	private:
		float m_fov;
	};

}
