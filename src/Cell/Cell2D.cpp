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
	//return std::sqrt(Velocity.first * Velocity.first + Velocity.second * Velocity.second);
	return std::abs(Velocity.first) + std::abs(Velocity.second);
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

	float SedimentChange = 
		LeftCell.GetSedimentForWaterVolume(Variables, LeftCell.Right.FlowVolume * Variables.DT) + RightCell.GetSedimentForWaterVolume(Variables, RightCell.Left.FlowVolume * Variables.DT)
		+ UpCell.GetSedimentForWaterVolume(Variables, UpCell.Down.FlowVolume * Variables.DT) + DownCell.GetSedimentForWaterVolume(Variables, DownCell.Up.FlowVolume * Variables.DT)
		- GetSedimentForWaterVolume(Variables, (Left.FlowVolume + Right.FlowVolume + Up.FlowVolume + Down.FlowVolume) * Variables.DT);

	TempSediment = Sediment + SedimentChange;

	float VelocityX = (LeftCell.Right.FlowVolume - Left.FlowVolume - RightCell.Left.FlowVolume + Right.FlowVolume) / 2;
	float VelocityY = (DownCell.Up.FlowVolume - Down.FlowVolume - UpCell.Down.FlowVolume + Up.FlowVolume) / 2;
	Velocity = std::make_pair(VelocityX, VelocityY);
}

void Cell2D::UpdateSteepness(const SimulationVariables& Variables, Cell2D& LeftCell, Cell2D& RightCell, Cell2D& UpCell, Cell2D& DownCell)
{
	float Change = (GetHeightChange(Variables, LeftCell) + GetHeightChange(Variables, RightCell) + GetHeightChange(Variables, UpCell) + GetHeightChange(Variables, DownCell)) / 4;
	TempTerrainHeight = TerrainHeight + Change;
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
		Ptr[x + 1 * SizeX].Up.FlowVolume = 0;
		Ptr[x + (SizeY - 2) * SizeX].Down.FlowVolume = 0;
	}
	for (int y = 1; y < SizeY - 1; y++)
	{
		Ptr[1 + y * SizeX].Left.FlowVolume = 0;
		Ptr[SizeX - 2 + y * SizeX].Right.FlowVolume = 0;
	}
	
	RunFunc([&](int i) { Ptr[i].UpdateWaterSurfaceAndSediment(Variables, Ptr[i - 1], Ptr[i + 1], Ptr[i - SizeX], Ptr[i + SizeX]); Ptr[i].UpdateSteepness(Variables, Ptr[i - 1], Ptr[i + 1], Ptr[i - SizeX], Ptr[i + SizeX]); });
	RunFunc([&](int i) { Ptr[i].FinishWaterSurfaceAndSediment(); Ptr[i].UpdateErosionAndDeposition(Variables); Ptr[i].UpdateEvaporation(Variables); });
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

static float map(float v, float MinIn, float MaxIn, float MinOut, float MaxOut)
{
	float ZeroToOne = (v - MinIn) / (MaxIn - MinIn);
	return MinOut + ZeroToOne * (MaxOut - MinOut);
}
static float sigmoid(float v)
{
	return v / (1 + std::abs(v));
}

static int ToColor(float r, float g, float b)
{
	return ToByte(r) << 24 | ToByte(g) << 16 | ToByte(b) << 8 | 255;
}

static std::pair<float, float> GetMinMax(Cell2D* Ptr, int SizeX, int SizeY, int StartX, int StartY, int EndX, int EndY)
{
	float Min = 100000;
	float Max = -Min;

	for (int x = StartX; x < EndX; x++)
		for (int y = StartY; y < EndY; y++)
		{
			int i = x + y * SizeX;

			Min = std::min(Min, Ptr[i].TerrainHeight);
			Max = std::max(Max, Ptr[i].TerrainHeight);
		}
	
	return std::make_pair(Min, Max);
}

void Cell2D::DrawImage(const SimulationVariables& Variables, mlx_image_t* img, Cell2D* Ptr, int SizeX, int SizeY, float Min, float Max, int PixelSize, int StartX, int StartY, int EndX, int EndY)
{
	if (EndX < 0) EndX += SizeX + 1;
	if (EndY < 0) EndY += SizeY + 1;

	if (Min >= Max)
	{
		float MinSize = Min - Max;

		auto MinMax = GetMinMax(Ptr, SizeX, SizeY, StartX, StartY, EndX, EndY);
		Min = MinMax.first;
		Max = std::max(Min + MinSize, MinMax.second);
	}

	//std::cout << "Draw (" << StartX << ", " << StartY << ") (" << EndX << ", " << EndY << ") x " << PixelSize << std::endl;

	for (int x = StartX; x < EndX; x++)
		for (int y = StartY; y < EndY; y++)
		{
			int i = x + y * SizeX;

			float TerrainHeight = map(Ptr[i].TerrainHeight, Min, Max, 0, 1);
			float SedimentHeight = Ptr[i].Sediment;
			float WaterHeight = Ptr[i].WaterHeight;

			float r = TerrainHeight;
			float g = TerrainHeight;
			float b = TerrainHeight;

			float WaterPR = sigmoid(WaterHeight * 3);
			float SedimentPR = sigmoid(SedimentHeight * 3);
			
			r = lerp(r, 0, WaterPR);
			g = lerp(g, 0, WaterPR);
			b = lerp(b, 1, WaterPR);

			r = lerp(r, 0, SedimentPR);
			g = lerp(g, 1, SedimentPR);
			b = lerp(b, 1, SedimentPR);

			/*
			float STC = Ptr[i].GetSedimentTransportCapacity(Variables);
			r = STC * 4;
			g = r;
			b = r;
			*/

			int Color = ToColor(r, g, b);

			int DrawX = (x - StartX) * PixelSize;
			int DrawY = (y - StartY) * PixelSize;
			for (int dx = 0; dx < PixelSize; dx++)
				for (int dy = 0; dy < PixelSize; dy++)
				{
					//if (PixelSize != 1)
					//	std::cout << "(" << DrawX << ", " << DrawY << ") / (" << dx << ", " << dy << ")" << std::endl;
					mlx_put_pixel(img, DrawX + dx, DrawY + dy, Color);
				}
		}
}