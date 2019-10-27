#pragma once
#include "RHI/BaseTexture.h"
class DeviceContext;
class DescriptorHeap;
class D3D12DeviceContext;
class D3D12CommandList;
class DXDescriptor;
class GPUResource;
class D3D12Texture : public BaseTexture
{
public:
	static float MipCreationTime;
	D3D12Texture(DeviceContext * inDevice = nullptr);
	virtual ~D3D12Texture();
	bool CreateFromFile(AssetPathRef FileName) override;
	void BindToSlot(D3D12CommandList * list, int slot);
	virtual void CreateTextureFromDesc(const TextureDescription& desc) override;
	virtual void CreateAsNull() override;
	ID3D12Resource* GetResource();
	int Width = 0;
	int Height = 0;
	UINT16 Miplevels = 6;
	int	MipLevelsReadyNow = 1;
	bool CheckDevice(int index);
	DXDescriptor* GetDescriptor(RHIViewDesc Desc, DescriptorHeap* heap = nullptr);
protected:
	void Release() override;
private:
	unsigned char * GenerateMip(int & startwidth, int & startheight, int bpp, unsigned char * StartData, int & mipsize, float ratio = 2.0f);
	unsigned char * GenerateMips(int count, int StartWidth, int StartHeight, unsigned char * startdata);
	bool CLoad(AssetPathRef name);
	bool LoadDDS(std::string filename);
	D3D12_SUBRESOURCE_DATA Texturedatarray[9];
	D3D12DeviceContext * Device = nullptr;
	DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM;
	//#TextureLoading Remove this hack
	bool UsingDDSLoad = false;
	ID3D12Resource* m_texture = nullptr;
	GPUResource* TextureResource = nullptr;
	int FrameCreated = -1;
};

