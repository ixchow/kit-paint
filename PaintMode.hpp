#pragma once

#include <kit/kit.hpp>
#include <kit/GLBuffer.hpp>
#include <kit/GLVertexArray.hpp>

#include <deque>

struct PaintMode : public kit::Mode {
	PaintMode();
	virtual ~PaintMode();
	virtual void update(float elapsed) override;
	virtual void draw() override;
	virtual void pointer_action(kit::PointerID pointer, kit::PointerAction action, kit::Pointer const &old_state, kit::Pointer const &new_state) override;

	//canvas holds image that is being drawn on:
	glm::uvec2 canvas_size = glm::uvec2(0,0);
	GLuint canvas_tex = 0;
	GLuint canvas_fb = 0;

	//views of the canvas:
	glm::vec2 view_center = glm::vec2(0.0f, 0.0f); //the center of the screen (in canvas pixel coordinates)
	float view_scale = 1.0f; //size of canvas pixels relative to screen pixels

	//helper: set view_center / view_scale so whole canvas is visible:
	void view_all();

	//convert from [-1,1]x[-1,1] display coordinates to [0,canvas_size.x]x[0,canvas_size.y] canvas coordinates:
	glm::vec2 display_to_canvas(glm::vec2 const &display);

	//information about the current brush:
	struct Brush {
		GLuint tex = 0;
		float radius = 100.0f; //in canvas pixels
		float interval = 0.2f; //as a factor of brush radius
		glm::u8vec4 tint = glm::u8vec4(0x00, 0x00, 0x00, 0xff);
	} brush;

	//information about current strokes:
	struct Stroke {
		std::deque< glm::vec3 > points; //accumulated input points (px.x, px.y, pressure)
		float remain = 0.0f; //distance remaining before next stamp
	};
	std::unordered_map< kit::PointerID, Stroke > strokes;

	//helper: draw splats to canvas for all current strokes:
	void splat_strokes();

	GLAttribBuffer< glm::vec2, glm::u8vec4 > ui_buffer;
	GLVertexArray color_program_ui_buffer;

	GLAttribBuffer< glm::vec2, glm::u8vec4, glm::vec2 > splat_buffer;
	GLVertexArray texture_program_splat_buffer;

};
