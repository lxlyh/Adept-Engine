#pragma once
//this class is to wrap all d3d12 rhi stuff up in
//and handle object creation
//and prevent RHI.cpp from getting extremly large
#include <d3d12.h>
#include <DXGI1_4.h>
#include "d3dx12.h"
#include <vector>
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxguid.lib")
//#pragma comment(lib, "dxgidebug.lib")
#include "d3d12Shader.h"
#include "D3D12Texture.h"
#include "D3D12Helpers.h"
#define TEST_RHI 1

#define USEGPUTOGENMIPS_ATRUNTIME 0

class D3D12RHI 
#if TEST_RHI
	: public RHIClass
#endif
{
public:
	static D3D12RHI* Instance;
	D3D12RHI();
	~D3D12RHI();
	void InitContext();
	void DestroyContext();
	void PresentFrame();
	void ClearRenderTarget(ID3D12GraphicsCommandList * MainList);
	void RenderToScreen(ID3D12GraphicsCommandList * list);
	static void PreFrameSetUp(ID3D12GraphicsCommandList * list, D3D12Shader * Shader);
	void PreFrameSwap(ID3D12GraphicsCommandList* list);
	void SetScreenRenderTarget(ID3D12GraphicsCommandList * list);

	void DisplayDeviceDebug();
	std::string GetMemory();
	void ReportObjects();
	void LoadPipeLine();
	void CreateSwapChainRTs();
	void InitMipmaps();
	void InternalResizeSwapChain(int x, int y);
	void ReleaseSwapRTs();
	
	void CreateDepthStencil(int width, int height);
	void LoadAssets();
	void ToggleFullScreenState();
	void ExecSetUpList();
	void ReleaseUploadHeap();
	void AddUploadToUsed(ID3D12Resource * Target);

	void PostFrame(ID3D12GraphicsCommandList * list);
	void WaitForPreviousFrame();
	void FindAdaptors(IDXGIFactory2 * pFactory);
	void WaitForGpu();
	void MoveToNextFrame();
	static ID3D12Device* GetDevice();
	ID3D12GraphicsCommandList* m_SetupCommandList;
	int m_width = 100;
	int m_height = 100;
	float m_aspectRatio = 0.0f;
	bool HasSetup = false;
	bool IsFullScreen = false;
	class ShaderMipMap* MipmapShader = nullptr;
	ID3D12DescriptorHeap* BaseTextureHeap;
	ID3D12CommandQueue* GetCommandQueue();
	static DeviceContext* GetDeviceContext(int index = 0);

	static DeviceContext* GetDefaultDevice();

	void AddLinkedFrameBuffer(FrameBuffer* target);
	//helper fucntions 
	static void CheckFeatures(ID3D12Device * pDevice);
	static D3D_FEATURE_LEVEL GetMaxSupportedFeatureLevel(ID3D12Device * pDevice);
	void WaitForAllDevices();

	void ResizeSwapChain(int x, int y);
#if TEST_RHI
	 bool InitRHI(int w, int h) override;
	 bool DestoryRHI() override;
	 BaseTexture* CreateTexture(DeviceContext* Device = nullptr) override;
	 FrameBuffer* CreateFrameBuffer(DeviceContext* Device, RHIFrameBufferDesc& Desc) override;
	 ShaderProgramBase* CreateShaderProgam(DeviceContext* Device = nullptr) override;
	 RHITextureArray * CreateTextureArray(DeviceContext * Device, int Length) override;
	 RHIBuffer* CreateRHIBuffer(RHIBuffer::BufferType type, DeviceContext* Device = nullptr) override;
	 RHIUAV* CreateUAV(DeviceContext* Device = nullptr) override;
	 RHICommandList* CreateCommandList(ECommandListType::Type Type = ECommandListType::Graphics, DeviceContext* Device = nullptr)override;

	 void RHISwapBuffers() override;
	 void RHIRunFirstFrame() override;
#endif
private:
	class	DeviceContext* PrimaryDevice = nullptr;
	class	DeviceContext* SecondaryDevice = nullptr;
	void ExecList(ID3D12GraphicsCommandList * list, bool block = false);
	CD3DX12_VIEWPORT m_viewport;
	CD3DX12_RECT m_scissorRect;
	IDXGISwapChain3* m_swapChain;
	ID3D12Resource* m_SwaprenderTargets[RHI::CPUFrameCount];
	ID3D12DescriptorHeap* m_rtvHeap;
	ID3D12DescriptorHeap* m_dsvHeap;
	UINT m_rtvDescriptorSize;
	ID3D12Resource * m_depthStencil;
	bool RequestedResize = false;
	int newwidth = 0;
	int newheight = 0;

	// Synchronization objects.
	UINT m_frameIndex;
	HANDLE m_fenceEvent;                                        
	ID3D12Fence* m_fence;
	UINT64 M_ShadowFence = 0;
	ID3D12Fence* pShadowFence;
	UINT64 m_fenceValues[RHI::CPUFrameCount];

	//	HANDLE ShadowExechandle;
	ID3D12Debug* debugController;
	HANDLE m_VideoMemoryBudgetChange;
	DWORD m_BudgetNotificationCookie;
	int CPUAheadCount = 0;
	//todo : Better!
	std::vector<ID3D12Resource *> UsedUploadHeaps;
	class GPUResource* m_RenderTargetResources[RHI::CPUFrameCount];
	size_t usedVRAM = 0;
	size_t totalVRAM = 0;
	int PerfCounter = 0;
	std::vector<FrameBuffer*> FrameBuffersLinkedToSwapChain;

	bool Omce = false;
	HANDLE EventHandle = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	
};
#include "D3D12Helpers.h"
//helper functions!
static inline void ThrowIfFailed(HRESULT hr)
{
	if (FAILED(hr))
	{
		//__debugbreak();		
		ensureMsgf(hr== S_OK, + (std::string)D3D12Helpers::DXErrorCodeToString(hr));
	}
}
void GetHardwareAdapter(IDXGIFactory2* pFactory, IDXGIAdapter1** ppAdapter);
