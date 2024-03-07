#pragma once

namespace SvgRenderer {

	class Pipeline
	{
	public:
		virtual ~Pipeline() = default;

		virtual void Init() = 0;
		virtual void Shutdown() = 0;

		virtual void Render() = 0;
		virtual void Final() = 0;
	};

}
