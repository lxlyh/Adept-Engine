#include "stdafx.h"
#include <iostream>
#include "Core/Assets/ImageIO.h"
#include "Core/Engine.h"
#include "Core/Performance/PerfManager.h"
#include "Core/Utils/FileUtils.h"
#include "Core/Utils/StringUtil.h"
#include "Rendering/Shaders/ShaderMipMap.h"
#include "GPUResource.h"
#include "RHI/DeviceContext.h"
#include "ThirdParty/NVDDS/DDSTextureLoader12.h"
#include "DescriptorHeap.h"
#include "D3D12Texture.h"
#include "D3D12RHI.h"
#define USE_CPUFALLBACK_TOGENMIPS_ATRUNTIME 0
float D3D12Texture::MipCreationTime = 0;
D3D12Texture::D3D12Texture(DeviceContext* inDevice)
{
	if (inDevice == nullptr)
	{
		Device = D3D12RHI::GetDefaultDevice();
	}
	else
	{
		Device = inDevice;
	}
}

unsigned char * D3D12Texture::GenerateMip(int& startwidth, int& startheight, int bpp, unsigned char * StartData, int&mipsize, float ratio)
{
	std::string rpath = Engine::GetRootDir();
	rpath.append("\\asset\\DerivedDataCache\\");
	if (!FileUtils::File_ExistsTest(rpath))
	{
		FileUtils::TryCreateDirectory(rpath);
	}
	//todo Proper DDC
	StringUtils::RemoveChar(TextureName, "\\asset\\texture\\");
	rpath.append(TextureName);
	rpath.append("_mip_");

	int width = (int)(startwidth / ratio);
	int height = (int)(startheight / ratio);
	rpath.append(std::to_string(width));

	unsigned char *buffer = NULL;
	mipsize = (width*height*bpp);
	buffer = new unsigned char[mipsize];
	int stride = 4;
	int Sourcex = 0;
	int sourcey = 0;
	int nChannels = 0;
	float x_ratio = ((float)(startwidth - 1)) / width;
	float y_ratio = ((float)(startheight - 1)) / height;
	rpath.append(".bmp");
	if (FileUtils::File_ExistsTest(rpath))
	{
		if (ImageIO::LoadTexture2D(rpath.c_str(), &buffer, &width, &height, &nChannels) != E_IMAGEIO_SUCCESS)
		{
			return buffer;
		}
	}
	else
	{

		for (int x = 0; x < width*stride; x += stride)
		{
			for (int y = 0; y < height*stride; y += stride)
			{
				int y2 = (int)(y * ratio);
				int x2 = (int)(x * ratio);
				Sourcex = x2;
				sourcey = y2;
				glm::vec4 output;
#if 1
				glm::vec4 pixelA = glm::vec4(StartData[Sourcex + sourcey * startwidth], StartData[Sourcex + 1 + sourcey * startwidth], StartData[Sourcex + 2 + sourcey * startwidth], StartData[Sourcex + 3 + sourcey * startwidth]);
				glm::vec4 pixelB = glm::vec4(StartData[Sourcex + 4 + sourcey * startwidth], StartData[Sourcex + 4 + 1 + sourcey * startwidth], StartData[Sourcex + 4 + 2 + sourcey * startwidth], StartData[Sourcex + 4 + 3 + sourcey * startwidth]);
				int Targety = y2 + 1;
				glm::vec4 pixelC = glm::vec4(StartData[Sourcex + Targety * startwidth], StartData[Sourcex + 1 + Targety * startwidth], StartData[Sourcex + 2 + Targety * startwidth], StartData[Sourcex + 3 + Targety * startwidth]);
				glm::vec4 pixelD = glm::vec4(StartData[Sourcex + 4 + Targety * startwidth], StartData[Sourcex + 4 + 1 + Targety * startwidth], StartData[Sourcex + 4 + 2 + Targety * startwidth], StartData[Sourcex + 4 + 3 + Targety * startwidth]);
				float xdiff = (x_ratio*x) - (x_ratio*x);
				float ydiff = (y_ratio*y) - (y_ratio*y);
				output = (pixelA*(1 - xdiff)*(1 - ydiff)) + (pixelB * (xdiff)*(1 - ydiff)) +
					(pixelC*(ydiff)*(1 - xdiff)) + (pixelD * (xdiff*ydiff));
#else
				glm::vec4 nearn = glm::vec4(StartData[(y2 *startwidth) + x2], StartData[(y2 *startwidth) + 1 + x2], StartData[(y2 *startwidth) + 2 + x2], StartData[(y2 *startwidth) + 3 + x2]);
				output = nearn;
#endif

				buffer[x + (y*width)] = (unsigned char)output.r;
				buffer[x + 1 + (y*width)] = (unsigned char)output.g;
				buffer[x + 2 + (y*width)] = (unsigned char)output.b;
				buffer[x + 3 + (y*width)] = (unsigned char)output.a;
			}
		}
		if (startheight != 2048)
		{
			//			SOIL_save_image(rpath.c_str(), SOIL_SAVE_TYPE_BMP, width, height, 4, buffer);
		}
	}
	startheight = height;
	startwidth = width;
	return buffer;
}

struct Mipdata
{
	unsigned char* data;
	int size = 0;
};

unsigned char* D3D12Texture::GenerateMips(int count, int StartWidth, int StartHeight, unsigned char* startdata)
{
	long StartTime = PerfManager::get_nanos();
	int bpp = 4;
	int mipwidth = StartWidth;
	int mipheight = StartHeight;
	int mip0size = (StartWidth * StartHeight*bpp);
	int totalsize = mip0size;
	std::vector<Mipdata> Mips;
	unsigned char* output = startdata;
	for (int i = 0; i < count; i++)
	{
		if (mipwidth == 1 && mipheight == 1)
		{
			Miplevels = i;
			break;
		}
		int mipsize = 0;
		output = GenerateMip(mipwidth, mipheight, bpp, output, mipsize);

		totalsize += mipsize;
		Mipdata data;
		data.data = output;
		data.size = mipsize;
		Mips.push_back(data);
		Texturedatarray[i + 1].RowPitch = (mipwidth)* bpp;
		Texturedatarray[i + 1].SlicePitch = Texturedatarray[i + 1].RowPitch * (mipheight);
	}
	unsigned char*  finalbuffer = new unsigned char[totalsize];
	int Lastoffset = mip0size;
	memcpy(finalbuffer, startdata, mip0size);
	for (int i = 0; i < Mips.size(); i++)
	{
		memcpy((void*)(finalbuffer + Lastoffset), Mips[i].data, Mips[i].size);
		Texturedatarray[i + 1].pData = (finalbuffer + Lastoffset);
		Lastoffset += Mips[i].size;
	}
	long endtime = PerfManager::get_nanos();
	MipCreationTime += ((float)(endtime - StartTime) / 1e6f);
	return finalbuffer;
}

bool D3D12Texture::CLoad(AssetManager::AssetPathRef name)
{

	unsigned char *buffer = NULL;
	int bpp = 0;
	int nChannels;

	TextureName = name.BaseName;
	TexturePath = name.GetRelativePathToAsset();
	if (name.GetFileType() == AssetManager::AssetFileType::DDS)
	{
		return LoadDDS(name.GetFullPathToAsset());
	}
	else if (name.GetExtention().find("tga") != -1)
	{
		if (ImageIO::LoadTGA(name.GetFullPathToAsset().c_str(), &buffer, &width, &height, &bpp, &nChannels) != E_IMAGEIO_SUCCESS)
		{
			return false;
		}
		Miplevels = 1;
	}	 
	else
	{
		if (ImageIO::LoadTexture2D(name.GetFullPathToAsset().c_str(), &buffer, &width, &height, &nChannels) != E_IMAGEIO_SUCCESS)
		{
			return false;
		}
	}

#if USE_CPUFALLBACK_TOGENMIPS_ATRUNTIME
	if (width == 0 || height == 0)
	{
		return;
	}
	unsigned char*  finalbuffer = GenerateMips(Miplevels - 1, width, height, buffer);
	MipLevelsReadyNow = Miplevels;
#else 
	unsigned char*  finalbuffer = buffer;
#endif
	CreateTextureFromData(finalbuffer, 0, width, height, 4);
	return true;
}

bool D3D12Texture::LoadDDS(std::string filename)
{
	srvHeap = new DescriptorHeap(Device, 1, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);
	srvHeap->SetName(L"Texture SRV");
	std::unique_ptr<uint8_t[]> ddsData;
	std::vector<D3D12_SUBRESOURCE_DATA> subresources;
	bool IsCubeMap = false;
	DirectX::DDS_ALPHA_MODE ALPHA_MODE;
	HRESULT hr = DirectX::LoadDDSTextureFromFile(Device->GetDevice(), StringUtils::ConvertStringToWide(filename).c_str(), &m_texture, ddsData, subresources, 0ui64, &ALPHA_MODE, &IsCubeMap);
	if (hr != S_OK)
	{
		return false;
	}
	if (IsCubeMap)
	{
		CurrentTextureType = ETextureType::Type_CubeMap;
		MipLevelsReadyNow = (int)subresources.size() / 6;
	}
	else
	{
		CurrentTextureType = ETextureType::Type_2D;
		MipLevelsReadyNow = (int)subresources.size();
	}
	
	const UINT64 uploadBufferSize = GetRequiredIntermediateSize(m_texture, 0, (UINT)subresources.size());
	ID3D12Resource* textureUploadHeap = nullptr;
	format = m_texture->GetDesc().Format;
	// Create the GPU upload buffer.
	ThrowIfFailed(Device->GetDevice()->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&textureUploadHeap)));

	UpdateSubresources(Device->GetCopyList(), m_texture, textureUploadHeap, 0, 0, (UINT)subresources.size(), subresources.data());
	Device->GetCopyList()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_texture, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));
	m_texture->SetName(L"Loaded Texture");
	Device->NotifyWorkForCopyEngine();
	UpdateSRV();
	return true;
}

D3D12Texture::~D3D12Texture()
{
	if (m_texture)
	{
		m_texture->Release();
		m_texture = nullptr;
	}
	if (srvHeap)
	{
		srvHeap->Release();
	}
}

bool D3D12Texture::CreateFromFile(AssetManager::AssetPathRef FileName)
{
	return CLoad(FileName);
}

void D3D12Texture::BindToSlot(ID3D12GraphicsCommandList* list, int slot)
{
	srvHeap->BindHeap(list);
	list->SetGraphicsRootDescriptorTable(slot, srvHeap->GetGpuAddress(0));
}

void D3D12Texture::CreateTextureFromData(void * data, int type, int width, int height, int bits)
{
	srvHeap = new DescriptorHeap(Device, 1, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);
	srvHeap->SetName(L"Texture SRV");
	ID3D12Resource* textureUploadHeap;
	// Describe and create a Texture2D.
	D3D12_RESOURCE_DESC textureDesc = {};
	if (type == RHI::TextureType::Text)
	{
		format = DXGI_FORMAT_R8_UNORM;
		Miplevels = 1;
		MipLevelsReadyNow = 1;
		textureDesc.Alignment = 0;
	}
	textureDesc.MipLevels = Miplevels;// Miplevels;
	textureDesc.Width = width;
	textureDesc.Height = height;
	textureDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	textureDesc.DepthOrArraySize = 1;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.SampleDesc.Quality = 0;
	textureDesc.Format = format;
	textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	//format = textureDesc.Format;
	ThrowIfFailed(Device->GetDevice()->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&textureDesc,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&m_texture)));

	const UINT64 uploadBufferSize = GetRequiredIntermediateSize(m_texture, 0, MipLevelsReadyNow);

	// Create the GPU upload buffer.
	ThrowIfFailed(Device->GetDevice()->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&textureUploadHeap)));

	D3D12_SUBRESOURCE_DATA textureData = {};
	textureData.pData = data;
	textureData.RowPitch = width * bits;
	textureData.SlicePitch = textureData.RowPitch * height;
	Texturedatarray[0] = textureData;

	UpdateSubresources(Device->GetCopyList(), m_texture, textureUploadHeap, 0, 0, MipLevelsReadyNow, &Texturedatarray[0]);
	Device->GetCopyList()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_texture, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));
	D3D12RHI::Instance->AddUploadToUsed(textureUploadHeap);
	Device->NotifyWorkForCopyEngine();
	m_texture->SetName(L"Texture");
	textureUploadHeap->SetName(L"Upload");
	// Describe and create a SRV for the texture.
	UpdateSRV();
	//gen mips
#if	USEGPUTOGENMIPS_ATRUNTIME
	if (type != RHI::TextureType::Text && D3D12RHI::Instance->MipmapShader != nullptr)
	{
		D3D12RHI::Instance->MipmapShader->Targets.push_back(this);
	}
#endif
}

void D3D12Texture::UpdateSRV()
{
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	ZeroMemory(&srvDesc, sizeof(D3D12_SHADER_RESOURCE_VIEW_DESC));
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = format;
	if (CurrentTextureType == ETextureType::Type_CubeMap)
	{
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
		srvDesc.TextureCube.MipLevels = MipLevelsReadyNow;
		srvDesc.TextureCube.MostDetailedMip = 0;
	}
	else
	{
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = MipLevelsReadyNow;
		srvDesc.Texture2D.MostDetailedMip = 0;
	}

#if 0
	//test for streaming data like mips of disc!
	const int testmip = 8;
	if (MipLevelsReadyNow > testmip)
	{
		
		srvDesc.Texture2D.MipLevels = MipLevelsReadyNow - testmip;
		srvDesc.Texture2D.MostDetailedMip = testmip;
	}
#endif
	Device->GetDevice()->CreateShaderResourceView(m_texture, &srvDesc, srvHeap->GetCPUAddress(0));
	}

ID3D12Resource * D3D12Texture::GetResource()
{
	return m_texture;
}

bool D3D12Texture::CheckDevice(int index)
{
	if (Device != nullptr)
	{
		return (Device->GetDeviceIndex() == index);
	}
	return false;
}

void D3D12Texture::CreateAsNull()
{
	srvHeap = new DescriptorHeap(Device, 1, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);
	srvHeap->SetName(L"Texture SRV");
	UpdateSRV();
}