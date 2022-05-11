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
	mlx_image_t *zoom_img;

	SimulationVariables& Variables;
	Cell2D* Cells;

	const int SIZEX;
	const int SIZEY;
	const int ZOOM_SIZE;
	const int ZOOM_SCALE;

	HookData(mlx_t* mlx, mlx_image_t *img, mlx_image_t *zoom_img, SimulationVariables& Variables, Cell2D* Cells, int SIZEX, const int SIZEY, int ZOOM_SIZE, int ZOOM_SCALE) : mlx(mlx), img(img), zoom_img(zoom_img), Variables(Variables), Cells(Cells), SIZEX(SIZEX), SIZEY(SIZEY), ZOOM_SIZE(ZOOM_SIZE), ZOOM_SCALE(ZOOM_SCALE) { }
};

template<class T>
void Apply(Cell2D* Cells, int SizeX, int SizeY, int x, int y, int Range, T ApplyFunc)
{
	int LowX = std::max(x - Range, 1);
	int HighX = std::min(x + Range, SizeX - 1);
	int LowY = std::max(y - Range, 1);
	int HighY = std::min(y + Range, SizeY - 1);

	float MaxSqrDist = Range * Range;

	for (int ix = LowX; ix < HighX; ix++)
		for (int iy = LowY; iy < HighY; iy++)
		{
			float OX = ix - x;
			float OY = iy - y;

			float SqrDist = OX * OX + OY * OY;

			float Strength = 1 - (SqrDist / MaxSqrDist);
			if (Strength < 0)
				continue;

			ApplyFunc(Cells[ix + iy * SizeX], Strength);
		}
}

static void	hook(void *param)
{
	HookData	*data = (HookData*)param;

	if (mlx_is_key_down(data->mlx, MLX_KEY_ESCAPE))
		mlx_close_window(data->mlx);
	
	if (mlx_is_key_down(data->mlx, MLX_KEY_DOWN))
		data->Variables.RAINFALL /= 1.1f;
	if (mlx_is_key_down(data->mlx, MLX_KEY_UP))
	{
		if (data->Variables.RAINFALL <= 0)
			data->Variables.RAINFALL = 0.0000001f;
		data->Variables.RAINFALL *= 1.1f;
	}

	if (mlx_is_key_down(data->mlx, MLX_KEY_E))
		data->Variables.MAX_STEP /= 1.1f;
	if (mlx_is_key_down(data->mlx, MLX_KEY_D))
	{
		if (data->Variables.MAX_STEP <= 0)
			data->Variables.MAX_STEP = 0.0000001f;
		data->Variables.MAX_STEP *= 1.1f;
	}

	int32_t x, y;
	mlx_get_mouse_pos(data->mlx, &x, &y);
	if (x >= 0 && y >= 0 && x < data->SIZEX && y < data->SIZEY)
	{
		int Range = 5;
		if (mlx_is_key_down(data->mlx, MLX_KEY_1))
			Range *= 5;
		if (mlx_is_key_down(data->mlx, MLX_KEY_2))
			Range *= 25;
		if (mlx_is_key_down(data->mlx, MLX_KEY_3))
			Range *= 50;
		if (mlx_is_key_down(data->mlx, MLX_KEY_4))
			Range *= 100;

		if (mlx_is_key_down(data->mlx, MLX_KEY_Q)) {
			float TargetHeight = data->Cells[x + y * data->SIZEX].TerrainHeight;

			Apply(data->Cells, data->SIZEX, data->SIZEY, x, y, Range, [TargetHeight](Cell2D& Cell, float Strength) { Cell.TerrainHeight += (TargetHeight - Cell.TerrainHeight) * Strength; });
		} else if (mlx_is_key_down(data->mlx, MLX_KEY_SPACE)) {
			if (mlx_is_mouse_down(data->mlx, MLX_MOUSE_BUTTON_LEFT))
				Apply(data->Cells, data->SIZEX, data->SIZEY, x, y, Range, [](Cell2D& Cell, float Strength) { Cell.TerrainHeight += Strength / 5; });
			if (mlx_is_mouse_down(data->mlx, MLX_MOUSE_BUTTON_RIGHT))
				Apply(data->Cells, data->SIZEX, data->SIZEY, x, y, Range, [](Cell2D& Cell, float Strength) { Cell.TerrainHeight -= Strength / 5; });
		} else if (mlx_is_key_down(data->mlx, MLX_KEY_W)) {
			if (mlx_is_mouse_down(data->mlx, MLX_MOUSE_BUTTON_LEFT))
				Apply(data->Cells, data->SIZEX, data->SIZEY, x, y, Range, [](Cell2D& Cell, float Strength) { Cell.Sediment += Strength / 5; });
			if (mlx_is_mouse_down(data->mlx, MLX_MOUSE_BUTTON_RIGHT))
				Apply(data->Cells, data->SIZEX, data->SIZEY, x, y, Range, [](Cell2D& Cell, float Strength) { Cell.Sediment *= 1 - Strength; });
		} else {
			if (mlx_is_mouse_down(data->mlx, MLX_MOUSE_BUTTON_LEFT))
				Apply(data->Cells, data->SIZEX, data->SIZEY, x, y, Range, [](Cell2D& Cell, float Strength) { Cell.WaterHeight += Strength / 5; });
			if (mlx_is_mouse_down(data->mlx, MLX_MOUSE_BUTTON_RIGHT))
				Apply(data->Cells, data->SIZEX, data->SIZEY, x, y, Range, [](Cell2D& Cell, float Strength) { Cell.WaterHeight *= 1 - Strength; });
		}
	}

	Cell2D::UpdateCells(data->Variables, data->Cells, data->SIZEX, data->SIZEY);
	Cell2D::DrawImage(data->Variables, data->img, data->Cells, data->SIZEX, data->SIZEY, 0, -10);

	if (x >= 0 && y >= 0 && x < data->SIZEX && y < data->SIZEY)
	{
		int lx = std::max(0, std::min(x - data->ZOOM_SIZE / 2, data->SIZEX - data->ZOOM_SIZE));
		int ly = std::max(0, std::min(y - data->ZOOM_SIZE / 2, data->SIZEY - data->ZOOM_SIZE));
		int hx = lx + data->ZOOM_SIZE;
		int hy = ly + data->ZOOM_SIZE;
		Cell2D::DrawImage(data->Variables, data->zoom_img, data->Cells, data->SIZEX, data->SIZEY, 0, 0, data->ZOOM_SCALE, lx, ly, hx, hy);
	}


	
	//for (unsigned int x = 0; x < data->img->width; x++)
	//	for(unsigned int y= 0; y < data->img->height; y++)
	//		mlx_put_pixel(data->img, x, y, rand() % RAND_MAX);
}

void DoCell2DTest(SimulationVariables& Variables)
{
	const int SIZEX = 256;
	const int SIZEY = 256;
	const int ZOOM_SIZE = 32;
	const int ZOOM_SCALE = 8;

	Cell2D* Cells = new Cell2D[SIZEX * SIZEY];

	float CenterX = SIZEX / 2.0f;
	float CenterY = SIZEY;//SIZEY / 2.0f;

	float MaxDistX = std::max(CenterX, SIZEX - CenterX);
	float MaxDistY = std::max(CenterY, SIZEY - CenterY);
	float SqrMax = MaxDistX * MaxDistX + MaxDistY * MaxDistY;
	float Max = sqrt(SqrMax);

	float Radius = SIZEX / 6;

	for (int x = 0; x < SIZEX; x++)
		for (int y = 0; y < SIZEY; y++)
		{
			float OX = x - CenterX;
			float OY = y - CenterY;

			float SqrDist = OX * OX + OY * OY;

			float BowlShape = SqrDist / SqrMax;
			float Dist = sqrt(SqrDist);
			float InvertedBowlShape = (SqrMax - (Dist - Max) * (Dist - Max)) / SqrMax;

			float IslandShape = (Radius * Radius - SqrDist) / (Radius * Radius);
			if (IslandShape < 0) IslandShape = 0;
			IslandShape = 0;

			//Cells[x + y * SIZEX].TerrainHeight = (BowlShape + IslandShape) * SIZEX * Variables.PIPE_LENGTH / 4;
			Cells[x + y * SIZEX].TerrainHeight = InvertedBowlShape * SIZEX * Variables.PIPE_LENGTH / 4;
		}

	for (int x = 0; x < SIZEX; x++)
		for (int y = 0; y < SIZEY; y++)
			Cells[x + y * SIZEX].TerrainHeight *= 1 + ((float)rand() / RAND_MAX) / 10;

	//Cells[SIZEX / 2 + SIZEY / 2 * SIZEX].WaterHeight = 1;

	mlx_t* mlx = mlx_init(SIZEX + ZOOM_SIZE * ZOOM_SCALE, SIZEY, "Water sim", false);
	if (!mlx)
		exit(EXIT_FAILURE);
	mlx_image_t *img = mlx_new_image(mlx, SIZEX, SIZEY);
	mlx_image_t *zoom_img = mlx_new_image(mlx, ZOOM_SIZE * ZOOM_SCALE, ZOOM_SIZE * ZOOM_SCALE);
	memset(img->pixels, 255, img->width * img->height * sizeof(int));
	memset(zoom_img->pixels, 255, zoom_img->width * zoom_img->height * sizeof(int));

	mlx_image_to_window(mlx, img, 0, 0);
	mlx_image_to_window(mlx, zoom_img, img->width, 0);

	HookData Data(mlx, img, zoom_img, Variables, Cells, SIZEX, SIZEY, ZOOM_SIZE, ZOOM_SCALE);
	mlx_loop_hook(mlx, &hook, &Data);
	mlx_loop(mlx);
	mlx_terminate(mlx);
}

int main()
{
	std::srand(0);

	SimulationVariables Variables;

	/*
	std::cout << "MaxStep: " << Variables.MAX_STEP << std::endl;
	std::cout << "Aka: " << std::atan(Variables.MAX_STEP / Variables.PIPE_LENGTH) << " radians" << std::endl;
	std::cout << "Aka: " << std::atan(Variables.MAX_STEP / Variables.PIPE_LENGTH) / M_PI * 180 << " degrees" << std::endl;
	*/

	//DoCell1DTest(Variables);

	Variables.DT /= 2;
	Variables.RAINFALL /= 20;
	DoCell2DTest(Variables);
}