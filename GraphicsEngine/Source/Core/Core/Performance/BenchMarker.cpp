
#include "BenchMarker.h"
#include "PerfManager.h"
#include "Core/Assets/AssetManager.h"
#include "Core/Utils/FileUtils.h"
#include "RHI/RHI.h"
#include <algorithm>


BenchMarker::BenchMarker()
{
	CoreStats[ECoreStatName::FrameTime] = new PerformanceLogStat("Frame");
	CoreStats[ECoreStatName::FrameRate] = new PerformanceLogStat("Frame Rate");
	CoreStats[ECoreStatName::CPU] = new PerformanceLogStat("CPU");
	CoreStats[ECoreStatName::GPU] = new PerformanceLogStat("GPU");
}

BenchMarker::~BenchMarker()
{
#if _DEBUG
	WriteSummaryToDisk();
#endif
}

void BenchMarker::TickBenchMarker()
{
	if (CurrentMode == EBenchMarkerMode::Off || CurrentMode == EBenchMarkerMode::Limit)
	{
		return;
	}
	CapturePerfMarkers();
}

void BenchMarker::StartBenchMark()
{
	StartCapture();
	CurrentMode = EBenchMarkerMode::RunningBench;
}

void BenchMarker::StopBenchMark()
{
	EndCapture();
}

void BenchMarker::StartCapture()
{
	Log::LogMessage("Performance Capture Started");
	CurrentMode = EBenchMarkerMode::Capturing;
	SummaryOutputFileName = AssetManager::GetGeneratedDir();
	CSV = new FileUtils::CSVWriter(AssetManager::GetGeneratedDir() + "\\PerfData_" + FileSuffix + ".csv");
}

void BenchMarker::EndCapture()
{
	if (CurrentMode != EBenchMarkerMode::Off)
	{
		Log::LogMessage("Performance Capture finished");
		WriteSummaryToDisk(true);
		WriteCSV(false);
		StatLogs.clear();
	}
	CurrentMode = EBenchMarkerMode::Off;
}

void BenchMarker::WriteStat(int statid, float value)
{
	BenchMarker::PerformanceLogStat* stat = nullptr;
	std::map<int, PerformanceLogStat*>::iterator finditor = StatLogs.find(statid);
	if (finditor != StatLogs.end())
	{
		stat = finditor->second;
		stat->AddData(value);
	}
	else
	{
		stat = new BenchMarker::PerformanceLogStat();
		stat->id = statid;
		TimerData* data = PerfManager::Get()->GetTimerData(statid);
		stat->name = data->name;
		if (data->IsGPUTimer)
		{
			stat->name.append(" (GPU)");
		}
		stat->AddData(value);
		StatLogs.emplace(statid, stat);
	}
}

void BenchMarker::WriteCoreStat(ECoreStatName::Type stat, float value)
{
	if (CoreStats[stat] != nullptr)
	{
		CoreStats[stat]->AddData(value);
	}
}

void BenchMarker::SetTestFileSufix(std::string suffix)
{
	FileSuffix = suffix;
	SafeDelete(CSV);
	CSV = new FileUtils::CSVWriter(AssetManager::GetGeneratedDir() + "\\PerfData_" + FileSuffix + ".csv");
}

void BenchMarker::CapturePerfMarkers()
{
	PerfManager::Get()->WriteLogStreams(true);
}

void BenchMarker::WriteFullStatsHeader(bool OnlyCoreStats)
{
	for (int id = 0; id < ECoreStatName::Limit; id++)
	{
		CSV->AddEntry(CoreStats[id]->name);
	}
	if (!OnlyCoreStats)
	{
		for (std::map<int, PerformanceLogStat*>::iterator it = StatLogs.begin(); it != StatLogs.end(); ++it)
		{
			//write the header data
			CSV->AddEntry(it->second->name);
		}
	}
	CSV->AddLineBreak();
}

void BenchMarker::WriteSummaryToDisk(bool log /*= false*/)
{
	std::string summary = "";
	for (int i = 0; i < ECoreStatName::Limit; i++)
	{
		summary += GetCoreTimerSummary((ECoreStatName::Type)i);
	}
	for (int i = 0; i < MAX_GPU_DEVICE_COUNT; i++)
	{
		summary += GetTimerSummary("GPU" + std::to_string(i) + "_GRAPHICS_PC");
		summary += GetTimerSummary("GPU" + std::to_string(i) + "_GRAPHICS_CLOCK");
		summary += GetTimerSummary("MGPU Copy" + std::to_string(i));
		summary += GetTimerSummary("Shadow Copy" + std::to_string(i));
		summary += GetTimerSummary("2Shadow Copy2" + std::to_string(i));
		summary += GetTimerSummary("GPU0 Wait On GPU1" + std::to_string(i));
		summary += GetTimerSummary("TransferBytes" + std::to_string(i));
		summary += GetTimerSummary("Main Pass" + std::to_string(i));
		summary += GetTimerSummary("Point Shadow" + std::to_string(i));
		summary += GetTimerSummary("Shadow PreSample" + std::to_string(i));
	}
	FileUtils::WriteToFile(SummaryOutputFileName + "\\PerfLog_" + FileSuffix + ".txt", summary);
	if (log)
	{
		Log::LogMessage(summary);
	}
}

std::string BenchMarker::GetCoreTimerSummary(ECoreStatName::Type CoreStat)
{
	return ProcessTimerData(CoreStats[CoreStat]);
}

std::string BenchMarker::GetTimerSummary(std::string statname)
{
	return GetTimerSummary(PerfManager::Get()->GetTimerIDByName(statname));
}

std::string BenchMarker::GetTimerSummary(int Statid)
{
	std::string output = "";
	std::map<int, PerformanceLogStat*>::iterator finditor = StatLogs.find(Statid);
	if (finditor == StatLogs.end())
	{
		output = "No data\n";
		return output;
	}
	return ProcessTimerData(finditor->second, (finditor->second == CoreStats[ECoreStatName::FrameRate]));
}

void BenchMarker::WriteCSV(bool OnlyCoreStats)
{
	CSV->Clear();
	WriteFullStatsHeader(OnlyCoreStats);
	int LongestStatArray = 1;
	for (int i = 0; i < LongestStatArray; i++)
	{
		for (int id = 0; id < ECoreStatName::Limit; id++)
		{
			if (i < CoreStats[id]->GetData().size())
			{
				LongestStatArray = std::max(LongestStatArray, (int)CoreStats[id]->GetData().size());
				CSV->AddEntry(std::to_string(CoreStats[id]->GetData()[i]));
			}
		}
		if (!OnlyCoreStats)
		{
			for (std::map<int, PerformanceLogStat*>::iterator it = StatLogs.begin(); it != StatLogs.end(); ++it)
			{
				//write the header data
				if (i < it->second->GetData().size())
				{
					CSV->AddEntry(std::to_string(it->second->GetData()[i]));
					LongestStatArray = std::max(LongestStatArray, (int)it->second->GetData().size());
				}
			}
		}
		CSV->AddLineBreak();
	}
	CSV->Save();
}

float Percentile(std::vector<float> sequence, float excelPercentile)
{
	std::sort(sequence.begin(), sequence.end());
	int N = (int)sequence.size();
	float n = (N - 1) * excelPercentile + 1;
	// Another method: double n = (N + 1) * excelPercentile;
	if (n == 1.0f) return sequence[0];
	else if (n == N) return sequence[N - 1];
	else
	{
		int k = (int)n;
		float d = n - k;
		return sequence[k - 1] + d * (sequence[k] - sequence[k - 1]);
	}
}

std::string BenchMarker::ProcessTimerData(PerformanceLogStat * PLS, bool Flip /*= false*/)
{
	std::vector<float> data = PLS->GetData();
	if (data.size() == 0)
	{
		return "NoData";
	}
	float AVG = 0;
	float Max = 0;
	float Min = FLT_MAX;
	for (int i = 0; i < data.size(); i++)
	{
		if (i == 0)
		{
			continue;
		}
		AVG += data[i];
		Max = glm::max(Max, data[i]);
		Min = fminf(Min, data[i]);
	}
	AVG /= data.size() - 1;
	std::stringstream stream;
	stream << std::fixed << std::setprecision(3) << "Name: " << PLS->name
		<< " AVG: " << AVG << " Min: " << Min << " Max: " << Max << " 1% Low: "
		<< Percentile(data, Flip ? 0.01f : 0.99f) << " 0.1% Low: " << Percentile(data, Flip ? 0.001f : 0.999f) << " \n";
	return stream.str();
}

