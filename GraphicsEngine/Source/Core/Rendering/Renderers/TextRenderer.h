#pragma once
#include <string>
#include "glm\fwd.hpp"
#include <ft2build.h>
#include <map>
#include "include/glm/ext.hpp"
#include "RHI/RHI.h"
#include <vector>
#include FT_FREETYPE_H  
class BaseTexture;
class Text_Shader;
class ShaderProgramBase;
struct Character
{
	BaseTexture* Texture;   // ID of the glyph texture
	glm::ivec2 Size;    // Size of glyph
	glm::ivec2 Bearing;  // Offset 
	int Advance;    // offset to next glyph
};
struct atlas;
class TextRenderer
{
public:
	static TextRenderer* instance;
	TextRenderer(int width, int height);
	~TextRenderer();

	void RenderFromAtlas(std::string text, float x, float y, float scale, glm::vec3 color = glm::vec3(1, 1, 1),bool Reset = false);
	void Finish();
	void Reset();
	void LoadText();
	void UpdateSize(int width, int height);
	void NotifyFrameEnd();
	bool RunOnSecondDevice = false;
private:	
	bool UseFrameBuffer = true;
	int TextDataLength = 0;
	int m_width, m_height = 0;
	Text_Shader * m_TextShader;
	FT_Library ft;
	FT_Face face;	
	RHIBuffer* VertexBuffer = nullptr;
	RHICommandList* TextCommandList = nullptr;
	//when run on second gpu
	FrameBuffer* Renderbuffer = nullptr;
	float ScaleFactor = 1.0f;
	struct point
	{
		float x;
		float y;
		float s;
		float t;
		glm::vec3 colour;
	};

	std::vector<point> coords;
	int currentsize = 0;
	const int MAX_BUFFER_SIZE = 1000;
	bool NeedsClearRT = true;
	struct atlas
	{
		BaseTexture* Texture;
		unsigned int w;			// width of texture in pixels
		unsigned int h;			// height of texture in pixels

		struct
		{
			float ax;	// advance.x
			float ay;	// advance.y

			float bw;	// bitmap.width;
			float bh;	// bitmap.height;

			float bl;	// bitmap_left;
			float bt;	// bitmap_top;

			float tx;	// x offset of glyph in texture coordinates
			float ty;	// y offset of glyph in texture coordinates
		} c[128];		// character information
		
		atlas(FT_Face face, int height, bool RunOnSecondDevice);
		~atlas();
	};
	atlas* TextAtlas;
};
