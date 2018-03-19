#include "PaintMode.hpp"

#include <kit/Load.hpp>
#include <kit/GLProgram.hpp>
#include <kit/GLVertexArray.hpp>

#include <kit/gl_errors.hpp>
#include <kit/check_fb.hpp>

GLint color_program_mvp = -1;

kit::Load< GLProgram > color_program(kit::LoadTagInit, []() -> const GLProgram * {
	GLProgram *ret = new GLProgram(
		"#version 330\n"
		"#line " STR(__LINE__) "\n"
		"uniform mat4 mvp;\n"
		"in vec4 Position;\n"
		"in vec4 Color;\n"
		"out vec4 color;\n"
		"void main() {\n"
		"	gl_Position = mvp * Position;\n"
		"	color = Color;\n"
		"}\n"
		,
		"#version 330\n"
		"#line " STR(__LINE__) "\n"
		"in vec4 color;\n"
		"layout(location = 0) out vec4 fragColor;\n"
		"void main() {\n"
		"	fragColor = color;\n"
		"}\n"
	);

	color_program_mvp = ret->getUniformLocation("mvp");

	GL_ERRORS();

	return ret;
});

//----------------------

GLint texture_program_mvp = -1;

kit::Load< GLProgram > texture_program(kit::LoadTagInit, []() -> const GLProgram * {
	GLProgram *ret = new GLProgram(
		"#version 330\n"
		"#line " STR(__LINE__) "\n"
		"uniform mat4 mvp;\n"
		"in vec4 Position;\n"
		"in vec4 Color;\n"
		"in vec2 TexCoord;\n"
		"out vec4 color;\n"
		"out vec2 texCoord;\n"
		"void main() {\n"
		"	gl_Position = mvp * Position;\n"
		"	color = Color;\n"
		"	texCoord = TexCoord;\n"
		"}\n"
		,
		"#version 330\n"
		"#line " STR(__LINE__) "\n"
		"uniform sampler2D tex;\n"
		"in vec4 color;\n"
		"in vec2 texCoord;\n"
		"layout(location = 0) out vec4 fragColor;\n"
		"void main() {\n"
		"	fragColor = color * texture(tex, texCoord);\n"
		"}\n"
	);


	texture_program_mvp = ret->getUniformLocation("mvp");

	GLint texture_program_tex = ret->getUniformLocation("tex");

	glUseProgram(ret->program);
	glUniform1i(texture_program_tex, 0);
	glUseProgram(0);

	GL_ERRORS();

	return ret;
});

kit::Load< GLAttribBuffer< glm::vec2, glm::u8vec4, glm::vec2 > > canvas_buffer(kit::LoadTagInit, []()->const GLAttribBuffer< glm::vec2, glm::u8vec4, glm::vec2 > *{
	GLAttribBuffer< glm::vec2, glm::u8vec4, glm::vec2 > *ret = new GLAttribBuffer< glm::vec2, glm::u8vec4, glm::vec2  >();
	std::vector< GLAttribBuffer< glm::vec2, glm::u8vec4, glm::vec2 >::Vertex > data;
	data.emplace_back(glm::vec2(0.0f, 0.0f), glm::u8vec4(0xff), glm::vec2(0.0f, 0.0f));
	data.emplace_back(glm::vec2(0.0f, 1.0f), glm::u8vec4(0xff), glm::vec2(0.0f, 1.0f));
	data.emplace_back(glm::vec2(1.0f, 0.0f), glm::u8vec4(0xff), glm::vec2(1.0f, 0.0f));
	data.emplace_back(glm::vec2(1.0f, 1.0f), glm::u8vec4(0xff), glm::vec2(1.0f, 1.0f));
	ret->set(data, GL_STATIC_DRAW);
	return ret;
});

kit::Load< GLVertexArray > texture_program_canvas_buffer(kit::LoadTagDefault, []()->const GLVertexArray * {
	return new GLVertexArray(GLVertexArray::make_binding(texture_program->program, {
		{texture_program->getAttribLocation("Position", GLProgram::MissingIsWarning), (*canvas_buffer)[0]},
		{texture_program->getAttribLocation("Color", GLProgram::MissingIsWarning), (*canvas_buffer)[1]},
		{texture_program->getAttribLocation("TexCoord", GLProgram::MissingIsWarning), (*canvas_buffer)[2]}
	}));
});

PaintMode::PaintMode() {
	//Allocate and render brush texture:
	glGenTextures(1, &brush.tex);
	{
		glm::uvec2 size(256,256);
		std::vector< glm::vec4 > data(size.x * size.y);
		for (uint32_t y = 0; y < size.y; ++y) {
			for (uint32_t x = 0; x < size.x; ++x) {
				glm::vec2 at = glm::vec2(x + 0.5f, y + 0.5f) / glm::vec2(size);
				at = 2.0f * (at - 0.5f);
				float val = std::max(0.0f, 1.0f - glm::length(at));
				val = 1.0f - (1.0f - val) * (1.0f - val);
				data[y * size.x + x] = glm::vec4(1.0f, 1.0f, 1.0f, val);
			}
		}
		glBindTexture(GL_TEXTURE_2D, brush.tex);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size.x, size.y, 0, GL_RGBA, GL_FLOAT, data.data());
		GL_ERRORS();
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glGenerateMipmap(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, 0);
		GL_ERRORS();
	}

	canvas_size = glm::uvec2(1000, 1000);

	//Allocate canvas texture:
	glGenTextures(1, &canvas_tex);
	glBindTexture(GL_TEXTURE_2D, canvas_tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, canvas_size.x, canvas_size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_2D, 0);

	//Create canvas framebuffer:
	glGenFramebuffers(1, &canvas_fb);
	glBindFramebuffer(GL_FRAMEBUFFER, canvas_fb);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, canvas_tex, 0);
	check_fb();

	//clear the canvas:
	glViewport(0,0,canvas_size.x, canvas_size.y);
	glClearColor(1.0, 1.0, 1.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);


	view_center = 0.5f * glm::vec2(canvas_size);
	view_scale = 1.0f;

	//set up vertex arrays for binding the ui_buffer and splat_buffer:
	color_program_ui_buffer = GLVertexArray::make_binding(color_program->program, {
		{color_program->getAttribLocation("Position", GLProgram::MissingIsWarning), ui_buffer[0]},
		{color_program->getAttribLocation("Color", GLProgram::MissingIsWarning), ui_buffer[1]}
	});

	texture_program_splat_buffer = GLVertexArray::make_binding(texture_program->program, {
		{texture_program->getAttribLocation("Position", GLProgram::MissingIsWarning), splat_buffer[0]},
		{texture_program->getAttribLocation("Color", GLProgram::MissingIsWarning), splat_buffer[1]},
		{texture_program->getAttribLocation("TexCoord", GLProgram::MissingIsWarning), splat_buffer[2]}
	});

	GL_ERRORS();
}

PaintMode::~PaintMode() {
	glDeleteFramebuffers(1, &canvas_fb);
	glDeleteTextures(1, &canvas_tex);
}

void PaintMode::update(float elapsed) {
	splat_strokes();
}

void PaintMode::draw() {
	view_all();

	glm::mat4 canvas_to_screen;
	{ //transformation matrix from canvas pixels to view coordinates:
		//'px' -- size of a canvas pixel in viewport ([-1,1]^2) coordinates
		glm::vec2 px(
			2.0f * (view_scale / kit::display.size.x),
			2.0f * (view_scale / kit::display.size.y)
		);
		//NOTE: column-major order:
		canvas_to_screen = glm::mat4(
			glm::vec4(px.x, 0.0f, 0.0f, 0.0f),
			glm::vec4(0.0f, px.y, 0.0f, 0.0f),
			glm::vec4(0.0f, 0.0f, 1.0f, 0.0f),
			glm::vec4(-view_center.x * px.x, -view_center.y * px.y, 0.0f, 1.0f)
		);
	}

	//clear the screen:
	glViewport(0,0,kit::display.size.x, kit::display.size.y);
	glClearColor(0.1, 0.1, 0.1, 0.0);
	glClear(GL_COLOR_BUFFER_BIT);

	//draw canvas:
	glUseProgram(texture_program->program);

	{ //set transformation matrix to scale the [0,1] canvas buffer to the proper size, then map using canvas_to_screen:
		glm::mat4 mvp = canvas_to_screen * glm::mat4(
			glm::vec4(canvas_size.x, 0.0f, 0.0f, 0.0f),
			glm::vec4(0.0f, canvas_size.y, 0.0f, 0.0f),
			glm::vec4(0.0f, 0.0f, 1.0f, 0.0f),
			glm::vec4(0.0f, 0.0f, 0.0f, 1.0f)
		);
		glUniformMatrix4fv(texture_program_mvp, 1, GL_FALSE, glm::value_ptr(mvp));
	}

	//send vertex attributes for canvas:
	glBindVertexArray(texture_program_canvas_buffer->array);
	glBindTexture(GL_TEXTURE_2D, canvas_tex);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, canvas_buffer->count);
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindVertexArray(0);

	glUseProgram(0);

	{ //generate cursor geometry:

		//pre-calculate a circle:
		static std::vector< glm::vec2 > circle = [](){
			std::vector< glm::vec2 > circle;
			constexpr uint32_t Angles = 32;
			circle.reserve(Angles);
			for (uint32_t a = 0; a < Angles; ++a) {
				float ang = (M_PI * 2.0f * a) / Angles;
				circle.emplace_back(std::cos(ang), std::sin(ang));
			}
			return circle;
		}();

		std::vector< GLAttribBuffer< glm::vec2, glm::u8vec4 >::Vertex > data;
		for (auto const &ip : kit::pointers) {
			kit::Pointer const &p = ip.second;
			glm::vec2 at = display_to_canvas(p.at);
			float r1 = brush.radius;
			glm::u8vec4 c1 = glm::u8vec4(0x00, 0x00, 0x00, 0x88);
			float r2 = brush.radius + 2.0f * kit::display.pixel_ratio;
			glm::u8vec4 c2 = glm::u8vec4(0x00, 0x00, 0x00, 0x00);
			for (uint32_t i = 0; i < circle.size(); ++i) {
				data.emplace_back(at + circle[i] * r1, c1);
				if (i == 0) data.emplace_back(data.back());
				data.emplace_back(at + circle[i] * r2, c2);
			}
			data.emplace_back(at + circle[0] * r1, c1);
			data.emplace_back(at + circle[0] * r2, c2);
			data.emplace_back(data.back());
		}

		ui_buffer.set(data, GL_STREAM_DRAW);
	}

	glEnable(GL_BLEND);
	glBlendEquation(GL_FUNC_ADD);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	//draw cursor(s):
	glUseProgram(color_program->program);
	glUniformMatrix4fv(color_program_mvp, 1, GL_FALSE, glm::value_ptr(canvas_to_screen));

	glBindVertexArray(color_program_ui_buffer.array);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, ui_buffer.count);
	glBindVertexArray(0);

	glUseProgram(0);

	GL_ERRORS();
}

void PaintMode::pointer_action(kit::PointerID pointer, kit::PointerAction action, kit::Pointer const &old_state, kit::Pointer const &new_state) {
	auto f = strokes.find(pointer);
	if (f != strokes.end()) { //pointer is drawing a stroke
		f->second.points.emplace_back(display_to_canvas(new_state.at), new_state.pressure);
		if (action == kit::PointerUp) {
			//stroke is over
			splat_strokes();
			strokes.erase(f);
		}
	} else { //pointer is not drawing a stroke
		if (action == kit::PointerDown) {
			//start a stroke
			strokes[pointer].points.emplace_back(display_to_canvas(new_state.at), new_state.pressure);
		}
	}
}

void PaintMode::view_all() {
	view_center = 0.5f * glm::vec2(canvas_size);
	view_scale = std::min(kit::display.size.x / float(canvas_size.x), kit::display.size.y / float(canvas_size.y));
	view_scale = std::min(1.0f, view_scale); //don't go beyond 1-1
}

glm::vec2 PaintMode::display_to_canvas(glm::vec2 const &display) {
	return glm::vec2(
		(0.5f * display.x * kit::display.size.x) / view_scale + view_center.x,
		(0.5f * display.y * kit::display.size.y) / view_scale + view_center.y
	);
}

void PaintMode::splat_strokes() {

	std::vector< GLAttribBuffer< glm::vec2, glm::u8vec4, glm::vec2 >::Vertex > data;

	//add a single quad with a given radius and tint to the list of splats:
	auto splat = [&](glm::vec2 const &pt, float radius, glm::u8vec4 tint) {
		data.emplace_back(pt + glm::vec2(-radius, -radius), tint, glm::vec2(0.0f, 0.0f));
		data.emplace_back(data.back());
		data.emplace_back(pt + glm::vec2(-radius,  radius), tint, glm::vec2(0.0f, 1.0f));
		data.emplace_back(pt + glm::vec2( radius, -radius), tint, glm::vec2(1.0f, 0.0f));
		data.emplace_back(pt + glm::vec2( radius,  radius), tint, glm::vec2(1.0f, 1.0f));
		data.emplace_back(data.back());
	};

	for (auto &id_stroke : strokes) {
		auto &stroke = id_stroke.second;
		assert(!stroke.points.empty());

		//pressure sensitivity functions:
		auto compute_radius = [&](float z) {
			return std::max(0.5f, z * brush.radius);
		};
		auto compute_tint = [&](float z) {
			return brush.tint;
		};

		if (stroke.remain == 0.0f) {
			//special case for stroke start:
			stroke.remain = compute_radius(stroke.points[0].z) * brush.interval;
			splat(glm::vec2(stroke.points[0]), compute_radius(stroke.points[0].z), compute_tint(stroke.points[0].z));
		}

		for (uint32_t i = 0; i + 1 < stroke.points.size(); ++i) {
			glm::vec3 const &a = stroke.points[i];
			glm::vec3 const &b = stroke.points[i+1];

			//moving along the line from a-b, the remaining distance before a splat decreases by length,
			//and increases by the difference in step size at both ends:
			float step = -glm::length(glm::vec2(b)-glm::vec2(a))
				+ (compute_radius(b.z) * brush.interval - compute_radius(a.z) * brush.interval);

			float t = 0.0f;
			while (stroke.remain + (1.0f - t) * step < 0.0f) {
				float d = -stroke.remain / step;
				t += d;
				assert(t <= 1.0001f);
				float z = glm::mix(a.z, b.z, t);
				stroke.remain = compute_radius(z) * brush.interval;
				assert(stroke.remain > 0.0f);
				splat(glm::mix(glm::vec2(a), glm::vec2(b), t), compute_radius(z), compute_tint(z));
			}
			stroke.remain += (1.0f - t) * step;
		}
		stroke.points.erase(stroke.points.begin(), stroke.points.end()-1);
		assert(stroke.points.size() == 1);
	}

	if (data.empty()) return; //no splats to draw

	//send all splats to GPU:
	splat_buffer.set(data, GL_STREAM_DRAW);

	glEnable(GL_BLEND);
	glBlendEquation(GL_FUNC_ADD);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	//draw splats to framebuffer:
	glBindFramebuffer(GL_FRAMEBUFFER, canvas_fb);
	glViewport(0,0,canvas_size.x, canvas_size.y);

	//draw canvas:
	glUseProgram(texture_program->program);

	{ //set transformation matrix to scale from splats in pixel space to [-1,1] coordinates:
		glm::mat4 px_to_canvas = glm::mat4(
			glm::vec4(2.0f / canvas_size.x, 0.0f, 0.0f, 0.0f),
			glm::vec4(0.0f, 2.0f / canvas_size.y, 0.0f, 0.0f),
			glm::vec4(0.0f, 0.0f, 1.0f, 0.0f),
			glm::vec4(-1.0f,-1.0f, 0.0f, 1.0f)
		);
		glUniformMatrix4fv(texture_program_mvp, 1, GL_FALSE, glm::value_ptr(px_to_canvas));
	}

	//send vertex attributes for canvas:
	glBindVertexArray(texture_program_splat_buffer.array);
	glBindTexture(GL_TEXTURE_2D, brush.tex);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, splat_buffer.count);
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindVertexArray(0);

	glUseProgram(0);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	GL_ERRORS();

}
