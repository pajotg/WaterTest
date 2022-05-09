#ifndef SIMULATIONVARIABLES_HPP
#define SIMULATIONVARIABLES_HPP

#include <ostream>

struct SimulationVariables {
	float RAINFALL = 0.2f;
	float EVAPORATION = 1.0f;
	float DT = 0.004f;
	float GRAVITY = 9.81f;
	float PIPE_LENGTH = 0.1f;
	float SEDIMENT_CAPACITY = 1.0f;
	float DISSOLVE_CONSTANT = 4.0f;
	float DEPOSITION_CONSTANT = 4.0f;
};

#endif
