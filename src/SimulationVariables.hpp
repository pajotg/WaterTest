#ifndef SIMULATIONVARIABLES_HPP
#define SIMULATIONVARIABLES_HPP

#include <ostream>
#include <cmath>

struct SimulationVariables {
	float RAINFALL = 0.2f;
	float EVAPORATION = 1.0f;
	float DT = 0.004f;
	float GRAVITY = 9.81f;
	float PIPE_LENGTH = 0.1f;
	float SEDIMENT_CAPACITY = 0.8f;
	float DISSOLVE_CONSTANT = 0.5f;
	float DEPOSITION_CONSTANT = 5.5f;
	float MAX_STEP = std::tan(70 * M_PI / 180) * PIPE_LENGTH;
};

#endif
