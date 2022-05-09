#include "Cell2D.hpp"
#include <cmath>
#include <thread>

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

typedef (*CallFunc)(const SimulationVariables& Variables, int i);

struct CallRange
{
	const SimulationVariables& Variables;
	int StartIndex;
	int EndIndex;
	CallFunc Func;

	void Run()
	{
		for (int i = Data.StartIndex; i < Data.EndIndex; i++)
			Data.Func(Data.Variables, i);
	}

	CallRange(const SimulationVariables& Variables, int StartIndex, int EndIndex, CallFunc Func) : Variables(Variables), StartIndex(StartIndex), EndIndex(EndIndex), Func(Func) { }
}

static void RunThreaded(const SimulationVariables& Variables, Cell2D* Ptr, int SizeX, int SizeY, int NumThreads, CallFunc Func)
{
	std::vector<CallRange> Ranges;

	for (int y = 1; y < SizeY - 1; y++)
		Ranges.push_back(CallRange(Variables, 1 + y * SizeY, SizeX - 1 + y * SizeY, Func));

	// TODO: Spawn threads, make each thread pop one range, stop once no more ranges exist, join all threads
}

void Cell2D::UpdateCells(const SimulationVariables& Variables, Cell2D* Ptr, int SizeX, int SizeY)
{
	for (int x = 1; x < SizeX - 1; x++)
		for (int y = 1; y < SizeY - 1; y++)
			Ptr[x + y * SizeX].UpdateRainfall(Variables, (std::rand() % 10) == 0 ? Variables.RAINFALL * 10 : 0);

	for (int x = 1; x < SizeX - 1; x++)
		for (int y = 1; y < SizeY - 1; y++)
		{
			int i = x + y * SizeX;
			Ptr[i].UpdatePipes(Variables, Ptr[i - 1], Ptr[i + 1], Ptr[i - SizeX], Ptr[i + SizeX]);
		}
	
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

	for (int x = 1; x < SizeX - 1; x++)
		for (int y = 1; y < SizeY - 1; y++)
		{
			int i = x + y * SizeX;
			Ptr[i].UpdateWaterSurfaceAndSediment(Variables, Ptr[i - 1], Ptr[i + 1], Ptr[i - SizeX], Ptr[i + SizeX]);
		}
	
	for (int x = 1; x < SizeX - 1; x++)
		for (int y = 1; y < SizeY - 1; y++)
			Ptr[x + y * SizeX].FinishWaterSurfaceAndSediment();

	for (int x = 1; x < SizeX - 1; x++)
		for (int y = 1; y < SizeY - 1; y++)
		Ptr[x + y * SizeX].UpdateErosionAndDeposition(Variables);

	for (int x = 1; x < SizeX - 1; x++)
		for (int y = 1; y < SizeY - 1; y++)
			Ptr[x + y * SizeX].UpdateEvaporation(Variables);
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