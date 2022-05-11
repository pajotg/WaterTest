#ifndef Cell_HPP
#define Cell_HPP

#include <ostream>
#include "Pipe.hpp"

// A Slight modification from https://hal.inria.fr/inria-00402079/document
// Only sediment transport is changed, maybe i implemented the transport wrong, but now every bit of terrain is preserved, Also no local tilt angle


class Cell {
	public:
		float TerrainHeight;
		float WaterHeight;
		float Sediment;

		Cell();
		Cell(const Cell& From);

		virtual ~Cell();

		Cell& operator = (const Cell& From);

		virtual float GetVelocityMagnitude() const = 0;
		float GetHeightChange(const SimulationVariables& Variables, const Cell& Other, float Distance) const;
		float GetLiquidHeight() const;
		float GetCombinedHeight() const;
		float GetSedimentTransportCapacity(const SimulationVariables& Variables) const;

		float GetVolumePR(const SimulationVariables& Variables, float Volume);
		float GetWaterForVolume(const SimulationVariables& Variables, float Volume);
		float GetSedimentForVolume(const SimulationVariables& Variables, float Volume);

		void UpdateRainfall(const SimulationVariables& Variables, float Rainfall);

		void FinishWaterSurfaceAndSediment();

		void UpdateErosionAndDeposition(const SimulationVariables& Variables);

		void UpdateEvaporation(const SimulationVariables& Variables);
	protected:
		float TempTerrainHeight;
		float TempWaterHeight;
		float TempSediment;
		
};

#endif
