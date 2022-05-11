#include "Cell1D.hpp"
#include <cmath>
#include <iostream>

#include <chrono>
#include <thread>

Cell1D::Cell1D() : Cell(), Velocity(), Left(), Right() { }
Cell1D::Cell1D(const Cell1D& From)
{
	this->operator=(From);
}

Cell1D::~Cell1D() { }

Cell1D& Cell1D::operator = (const Cell1D& From)
{
	static_cast<Cell&>(*this) = From;

	Velocity = From.Velocity;
	Left = From.Left;
	Right = From.Right;

	// return the existing object so we can chain this operator
	return *this;
}

float Cell1D::GetVelocityMagnitude() const
{
	return std::abs(Velocity);
}

void Cell1D::UpdatePipes(const SimulationVariables& Variables, Cell1D& LeftCell, Cell1D& RightCell)
{
	//std::cout << "Cell: " << std::endl;

	//std::cout << "\tTerrainHeight: " << TerrainHeight << std::endl;
	//std::cout << "\tWaterHeight: " << WaterHeight << std::endl;

	float Height = GetCombinedHeight();
	Left.Update(Variables, Height, LeftCell.GetCombinedHeight());
	Right.Update(Variables, Height, RightCell.GetCombinedHeight());

	float Total = Left.FlowVolume + Right.FlowVolume;
	//std::cout << "\tTotal Flux: " << Total << std::endl;

	float CurrentVolume = WaterHeight * Variables.PIPE_LENGTH * Variables.PIPE_LENGTH;
	//std::cout << "\tVolume: " << CurrentVolume << std::endl;
	float K = std::min(1.0f, CurrentVolume / (Total * Variables.DT));
	if (std::isinf(K))
		K = 0;

	Left.ScaleBack(K);
	Right.ScaleBack(K);
}

/*
void Cell1D::UpdateWaterSurface(const SimulationVariables& Variables, Cell1D& LeftCell, Cell1D& RightCell)
{
	float VolumeChange = LeftCell.Right.FlowVolume + RightCell.Left.FlowVolume
						- Left.FlowVolume - Right.FlowVolume;
	VolumeChange *= Variables.DT;
	
	WaterHeight += VolumeChange / (Variables.PIPE_LENGTH * Variables.PIPE_LENGTH);
	Velocity = (LeftCell.Right.FlowVolume - Left.FlowVolume - RightCell.Left.FlowVolume + Right.FlowVolume) / 2;
}
*/

void Cell1D::UpdateWaterSurfaceAndSediment(const SimulationVariables& Variables, Cell1D& LeftCell, Cell1D& RightCell)
{
	TempWaterHeight = WaterHeight 
		+ LeftCell.GetWaterForVolume(Variables, LeftCell.Right.FlowVolume * Variables.DT) + RightCell.GetWaterForVolume(Variables, RightCell.Left.FlowVolume * Variables.DT)
		- GetWaterForVolume(Variables, (Left.FlowVolume + Right.FlowVolume) * Variables.DT);

	// Now for the sediment
	TempSediment = Sediment
		+ LeftCell.GetSedimentForVolume(Variables, LeftCell.Right.FlowVolume * Variables.DT) + RightCell.GetSedimentForVolume(Variables, RightCell.Left.FlowVolume * Variables.DT)
		- GetSedimentForVolume(Variables, (Left.FlowVolume + Right.FlowVolume) * Variables.DT);

	Velocity = (LeftCell.Right.FlowVolume - Left.FlowVolume - RightCell.Left.FlowVolume + Right.FlowVolume) / 2;

	TempTerrainHeight = TerrainHeight;	// No steepness function
}

/*
void Cell1D::UpdateSedimentTransport(Cell1D* Ptr, int Size, int Index)
{
	float Offset = -Velocity * DT;

	float PR = Offset - std::floor(Offset);

	int LowIndex = (int)std::floor(Index - Offset);
	int HighIndex = LowIndex + 1;

	float LowSediment = (LowIndex >= 0 && LowIndex < Size ? Ptr[LowIndex].Sediment : 0);
	float HighSediment = (HighIndex >= 0 && HighIndex < Size ? Ptr[HighIndex].Sediment : 0);

	TempSediment = LowSediment + (HighSediment - LowSediment) * PR;
}

void Cell1D::FinishSedimentTransport()
{
	Sediment = TempSediment;
}
*/


void Cell1D::UpdateCells(const SimulationVariables& Variables, Cell1D* Ptr, int Size)
{
	for (int i = 1; i < Size - 1; i++)
		//Ptr[i].UpdateRainfall(RAINFALL);
		Ptr[i].UpdateRainfall(Variables, i > Size / 2 ? Variables.RAINFALL : 0);

	for (int i = 1; i < Size - 1; i++)
		Ptr[i].UpdatePipes(Variables, Ptr[i - 1], Ptr[i + 1]);
	
	// Boundary condition
	Ptr[1].Left.FlowVolume = 0;
	Ptr[Size - 2].Right.FlowVolume = 0;

	/*
	for (int i = 1; i < Size - 1; i++)
		Ptr[i].UpdateWaterSurface(Variables, Ptr[i - 1], Ptr[i + 1]);
	*/

	for (int i = 1; i < Size - 1; i++)
		Ptr[i].UpdateWaterSurfaceAndSediment(Variables, Ptr[i - 1], Ptr[i + 1]);
	for (int i = 1; i < Size - 1; i++)
		Ptr[i].FinishWaterSurfaceAndSediment();

	for (int i = 1; i < Size - 1; i++)
		Ptr[i].UpdateErosionAndDeposition(Variables);

	/*
	for (int i = 1; i < Size - 1; i++)
		Ptr[i].FinishSedimentTransport();
	*/

	for (int i = 1; i < Size - 1; i++)
		Ptr[i].UpdateEvaporation(Variables);
}

/*
static std::string GetHeightPrintDouble(Cell1D* Ptr, int Size, float Height)
{
	std::string PrintA = "";
	std::string PrintB = "";
	for (int i = 0; i < Size; i++)
	{
		Cell1D& Curr = Ptr[i];

		if (std::isnan(Curr.WaterHeight))
		{
			PrintA += "!!";
			PrintB += "!!";
		}
		else if (Height < Curr.TerrainHeight)
		{
			PrintA += "┌┐";
			PrintB += "└┘";
		}
		else if (Height < Curr.TerrainHeight + Curr.Sediment)
		{
			PrintA += "..";
			PrintB += "..";
		}
		else if (Height < Curr.GetCombinedHeight())
		{
			char LeftChar = (Curr.Left.FlowVolume > 0) ? '<' : '|';
			char RightChar = (Curr.Right.FlowVolume > 0) ? '>' : '|';


			PrintA += LeftChar;
			PrintA += RightChar;

			PrintB += LeftChar;
			PrintB += RightChar;
		}
		else
		{
			PrintA += "  ";
			PrintB += "  ";
		}
	}

	return PrintA + "\n" + PrintB + "\n";
}
*/

static std::string GetHeightPrint(Cell1D* Ptr, int Size, float Height)
{
	std::string Print = "";
	for (int i = 0; i < Size; i++)
	{
		Cell1D& Curr = Ptr[i];

		if (std::isnan(Curr.WaterHeight))
			Print += "!";
		else if (Height < Curr.TerrainHeight)
			Print += "☐";
		else if (Height < Curr.TerrainHeight + Curr.Sediment)
			Print += ".";
		else if (Height < Curr.GetCombinedHeight())
		{
			if (Curr.Velocity > 0)
				Print += ">";
			else
				Print += "<";
		}
		else
			Print += " ";
	}

	return Print + "\n";
}

void Cell1D::DrawCells(const SimulationVariables& Variables, Cell1D* Ptr, int Size, int NumPartitions, float HeightScale)
{
	bool ErrorHeight = false;

	float TotalWaterVolume = 0;
	float TotalSedimentVolume = 0;
	float TotalTerrainVolume = 0;

	for (int i = 0; i < Size; i++)
	{
		TotalWaterVolume += Ptr[i].WaterHeight * Variables.PIPE_LENGTH * Variables.PIPE_LENGTH;
		TotalSedimentVolume += Ptr[i].Sediment * Variables.PIPE_LENGTH * Variables.PIPE_LENGTH;
		TotalTerrainVolume += Ptr[i].TerrainHeight * Variables.PIPE_LENGTH * Variables.PIPE_LENGTH;

		ErrorHeight |= Ptr[i].WaterHeight < 0 || Ptr[i].Sediment < 0;
	}

	std::string HeightPrint = "";
	for (int i = 0; i < NumPartitions; i++)
	{
		float Height = (NumPartitions - 1 - i) * Variables.PIPE_LENGTH * HeightScale;
		HeightPrint += GetHeightPrint(Ptr, Size, Height);
	}

	std::cout << HeightPrint;

	std::cout << "Water Volume: " << TotalWaterVolume << std::endl;
	std::cout << "Sediment Volume: " << TotalSedimentVolume << std::endl;
	std::cout << "Terrain Volume: " << TotalTerrainVolume << std::endl;
	std::cout << "Sediment + Terrain Volume: " << (TotalSedimentVolume + TotalTerrainVolume) << std::endl;

	if (ErrorHeight || std::isnan(TotalWaterVolume) || std::isnan(TotalSedimentVolume) || std::isnan(TotalTerrainVolume))
		std::this_thread::sleep_for(std::chrono::seconds(1));
}
