#include "PaintMode.hpp"

#include <kit/kit.hpp>
#include <kit/Load.hpp>

#include <fstream>
#include <sstream>

kit::Config kit_config() {
	kit::Config config;
	config.size = glm::uvec2(1000, 700);
	config.title = "Kit-Paint";

	return config;
}

std::shared_ptr< kit::Mode > kit_mode() {
	kit::call_load_functions(); //only needed if using kit::Load< > for resource loading

	//you can parse arguments here:
	//for (uint32_t argi = 1; argi < kit::args.size(); ++argi) ...

	return std::make_shared< PaintMode >();
}

