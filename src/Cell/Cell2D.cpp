#include "Cell2D.hpp"
#include <cmath>
#include <thread>
#include <vector>
#include <atomic>

Cell2D::Cell2D() : Cell(), Velocity(), Left(), Right(), Up(), Down() { }
Cell2D::Cell2D(const Cell2D& From)
{
	this->operator=(From);
}

Cell2D::~Cell2D() { }

Cell2D& Cell2D::operator = (const Cell2D& From)
{
	static_cast<Cell&>(*this) = From;

	Velocity = From.Velocity;
	Left = From.Left;
	Right = From.Right;

	// return the existing object so we can chain this operator
	return *this;
}

float Cell2D::GetVelocityMagnitude() const
{
	return std::sqrt(Velocity.first * Velocity.first + Velocity.second * Velocity.second);
}

void Cell2D::UpdatePipes(const SimulationVariables& Variables, Cell2D& LeftCell, Cell2D& RightCell, Cell2D& UpCell, Cell2D& DownCell)
{
	float Height = GetCombinedHeight();
	Left.Update(Variables, Height, LeftCell.GetCombinedHeight());
	Right.Update(Variables, Height, RightCell.GetCombinedHeight());
	Up.Update(Variables, Height, UpCell.GetCombinedHeight());
	Down.Update(Variables, Height, DownCell.GetCombinedHeight());

	float Total = Left.FlowVolume + Right.FlowVolume + Up.FlowVolume + Down.FlowVolume;

	float CurrentVolume = WaterHeight * Variables.PIPE_LENGTH * Variables.PIPE_LENGTH;
	float K = std::min(1.0f, CurrentVolume / (Total * Variables.DT));
	if (std::isinf(K))
		K = 0;

	Left.ScaleBack(K);
	Right.ScaleBack(K);
	Up.ScaleBack(K);
	Down.ScaleBack(K);
}

void Cell2D::UpdateWaterSurfaceAndSediment(const SimulationVariables& Variables, Cell2D& LeftCell, Cell2D& RightCell, Cell2D& UpCell, Cell2D& DownCell)
{
	float VolumeChange = LeftCell.Right.FlowVolume + RightCell.Left.FlowVolume + DownCell.Up.FlowVolume + UpCell.Down.FlowVolume
						- Left.FlowVolume - Right.FlowVolume - Up.FlowVolume - Down.FlowVolume;
	VolumeChange *= Variables.DT;
	
	TempWaterHeight = WaterHeight + VolumeChange / (Variables.PIPE_LENGTH * Variables.PIPE_LENGTH);
	float VelocityX = (LeftCell.Right.FlowVolume - Left.FlowVolume - RightCell.Left.FlowVolume + Right.FlowVolume) / 2;
	float VelocityY = (DownCell.Up.FlowVolume - Down.FlowVolume - UpCell.Down.FlowVolume + Up.FlowVolume) / 2;
	Velocity = std::make_pair(VelocityX, VelocityY);

	TempSediment = Sediment - GetSedimentForWaterVolume(Variables, (Left.FlowVolume + Right.FlowVolume + Up.FlowVolume + Down.FlowVolume) * Variables.DT)
		+ LeftCell.GetSedimentForWaterVolume(Variables, LeftCell.Right.FlowVolume * Variables.DT) + RightCell.GetSedimentForWaterVolume(Variables, RightCell.Left.FlowVolume * Variables.DT)
		+ UpCell.GetSedimentForWaterVolume(Variables, UpCell.Down.FlowVolume * Variables.DT) + DownCell.GetSedimentForWaterVolume(Variables, DownCell.Up.FlowVolume * Variables.DT);
}

template<class T>
struct CallRange
{
	int StartIndex;
	int EndIndex;
	T Func;

	void Run()
	{
		for (int i = StartIndex; i < EndIndex; i++)
			Func(i);
	}

	CallRange(int StartIndex, int EndIndex, T Func) : StartIndex(StartIndex), EndIndex(EndIndex), Func(Func) { }
};

#include <iostream>
template<class T>
static void RunThreaded(const SimulationVariables& Variables, Cell2D* Ptr, int SizeX, int SizeY, int NumThreads, T Func)
{
	std::vector<CallRange<T>> Ranges;

	for (int y = 1; y < SizeY - 1; y++)
		Ranges.push_back(CallRange<T>(1 + y * SizeY, SizeX - 1 + y * SizeY, Func));

	std::atomic_size_t Current(0);

	std::thread* Threads = new std::thread[NumThreads];
	for (int i = 0; i < NumThreads; i++)
		Threads[i] = std::thread([&]() {	// I dont like that i have to use pointers, i'd rather use a reference
			while (true)
			{
				size_t I = Current.fetch_add(1);
				if (I >= Ranges.size())
					break;

				Ranges[I].Run();
			}
		});
	
	for (int i = 0; i < NumThreads; i++)
		Threads[i].join();
	delete[] Threads;
}

void Cell2D::UpdateCells(const SimulationVariables& Variables, Cell2D* Ptr, int SizeX, int SizeY)
{
	// Lambdas are AWESOME!
	auto RunFunc = [&](auto Lambda) { RunThreaded(Variables, Ptr, SizeX, SizeY, std::thread::hardware_concurrency(), Lambda); };

	RunFunc([&](int i) { Ptr[i].UpdateRainfall(Variables, (std::rand() % 10) == 0 ? Variables.RAINFALL * 10 : 0); });
	RunFunc([&](int i) { Ptr[i].UpdatePipes(Variables, Ptr[i - 1], Ptr[i + 1], Ptr[i - SizeX], Ptr[i + SizeX]); });
	
	// Boundary condition
	for (int x = 1; x < SizeX - 1; x++)
	{
		Ptr[x + 1 * SizeX].Left.FlowVolume = 0;
		Ptr[x + (SizeY - 2) * SizeX].Right.FlowVolume = 0;
	}
	for (int y = 1; y < SizeY - 1; y++)
	{
		Ptr[1 + y * SizeX].Up.FlowVolume = 0;
		Ptr[SizeX - 2 + y * SizeX].Down.FlowVolume = 0;
	}
	
	RunFunc([&](int i) { Ptr[i].UpdateWaterSurfaceAndSediment(Variables, Ptr[i - 1], Ptr[i + 1], Ptr[i - SizeX], Ptr[i + SizeX]); });
	RunFunc([&](int i) { Ptr[i].FinishWaterSurfaceAndSediment(); });
	RunFunc([&](int i) { Ptr[i].UpdateErosionAndDeposition(Variables); });
	RunFunc([&](int i) { Ptr[i].UpdateEvaporation(Variables); });
}

static float clamp(float v, float min, float max)
{
	if (v < min)
		return min;
	else if (v > max)
		return max;
	return v;
}
static unsigned char ToByte(float f)
{
	return floor(clamp(f, 0, 1) * 255);
}
static float lerp(float a, float b, float t)
{
	return a + (b - a) * t;
}

static int ToColor(float r, float g, float b)
{
	return ToByte(r) << 24 | ToByte(g) << 16 | ToByte(b) << 8 | 255;
}

void Cell2D::DrawImage(mlx_image_t* img, Cell2D* Ptr, int SizeX, int SizeY, float HeightScale)
{
	for (int x = 0; x < SizeX; x++)
		for (int y = 0; y < SizeY; y++)
		{
			int i = x + y * SizeX;

			float WaterHeight = Ptr[i].WaterHeight * HeightScale;
			float TerrainHeight = Ptr[i].TerrainHeight * HeightScale;
			float SedimentHeight = Ptr[i].Sediment * HeightScale;

			float r = Ptr[i].TerrainHeight;
			float g = Ptr[i].TerrainHeight;
			float b = Ptr[i].TerrainHeight;

			float WaterPR = clamp(WaterHeight * 5, 0, 1);
			float SedimentPR = clamp(WaterHeight * 5, 0, 1);
			
			b = lerp(b, 1, WaterPR);
			g = lerp(g, 1, SedimentPR);

			mlx_put_pixel(img, x, y, ToColor(r, g, b));
		}
}