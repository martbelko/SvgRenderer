#pragma once

namespace SvgRenderer {

	enum class FramebufferTextureFormat
	{
		None = 0,

		// Color
		RGBA8,
		RED_INTEGER,

		// Depth/stencil
		DEPTH24STENCIL8,

		// Defaults
		Depth = DEPTH24STENCIL8
	};

	struct FramebufferTextureDesc
	{
		FramebufferTextureDesc() = default;
		FramebufferTextureDesc(FramebufferTextureFormat format)
			: textureFormat(format) {}

		FramebufferTextureFormat textureFormat = FramebufferTextureFormat::None;
		// TODO: filtering/wrap
	};

	struct FramebufferAttachmentDesc
	{
		FramebufferAttachmentDesc() = default;
		FramebufferAttachmentDesc(std::initializer_list<FramebufferTextureDesc> attachments)
			: attachments(attachments) {}

		std::vector<FramebufferTextureDesc> attachments;
	};

	struct FramebufferDesc
	{
		uint32_t width = 0, height = 0;
		FramebufferAttachmentDesc attachments;
		uint32_t samples = 1;

		bool swapChainTarget = false;
	};

	class Framebuffer
	{
	public:
		Framebuffer(const FramebufferDesc& desc);
		~Framebuffer();

		void Bind() const;

		void Resize(uint32_t width, uint32_t height);
		int ReadPixel(uint32_t attachmentIndex, int x, int y);

		void ClearAttachment(uint32_t attachmentIndex, int value);

		uint32_t GetColorAttachmentRendererID(uint32_t index = 0) const { return m_ColorAttachments[index]; }

		const FramebufferDesc& GetSpecification() const { return m_Desc; }
	public:
		static void BindDefaultFramebuffer();

		static Ref<Framebuffer> Create(const FramebufferDesc& desc)
		{
			return CreateRef<Framebuffer>(desc);
		}
	private:
		void Invalidate();
	private:
		uint32_t m_RendererID = 0;
		FramebufferDesc m_Desc;

		std::vector<FramebufferTextureDesc> m_ColorAttachmentDescs;
		FramebufferTextureDesc m_DepthAttachmentDesc = FramebufferTextureFormat::None;

		std::vector<uint32_t> m_ColorAttachments;
		uint32_t m_DepthAttachment = 0;
	};

}
