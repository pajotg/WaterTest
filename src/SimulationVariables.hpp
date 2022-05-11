#ifndef SIMULATIONVARIABLES_HPP
#define SIMULATIONVARIABLES_HPP

#include <ostream>
#include <cmath>

struct SimulationVariables {
	float RAINFALL = 0.2f;
	float EVAPORATION = 0.05f;
	float DT = 0.1f;
	float GRAVITY = 9.81f;
	float PIPE_LENGTH = 1.0f;
	float SEDIMENT_CAPACITY = 0.25f;
	float DISSOLVE_CONSTANT = 0.1f;
	float DEPOSITION_CONSTANT = 0.1f;
	float MAX_STEP = std::tan(35 * M_PI / 180) * PIPE_LENGTH;
	int RainRandom = 10;
};

#endif
