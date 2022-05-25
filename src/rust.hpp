#pragma once

// Just for my editor
#ifndef COMPILE
#define RUST
#endif

#ifdef RUST

struct RustSimulationVariables {
	size_t thread_count;
	float rainfall;
	float evaporation;
	float dt;
	float gravity;
	float pipe_length;
	float sediment_capacity;
	float dissolve_constant;
	float deposition_constant;
	float max_step;
	int rain_random;
};

struct RustWaterSimulator {
	void* magic;
};

extern "C" void print_stuff_in_rust();

extern "C" RustSimulationVariables new_simulation_variables();
extern "C" RustWaterSimulator new_simulation(RustSimulationVariables Variables, size_t size_x, size_t size_y);

extern "C" void step_simulation(void* magic);

extern "C" float get_terrain_height(void* magic, size_t x, size_t y);
extern "C" float get_water_height(void* magic, size_t x, size_t y);
extern "C" float get_sediment_height(void* magic, size_t x, size_t y);

#endif