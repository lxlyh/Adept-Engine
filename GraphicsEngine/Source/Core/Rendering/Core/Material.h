#pragma once
#include "RHI/BaseTexture.h"
#include "Mesh/MeshPipelineController.h"
struct EMaterialRenderType
{
	enum Type
	{
		Opaque,
		Transparent,
		Limit
	};
};
class RHIBuffer;
class Material
{
public:
	struct TextureBindData
	{
		SharedPtr<BaseTexture> TextureObj;
		int RootSigSlot = 0;
		int RegisterSlot = 0;
	};
	struct TextureBindSet
	{
		std::map<std::string, Material::TextureBindData> BindMap;
		void AddBind(std::string name, int index, int Register)
		{
			BindMap.emplace(name, Material::TextureBindData{ nullptr, index ,Register });
		}
	};
	struct MaterialShaderData
	{
		float Roughness = 1.0f;
		float Metallic = 0.0f;
	};
	struct MaterialProperties
	{
		bool UseMainShader = true;
		bool IsReflective = false;
		class Shader* ShaderInUse = nullptr;
		const TextureBindSet* TextureBinds = nullptr;
	};
	void UpdateShaderData();
	void SetMaterialData(MaterialShaderData Data);
	CORE_API Material(BaseTexture* Diff, MaterialProperties props = MaterialProperties());
	CORE_API Material(MaterialProperties props = MaterialProperties());
	~Material();
	void SetMaterialActive(class RHICommandList * list, ERenderPass::Type Pass);

	void UpdateBind(std::string Name, BaseTextureRef NewTex);
	BaseTexture* GetTexturebind(std::string Name);
	typedef std::pair<std::string, Material::TextureBindData> FlatMap;
	CORE_API MaterialProperties* GetProperties();
	void SetDisplacementMap(BaseTexture* tex);
	CORE_API void SetNormalMap(BaseTexture * tex);
	CORE_API void SetDiffusetexture(BaseTextureRef tex);
	bool HasNormalMap();
	CORE_API static Material* CreateDefaultMaterialInstance();
	static Material * GetDefaultMaterial();
	CORE_API static Shader* GetDefaultMaterialShader();
	void ProcessSerialArchive(class Archive* A);
	static constexpr const char* DefuseBindName = "DiffuseMap";
	RHIBuffer* MaterialData = nullptr;
	EMaterialRenderType::Type GetRenderPassType();
	EMaterialRenderType::Type MateralRenderType = EMaterialRenderType::Opaque;
private:
	
	TextureBindSet * CurrentBindSet = nullptr;
	void SetupDefaultBinding(TextureBindSet* TargetSet);
	MaterialProperties Properties;
	MaterialShaderData ShaderProperties;
	//bind to null

};

