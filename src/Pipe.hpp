#ifndef PIPE_HPP
#define PIPE_HPP

#include <ostream>
#include "SimulationVariables.hpp"

struct Pipe {
	float FlowVolume;	// AKA: Flux

	Pipe();
	Pipe(const Pipe& From);

	~Pipe();

	Pipe& operator = (const Pipe& From);

	void Update(const SimulationVariables& Variables, float Height, float HeightOut);
	void ScaleBack(float K);
};

#endif
