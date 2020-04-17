#include "GPUResource.h"
#include "D3D12DeviceContext.h"
#include "GPUMemoryPage.h"
#include "Core\Performance\PerfManager.h"
#include "D3D12CommandList.h"
//todo: More Detailed Error checking!

GPUResource::GPUResource()
{
}

GPUResource::GPUResource(ID3D12Resource* Target, D3D12_RESOURCE_STATES InitalState) :GPUResource(Target, InitalState, RHI::GetDefaultDevice())
{
}

GPUResource::GPUResource(ID3D12Resource* Target, D3D12_RESOURCE_STATES InitalState, DeviceContext* device)
{

	resource = Target;
	SetName(L"GPU Resource");
	SetDebugName("GPU Resource");
	CurrentResourceState = InitalState;
	TargetState = CurrentResourceState;
	Device = D3D12RHI::DXConv(device);
	PerfManager::Get()->AddTimer("ResourceTransitons", "RHI");
}

GPUResource::~GPUResource()
{
	if (!IsReleased)
	{
		Release();
	}
}

void GPUResource::SetName(LPCWSTR name)
{
	resource->SetName(name);
}

void GPUResource::Evict()
{
	ensure(currentState != EResourcePageState::Evicted);
	ID3D12Pageable* Pageableresource = resource;
	ThrowIfFailed(Device->GetDevice()->Evict(1, &Pageableresource));
	currentState = EResourcePageState::Evicted;
}

void GPUResource::MakeResident()
{
	ensure(currentState != EResourcePageState::Resident);
	ID3D12Pageable* Pageableresource = resource;
	ThrowIfFailed(Device->GetDevice()->MakeResident(1, &Pageableresource));
	currentState = EResourcePageState::Resident;
}

bool GPUResource::IsResident()
{
	return (currentState == EResourcePageState::Resident);
}

EResourcePageState::Type GPUResource::GetState()
{
	return currentState;
}

bool GPUResource::IsValidStateForList(D3D12CommandList* List)
{
	if (CurrentResourceState == D3D12_RESOURCE_STATE_COMMON)
	{
		return true;
	}
	if (List->IsGraphicsList())
	{
		return true;
	}
	if (List->IsComputeList() || List->IsRaytracingList())
	{
		return CurrentResourceState | ~D3D12_RESOURCE_STATE_DEPTH_WRITE || CurrentResourceState | ~D3D12_RESOURCE_STATE_RENDER_TARGET;
	}
	if (List->IsCopyList())
	{

	}
	return false;
}

void GPUResource::SetResourceState(D3D12CommandList* List, D3D12_RESOURCE_STATES newstate, EResourceTransitionMode::Type Mode/* = EResourceTransitionMode::Direct*/, int SubResource/* = -1*/)
{
	if (newstate != CurrentResourceState || SubResource != -1 || SubResourceStates.size())
	{
#if LOG_RESOURCE_TRANSITIONS
		Log::LogMessage("GPU" + std::to_string(Device->GetDeviceIndex()) + ": Transition: Resource \"" + std::string(GetDebugName()) + "\" From " +
			D3D12Helpers::ResouceStateToString(CurrentResourceState) + " TO " + D3D12Helpers::ResouceStateToString(newstate));
#endif
		
		if (Mode == EResourceTransitionMode::Direct)
		{
			ensure(!IsTransitioning());
			if (SubResource != -1)
			{
				SubResouceState* pState = nullptr;
				for (int i = 0; i < SubResourceStates.size(); i++)
				{
					if (SubResourceStates[i].Index == SubResource)
					{
						pState = &SubResourceStates[i];
						break;
					}
				}
				if (pState == nullptr)
				{
					SubResouceState State;
					State.Index = SubResource;
					State.State = CurrentResourceState;
					SubResourceStates.push_back(State);
					pState = &SubResourceStates[SubResourceStates.size() - 1];
				}
				List->AddTransition(CD3DX12_RESOURCE_BARRIER::Transition(resource, pState->State, newstate, SubResource));
				pState->State = newstate;
			}
			if (SubResource == -1 && SubResourceStates.size())
			{
				for (int i = 0; i < SubResourceStates.size(); i++)
				{
					List->AddTransition(CD3DX12_RESOURCE_BARRIER::Transition(resource, SubResourceStates[i].State, CurrentResourceState, SubResourceStates[i].Index));
				}
				SubResourceStates.clear();
				CurrentResourceState = newstate;
				TargetState = newstate;
			}
			if (SubResource == -1 && newstate != CurrentResourceState)
			{
				List->AddTransition(CD3DX12_RESOURCE_BARRIER::Transition(resource, CurrentResourceState, newstate, SubResource));
				CurrentResourceState = newstate;
				TargetState = newstate;
			}
		}
		else if (Mode == EResourceTransitionMode::Start)
		{
			ensure(SubResource == -1);
			ensure(!IsTransitioning());
			StartResourceTransition(List, newstate);
		}
		else if (Mode == EResourceTransitionMode::End)
		{
			ensure(SubResource == -1);
			ensure(IsTransitioning());
			EndResourceTransition(List, newstate);
		}

		PerfManager::Get()->AddToCountTimer("ResourceTransitons", 1);
	}
}

void GPUResource::StartResourceTransition(D3D12CommandList* List, D3D12_RESOURCE_STATES newstate)
{
	if (newstate != CurrentResourceState)
	{
		D3D12_RESOURCE_BARRIER BarrierDesc = {};
		BarrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		BarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_BEGIN_ONLY;
		BarrierDesc.Transition.StateBefore = CurrentResourceState;
		BarrierDesc.Transition.StateAfter = newstate;
		BarrierDesc.Transition.pResource = resource;
		List->AddTransition(BarrierDesc);
		TargetState = newstate;
	}
}

void GPUResource::EndResourceTransition(D3D12CommandList* List, D3D12_RESOURCE_STATES newstate)
{
	if (newstate != CurrentResourceState)
	{
		D3D12_RESOURCE_BARRIER BarrierDesc = {};
		BarrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		BarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_END_ONLY;
		BarrierDesc.Transition.StateBefore = CurrentResourceState;
		BarrierDesc.Transition.StateAfter = newstate;
		BarrierDesc.Transition.pResource = resource;
		List->AddTransition(BarrierDesc);
		CurrentResourceState = newstate;
	}
}
bool GPUResource::IsTransitioning()
{
	return (CurrentResourceState != TargetState);
}

void GPUResource::SetGPUPage(GPUMemoryPage* page)
{
	Page = page;
}

void GPUResource::UpdateUnTrackedState(D3D12_RESOURCE_STATES newstate)
{
	CurrentResourceState = newstate;
	TargetState = newstate;
}

EPhysicalMemoryState::Type GPUResource::GetCurrentAliasState() const { return CurrentAliasState; }

void GPUResource::SetCurrentAliasState(EPhysicalMemoryState::Type val) { CurrentAliasState = val; }

bool GPUResource::NeedsClear() const
{
	return CurrentAliasState == EPhysicalMemoryState::Active_NoClear;
}

void GPUResource::NotifyClearComplete()
{
	ensure(NeedsClear());
	CurrentAliasState = EPhysicalMemoryState::Active_NoClear;
}

ResourceMipInfo* GPUResource::GetMipData(int index)
{
	return &m_mips[index];
}

void GPUResource::SetupMipMapping()
{
	UINT numTiles = 0;
	D3D12_PACKED_MIP_INFO m_packedMipInfo;
	D3D12_TILE_SHAPE tileShape;
	UINT subresourceCount = GetDesc().MipLevels;
	std::vector<D3D12_SUBRESOURCE_TILING> tilings(subresourceCount);
	Device->GetDevice()->GetResourceTiling(GetResource(), &numTiles, &m_packedMipInfo, &tileShape, &subresourceCount, 0, &tilings[0]);


	m_mips.resize(subresourceCount);
	UINT heapCount = m_packedMipInfo.NumStandardMips + (m_packedMipInfo.NumPackedMips > 0 ? 1 : 0);
	for (UINT n = 0; n < m_mips.size(); n++)
	{
		int SubIndex = n;
		if (SubIndex < m_packedMipInfo.NumStandardMips)
		{
			m_mips[n].heapIndex = SubIndex;
			m_mips[n].packedMip = false;
			m_mips[n].mapped = false;
			m_mips[n].startCoordinate = CD3DX12_TILED_RESOURCE_COORDINATE(0, 0, 0, n);
			m_mips[n].regionSize.Width = tilings[n].WidthInTiles;
			m_mips[n].regionSize.Height = tilings[n].HeightInTiles;
			m_mips[n].regionSize.Depth = tilings[n].DepthInTiles;
			m_mips[n].regionSize.NumTiles = tilings[n].WidthInTiles * tilings[n].HeightInTiles * tilings[n].DepthInTiles;
			m_mips[n].regionSize.UseBox = TRUE;
		}
		else
		{
			// All of the packed mips will go into the last heap.
			m_mips[n].heapIndex = heapCount - 1;
			m_mips[n].packedMip = true;
			m_mips[n].mapped = false;

			// Mark all of the packed mips as having the same start coordinate and size.
			m_mips[n].startCoordinate = CD3DX12_TILED_RESOURCE_COORDINATE(0, 0, 0, heapCount - 1);
			m_mips[n].regionSize.NumTiles = m_packedMipInfo.NumTilesForPackedMips;
			m_mips[n].regionSize.UseBox = FALSE;    // regionSize.Width/Height/Depth will be ignored.
		}
	}
}

D3D12_RESOURCE_STATES GPUResource::GetCurrentState()
{
	ensure(!IsTransitioning());
	return CurrentResourceState;
}

ID3D12Resource* GPUResource::GetResource()
{
	return resource;
}

void GPUResource::Release()
{
	IRHIResourse::Release();
	if (Page != nullptr)
	{
		Page->Deallocate(this);
	}
	//if the driver crashes here then (most likely) there is a resource contention issue with gpu 0 and 1 
	//where GPU will move forward and delete resources before GPU 1 has finished with resource.
	SafeRelease(resource);	
}


