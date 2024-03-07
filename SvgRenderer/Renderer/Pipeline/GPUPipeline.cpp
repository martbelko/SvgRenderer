#include "Renderer/Pipeline/GPUPipeline.h"

#include "Core/Filesystem.h"
#include "Core/Timer.h"

#include "Renderer/Defs.h"
#include "Renderer/Flattening.h"
#include "Renderer/Bezier.h"

#include <glad/glad.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace SvgRenderer {

	void GPUPipeline::Init()
	{
		Globals::AllPaths.simpleCommands.resize(2'000'000);
		Globals::Tiles.tiles.resize(1'000'000); // This remains fixed for the whole run of the program, may change in the future
		constexpr uint32_t maxNumberOfQuads = 150'000;
		m_TileBuilder.vertices.resize(maxNumberOfQuads * 4);
		m_TileBuilder.indices.reserve(maxNumberOfQuads * 6);

		uint32_t base = 0;
		for (size_t i = 0; i < maxNumberOfQuads * 6; i += 6)
		{
			m_TileBuilder.indices.push_back(base);
			m_TileBuilder.indices.push_back(base + 1);
			m_TileBuilder.indices.push_back(base + 2);
			m_TileBuilder.indices.push_back(base);
			m_TileBuilder.indices.push_back(base + 2);
			m_TileBuilder.indices.push_back(base + 3);
			base += 4;
		}

		glCreateBuffers(1, &m_Vbo);
		{
			GLenum bufferFlags = GL_CLIENT_STORAGE_BIT | GL_MAP_READ_BIT;
			glNamedBufferStorage(m_Vbo, m_TileBuilder.vertices.size() * sizeof(Vertex), m_TileBuilder.vertices.data(), bufferFlags);
		}

		glCreateBuffers(1, &m_Ibo);
		glNamedBufferData(m_Ibo, m_TileBuilder.indices.size() * sizeof(uint32_t), m_TileBuilder.indices.data(), GL_STATIC_DRAW);

		glCreateVertexArrays(1, &m_Vao);
		glVertexArrayVertexBuffer(m_Vao, 0, m_Vbo, 0, sizeof(Vertex));
		glVertexArrayElementBuffer(m_Vao, m_Ibo);

		glBindVertexArray(m_Vao);

		glEnableVertexArrayAttrib(m_Vao, 0);
		glVertexArrayAttribFormat(m_Vao, 0, 2, GL_INT, GL_FALSE, 0);
		glVertexArrayAttribBinding(m_Vao, 0, 0);

		glEnableVertexArrayAttrib(m_Vao, 1);
		glVertexArrayAttribFormat(m_Vao, 1, 2, GL_UNSIGNED_INT, GL_FALSE, 8);
		glVertexArrayAttribBinding(m_Vao, 1, 0);

		glEnableVertexArrayAttrib(m_Vao, 2);
		glVertexArrayAttribFormat(m_Vao, 2, 4, GL_UNSIGNED_BYTE, GL_TRUE, 16);
		glVertexArrayAttribBinding(m_Vao, 2, 0);

		glCreateTextures(GL_TEXTURE_2D, 1, &m_AlphaTexture);
		glTextureParameteri(m_AlphaTexture, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTextureParameteri(m_AlphaTexture, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTextureStorage2D(m_AlphaTexture, 1, GL_R32F, ATLAS_SIZE, ATLAS_SIZE);

		m_FinalShader = Shader::Create(Filesystem::AssetsPath() / "shaders" / "Main.vert", Filesystem::AssetsPath() / "shaders" / "Main.frag");

		float firstAlpha = 0.0f;
		glClearTexImage(m_AlphaTexture, 0, GL_RED, GL_FLOAT, &firstAlpha);

		float clearValueAlpha = 1.0f;
		glClearTexSubImage(m_AlphaTexture, 0, 0, 0, 0, 1, 1, 1, GL_RED, GL_FLOAT, &clearValueAlpha);

		GLuint cmdBuf, pathBuf, simpleCmdBuf, atomicBuf, tilesBuf, bugBuf;
		glCreateBuffers(1, &cmdBuf);
		glCreateBuffers(1, &pathBuf);
		glCreateBuffers(1, &simpleCmdBuf);
		glCreateBuffers(1, &atomicBuf);
		glCreateBuffers(1, &tilesBuf);
		glCreateBuffers(1, &bugBuf);

		GLenum bufferFlags = GL_CLIENT_STORAGE_BIT | GL_MAP_READ_BIT;
		glNamedBufferStorage(cmdBuf, Globals::AllPaths.commands.size() * sizeof(PathRenderCmd), Globals::AllPaths.commands.data(), bufferFlags);
		glNamedBufferStorage(pathBuf, Globals::AllPaths.paths.size() * sizeof(PathRender), Globals::AllPaths.paths.data(), bufferFlags);
		glNamedBufferStorage(simpleCmdBuf, Globals::AllPaths.simpleCommands.size() * sizeof(SimpleCommand), Globals::AllPaths.simpleCommands.data(), bufferFlags);
		glNamedBufferStorage(tilesBuf, Globals::Tiles.tiles.size() * sizeof(Tile), Globals::Tiles.tiles.data(), bufferFlags);
		glNamedBufferStorage(bugBuf, sizeof(float), m_TileBuilder.atlas.data(), bufferFlags);

		uint32_t atomCounter = 0;
		glNamedBufferStorage(atomicBuf, sizeof(uint32_t), &atomCounter, bufferFlags);

		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, pathBuf);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, cmdBuf);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, simpleCmdBuf);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, atomicBuf);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, tilesBuf);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, m_Vbo);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, bugBuf);

		shaderTransform = Shader::CreateCompute(Filesystem::AssetsPath() / "shaders" / "Transform.comp");
		shaderPreFlatten = Shader::CreateCompute(Filesystem::AssetsPath() / "shaders" / "PreFlatten.comp");
		shaderFlatten = Shader::CreateCompute(Filesystem::AssetsPath() / "shaders" / "Flatten.comp");
		shaderPreFill = Shader::CreateCompute(Filesystem::AssetsPath() / "shaders" / "PreFill.comp");
		shaderFill = Shader::CreateCompute(Filesystem::AssetsPath() / "shaders" / "Fill.comp");
		shaderCalculateQuads = Shader::CreateCompute(Filesystem::AssetsPath() / "shaders" / "CalculateQuads.comp");
		shaderPrefixSum = Shader::CreateCompute(Filesystem::AssetsPath() / "shaders" / "PrefixSum.comp");
		shaderCoarse = Shader::CreateCompute(Filesystem::AssetsPath() / "shaders" / "Coarse.comp");
		shaderFine = Shader::CreateCompute(Filesystem::AssetsPath() / "shaders" / "Fine.comp");

		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_BLEND);
	}

	void GPUPipeline::Shutdown()
	{

	}

	void GPUPipeline::Render()
	{
		Timer globalTimer;

		// 1.step: Transform the paths
		{
			Timer tsTimer;

			shaderTransform->Bind();
			shaderTransform->SetUniformMat4(0, Globals::GlobalTransform);

			constexpr uint32_t wgSize = 256;

			uint32_t xSize = glm::ceil(Globals::AllPaths.commands.size() / static_cast<float>(wgSize));
			const uint32_t ySize = glm::max(glm::ceil(static_cast<float>(xSize) / 65535.0f), 1.0f);
			shaderTransform->Dispatch(xSize, ySize, 1);

			glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

			SR_TRACE("Transforming paths: {0} ms", tsTimer.ElapsedMillis());
		}

		// 2.step: Calculate number of simple commands and their indexes for each path and each command in the path
		Timer timer2;
		shaderPreFlatten->Bind();
		{
			uint32_t ySize = glm::max(glm::ceil(Globals::AllPaths.paths.size() / 65535.0f), 1.0f);
			shaderPreFlatten->Dispatch(65535, ySize, 1);
		}

		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

		SR_TRACE("Step 2: {0} ms", timer2.ElapsedMillis());

		// 3.step: Simplify the commands and store in the array

		// 3.1: Flattening and computing bbox
		{
			Timer timerFlatten;
			shaderFlatten->Bind();
			{
				uint32_t ySize = glm::max(glm::ceil(Globals::AllPaths.paths.size() / 65535.0f), 1.0f);
				shaderFlatten->Dispatch(65535, ySize, 1);
			}

			glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

			SR_TRACE("Flattening: {0} ms", timerFlatten.ElapsedMillis());
		}

		// 4.step: The rest

		// 4.1: Calculate correct tile indices for each path according to its bounding box
		{
			Timer timerPreFill;
			shaderPreFill->Bind();
			{
				uint32_t ySize = glm::max(glm::ceil(Globals::AllPaths.paths.size() / 65535.0f), 1.0f);
				shaderPreFill->Dispatch(65535, ySize, 1);
			}

			glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

			SR_TRACE("Pre fill: {0} ms", timerPreFill.ElapsedMillis());
		}

		// 4.2: Filling
		{
			Timer timerFill;
			shaderFill->Bind();
			{
				uint32_t ySize = glm::max(glm::ceil(Globals::AllPaths.commands.size() / 65535.0f), 1.0f);
				shaderFill->Dispatch(65535, ySize, 1);
			}

			glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

			SR_TRACE("Filling: {0} ms", timerFill.ElapsedMillis());
		}

		// 4.3: Calculate correct count and indices for vertices of each path
		{
			Timer timerCalcQuads;
			shaderCalculateQuads->Bind();
			{
				uint32_t ySize = glm::max(glm::ceil(Globals::AllPaths.paths.size() / 65535.0f), 1.0f);
				shaderCalculateQuads->Dispatch(65535, ySize, 1);
			}

			glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

			SR_TRACE("Calculating quad counts: {0} ms", timerCalcQuads.ElapsedMillis());
		}

		// 4.4: Calculate correct tile indices for each path
		{
			Timer timerPrefixSum;
			shaderPrefixSum->Bind();
			shaderPrefixSum->Dispatch(1, 1, 1);

			glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

			SR_TRACE("Prefix sum: {0} ms", timerPrefixSum.ElapsedMillis());
		}

		// 4.5: Coarse
		{
			Timer timerCoarse;
			shaderCoarse->Bind();
			{
				uint32_t ySize = glm::max(glm::ceil(Globals::AllPaths.paths.size() / 65535.0f), 1.0f);
				shaderCoarse->Dispatch(65535, ySize, 1);
			}

			glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

			SR_TRACE("Coarse: {0} ms", timerCoarse.ElapsedMillis());
		}

		// 4.6: Fine
		{
			Timer timerFine;
			shaderFine->Bind();
			glBindImageTexture(0, m_AlphaTexture, 0, 0, 0, GL_WRITE_ONLY, GL_R32F);
			{
				uint32_t ySize = glm::max(glm::ceil(Globals::AllPaths.paths.size() / 65535.0f), 1.0f);
				shaderFine->Dispatch(65535, ySize, 1);
			}

			glMemoryBarrier(GL_ALL_BARRIER_BITS);

			SR_TRACE("Fine: {0} ms", timerFine.ElapsedMillis());
		}

		SR_INFO("Total execution time: {0} ms", globalTimer.ElapsedMillis());
	}

	void GPUPipeline::Final()
	{
		m_FinalShader->Bind();

		glUniform2ui(0, SCREEN_WIDTH, SCREEN_HEIGHT);
		glUniform2ui(1, ATLAS_SIZE, ATLAS_SIZE);
		glBindTextureUnit(0, m_AlphaTexture);

		glClearColor(1.0, 1.0, 1.0, 1.0);
		glClear(GL_COLOR_BUFFER_BIT);
		glDrawElements(GL_TRIANGLES, m_TileBuilder.indices.size(), GL_UNSIGNED_INT, nullptr);
	}

}