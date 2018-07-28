#include "Shader_SSAO.h"
#include "RHI/RHI.h"

float lerp(float a,float b,float f)
{
	return a + f * (b - a);
}
Shader_SSAO::Shader_SSAO()
{
	/*noisetex = new OGLTexture();
	noisetex->GenerateNoiseTex();*/
	//Initialise OGL shader
	m_Shader = RHI::CreateShaderProgam();

	m_Shader->CreateShaderProgram();
	m_Shader->AttachAndCompileShaderFromFile("SSAO", SHADER_VERTEX);
	m_Shader->AttachAndCompileShaderFromFile("SSAO", SHADER_FRAGMENT);

	m_Shader->BindAttributeLocation(0, "pos");
	m_Shader->BindAttributeLocation(1, "Normal");
	m_Shader->BindAttributeLocation(2, "texCoords");

	//	glBindFragDataLocation(m_Shader->GetProgramHandle(), 0, "FragColor");

	m_Shader->BuildShaderProgram();
	m_Shader->ActivateShaderProgram();
#if BUILD_OPENGL
	static const float g_quad_vertex_buffer_data[] = {
		-1.0f, -1.0f, 0.0f,
		1.0f, -1.0f, 0.0f,
		-1.0f,  1.0f, 0.0f,
		-1.0f,  1.0f, 0.0f,
		1.0f, -1.0f, 0.0f,
		1.0f,  1.0f, 0.0f,
	};


	glGenBuffers(1, &quad_vertexbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, quad_vertexbuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(g_quad_vertex_buffer_data), g_quad_vertex_buffer_data, GL_STATIC_DRAW);

	glUniform1i(glGetUniformLocation(m_Shader->GetProgramHandle(), "gPosition"), 0);
	glUniform1i(glGetUniformLocation(m_Shader->GetProgramHandle(), "gNormal"), 1);
	glUniform1i(glGetUniformLocation(m_Shader->GetProgramHandle(), "texNoise"), NoiseTextureUnit);
#endif
	std::uniform_real_distribution<float> randomFloats(0.0, 1.0); // random floats between 0.0 - 1.0
	std::default_random_engine generator;

	for (int i = 0; i < 64; ++i)
	{
		glm::vec3 sample(
			randomFloats(generator) * 2.0 - 1.0,
			randomFloats(generator) * 2.0 - 1.0,
			randomFloats(generator)
		);
		sample = glm::normalize(sample);
		sample *= randomFloats(generator);
		float scale = float(i) / static_cast<float>(64.0);
		scale = lerp(0.1f, 1.0f, scale * scale);
		sample *= scale;
		ssaoKernel.push_back(sample);
	}

}


Shader_SSAO::~Shader_SSAO()
{
}

void Shader_SSAO::RenderPlane()
{
#if BUILD_OPENGL
	// 1rst attribute buffer : vertices
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, quad_vertexbuffer);
	glVertexAttribPointer(
		0,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
		3,                  // size
		GL_FLOAT,           // type
		GL_FALSE,           // normalized?
		0,                  // stride
		(void*)0            // array buffer offset
	);

	// Draw the triangles !
	glDrawArrays(GL_TRIANGLES, 0, 6); // 2*3 indices starting at 0 -> 2 triangles

	glDisableVertexAttribArray(0);
#endif
}

void Shader_SSAO::UpdateOGLUniforms(Transform * , Camera * c, std::vector<Light*> lights)
{
#if BUILD_OPENGL
	noisetex->Bind(NoiseTextureUnit);
	glUniformMatrix4fv(glGetUniformLocation(m_Shader->GetProgramHandle(), "projection"), 1, GL_FALSE, &c->GetProjection()[0][0]);
	glUniform1i(glGetUniformLocation(m_Shader->GetProgramHandle(), "width"), mwidth);
	glUniform1i(glGetUniformLocation(m_Shader->GetProgramHandle(), "height"), mheight);
	for (size_t i = 0; i < ssaoKernel.size(); i++)
	{
		glUniform3fv(glGetUniformLocation(m_Shader->GetProgramHandle(), ("samples[" + std::to_string(i) + "]").c_str()), 1, glm::value_ptr(ssaoKernel[i]));
	}
#endif
}
void Shader_SSAO::Resize(int width, int height)
{
	mwidth = width;
	mheight = height;
}

void Shader_SSAO::UpdateD3D11Uniforms(Transform * , Camera * , std::vector<Light*> lights)
{
}