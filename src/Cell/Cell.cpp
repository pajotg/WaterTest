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

float Cell::GetHeightChange(const SimulationVariables& Variables, const Cell& Other, float Distance) const {
	float Diff = TerrainHeight - Other.TerrainHeight;
	float Step = Variables.MAX_STEP * Distance;

	if (Diff > Step)
		return (Step - Diff) / 2;
	else if (Diff < -Step)
		return (- Step - Diff) / 2;
	return 0;
}

//float Cell::GetLiquidHeight() const { return WaterHeight; }	// How its in the paper
float Cell::GetLiquidHeight() const { return WaterHeight + Sediment; }	// My modification (I had waves of sediment moving upstream, makes no sense, the reason i thought it occurred was that the velocity at the wave tip went down, sediment deposited, the height increased, and then gravity moves it forward again, This change makes it so that depositing sediment does NOT increase the height, thus no weird upstream waves, Try increasing DEPOSITION_CONSTANT to 10 and SEDIMENT_CAPACITY to 0.15, and use the original GetLiquidHeight() for the effect)

float Cell::GetCombinedHeight() const { return TerrainHeight + GetLiquidHeight(); }
float Cell::GetSedimentTransportCapacity(const SimulationVariables& Variables) const { return Variables.SEDIMENT_CAPACITY * GetVelocityMagnitude(); }	// Note: This does not take into account the local tilt angle, should be multiplied by sin(angle)

float Cell::GetVolumePR(const SimulationVariables& Variables, float Volume)
{
	float CurrentWaterVolume = GetLiquidHeight() * Variables.PIPE_LENGTH * Variables.PIPE_LENGTH;
	if (CurrentWaterVolume <= 0)
		return 0;

	return Volume / CurrentWaterVolume;
}
float Cell::GetWaterForVolume(const SimulationVariables& Variables, float Volume)
{
	return GetVolumePR(Variables, Volume) * WaterHeight;
}
float Cell::GetSedimentForVolume(const SimulationVariables& Variables, float Volume) {
	return GetVolumePR(Variables, Volume) * Sediment;
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