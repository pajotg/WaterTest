#ifndef SIMULATIONVARIABLES_HPP
#define SIMULATIONVARIABLES_HPP

#include <ostream>
#include <cmath>

struct SimulationVariables {
	float RAINFALL = 0.4f;
	float EVAPORATION = 0.05f;
	float DT = 0.1f;
	float GRAVITY = 9.81f;
	float PIPE_LENGTH = 1.0f;
	float SEDIMENT_CAPACITY = 0.15f;
	float DISSOLVE_CONSTANT = 0.025f;
	float DEPOSITION_CONSTANT = 10.0f;
	float MAX_STEP = std::tan(35 * M_PI / 180) * PIPE_LENGTH;
	int RainRandom = 10;
};

#endif
