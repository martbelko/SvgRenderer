#include "Renderer/Pipeline/GPUPipeline.h"

#include "Core/Filesystem.h"
#include "Core/Timer.h"

#include "Renderer/Flattening.h"
#include "Renderer/Rasterizer.h"

#include <glad/glad.h>

namespace SvgRenderer {

	static glm::vec2 ApplyTransform(const glm::mat4& transform, const glm::vec2& point)
	{
		return Globals::GlobalTransform * (transform * glm::vec4(point, 1.0f, 1.0f));
	}

	static void TransformCurve(PathRenderCmd& cmd)
	{
		uint32_t pathIndex = GET_CMD_PATH_INDEX(cmd.pathIndexCmdType);
		cmd.transformedPoints[0] = ApplyTransform(Globals::AllPaths.paths[pathIndex].transform, cmd.points[0]);
		cmd.transformedPoints[1] = ApplyTransform(Globals::AllPaths.paths[pathIndex].transform, cmd.points[1]);
		cmd.transformedPoints[2] = ApplyTransform(Globals::AllPaths.paths[pathIndex].transform, cmd.points[2]);
	}

	static glm::vec2 GetPreviousPoint(const PathRender& path, uint32_t index)
	{
		if (index == path.startCmdIndex)
		{
			return glm::vec2(0, 0);
		}

		PathRenderCmd& prevCmd = Globals::AllPaths.commands[index - 1];
		uint32_t pathType = GET_CMD_TYPE(prevCmd.pathIndexCmdType);
		switch (pathType)
		{
		case MOVE_TO:
		case LINE_TO:
			return prevCmd.transformedPoints[0];
		case QUAD_TO:
			return prevCmd.transformedPoints[1];
		case CUBIC_TO:
			return prevCmd.transformedPoints[2];
		}
	}

	static glm::vec2 GetPreviousFlattenedPoint(uint32_t pathIndex, uint32_t cmdIndex)
	{
		const PathRender& path = Globals::AllPaths.paths[pathIndex];
		if (cmdIndex == path.startCmdIndex)
		{
			return glm::vec2(0, 0);
		}

		const PathRenderCmd& cmd = Globals::AllPaths.commands[cmdIndex - 1];
		if (cmd.startIndexSimpleCommands != cmd.endIndexSimpleCommands)
		{
			return Globals::AllPaths.simpleCommands[cmd.endIndexSimpleCommands - 1].point;
		}

		uint32_t pathType = GET_CMD_TYPE(cmd.pathIndexCmdType);
		switch (pathType)
		{
		case MOVE_TO:
		{
			return cmd.transformedPoints[0];
		}
		}

		SR_ASSERT(false, "Invalid path type");
		return glm::vec2(0, 0);
	}

	// This remains fixed for the whole run of the program, may change in the future
	// For now, it is designed to store all the data from paris.svg,
	// but I am sure there is a way to resize this according to the size
	// we need for the SVG
	static constexpr uint32_t SIMPLE_COMMANDS_COUNT = 2'000'000;
	static constexpr uint32_t TILES_COUNT = 1'000'000;
	static constexpr uint32_t QUADS_COUNT = 250'000;
	static constexpr uint32_t VERTICES_COUNT = QUADS_COUNT * 4;
	static constexpr uint32_t INDICES_COUNT = QUADS_COUNT * 6;

	void GPUPipeline::Init()
	{
		Globals::PathsCount = static_cast<uint32_t>(Globals::AllPaths.paths.size());
		Globals::CommandsCount = static_cast<uint32_t>(Globals::AllPaths.commands.size());

		Globals::AllPaths.simpleCommands.resize(SIMPLE_COMMANDS_COUNT);
		Globals::Tiles.tiles.resize(TILES_COUNT);
		m_TileBuilder.vertices.resize(VERTICES_COUNT);
		m_TileBuilder.indices.reserve(INDICES_COUNT);
		m_TileBuilder.atlas.resize(ATLAS_SIZE * ATLAS_SIZE, 0);

		// 4 vertices, 6 indices for 1 quad
		uint32_t vertexIndex = 0;
		for (size_t i = 0; i < INDICES_COUNT; i += 6)
		{
			m_TileBuilder.indices.push_back(vertexIndex + 0);
			m_TileBuilder.indices.push_back(vertexIndex + 1);
			m_TileBuilder.indices.push_back(vertexIndex + 2);
			m_TileBuilder.indices.push_back(vertexIndex + 0);
			m_TileBuilder.indices.push_back(vertexIndex + 2);
			m_TileBuilder.indices.push_back(vertexIndex + 3);
			vertexIndex += 4;
		}

		glCreateTextures(GL_TEXTURE_2D, 1, &m_AlphaTexture);
		glTextureParameteri(m_AlphaTexture, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTextureParameteri(m_AlphaTexture, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTextureStorage2D(m_AlphaTexture, 1, GL_R32F, ATLAS_SIZE, ATLAS_SIZE);

		glCreateBuffers(1, &m_Vbo);
		glCreateBuffers(1, &m_Ibo);

		glNamedBufferData(m_Ibo, m_TileBuilder.indices.size() * sizeof(uint32_t), m_TileBuilder.indices.data(), GL_STATIC_DRAW);

		glCreateVertexArrays(1, &m_Vao);
		glVertexArrayVertexBuffer(m_Vao, 0, m_Vbo, 0, sizeof(Vertex));
		glVertexArrayElementBuffer(m_Vao, m_Ibo);

		glEnableVertexArrayAttrib(m_Vao, 0);
		glVertexArrayAttribFormat(m_Vao, 0, 2, GL_INT, GL_FALSE, 0);
		glVertexArrayAttribBinding(m_Vao, 0, 0);

		glEnableVertexArrayAttrib(m_Vao, 1);
		glVertexArrayAttribFormat(m_Vao, 1, 2, GL_UNSIGNED_INT, GL_FALSE, 8);
		glVertexArrayAttribBinding(m_Vao, 1, 0);

		glEnableVertexArrayAttrib(m_Vao, 2);
		glVertexArrayAttribFormat(m_Vao, 2, 4, GL_UNSIGNED_BYTE, GL_TRUE, 16);
		glVertexArrayAttribBinding(m_Vao, 2, 0);

		glCreateBuffers(1, &m_ParamsBuf);
		glCreateBuffers(1, &m_PathsBuf);
		glCreateBuffers(1, &m_CmdsBuf);
		glCreateBuffers(1, &m_SimpleCmdsBuf);
		glCreateBuffers(1, &m_TilesBuf);
		glCreateBuffers(1, &m_VerticesBuf);
		glCreateBuffers(1, &m_AtlasBuf);
		glCreateBuffers(1, &m_AtomicsBuf);

		constexpr GLenum bufferFlags = GL_CLIENT_STORAGE_BIT | GL_MAP_READ_BIT | GL_DYNAMIC_STORAGE_BIT;
		glNamedBufferStorage(m_ParamsBuf, sizeof(ParamsBuf), nullptr, GL_DYNAMIC_STORAGE_BIT);
		glNamedBufferStorage(m_PathsBuf, Globals::AllPaths.paths.size() * sizeof(PathRender), Globals::AllPaths.paths.data(), bufferFlags);
		glNamedBufferStorage(m_CmdsBuf, Globals::AllPaths.commands.size() * sizeof(PathRenderCmd), Globals::AllPaths.commands.data(), bufferFlags);
		glNamedBufferStorage(m_SimpleCmdsBuf, Globals::AllPaths.simpleCommands.size() * sizeof(SimpleCommand), Globals::AllPaths.simpleCommands.data(), bufferFlags);
		glNamedBufferStorage(m_TilesBuf, Globals::Tiles.tiles.size() * sizeof(Tile), Globals::Tiles.tiles.data(), bufferFlags);
		glNamedBufferStorage(m_VerticesBuf, m_TileBuilder.vertices.size() * sizeof(Vertex), m_TileBuilder.vertices.data(), bufferFlags);
		glNamedBufferStorage(m_AtlasBuf, m_TileBuilder.atlas.size() * sizeof(float), m_TileBuilder.atlas.data(), bufferFlags);
		glNamedBufferStorage(m_AtomicsBuf, 3 * sizeof(uint32_t), nullptr, bufferFlags);

		glBindBufferBase(GL_UNIFORM_BUFFER, 0, m_ParamsBuf);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_PathsBuf);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, m_CmdsBuf);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, m_SimpleCmdsBuf);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, m_TilesBuf);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, m_VerticesBuf);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, m_AtlasBuf);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, m_AtomicsBuf);

		m_FinalShader = Shader::Create(Filesystem::AssetsPath() / "shaders" / "Main.vert", Filesystem::AssetsPath() / "shaders" / "Main.frag");
		m_ResetShader = Shader::CreateCompute(Filesystem::AssetsPath() / "shaders" / "Reset.comp");
		m_TransformShader = Shader::CreateCompute(Filesystem::AssetsPath() / "shaders" / "Transform.comp");
		m_CoarseBboxShader = Shader::CreateCompute(Filesystem::AssetsPath() / "shaders" / "CoarseBbox.comp");
		m_PreFlattenShader = Shader::CreateCompute(Filesystem::AssetsPath() / "shaders" / "PreFlatten.comp");
		m_FlattenShader = Shader::CreateCompute(Filesystem::AssetsPath() / "shaders" / "Flatten.comp");
		m_CalcBboxShader = Shader::CreateCompute(Filesystem::AssetsPath() / "shaders" / "CalcBbox.comp");
		m_PreFillShader = Shader::CreateCompute(Filesystem::AssetsPath() / "shaders" / "PreFill.comp");
		m_FillShader = Shader::CreateCompute(Filesystem::AssetsPath() / "shaders" / "Fill.comp");
		m_CalcQuadsShader = Shader::CreateCompute(Filesystem::AssetsPath() / "shaders" / "CalcQuads.comp");
		m_PrefixSumShader = Shader::CreateCompute(Filesystem::AssetsPath() / "shaders" / "PrefixSum.comp");
		m_CoarseShader = Shader::CreateCompute(Filesystem::AssetsPath() / "shaders" / "Coarse.comp");
	}

	void GPUPipeline::Shutdown()
	{
		glDeleteBuffers(1, &m_Vbo);
		glDeleteBuffers(1, &m_Ibo);
		glDeleteVertexArrays(1, &m_Vao);
		glDeleteTextures(1, &m_AlphaTexture);

		glDeleteBuffers(1, &m_ParamsBuf);
		glDeleteBuffers(1, &m_PathsBuf);
		glDeleteBuffers(1, &m_CmdsBuf);
		glDeleteBuffers(1, &m_SimpleCmdsBuf);
		glDeleteBuffers(1, &m_TilesBuf);
		glDeleteBuffers(1, &m_VerticesBuf);
		glDeleteBuffers(1, &m_AtlasBuf);
		glDeleteBuffers(1, &m_AtomicsBuf);
	}

	void GPUPipeline::Render()
	{
		Timer globalTimer;

		m_Params.globalTransform = Globals::GlobalTransform;
		m_Params.screenWidth = Globals::WindowWidth;
		m_Params.screenHeight = Globals::WindowHeight;

		glNamedBufferSubData(m_ParamsBuf, 0, sizeof(ParamsBuf), &m_Params);

		int maxWgCountX;
		int maxWgCountY;
		int maxWgCountZ;
		glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &maxWgCountX);
		glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, &maxWgCountY);
		glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, &maxWgCountZ);

		auto readData = [this]()
		{
			Timer timerCopy;
			glGetNamedBufferSubData(m_PathsBuf, 0, Globals::AllPaths.paths.size() * sizeof(PathRender), Globals::AllPaths.paths.data());
			glGetNamedBufferSubData(m_CmdsBuf, 0, Globals::AllPaths.commands.size() * sizeof(PathRenderCmd), Globals::AllPaths.commands.data());
			glGetNamedBufferSubData(m_SimpleCmdsBuf, 0, Globals::AllPaths.simpleCommands.size() * sizeof(SimpleCommand), Globals::AllPaths.simpleCommands.data());
			glGetNamedBufferSubData(m_TilesBuf, 0, Globals::Tiles.tiles.size() * sizeof(Tile), Globals::Tiles.tiles.data());
			glGetNamedBufferSubData(m_VerticesBuf, 0, m_TileBuilder.vertices.size() * sizeof(Vertex), m_TileBuilder.vertices.data());
			glGetNamedBufferSubData(m_AtlasBuf, 0, m_TileBuilder.atlas.size() * sizeof(float), m_TileBuilder.atlas.data());
			SR_WARN("Reading data: {0} ms", timerCopy.ElapsedMillis());
		};

		// 1.step: Reset all the data
		{
			Timer timer;

			constexpr uint32_t wgSize = 256;
			uint32_t threadsNeeded = std::max({ TILES_COUNT, VERTICES_COUNT, ATLAS_SIZE * ATLAS_SIZE, Globals::PathsCount });
			uint32_t wgs = glm::ceil(threadsNeeded / static_cast<float>(wgSize));
			uint32_t ySize = glm::ceil(wgs / static_cast<float>(maxWgCountX));
			uint32_t xSize = ySize == 1 ? wgs : maxWgCountX;

			m_ResetShader->Bind();
			m_ResetShader->Dispatch(xSize, ySize, 1);
			glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

			SR_TRACE("Reseting: {0} ms", timer.ElapsedMillis());
		}

		// 2.step: Transform the paths
		{
			Timer timer;

			constexpr uint32_t wgSize = 256;
			const uint32_t xSize = glm::ceil(Globals::CommandsCount / static_cast<float>(wgSize));
			const uint32_t ySize = glm::max(glm::ceil(static_cast<float>(xSize) / maxWgCountX), 1.0f);

			m_TransformShader->Bind();
			m_TransformShader->Dispatch(xSize, ySize, 1);
			glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

			SR_TRACE("Transforming: {0} ms", timer.ElapsedMillis());
		}

		// 3.step: Calculate coarse bounding box
		{
			Timer timer;

			constexpr uint32_t wgSize = 256;
			const uint32_t ySize = glm::max(glm::ceil(static_cast<float>(Globals::PathsCount) / maxWgCountX), 1.0f);
			const uint32_t xSize = ySize == 1 ? Globals::PathsCount : maxWgCountX;

			m_CoarseBboxShader->Bind();
			m_CoarseBboxShader->Dispatch(xSize, ySize, 1);
			glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

			SR_TRACE("Calculating Coarse BBOX: {0} ms", timer.ElapsedMillis());
		}

		// 4.step: Calculate number of simple commands for each path command and their indices (for flattening)
		{
			Timer timer;

			constexpr uint32_t wgSize = 256;
			uint32_t wgs = glm::ceil(Globals::CommandsCount / static_cast<float>(wgSize));
			uint32_t ySize = glm::ceil(wgs / static_cast<float>(maxWgCountX));
			uint32_t xSize = ySize == 1 ? wgs : maxWgCountX;

			m_PreFlattenShader->Bind();
			m_PreFlattenShader->Dispatch(xSize, ySize, 1);
			glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

			SR_TRACE("Pre-flatten: {0} ms", timer.ElapsedMillis());
		}

		// 5.step: Actually flatten all the commands
		{
			Timer timer;

			constexpr uint32_t wgSize = 256;
			uint32_t wgs = glm::ceil(Globals::CommandsCount / static_cast<float>(wgSize));
			uint32_t ySize = glm::ceil(wgs / static_cast<float>(maxWgCountX));
			uint32_t xSize = ySize == 1 ? wgs : maxWgCountX;

			m_FlattenShader->Bind();
			m_FlattenShader->Dispatch(xSize, ySize, 1);
			glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

			SR_TRACE("Flattening: {0} ms", timer.ElapsedMillis());
		}

		// 6.step: Calculating BBOX
		{
			Timer timer;

			uint32_t ySize = glm::ceil(Globals::PathsCount / static_cast<float>(maxWgCountX));
			uint32_t xSize = ySize == 1 ? Globals::PathsCount : maxWgCountX;

			m_CalcBboxShader->Bind();
			m_CalcBboxShader->Dispatch(xSize, ySize, 1);
			glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

			SR_TRACE("Calculating BBOX: {0} ms", timer.ElapsedMillis());
		}

		// 7.step: Calculate correct tile indices for each path according to its bounding box
		{
			Timer timer;

			uint32_t ySize = glm::ceil(Globals::PathsCount / static_cast<float>(maxWgCountX));
			uint32_t xSize = ySize == 1 ? Globals::PathsCount : maxWgCountX;

			m_PreFillShader->Bind();
			m_PreFillShader->Dispatch(xSize, ySize, 1);
			glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

			SR_TRACE("Pre-Fill: {0} ms", timer.ElapsedMillis());
		}

		// 8.step: Filling
		{
			Timer timer;

			uint32_t ySize = glm::ceil(Globals::CommandsCount / static_cast<float>(maxWgCountX));
			uint32_t xSize = ySize == 1 ? Globals::CommandsCount : maxWgCountX;

			m_FillShader->Bind();
			m_FillShader->Dispatch(xSize, ySize, 1);
			glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

			SR_TRACE("Fill: {0} ms", timer.ElapsedMillis());
		}

		// 9.step: Calculate correct count and indices for vertices of each path
		{
			Timer timer;

			uint32_t ySize = glm::ceil(Globals::PathsCount / static_cast<float>(maxWgCountX));
			uint32_t xSize = ySize == 1 ? Globals::PathsCount : maxWgCountX;

			m_CalcQuadsShader->Bind();
			m_CalcQuadsShader->Dispatch(xSize, ySize, 1);
			glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

			SR_TRACE("Calculating quads: {0} ms", timer.ElapsedMillis());
		}

		// 10.step: Prefix sum
		{
			Timer timer;

			uint32_t ySize = glm::ceil(Globals::PathsCount / static_cast<float>(maxWgCountX));
			uint32_t xSize = ySize == 1 ? Globals::PathsCount : maxWgCountX;

			m_PrefixSumShader->Bind();
			m_PrefixSumShader->Dispatch(1, 1, 1);
			glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

			SR_TRACE("Prefix sum: {0} ms", timer.ElapsedMillis());
		}

		// 11.step: Coarse
		{
			Timer timer;

			uint32_t ySize = glm::ceil(Globals::PathsCount / static_cast<float>(maxWgCountX));
			uint32_t xSize = ySize == 1 ? Globals::PathsCount : maxWgCountX;

			m_CoarseShader->Bind();
			m_CoarseShader->Dispatch(xSize, ySize, 1);
			glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

			SR_TRACE("Coarse: {0} ms", timer.ElapsedMillis());

			readData();
		}

		//{
		//	std::vector<uint32_t> indices;
		//	indices.resize(Globals::AllPaths.paths.size());
		//	std::iota(indices.begin(), indices.end(), 0);
		//
		//	Timer timerCoarse;
		//	ForEach(indices.cbegin(), indices.cend(), [this](uint32_t pathIndex)
		//		{
		//			const PathRender& path = Globals::AllPaths.paths[pathIndex];
		//			if (!path.isBboxVisible)
		//			{
		//				return;
		//			}
		//
		//			Rasterizer rast(pathIndex);
		//			rast.Coarse(m_TileBuilder);
		//		});
		//	SR_TRACE("Coarse: {0}", timerCoarse.ElapsedMillis());
		//}

		// 4.6: Fine
		{
			std::vector<uint32_t> indices;
			indices.resize(Globals::AllPaths.paths.size());
			std::iota(indices.begin(), indices.end(), 0);

			Timer timerFine;
			ForEach(indices.cbegin(), indices.cend(), [this](uint32_t pathIndex)
				{
					const PathRender& path = Globals::AllPaths.paths[pathIndex];
					if (!path.isBboxVisible)
					{
						return;
					}

					Rasterizer rast(pathIndex);
					rast.Fine(m_TileBuilder);
				});
			SR_TRACE("Fine: {0}", timerFine.ElapsedMillis());
		}

		glNamedBufferData(m_Vbo, m_TileBuilder.vertices.size() * sizeof(Vertex), m_TileBuilder.vertices.data(), GL_STATIC_DRAW);

		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		glTextureSubImage2D(m_AlphaTexture, 0, 0, 0, ATLAS_SIZE, ATLAS_SIZE, GL_RED, GL_FLOAT, m_TileBuilder.atlas.data());

		SR_INFO("Total execution time: {0} ms", globalTimer.ElapsedMillis());
	}

	void GPUPipeline::Final()
	{
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_BLEND);

		glBindVertexArray(m_Vao);
		m_FinalShader->Bind();

		glUniform2ui(0, Globals::WindowWidth, Globals::WindowHeight);
		glUniform2ui(1, ATLAS_SIZE, ATLAS_SIZE);
		glBindTextureUnit(0, m_AlphaTexture);

		glClearColor(1.0, 1.0, 1.0, 1.0);
		glClear(GL_COLOR_BUFFER_BIT);
		glDrawElements(GL_TRIANGLES, m_TileBuilder.indices.size(), GL_UNSIGNED_INT, nullptr);
	}

}