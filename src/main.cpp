#include <iostream>
#include <cmath>

extern "C" {
	#include "MLX42.h"
}

#include "Cell1D.hpp"
#include "Cell2D.hpp"
#include "SimulationVariables.hpp"

#include <chrono>
#include <thread>

void DoCell1DTest(const SimulationVariables& Variables)
{
	const int SIZE = 128;
	
	Cell1D Cells[SIZE];

	for (int i = 0; i < SIZE; i++)
	{
		int Offset = std::abs(i - (SIZE / 2));
		Cells[i].TerrainHeight = Offset * Offset * Variables.PIPE_LENGTH / (SIZE * SIZE / 4) * 32;
		//Cells[i].TerrainHeight = (i + std::rand() % 5) * Variables.PIPE_LENGTH / 2;
	}

	//for (int i = 0; i < SIZE; i++)
	//	Cells[i].TerrainHeight += Variables.PIPE_LENGTH * 2 + (std::abs(std::rand()) % 2) * Variables.PIPE_LENGTH;

	Cells[SIZE / 2].TerrainHeight += Variables.PIPE_LENGTH * 8;
	Cells[SIZE / 2 - 1].TerrainHeight += Variables.PIPE_LENGTH * 9;
	Cells[SIZE / 2 - 2].TerrainHeight += Variables.PIPE_LENGTH * 10;

	//RAINFALL = 0;
	Cells[SIZE - 2].WaterHeight = Variables.PIPE_LENGTH * 96;
	//Cells[SIZE - 2].Sediment = Variables.PIPE_LENGTH * 32;
	//Cells[1].WaterHeight = Variables.PIPE_LENGTH * 16;

	
	for (int IterCount = 0; true; IterCount++)
	{
		Cell1D::DrawCells(Variables, Cells, SIZE, 40, 1.0f);

		for (int i = 0; i < 10; i++)
			Cell1D::UpdateCells(Variables, Cells, SIZE);

		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
}

struct HookData
{
	mlx_t* mlx;
	mlx_image_t *img;

	const SimulationVariables& Variables;
	Cell2D* Cells;

	const int SIZEX;
	const int SIZEY;

	HookData(mlx_t* mlx, mlx_image_t *img, const SimulationVariables& Variables, Cell2D* Cells, int SIZEX, const int SIZEY) : mlx(mlx), img(img), Variables(Variables), Cells(Cells), SIZEX(SIZEX), SIZEY(SIZEY) { }
};

static void	hook(void *param)
{
	HookData	*data = (HookData*)param;

	if (mlx_is_key_down(data->mlx, MLX_KEY_ESCAPE))
		mlx_close_window(data->mlx);

	Cell2D::UpdateCells(data->Variables, data->Cells, data->SIZEX, data->SIZEY);
	Cell2D::DrawImage(data->img, data->Cells, data->SIZEX, data->SIZEY, 1);
	
	//for (unsigned int x = 0; x < data->img->width; x++)
	//	for(unsigned int y= 0; y < data->img->height; y++)
	//		mlx_put_pixel(data->img, x, y, rand() % RAND_MAX);
}

void DoCell2DTest(const SimulationVariables& Variables)
{
	const int SIZEX = 512;
	const int SIZEY = 512;
	Cell2D* Cells = new Cell2D[SIZEX * SIZEY];

	float CenterX = SIZEX / 2.0f;
	float CenterY = SIZEY / 2.0f;

	float Max = CenterX * CenterX + CenterY * CenterY;

	float Radius = SIZEX / 4;

	for (int x = 0; x < SIZEX; x++)
		for (int y = 0; y < SIZEY; y++)
		{
			float OX = x - CenterX;
			float OY = y - CenterY;

			float SqrDist = OX * OX + OY * OY;

			float BowlShape = SqrDist / Max;
			float IslandShape = (Radius * Radius - SqrDist) / (Radius * Radius);
			if (IslandShape < 0) IslandShape = 0;

			Cells[x + y * SIZEX].TerrainHeight = (BowlShape / 4) + (IslandShape / 4 * 3);
		}

	//Cells[SIZEX / 2 + SIZEY / 2 * SIZEX].WaterHeight = 1;

	mlx_t* mlx = mlx_init(SIZEX, SIZEY, "Water sim", false);
	if (!mlx)
		exit(EXIT_FAILURE);
	mlx_image_t *img = mlx_new_image(mlx, SIZEX, SIZEY);
	memset(img->pixels, 255, img->width * img->height * sizeof(int));
	mlx_image_to_window(mlx, img, 0, 0);

	HookData Data(mlx, img, Variables, Cells, SIZEX, SIZEY);
	mlx_loop_hook(mlx, &hook, &Data);
	mlx_loop(mlx);
	mlx_terminate(mlx);
}

int main()
{
	std::srand(0);

	SimulationVariables Variables;
	//DoCell1DTest(Variables);

	Variables.DT /= 2;
	DoCell2DTest(Variables);
}