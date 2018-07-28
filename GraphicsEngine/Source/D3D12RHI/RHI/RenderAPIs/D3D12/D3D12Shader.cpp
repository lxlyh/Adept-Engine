#include "stdafx.h"
#include "D3D12Shader.h"
#include "Core/Engine.h"
#include <iostream>
#include "D3D12RHI.h"
#include "D3D12CBV.h"
#include "Core/Utils/FileUtils.h"
#include "Core/Utils/StringUtil.h"
#include "Core/Asserts.h"
#include "RHI/DeviceContext.h"

#include <d3dcompiler.h>
#include "DxIncludeHandler.h"
#include "Core/Assets/AssetManager.h"
#include "D3D12DeviceContext.h"
//As shader based validation doesn't support include handlers use ours
#define USE_DX_INCLUDER 0
D3D12Shader::D3D12Shader(DeviceContext* Device)
{
	CurrentDevice = (D3D12DeviceContext*)Device;
	IncludeHandler = new DxIncludeHandler();
}

D3D12Shader::~D3D12Shader()
{
	delete IncludeHandler;
	CurrentDevice = nullptr;
	if (m_Shader.m_pipelineState)
	{
		m_Shader.m_pipelineState->Release();
	}
	if (m_Shader.m_rootSignature)
	{
		m_Shader.m_rootSignature->Release();
	}
	if (mBlolbs.csBlob)
	{
		mBlolbs.csBlob->Release();
		mBlolbs.csBlob = nullptr;
	}
	if (mBlolbs.fsBlob)
	{
		mBlolbs.fsBlob->Release();
		mBlolbs.fsBlob = nullptr;
	}
	if (mBlolbs.vsBlob)
	{
		mBlolbs.vsBlob->Release();
		mBlolbs.vsBlob = nullptr;
	}
	if (mBlolbs.gsBlob)
	{
		mBlolbs.gsBlob->Release();
		mBlolbs.gsBlob = nullptr;
	}
}

void D3D12Shader::CreateShaderProgram()
{}
void StripD3dShader(ID3DBlob** blob)
{
	if (*blob == nullptr)
	{
		return;
	}
	UINT stripflags = D3DCOMPILER_STRIP_REFLECTION_DATA | D3DCOMPILER_STRIP_DEBUG_INFO | D3DCOMPILER_STRIP_TEST_BLOBS;
	D3DStripShader(*blob, (*blob)->GetBufferSize(), stripflags, blob);
}
D3D_SHADER_MACRO* D3D12Shader::ParseDefines()
{
	D3D_SHADER_MACRO* out = new D3D_SHADER_MACRO[Defines.size() + 1];
	for (int i = 0; i < Defines.size(); i++)//array is set up as Name, Value
	{
		out[i].Name = Defines[i].Name.c_str();
		out[i].Definition = Defines[i].Value.c_str();
	}
	int last = (int)Defines.size();
	out[last].Definition = NULL;
	out[last].Name = NULL;
	return out;
}


EShaderError D3D12Shader::AttachAndCompileShaderFromFile(const char * shadername, EShaderType type)
{
	return AttachAndCompileShaderFromFile(shadername, type, "main");
}
EShaderError D3D12Shader::AttachAndCompileShaderFromFile(const char * shadername, EShaderType type, const char * Entrypoint)
{
	//convert to LPC 
	std::string path = Engine::GetRootDir();
	path.append("\\asset\\shader\\hlsl\\");
	std::string name = shadername;
	name.append(".hlsl");
	path.append(name);

	if (!FileUtils::File_ExistsTest(path))
	{
#ifdef  _DEBUG
		__debugbreak();
#endif
		return SHADER_ERROR_NOFILE;
	}
	if (type == SHADER_COMPUTE)
	{
		IsCompute = true;
	}

	std::string ShaderData = AssetManager::instance->LoadFileWithInclude(name);


	std::wstring newfile((int)path.size(), 0);
	MultiByteToWideChar(CP_UTF8, 0, &path[0], (int)path.size(), &newfile[0], (int)path.size());
	LPCWSTR filename = newfile.c_str();


	ID3DBlob* pErrorBlob = NULL;
	HRESULT hr = S_OK;
#if defined(_DEBUG)
	// Enable better shader debugging with the graphics debugging tools.
	UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else 
#if BUILD_SHIPPING
	UINT compileFlags = D3DCOMPILE_WARNINGS_ARE_ERRORS | D3DCOMPILE_OPTIMIZATION_LEVEL3 | D3DCOMPILE_ALL_RESOURCES_BOUND;
#else
	UINT compileFlags = D3DCOMPILE_OPTIMIZATION_LEVEL3 | D3DCOMPILE_ALL_RESOURCES_BOUND;//| D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_WARNINGS_ARE_ERRORS ;
#endif
#endif
	D3D_SHADER_MACRO* defines = ParseDefines();
	if (type == SHADER_VERTEX)
	{
#if USE_DX_INCLUDER
		hr = D3DCompileFromFile(filename, defines, IncludeHandler, "main", "vs_5_0",
			compileFlags, 0, &mBlolbs.vsBlob, &pErrorBlob);
#else
		hr = D3DCompile(ShaderData.c_str(), ShaderData.size(), shadername, defines, nullptr, "main", "vs_5_0",
			compileFlags, 0, &mBlolbs.vsBlob, &pErrorBlob);
#endif
		StripD3dShader(&mBlolbs.vsBlob);

	}
	else if (type == SHADER_FRAGMENT)
	{

#if USE_DX_INCLUDER
		hr = D3DCompileFromFile(filename, defines, D3D_COMPILE_STANDARD_FILE_INCLUDE, "main", "ps_5_0",
			compileFlags, 0, &mBlolbs.fsBlob, &pErrorBlob);
#else
		hr = D3DCompile(ShaderData.c_str(), ShaderData.size(), shadername, defines, nullptr, "main", "ps_5_1",
			compileFlags, 0, &mBlolbs.fsBlob, &pErrorBlob);
#endif
		StripD3dShader(&mBlolbs.fsBlob);
	}
	else if (type == SHADER_COMPUTE)
	{
#if USE_DX_INCLUDER
		hr = D3DCompileFromFile(filename, defines, D3D_COMPILE_STANDARD_FILE_INCLUDE, "main", "cs_5_0",
			compileFlags, 0, &mBlolbs.csBlob, &pErrorBlob);
#else
		hr = D3DCompile(ShaderData.c_str(), ShaderData.size(), shadername, defines, nullptr, Entrypoint, "cs_5_0",
			compileFlags, 0, &mBlolbs.csBlob, &pErrorBlob);
#endif
	}
	else if (type == SHADER_GEOMETRY)
	{
#if USE_DX_INCLUDER
		hr = D3DCompileFromFile(filename, defines, D3D_COMPILE_STANDARD_FILE_INCLUDE, "main", "gs_5_0",
			compileFlags, 0, &mBlolbs.gsBlob, &pErrorBlob);
#else
		hr = D3DCompile(ShaderData.c_str(), ShaderData.size(), shadername, defines, nullptr, "main", "gs_5_0",
			compileFlags, 0, &mBlolbs.gsBlob, &pErrorBlob);
#endif
	}

	if (pErrorBlob)
	{
		std::string Log = "Shader Compile Output: ";
		Log.append(StringUtils::ConvertWideToString(filename));
		Log.append("\n");
		Log.append(reinterpret_cast<const char*>(pErrorBlob->GetBufferPointer()));
		Log::OutS << Log << Log::OutS;
		if (FAILED(hr))
		{
			PlatformApplication::DisplayMessageBox("Shader Complie Error", Log);
			pErrorBlob->Release();
#ifndef NDEBUG
			__debugbreak();
#endif
			exit(-1);
			__debugbreak();
			return SHADER_ERROR_COMPILE;
		}
	}

	if (pErrorBlob)
	{
		pErrorBlob->Release();
	}
	if (FAILED(hr))
	{
		return SHADER_ERROR_CREATE;
	}
	return SHADER_ERROR_NONE;
}

void D3D12Shader::BuildShaderProgram()
{}

void D3D12Shader::DeleteShaderProgram()
{}

void D3D12Shader::ActivateShaderProgram()
{}
void D3D12Shader::DeactivateShaderProgram()
{}


D3D12Shader::PiplineShader D3D12Shader::CreateComputePipelineShader(PiplineShader &output, D3D12_INPUT_ELEMENT_DESC* inputDisc, int DescCount, ShaderBlobs* blobs, PipeLineState Depthtest,
	PipeRenderTargetDesc PRTD, DeviceContext* context)
{
	D3D12_COMPUTE_PIPELINE_STATE_DESC computePsoDesc = {};
	computePsoDesc.pRootSignature = output.m_rootSignature;
	computePsoDesc.CS = CD3DX12_SHADER_BYTECODE(blobs->csBlob);
	ThrowIfFailed(((D3D12DeviceContext*)context)->GetDevice()->CreateComputePipelineState(&computePsoDesc, IID_PPV_ARGS(&output.m_pipelineState)));
	return output;
}

D3D12Shader::PiplineShader D3D12Shader::CreatePipelineShader(PiplineShader &output, D3D12_INPUT_ELEMENT_DESC* inputDisc, int DescCount, ShaderBlobs* blobs, PipeLineState Depthtest,
	PipeRenderTargetDesc PRTD, DeviceContext* context)
{
	ensure(blobs->vsBlob != nullptr);
	ensure(blobs->fsBlob != nullptr);
	if (context == nullptr)
	{
		context = RHI::GetDeviceContext(0);
	}
	// Describe and create the graphics pipeline state object (PSO).
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = { 0 };
	psoDesc.InputLayout.pInputElementDescs = inputDisc;
	psoDesc.InputLayout.NumElements = DescCount;
	psoDesc.pRootSignature = output.m_rootSignature;
	psoDesc.VS = CD3DX12_SHADER_BYTECODE(blobs->vsBlob);
	psoDesc.PS = CD3DX12_SHADER_BYTECODE(blobs->fsBlob);
	if (blobs->gsBlob != nullptr)
	{
		psoDesc.GS = CD3DX12_SHADER_BYTECODE(blobs->gsBlob);
	}
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	if (Depthtest.RasterMode == PRIMITIVE_TOPOLOGY_TYPE::PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE)
	{
		//psoDesc.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_ON;
	}
	psoDesc.RasterizerState.CullMode = Depthtest.Cull ? D3D12_CULL_MODE_BACK : D3D12_CULL_MODE_NONE;
	if (Depthtest.Blending)
	{
		psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		psoDesc.BlendState.AlphaToCoverageEnable = true;
		psoDesc.BlendState.IndependentBlendEnable = FALSE;
		psoDesc.BlendState.RenderTarget[0].BlendEnable = TRUE;
	}
	else
	{
		psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	}
	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState.DepthEnable = Depthtest.DepthTest;
	psoDesc.DepthStencilState.DepthFunc = (D3D12_COMPARISON_FUNC)Depthtest.DepthCompareFunction;
	psoDesc.DepthStencilState.DepthWriteMask = Depthtest.DepthWrite ? D3D12_DEPTH_WRITE_MASK::D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK::D3D12_DEPTH_WRITE_MASK_ZERO;
	psoDesc.DepthStencilState.StencilEnable = false;
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = (D3D12_PRIMITIVE_TOPOLOGY_TYPE)Depthtest.RasterMode;
	psoDesc.NumRenderTargets = PRTD.NumRenderTargets;
	psoDesc.SampleDesc.Count = 1;

	for (int i = 0; i < 8; i++)
	{
		psoDesc.RTVFormats[i] = PRTD.RTVFormats[i];
	}
	psoDesc.DSVFormat = PRTD.DSVFormat;
	ThrowIfFailed(((D3D12DeviceContext*)context)->GetDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&output.m_pipelineState)));
	return output;
}

D3D12Shader::ShaderBlobs * D3D12Shader::GetShaderBlobs()
{
	return &mBlolbs;
}

void D3D12Shader::Init()
{
	if (IsCompute)
	{
		CreateComputePipelineShader();
	}
}

void D3D12Shader::CreateComputePipelineShader()
{

	// Compute root signature.
	{

		//The compute shader expects 2 floats, the source texture and the destination texture
		CD3DX12_DESCRIPTOR_RANGE srvCbvRanges[2];
		CD3DX12_ROOT_PARAMETER rootParameters[3];
		srvCbvRanges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);
		srvCbvRanges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0, 0);
		rootParameters[0].InitAsConstants(2, 0);
		rootParameters[1].InitAsDescriptorTable(1, &srvCbvRanges[0]);
		rootParameters[2].InitAsDescriptorTable(1, &srvCbvRanges[1]);

		//Static sampler used to get the linearly interpolated color for the mipmaps
		D3D12_STATIC_SAMPLER_DESC samplerDesc = {};
		samplerDesc.Filter = D3D12_FILTER_ANISOTROPIC;
		samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		samplerDesc.MipLODBias = 0.0f;
		samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
		samplerDesc.MinLOD = 0.0f;
		samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
		samplerDesc.MaxAnisotropy = 16;
		samplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
		samplerDesc.ShaderRegister = 0;
		samplerDesc.RegisterSpace = 0;
		samplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

		CD3DX12_ROOT_SIGNATURE_DESC computeRootSignatureDesc;
		//computeRootSignatureDesc.Init(_countof(rootParameters), rootParameters, 0, nullptr);

		ID3DBlob* signature;
		ID3DBlob* error;
		computeRootSignatureDesc.Init(_countof(rootParameters), rootParameters, 1, &samplerDesc, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
		ThrowIfFailed(D3D12SerializeRootSignature(&computeRootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
		ThrowIfFailed(CurrentDevice->GetDevice()->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_Shader.m_rootSignature)));
		NAME_D3D12_OBJECT(m_Shader.m_rootSignature);
	}
	D3D12_COMPUTE_PIPELINE_STATE_DESC computePsoDesc = {};
	computePsoDesc.pRootSignature = m_Shader.m_rootSignature;
	computePsoDesc.CS = CD3DX12_SHADER_BYTECODE(mBlolbs.csBlob);
	ThrowIfFailed(CurrentDevice->GetDevice()->CreateComputePipelineState(&computePsoDesc, IID_PPV_ARGS(&m_Shader.m_pipelineState)));

	D3D12Shader::PiplineShader t;
	CreateDefaultRootSig(t);
}

D3D12_INPUT_ELEMENT_DESC D3D12Shader::ConvertVertexFormat(Shader::VertexElementDESC* desc)
{
	D3D12_INPUT_ELEMENT_DESC output;
	output.SemanticName = desc->SemanticName;
	output.AlignedByteOffset = desc->AlignedByteOffset;
	output.InputSlot = desc->InputSlot;
	output.SemanticIndex = desc->SemanticIndex;
	output.Format = desc->Format;
	output.InstanceDataStepRate = desc->InstanceDataStepRate;
	//identical!
	output.InputSlotClass = (D3D12_INPUT_CLASSIFICATION)(int)desc->InputSlotClass;
	return output;
}

D3D12Shader::PiplineShader * D3D12Shader::GetPipelineShader()
{
	return &m_Shader;
}

bool D3D12Shader::ParseVertexFormat(std::vector<Shader::VertexElementDESC> desc, D3D12_INPUT_ELEMENT_DESC ** Data, int * length)
{
	*Data = new D3D12_INPUT_ELEMENT_DESC[desc.size()];
	*length = (int)desc.size();
	D3D12_INPUT_ELEMENT_DESC* Dataptr = *Data;
	for (int i = 0; i < desc.size(); i++)
	{
		Dataptr[i] = ConvertVertexFormat(&desc[i]);
	}
	return true;
}

void D3D12Shader::CreateRootSig(D3D12Shader::PiplineShader &output, std::vector<Shader::ShaderParameter> Params, DeviceContext* context)
{
	D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};

	// This is the highest version the sample supports. If CheckFeatureSupport succeeds, the HighestVersion returned will not be greater than this.
	featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;

	if (FAILED(((D3D12DeviceContext*)context)->GetDevice()->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
	{
		featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
	}
#define UAVRANGES 1
	//cbvs will be InitAsConstantBufferView
	//srvs and UAvs will be in Ranges
	CD3DX12_ROOT_PARAMETER1* rootParameters = nullptr;
	rootParameters = new CD3DX12_ROOT_PARAMETER1[Params.size()];
	int RangeNumber = 0;
	for (int i = 0; i < Params.size(); i++)
	{
		rootParameters[i] = CD3DX12_ROOT_PARAMETER1();
		if (Params[i].Type == Shader::ShaderParamType::SRV)
		{
			RangeNumber++;
		}
#if UAVRANGES
		else if (Params[i].Type == Shader::ShaderParamType::UAV)
		{
			RangeNumber++;
		}
#endif
	}
	D3D12_SHADER_VISIBILITY BaseSRVVis = D3D12_SHADER_VISIBILITY::D3D12_SHADER_VISIBILITY_PIXEL;
	if (output.IsCompute)
	{
		BaseSRVVis = D3D12_SHADER_VISIBILITY::D3D12_SHADER_VISIBILITY_ALL;
	}
	CD3DX12_DESCRIPTOR_RANGE1* ranges = nullptr;
	if (RangeNumber > 0)
	{
		ranges = new CD3DX12_DESCRIPTOR_RANGE1[Params.size()];
	}
	for (int i = 0; i < Params.size(); i++)
	{
		if (Params[i].Type == Shader::ShaderParamType::SRV)
		{
			ranges[Params[i].SignitureSlot].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, Params[i].NumDescriptors, Params[i].RegisterSlot, 0, D3D12_DESCRIPTOR_RANGE_FLAG_NONE, 0);
			rootParameters[Params[i].SignitureSlot].InitAsDescriptorTable(1, &ranges[Params[i].SignitureSlot], BaseSRVVis);
		}
		else if (Params[i].Type == Shader::ShaderParamType::CBV)
		{
			rootParameters[Params[i].SignitureSlot].InitAsConstantBufferView(Params[i].RegisterSlot, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_ALL);
		}
		else if (Params[i].Type == Shader::ShaderParamType::UAV)
		{
#if !UAVRANGES
			rootParameters[Params[i].SignitureSlot].InitAsUnorderedAccessView(Params[i].RegisterSlot, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_ALL);
#else
			ranges[Params[i].SignitureSlot].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, Params[i].NumDescriptors, Params[i].RegisterSlot, 0, D3D12_DESCRIPTOR_RANGE_FLAG_NONE, 0);
			rootParameters[Params[i].SignitureSlot].InitAsDescriptorTable(1, &ranges[Params[i].SignitureSlot], D3D12_SHADER_VISIBILITY_ALL);
#endif
		}
	}
	//todo: Samplers

	D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;
#define NUMSamples 3
	D3D12_STATIC_SAMPLER_DESC samplers[NUMSamples];

	D3D12_STATIC_SAMPLER_DESC sampler = {};
	//	sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
	sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sampler.MipLODBias = 0;
	sampler.MaxAnisotropy = 0;
	sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
	sampler.MinLOD = 0.0f;
	sampler.MaxLOD = D3D12_FLOAT32_MAX;
	sampler.ShaderRegister = 0;
	sampler.RegisterSpace = 0;
	sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	samplers[0] = sampler;

	sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
	sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	sampler.ShaderRegister = 1;
	sampler.RegisterSpace = 0;
	samplers[1] = sampler;

	sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	sampler.ShaderRegister = 2;
	sampler.RegisterSpace = 0;
	samplers[2] = sampler;

	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc((UINT)Params.size(), rootParameters, NUMSamples, &samplers[0], D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	ID3DBlob* signature;
	ID3DBlob* error;
	ThrowIfFailed(D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, featureData.HighestVersion, &signature, &error));
	ThrowIfFailed(((D3D12DeviceContext*)context)->GetDevice()->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&output.m_rootSignature)));

	std::wstring name = L"Root sig Length = ";
	name.append(std::to_wstring(Params.size()));
	output.m_rootSignature->SetName(name.c_str());
}

void D3D12Shader::CreateDefaultRootSig(D3D12Shader::PiplineShader &output)
{
	D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};

	// This is the highest version the sample supports. If CheckFeatureSupport succeeds, the HighestVersion returned will not be greater than this.
	featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;

	/*if (FAILED(D3D12RHI::GetDevice()->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
	{
		featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
	}*/

	CD3DX12_DESCRIPTOR_RANGE1 ranges[2];
	ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_NONE);
	ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 4, 0, D3D12_DESCRIPTOR_RANGE_FLAG_NONE);

	CD3DX12_ROOT_PARAMETER1 rootParameters[5];
	rootParameters[0].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_PIXEL);
	//	rootParameters[1].InitAsDescriptorTable(1, &ranges[1], D3D12_SHADER_VISIBILITY_VERTEX);
	rootParameters[1].InitAsConstantBufferView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_ALL);
	rootParameters[2].InitAsConstantBufferView(1, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_ALL);
	rootParameters[3].InitAsConstantBufferView(2, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_ALL);
	rootParameters[4].InitAsDescriptorTable(1, &ranges[1], D3D12_SHADER_VISIBILITY_PIXEL);
	//rootParameters[3].InitAsShaderResourceView(4, 0, D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC, D3D12_SHADER_VISIBILITY_ALL);
	D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;
	D3D12_STATIC_SAMPLER_DESC samplers[2];

	D3D12_STATIC_SAMPLER_DESC sampler = {};
	//	sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
	sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sampler.MipLODBias = 0;

	sampler.MaxAnisotropy = 0;
	sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
	sampler.MinLOD = 0.0f;
	sampler.MaxLOD = D3D12_FLOAT32_MAX;
	sampler.ShaderRegister = 0;
	sampler.RegisterSpace = 0;
	sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	samplers[0] = sampler;


	sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
	sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	sampler.ShaderRegister = 1;
	sampler.RegisterSpace = 0;
	samplers[1] = sampler;

	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc(_countof(rootParameters), rootParameters, 2, &samplers[0], D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	ID3DBlob* signature;
	ID3DBlob* error;
	ThrowIfFailed(D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, featureData.HighestVersion, &signature, &error));
	//ThrowIfFailed(D3D12RHI::GetDisplayDevice()->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&output.m_rootSignature)));
}