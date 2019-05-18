#pragma once
//this class is to wrap all d3d12 rhi stuff up in
//and handle object creation
//and prevent RHI.cpp from getting extremely large
#include "d3d12Shader.h"
#include "D3D12Texture.h"
#include "D3D12Helpers.h"
#include "Core/Utils/RefChecker.h"
#include "Core/Module/ModuleManager.h"
#include "Core/Platform/PlatformCore.h"
#define FORCE_RENDER_PASS_USE 0
#define AFTERMATH 0
#if AFTERMATH
#include <GFSDK_Aftermath.h>
#endif
#define DRED 1

class D3D12DeviceContext;
class D3D12GPUSyncEvent;
class D3D12RHI : public RHIClass
{
public:
	D3D12RHI();
	virtual ~D3D12RHI();
	void SetScreenRenderTarget(ID3D12GraphicsCommandList * list);
	void RenderToScreen(ID3D12GraphicsCommandList * list);
	void ClearRenderTarget(ID3D12GraphicsCommandList * MainList);
	static D3D12RHI* Instance;
	void AddObjectToDeferredDeleteQueue(IUnknown * Target);
	static D3D12RHI* Get();
	static D3D_FEATURE_LEVEL GetMaxSupportedFeatureLevel(ID3D12Device * pDevice);
	void ReportDeviceData();
	static bool DetectGPUDebugger();
	//temp To be RHI'D	
	std::string ReportMemory() override;
	static void HandleDeviceFailure();

	virtual RHIPipeLineStateObject* CreatePSO(const RHIPipeLineStateDesc& Desc, DeviceContext * Device) override;
#if	ALLOW_RESOURCE_CAPTURE
	RHI_VIRTUAL void TriggerWriteBackResources() override;
#endif

	RHI_VIRTUAL RHIGPUSyncEvent* CreateSyncEvent(DeviceContextQueue::Type WaitingQueue, DeviceContextQueue::Type SignalQueue, DeviceContext * Device, DeviceContext * SignalDevice) override;
	void SubmitToVRComposter(FrameBuffer * fb, EEye::Type eye);
#if AFTERMATH
	void RunTheAfterMath();
	std::vector<GFSDK_Aftermath_ContextHandle> handles;
#endif
#if DRED
	void RunDred();
#endif
private:
	void DestroyContext();
	void PresentFrame();
	void DisplayDeviceDebug();
	void ReportObjects();
	void LoadPipeLine();
	void CreateSwapChainRTs();
	void ReleaseSwapRTs();
	void CreateDepthStencil(int width, int height);
	void InitSwapChain();
	void SetFullScreenState(bool state);
	void WaitForAllGPUS();
	void ResetAllGPUCopyEngines();
	void UpdateAllCopyEngines();
	void ExecSetUpList();
	void ReleaseUploadHeaps(bool force = false);

	bool FindAdaptors(IDXGIFactory2 * pFactory, bool ForceFind);

	ID3D12DescriptorHeap* BaseTextureHeap;
	bool InitRHI()override;
	bool InitWindow(int w, int h) override;
	bool DestoryRHI() override;
	DeviceContext* GetDefaultDevice() override;
	DeviceContext* GetDeviceContext(int index = 0) override;
	BaseTexture* CreateTexture(const RHITextureDesc& Desc, DeviceContext* Device = nullptr) override;
	FrameBuffer* CreateFrameBuffer(DeviceContext* Device, const RHIFrameBufferDesc& Desc) override;
	ShaderProgramBase* CreateShaderProgam(DeviceContext* Device = nullptr) override;
	RHITextureArray * CreateTextureArray(DeviceContext * Device, int Length) override;
	RHIBuffer* CreateRHIBuffer(ERHIBufferType::Type type, DeviceContext* Device = nullptr) override;
	RHIUAV* CreateUAV(DeviceContext* Device = nullptr) override;
	RHICommandList* CreateCommandList(ECommandListType::Type Type = ECommandListType::Graphics, DeviceContext* Device = nullptr)override;
	virtual void TriggerBackBufferScreenShot() override;

	void WaitForGPU() override;
	void RHISwapBuffers() override;
	void RHIRunFirstFrame() override;

	void ResizeSwapChain(int x, int y) override;
	
private:
	D3D12GPUSyncEvent* AsyncSync = nullptr;
	bool IsFullScreen = false;
	D3D12DeviceContext * GetPrimaryDevice();
	D3D12DeviceContext* GetSecondaryDevice();
	D3D12DeviceContext* GetThridDevice();
	IDXGIFactory4 * factory = nullptr;

	ID3D12Device * GetDisplayDevice();
	D3D12DeviceContext* DeviceContexts[MAX_GPU_DEVICE_COUNT] = { nullptr };
	ID3D12GraphicsCommandList* m_SetupCommandList;
	int m_width = 0;
	int m_height = 0;
	float m_aspectRatio = 0.0f;
	bool HasSetup = false;
	CD3DX12_VIEWPORT m_viewport;
	CD3DX12_RECT m_scissorRect;
	IDXGISwapChain3* m_swapChain;
	ID3D12Resource* m_SwaprenderTargets[RHI::CPUFrameCount];
	ID3D12DescriptorHeap* m_rtvHeap;
	ID3D12DescriptorHeap* m_dsvHeap;
	UINT m_rtvDescriptorSize;
	ID3D12Resource * m_depthStencil;
	int m_frameIndex = 0;
	ID3D12Debug* debugController;

	typedef std::pair<IUnknown*, int64_t> UploadHeapStamped;
	std::vector<UploadHeapStamped> DeferredDeleteQueue;
	class GPUResource* m_RenderTargetResources[RHI::CPUFrameCount];
	class D3D12ReadBackCopyHelper* ScreenShotter = nullptr;
	bool Omce = false;
	bool RunScreenShot = false;
};
#include "D3D12Helpers.h"
//helper functions!
static inline void ThrowIfFailed(HRESULT hr)
{
	if (FAILED(hr))
	{
#if AFTERMATH
		D3D12RHI::Get()->RunTheAfterMath();
#endif
		D3D12RHI::HandleDeviceFailure();
 		ensureFatalMsgf(hr == S_OK, +(std::string)D3D12Helpers::DXErrorCodeToString(hr));
	}
}


