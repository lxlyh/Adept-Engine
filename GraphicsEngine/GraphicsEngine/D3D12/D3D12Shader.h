#pragma once
#include "../RHI/ShaderProgramBase.h"
#include <d3d12.h>
#include "glm\glm.hpp"
#include "../EngineGlobals.h"
#include "../RHI/Shader.h"
class D3D12Shader : public ShaderProgramBase
{
public:
	D3D12Shader();
	~D3D12Shader();
	struct ShaderBlobs
	{
		ID3DBlob*					vsBlob = nullptr;
		ID3DBlob*					fsBlob = nullptr;
		ID3DBlob*					csBlob = nullptr;
		ID3DBlob*					gsBlob = nullptr;
	};
	struct PiplineShader
	{
		ID3D12PipelineState* m_pipelineState = nullptr;
		ID3D12RootSignature* m_rootSignature = nullptr; 
	};
	struct PipeRenderTargetDesc
	{
		DXGI_FORMAT RTVFormats[8];
		DXGI_FORMAT DSVFormat = DXGI_FORMAT_UNKNOWN;
	};
	// Inherited via ShaderProgramBase
	virtual void CreateShaderProgram() override;
	virtual EShaderError AttachAndCompileShaderFromFile(const char * filename, EShaderType type) override;
	virtual void BuildShaderProgram() override;
	virtual void DeleteShaderProgram() override;
	virtual void ActivateShaderProgram() override;
	void ActivateShaderProgram(ID3D12GraphicsCommandList * list);
	virtual void DeactivateShaderProgram() override;
	static PiplineShader CreatePipelineShader(PiplineShader & output, D3D12_INPUT_ELEMENT_DESC * inputDisc, int DescCount, ShaderBlobs* blobs, PipeLineState Depthtest, PipeRenderTargetDesc PRTD);
	ShaderBlobs* GetShaderBlobs();
	static bool ParseVertexFormat(std::vector<Shader::VertexElementDESC>, D3D12_INPUT_ELEMENT_DESC** Data, int* length);
	static void CreateRootSig(D3D12Shader::PiplineShader &output, std::vector<Shader::ShaderParameter> Parms);
	static void CreateDefaultRootSig(D3D12Shader::PiplineShader & output);
	void Init();
	void CreateComputePipelineShader();
	CommandListDef* CreateShaderCommandList(int device = 0);
	ID3D12CommandAllocator* GetCommandAllocator();
	void ResetList(ID3D12GraphicsCommandList * list);
	static D3D12_INPUT_ELEMENT_DESC ConvertVertexFormat(Shader::VertexElementDESC * desc);
	enum ComputeRootParameters : UINT32
	{
		ComputeRootCBV = 0,
		ComputeRootSRVTable,
		ComputeRootUAVTable,
		ComputeRootParametersCount
	};

	
	PiplineShader* GetPipelineShader();
private:

	PiplineShader m_Shader;	
	ID3D12CommandAllocator* m_commandAllocator = nullptr;
	ID3D12DescriptorHeap* m_samplerHeap = nullptr;
	ShaderBlobs mBlolbs;

};

