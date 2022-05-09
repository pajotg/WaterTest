#include "Pipe.hpp"

Pipe::Pipe() : FlowVolume(0) {}
Pipe::Pipe(const Pipe& From)
{
	this->operator=(From);
}

Pipe::~Pipe() { }

Pipe& Pipe::operator = (const Pipe& From)
{
	FlowVolume = From.FlowVolume;

	// return the existing object so we can chain this operator
	return *this;
}

void Pipe::Update(const SimulationVariables& Variables, float Height, float HeightOut)
{
	float New = FlowVolume + Variables.DT * Variables.GRAVITY * (Height - HeightOut) / Variables.PIPE_LENGTH;

	FlowVolume = std::max(0.0f, New);
}

void Pipe::ScaleBack(float K)
{
	FlowVolume *= K;
}