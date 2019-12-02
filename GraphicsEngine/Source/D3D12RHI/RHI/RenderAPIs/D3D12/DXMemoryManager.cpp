#include "DXMemoryManager.h"
#include "GPUResource.h"
#include "D3D12DeviceContext.h"
#include "GPUMemoryPage.h"
#include "Core\Maths\Math.h"
static ConsoleVariable LogAllocations("VMEM.LogPages", 0, ECVarType::ConsoleAndLaunch);
static ConsoleVariable MemReport("VMEM.Report", ECVarType::ConsoleAndLaunch, [] { D3D12RHI::DXConv(RHI::GetDefaultDevice())->GetMemoryManager()->LogMemoryReport(); });
static ConsoleVariable FullMemReport("VMEM.Reportall", ECVarType::ConsoleAndLaunch, [] { D3D12RHI::DXConv(RHI::GetDefaultDevice())->GetMemoryManager()->LogMemoryReport(true); });
DXMemoryManager::DXMemoryManager(D3D12DeviceContext * D)
{
	Device = D;
	DeviceMemoryData Stats = D->GetMemoryData();
	Log::LogMessage("Booting On Device With " + StringUtils::ByteToGB(Stats.LocalSegment_TotalBytes) + "Local " +
		StringUtils::ByteToGB(Stats.HostSegment_TotalBytes) + "Host");

	AddPage(Math::MBToBytes<int>(256));
}

void DXMemoryManager::Compact()
{
	for (int i = AllocatedPages.size() - 1; i >= 0; i--)
	{
		if (AllocatedPages[i]->GetSizeInUse() == 0)
		{
			SafeDelete(AllocatedPages[i]);
			AllocatedPages.erase(AllocatedPages.begin() + i);
		}
	}
}

void DXMemoryManager::AddPage(int size)
{
	AllocDesc A = AllocDesc(size);
	A.PageAllocationType = EPageTypes::BufferUploadOnly;
	A.Name = "Default Page";
	GPUMemoryPage* Page = nullptr;
	AllocPage(A, &Page);
}

DXMemoryManager::~DXMemoryManager()
{
	MemoryUtils::DeleteVector(AllocatedPages);
}

EAllocateResult::Type DXMemoryManager::AllocUploadTemporary(AllocDesc & desc, GPUResource ** ppResource)
{
	desc.PageAllocationType = EPageTypes::BufferUploadOnly;
	desc.Segment = EGPUMemorysegment::Non_Local;
	return AllocResource(desc, ppResource);
}

EAllocateResult::Type DXMemoryManager::AllocTemporaryGPU(AllocDesc & desc, GPUResource ** ppResource)
{
	return AllocResource(desc, ppResource);
}

EAllocateResult::Type DXMemoryManager::AllocResource(AllocDesc & desc, GPUResource ** ppResource)
{
	desc.PageAllocationType = EPageTypes::TexturesOnly;
	if (desc.ResourceDesc.Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET || desc.ResourceDesc.Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL)
	{
		desc.PageAllocationType = EPageTypes::RTAndDS_Only;
	}
	if (desc.ResourceDesc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER)
	{
		if (desc.Segment == EGPUMemorysegment::Local)
		{
			desc.PageAllocationType = EPageTypes::BuffersOnly;
		}
		else
		{
			desc.PageAllocationType = EPageTypes::BufferUploadOnly;
		}
	}
	EAllocateResult::Type Error = FindFreePage(desc, AllocatedPages)->Allocate(desc, ppResource);
	ensure(Error == EAllocateResult::OK);
	return Error;
}

EAllocateResult::Type DXMemoryManager::AllocPage(AllocDesc & desc, GPUMemoryPage ** Page)
{
	*Page = new GPUMemoryPage(desc, Device);
	if (LogAllocations.GetBoolValue())
	{
		Log::LogMessage("Allocating Page of Size: " + StringUtils::ByteToMB(desc.Size) + " Of segment " + EGPUMemorysegment::ToString(desc.Segment));
	}
	AllocatedPages.push_back(*Page);
	//#DXMM: checks!
	return EAllocateResult::OK;
}

void DXMemoryManager::UpdateTotalAlloc()
{
	TotalPageAllocated = 0;
	TotalPageUsed = 0;
	for (GPUMemoryPage* P : AllocatedPages)
	{
		TotalPageAllocated += P->GetSize(true);
		TotalPageUsed += P->GetSizeInUse(true);
	}
}


void DXMemoryManager::LogMemoryReport(bool LogResources /*= false*/)
{
	UpdateTotalAlloc();
	Log::LogMessage("***GPU" + std::to_string(Device->GetDeviceIndex()) + " Memory Report***");
	for (GPUMemoryPage* P : AllocatedPages)
	{
		P->LogReport(LogResources);
	}
	Log::LogMessage("Total " + StringUtils::ByteToMB(TotalPageUsed) + " of " + StringUtils::ByteToMB(TotalPageAllocated) + " allocated (" +
		StringUtils::ToString(((float)TotalPageUsed / TotalPageAllocated) * 100) + "%)");

	Log::LogMessage("Device Total " + StringUtils::ByteToMB(TotalPageAllocated) + " of " + StringUtils::ByteToMB(Device->GetMemoryData().LocalSegment_TotalBytes) +
		" (" + StringUtils::ToString(((float)TotalPageAllocated / Device->GetMemoryData().LocalSegment_TotalBytes) * 100) + "%)");
	Log::LogMessage("***End GPU" + std::to_string(Device->GetDeviceIndex()) + " Memory Report***");
}

GPUMemoryPage * DXMemoryManager::FindFreePage(AllocDesc & desc, std::vector<GPUMemoryPage*>& pages)
{
	for (int i = 0; i < pages.size(); i++)
	{
		pages[i]->CalculateSpaceNeeded(desc);
		if (pages[i]->CheckSpaceForResource(desc))
		{
			return pages[i];
		}
	}
	GPUMemoryPage* newpage = nullptr;
	const UINT64 PageMinSize = 256 * 1024 * 1024u;
	desc.Size = Math::Max(desc.TextureAllocData.SizeInBytes, PageMinSize);
	AllocPage(desc, &newpage);
	if (AllocatedPages != pages)
	{
		pages.push_back(newpage);
	}
	return newpage;
}

UINT64 DXMemoryManager::GetTotalAllocated() const
{
	return TotalPageUsed;
}

UINT64 DXMemoryManager::GetTotalReserved() const
{
	return TotalPageAllocated;
}

