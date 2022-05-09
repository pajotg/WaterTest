#ifndef Cell1D_HPP
#define Cell1D_HPP

#include <ostream>
#include "Pipe.hpp"
#include "Cell.hpp"

class Cell1D : public Cell {
	public:
		float Velocity;
		Pipe Left;
		Pipe Right;

		Cell1D();
		Cell1D(const Cell1D& From);

		virtual ~Cell1D();

		Cell1D& operator = (const Cell1D& From);

		virtual float GetVelocityMagnitude() const;

		void UpdatePipes(const SimulationVariables& Variables, Cell1D& LeftCell, Cell1D& RightCell);

		/*
		void UpdateWaterSurface(const SimulationVariables& Variables, Cell1D& LeftCell, Cell1D& RightCell);
		*/

		void UpdateWaterSurfaceAndSediment(const SimulationVariables& Variables, Cell1D& LeftCell, Cell1D& RightCell);

		/*
		void UpdateSedimentTransport(Cell1D* Ptr, int Size, int Index);
		void FinishSedimentTransport();
		*/

		static void UpdateCells(const SimulationVariables& Variables, Cell1D* Ptr, int Size);
		static void DrawCells(const SimulationVariables& Variables, Cell1D* Ptr, int Size, int NumPartitions, float HeightScale = 1);
};

#endif
