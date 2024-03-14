#include "Renderer/Pipeline/CPUPipeline.h"

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

	static void TransformCurve(PathRenderCmd* cmd)
	{
		uint32_t pathIndex = GET_CMD_PATH_INDEX(cmd->pathIndexCmdType);
		cmd->transformedPoints[0] = ApplyTransform(Globals::AllPaths.paths[pathIndex].transform, cmd->points[0]);
		cmd->transformedPoints[1] = ApplyTransform(Globals::AllPaths.paths[pathIndex].transform, cmd->points[1]);
		cmd->transformedPoints[2] = ApplyTransform(Globals::AllPaths.paths[pathIndex].transform, cmd->points[2]);
	}

	static glm::vec2 GetPreviousPoint(const PathRender& path, uint32_t index)
	{
		if (index == path.startCmdIndex)
		{
			return glm::vec2(0, 0);
		}

		PathRenderCmd& rndCmd = Globals::AllPaths.commands[index - 1];
		uint32_t pathType = GET_CMD_TYPE(rndCmd.pathIndexCmdType);
		switch (pathType)
		{
		case MOVE_TO:
		case LINE_TO:
			return rndCmd.transformedPoints[0];
		case QUAD_TO:
			return rndCmd.transformedPoints[1];
		case CUBIC_TO:
			return rndCmd.transformedPoints[2];
		}
	}

	void CPUPipeline::Init()
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

		glCreateTextures(GL_TEXTURE_2D, 1, &m_AlphaTexture);
		glTextureParameteri(m_AlphaTexture, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTextureParameteri(m_AlphaTexture, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTextureStorage2D(m_AlphaTexture, 1, GL_R32F, ATLAS_SIZE, ATLAS_SIZE);

		m_FinalShader = Shader::Create(Filesystem::AssetsPath() / "shaders" / "Main.vert", Filesystem::AssetsPath() / "shaders" / "Main.frag");
	}

	void CPUPipeline::Shutdown()
	{

	}

	void CPUPipeline::Render()
	{
		// Everything after this line may be done in each frame
		Timer globalTimer;

		// 1.step: Transform the paths
		{
			std::vector<uint32_t> indices;
			indices.resize(Globals::AllPaths.commands.size());
			std::iota(indices.begin(), indices.end(), 0);

			Timer tsTimer;
			std::for_each(executionPolicy, indices.begin(), indices.end(), [](uint32_t cmdIndex)
			{
				TransformCurve(&Globals::AllPaths.commands[cmdIndex]);
			});
			SR_TRACE("Transforming paths: {0} ms", tsTimer.ElapsedMillis());
		}

		// 2.step: Flattening
		uint32_t simpleCmdIndex = 0;
		for (uint32_t pathIndex = 0; pathIndex < Globals::AllPaths.paths.size(); pathIndex++)
		{
			const PathRender& path = Globals::AllPaths.paths[pathIndex];
			glm::vec2 last = { 0, 0 };
			bool wasLastMove = false;
			for (uint32_t cmdIndex = path.startCmdIndex; cmdIndex <= path.endCmdIndex; cmdIndex++)
			{
				std::vector<SimpleCommand> simpleCmds = Flattening::Flatten(cmdIndex, last, TOLERANCE, wasLastMove);
				PathRenderCmd& cmd = Globals::AllPaths.commands[cmdIndex];
				cmd.startIndexSimpleCommands = simpleCmdIndex;
				for (uint32_t i = 0; i < simpleCmds.size(); i++)
				{
					char ch = simpleCmds[i].type == MOVE_TO ? 'M' : 'L';
					std::cout << ch << ' ' << simpleCmds[i].point.x << ' ' << simpleCmds[i].point.y << ' ';
					Globals::AllPaths.simpleCommands[simpleCmdIndex++] = simpleCmds[i];
				}

				cmd.endIndexSimpleCommands = simpleCmdIndex;

				uint32_t cmdType = GET_CMD_TYPE(cmd.pathIndexCmdType);
				wasLastMove = false;
				switch (cmdType)
				{
				case MOVE_TO:
					wasLastMove = true;
					last = cmd.transformedPoints[0];
					break;
				case LINE_TO:
					last = cmd.transformedPoints[0];
					break;
				case QUAD_TO:
					last = cmd.transformedPoints[1];
					break;
				case CUBIC_TO:
					last = cmd.transformedPoints[2];
					break;
				}
			}
		}

		//// 2.step: Calculate number of simple commands and their indexes for each path and each command in the path
		//Timer timer2;
		//
		//std::atomic_uint32_t simpleCommandsCount = 0;
		//{
		//	std::vector<uint32_t> indices;
		//	indices.resize(Globals::AllPaths.paths.size());
		//	std::iota(indices.begin(), indices.end(), 0);
		//
		//	std::for_each(executionPolicy, indices.cbegin(), indices.cend(), [&simpleCommandsCount](uint32_t pathIndex)
		//	{
		//		const PathRender& path = Globals::AllPaths.paths[pathIndex];
		//
		//		std::vector<uint32_t> idx;
		//		idx.resize(path.endCmdIndex - path.startCmdIndex + 1);
		//		std::iota(idx.begin(), idx.end(), path.startCmdIndex);
		//
		//		std::for_each(executionPolicy, idx.cbegin(), idx.cend(), [&simpleCommandsCount, &path](uint32_t cmdIndex)
		//		{
		//			PathRenderCmd& rndCmd = Globals::AllPaths.commands[cmdIndex];
		//			glm::vec2 last = GetPreviousPoint(path, cmdIndex);
		//			uint32_t count = Flattening::CalculateNumberOfSimpleCommands(rndCmd, last, TOLERANCE);
		//			uint32_t xx = simpleCommandsCount.fetch_add(count);
		//			rndCmd.startIndexSimpleCommands = xx;
		//			rndCmd.endIndexSimpleCommands = xx + count;
		//		});
		//	});
		//	SR_TRACE("Step 2: {0} ms", timer2.ElapsedMillis());
		//}

		// Globals::AllPaths.simpleCommands.resize(simpleCommandsCount);

		// 3.step: Simplify the commands and store in the array

		// 3.1: Flattening
		{
			std::vector<uint32_t> indices;
			indices.resize(Globals::AllPaths.paths.size());
			std::iota(indices.begin(), indices.end(), 0);

			constexpr uint32_t wgSize = 256;
			std::array<uint32_t, wgSize> wgIndices;
			std::iota(wgIndices.begin(), wgIndices.end(), 0);

			Timer timerFlatten;
			std::for_each(executionPolicy, indices.cbegin(), indices.cend(), [&wgIndices, &wgSize](uint32_t pathIndex)
			{
				const PathRender& path = Globals::AllPaths.paths[pathIndex];
				for (uint32_t offsetCmdIndex = path.startCmdIndex; offsetCmdIndex <= path.endCmdIndex; offsetCmdIndex += wgSize)
				{
					std::for_each(executionPolicy, wgIndices.cbegin(), wgIndices.cend(), [&path, &offsetCmdIndex](uint32_t wgIndex)
					{
						uint32_t cmdIndex = wgIndex + offsetCmdIndex;
						if (cmdIndex > path.endCmdIndex)
						{
							return;
						}

						PathRenderCmd& rndCmd = Globals::AllPaths.commands[cmdIndex];
						glm::vec2 last = GetPreviousPoint(path, cmdIndex);
						Flattening::FlattenIntoArray(rndCmd, last, TOLERANCE);
					});
				}
			});
			SR_TRACE("Flattening: {0} ms", timerFlatten.ElapsedMillis());
		}

		// 3.2: Calculating BBOX
		{
			std::vector<uint32_t> indices;
			indices.resize(Globals::AllPaths.paths.size());
			std::iota(indices.begin(), indices.end(), 0);

			Timer timerBbox;
			std::for_each(executionPolicy, indices.cbegin(), indices.cend(), [](uint32_t pathIndex)
			{
				PathRender& path = Globals::AllPaths.paths[pathIndex];
				for (uint32_t cmdIndex = path.startCmdIndex; cmdIndex <= path.endCmdIndex; cmdIndex++)
				{
					PathRenderCmd& rndCmd = Globals::AllPaths.commands[cmdIndex];
					for (uint32_t i = rndCmd.startIndexSimpleCommands; i < rndCmd.endIndexSimpleCommands; i++)
					{
						path.bbox.AddPoint(Globals::AllPaths.simpleCommands[i].point);
					}
				}

				path.bbox.AddPadding({ 1.0f, 1.0f });
			});
			SR_TRACE("Calculating BBOX: {0} ms", timerBbox.ElapsedMillis());
		}

		// 4.step: The rest

		// 4.1: Calculate correct tile indices for each path according to its bounding box
		{
			std::vector<uint32_t> indices;
			indices.resize(Globals::AllPaths.paths.size());
			std::iota(indices.begin(), indices.end(), 0);

			Timer timer41;
			std::atomic_uint32_t tileCount = 0;
			std::for_each(executionPolicy, indices.cbegin(), indices.cend(), [&tileCount](uint32_t pathIndex)
			{
				PathRender& path = Globals::AllPaths.paths[pathIndex];
				if (!Flattening::IsBboxInsideViewSpace(path.bbox))
				{
					return;
				}

				// TODO: We are not interested in tiles, where x-coord of the tile is above SCREEN_WIDTH, or y-coord is above SCREEN_HEIGHT
				const int32_t minBboxCoordX = glm::floor(path.bbox.min.x);
				const int32_t minBboxCoordY = glm::floor(path.bbox.min.y);
				const int32_t maxBboxCoordX = glm::ceil(path.bbox.max.x);
				const int32_t maxBboxCoordY = glm::ceil(path.bbox.max.y);

				const int32_t minTileCoordX = glm::floor(static_cast<float>(minBboxCoordX) / TILE_SIZE);
				const int32_t minTileCoordY = glm::floor(static_cast<float>(minBboxCoordY) / TILE_SIZE);
				const int32_t maxTileCoordX = glm::ceil(static_cast<float>(maxBboxCoordX) / TILE_SIZE);
				const int32_t maxTileCoordY = glm::ceil(static_cast<float>(maxBboxCoordY) / TILE_SIZE);

				int32_t m_TileStartX = minTileCoordX;
				int32_t m_TileStartY = minTileCoordY;
				uint32_t m_TileCountX = maxTileCoordX - minTileCoordX + 1;
				uint32_t m_TileCountY = maxTileCoordY - minTileCoordY + 1;

				const uint32_t count = m_TileCountX * m_TileCountY;

				uint32_t xx = tileCount.fetch_add(count);
				path.startTileIndex = xx;
				path.endTileIndex = xx + count - 1;
			});
			SR_TRACE("Step 4.1: {0} ms", timer41.ElapsedMillis());
		}

		// 4.2: Filling
		{
			std::vector<uint32_t> indices;
			indices.resize(Globals::AllPaths.paths.size());
			std::iota(indices.begin(), indices.end(), 0);

			Timer timer43;
			std::for_each(executionPolicy, indices.cbegin(), indices.cend(), [this](uint32_t pathIndex)
			{
				PathRender& path = Globals::AllPaths.paths[pathIndex];
				if (!Flattening::IsBboxInsideViewSpace(path.bbox))
				{
					return;
				}

				Rasterizer rast(path.bbox, pathIndex);
				rast.FillFromArray(pathIndex);
			});
			SR_TRACE("Filling: {0}", timer43.ElapsedMillis());
		}

		// 4.3: Calculate correct count and indices for vertices of each path
		{
			std::vector<uint32_t> indices;
			indices.resize(Globals::AllPaths.paths.size());
			std::iota(indices.begin(), indices.end(), 0);

			Timer timer43;
			std::for_each(executionPolicy, indices.cbegin(), indices.cend(), [this](uint32_t pathIndex)
			{
				PathRender& path = Globals::AllPaths.paths[pathIndex];
				if (!Flattening::IsBboxInsideViewSpace(path.bbox))
				{
					return;
				}

				Rasterizer rast(path.bbox, pathIndex);

				auto [coarseQuadCount, fineQuadCount] = rast.CalculateNumberOfQuads();
				path.startSpanQuadIndex = coarseQuadCount;
				path.startTileQuadIndex = fineQuadCount;
			});
			SR_TRACE("Step 4.3: {0}", timer43.ElapsedMillis());
		}

		// 4.4: Calculate correct tile indices for each path
		{
			Timer timerPrefixSum;
			uint32_t accumCount = 0;
			uint32_t accumTileCount = 0;
			for (uint32_t pathIndex = 0; pathIndex < Globals::AllPaths.paths.size(); pathIndex++)
			{
				PathRender& path = Globals::AllPaths.paths[pathIndex];
				if (!Flattening::IsBboxInsideViewSpace(path.bbox))
				{
					continue;
				}

				Rasterizer rast(path.bbox, pathIndex);

				uint32_t coarseQuadCount = path.startSpanQuadIndex;
				uint32_t fineQuadCount = path.startTileQuadIndex;
				path.startSpanQuadIndex = accumCount;
				path.startTileQuadIndex = accumCount + coarseQuadCount;
				accumCount += coarseQuadCount + fineQuadCount;

				path.startVisibleTileIndex = accumTileCount;
				accumTileCount += fineQuadCount;
			}

			m_RenderIndicesCount = accumCount * 6;
			SR_TRACE("Prefix sum: {0}", timerPrefixSum.ElapsedMillis());
		}

		// 4.5: Coarse
		{
			std::vector<uint32_t> indices;
			indices.resize(Globals::AllPaths.paths.size());
			std::iota(indices.begin(), indices.end(), 0);

			Timer timerCoarse;
			std::for_each(executionPolicy, indices.cbegin(), indices.cend(), [this](uint32_t pathIndex)
			{
				const PathRender& path = Globals::AllPaths.paths[pathIndex];
				if (!Flattening::IsBboxInsideViewSpace(path.bbox))
				{
					return;
				}

				Rasterizer rast(path.bbox, pathIndex);
				rast.Coarse(m_TileBuilder);
			});
			SR_TRACE("Coarse: {0}", timerCoarse.ElapsedMillis());
		}

		// 4.6: Fine
		{
			std::vector<uint32_t> indices;
			indices.resize(Globals::AllPaths.paths.size());
			std::iota(indices.begin(), indices.end(), 0);

			Timer timerFine;
			std::for_each(executionPolicy, indices.cbegin(), indices.cend(), [this](uint32_t pathIndex)
			{
				const PathRender& path = Globals::AllPaths.paths[pathIndex];
				if (!Flattening::IsBboxInsideViewSpace(path.bbox))
				{
					return;
				}

				Rasterizer rast(path.bbox, pathIndex);
				rast.Finish(m_TileBuilder);
			});
			SR_TRACE("Fine: {0}", timerFine.ElapsedMillis());
		}

		glCreateBuffers(1, &m_Vbo);
		{
			GLenum bufferFlags = GL_CLIENT_STORAGE_BIT | GL_MAP_READ_BIT;
			glNamedBufferData(m_Vbo, m_TileBuilder.vertices.size() * sizeof(Vertex), m_TileBuilder.vertices.data(), GL_STATIC_DRAW);
			//glNamedBufferStorage(m_Vbo, m_TileBuilder.vertices.size() * sizeof(Vertex), m_TileBuilder.vertices.data(), bufferFlags);
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

		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		glTextureSubImage2D(m_AlphaTexture, 0, 0, 0, ATLAS_SIZE, ATLAS_SIZE, GL_RED, GL_FLOAT, m_TileBuilder.atlas.data());

		SR_INFO("Total execution time: {0} ms", globalTimer.ElapsedMillis());
	}

	void CPUPipeline::Final()
	{
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_BLEND);

		m_FinalShader->Bind();

		glUniform2ui(0, SCREEN_WIDTH, SCREEN_HEIGHT);
		glUniform2ui(1, ATLAS_SIZE, ATLAS_SIZE);
		glBindTextureUnit(0, m_AlphaTexture);

		glClearColor(1.0, 1.0, 1.0, 1.0);
		glClear(GL_COLOR_BUFFER_BIT);
		glDrawElements(GL_TRIANGLES, m_TileBuilder.indices.size(), GL_UNSIGNED_INT, nullptr);
	}

}