#ifndef Cell2D_HPP
#define Cell2D_HPP

#include <ostream>
#include "Pipe.hpp"
#include "Cell.hpp"

extern "C" {
	#include "MLX42.h"
}

class Cell2D : public Cell {
	public:
		std::pair<float, float> Velocity;
		Pipe Left;
		Pipe Right;
		Pipe Up;
		Pipe Down;

		Cell2D();
		Cell2D(const Cell2D& From);

		virtual ~Cell2D();

		Cell2D& operator = (const Cell2D& From);

		virtual float GetVelocityMagnitude() const;

		void UpdatePipes(const SimulationVariables& Variables, Cell2D& LeftCell, Cell2D& RightCell, Cell2D& UpCell, Cell2D& DownCell);
		void UpdateWaterSurfaceAndSediment(const SimulationVariables& Variables, Cell2D& LeftCell, Cell2D& RightCell, Cell2D& UpCell, Cell2D& DownCell);

		static void UpdateCells(const SimulationVariables& Variables, Cell2D* Ptr, int SizeX, int SizeY);
		static void DrawImage(mlx_image_t* img, Cell2D* Ptr, int SizeX, int SizeY, float HeightScale);
};

#endif
