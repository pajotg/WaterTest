#include "Cell.hpp"
#include <cmath>

Cell::Cell() : TerrainHeight(0), WaterHeight(0), Sediment(0), TempWaterHeight(0), TempSediment(0) { }
Cell::Cell(const Cell& From)
{
	this->operator=(From);
}

Cell::~Cell() { }

Cell& Cell::operator = (const Cell& From)
{
	TerrainHeight = From.TerrainHeight;
	WaterHeight = From.WaterHeight;
	Sediment = From.Sediment;
	TempWaterHeight = From.TempWaterHeight;
	TempSediment = From.TempSediment;

	// return the existing object so we can chain this operator
	return *this;
}

float Cell::GetHeightChange(const SimulationVariables& Variables, const Cell& Other) const {
	float Diff = TerrainHeight - Other.TerrainHeight;
	if (Diff > Variables.MAX_STEP)
		return (Variables.MAX_STEP - Diff) / 2;
	else if (Diff < -Variables.MAX_STEP)
		return (- Variables.MAX_STEP - Diff) / 2;
	return 0;
}

float Cell::GetCombinedHeight() const { return TerrainHeight + WaterHeight; }
float Cell::GetSedimentTransportCapacity(const SimulationVariables& Variables) const { return Variables.SEDIMENT_CAPACITY * GetVelocityMagnitude(); }	// Note: This does not take into account the local tilt angle, should be multiplied by sin(angle)
float Cell::GetSedimentForWaterVolume(const SimulationVariables& Variables, float WaterVolume) {
	float CurrentWaterVolume = WaterHeight * Variables.PIPE_LENGTH * Variables.PIPE_LENGTH;
	if (CurrentWaterVolume <= 0)
		return 0;

	float GetPR = WaterVolume / CurrentWaterVolume;

	//if (std::isnan(GetPR) || std::isinf(GetPR))	// Can't check for WaterHeight <= 0, Cause WaterHeight * PIPE_LENGTH * PIPE_LENGTH may still be 0 due to rounding
	//	return 0;

	return GetPR * Sediment;
}

void Cell::UpdateRainfall(const SimulationVariables& Variables, float Rainfall)
{
	if ((std::rand() % Variables.RainRandom) == 0)
		WaterHeight += Rainfall * Variables.RainRandom * Variables.DT;
}

void Cell::FinishWaterSurfaceAndSediment()
{
	// Is max really needed?
	WaterHeight = std::max(TempWaterHeight, 0.0f);
	Sediment = std::max(TempSediment, 0.0f);
	TerrainHeight = TempTerrainHeight;
}

void Cell::UpdateErosionAndDeposition(const SimulationVariables& Variables)
{
	float STC = GetSedimentTransportCapacity(Variables);
	
	float Diff = STC - Sediment;

	float SedimentChange = Diff > 0 ? Diff * Variables.DISSOLVE_CONSTANT : Diff * Variables.DEPOSITION_CONSTANT;
	SedimentChange *= Variables.DT;

	TerrainHeight -= SedimentChange;
	Sediment += SedimentChange;
}

void Cell::UpdateEvaporation(const SimulationVariables& Variables)
{
	WaterHeight *= 1 - Variables.EVAPORATION * Variables.DT;
	//float RemovedSediment = Sediment * Variables.EVAPORATION * Variables.DT;

	//Sediment -= RemovedSediment;
	//TerrainHeight += RemovedSediment;
}