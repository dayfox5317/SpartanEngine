/*
Copyright(c) 2016-2018 Panos Karabelas

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
copies of the Software, and to permit persons to whom the Software is furnished
to do so, subject to the following conditions :

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

//= INCLUDES =====================
#include <memory>
#include <vector>
#include <unordered_map>
#include "../Math/Matrix.h"
#include "../Core/SubSystem.h"
#include "../RHI/RHI_Definition.h"
#include "../RHI/RHI_Pipeline.h"
//================================

namespace Directus
{
	class Actor;
	class Camera;
	class Skybox;
	class Light;
	class GBuffer;
	class Rectangle;
	class LightShader;
	class ResourceManager;
	class Font;
	class Variant;
	class Grid;
	namespace Math
	{
		class BoundingBox;
		class Frustum;
	}

	enum RenderMode : unsigned long
	{
		Render_Albedo				= 1UL << 0,
		Render_Normal				= 1UL << 1,
		Render_Material				= 1UL << 2,
		Render_Velocity				= 1UL << 3,
		Render_Depth				= 1UL << 4,	
		Render_Physics				= 1UL << 5,
		Render_AABB					= 1UL << 6,
		Render_PickingRay			= 1UL << 7,
		Render_SceneGrid			= 1UL << 8,
		Render_PerformanceMetrics	= 1UL << 9,
		Render_Light				= 1UL << 10,
		Render_Bloom				= 1UL << 11,
		Render_FXAA					= 1UL << 12,
		Render_SSDO					= 1UL << 13,
		Render_SSR					= 1UL << 14,
		Render_TAA					= 1UL << 15,
		Render_Sharpening			= 1UL << 16,
		Render_ChromaticAberration	= 1UL << 17,
		Render_Correction			= 1UL << 18, // Tone-mapping & Gamma correction
	};

	enum RenderableType
	{
		Renderable_ObjectOpaque,
		Renderable_ObjectTransparent,
		Renderable_Light,
		Renderable_Camera,
		Renderable_Skybox
	};

	class ENGINE_CLASS Renderer : public Subsystem
	{
	public:
		Renderer(Context* context, void* drawHandle);
		~Renderer();

		//= Subsystem =============
		bool Initialize() override;
		//=========================

		// Rendering
		void SetBackBufferAsRenderTarget(bool clear = true);
		void* GetFrameShaderResource();
		void Present();
		void Render();

		// The back-buffer is the final output (should match the display/window size)
		void SetBackBufferSize(unsigned int width, unsigned int height);
		// The actual frame that all rendering takes place (or the viewport window in the editor)
		void SetResolution(unsigned int width, unsigned int height);

		//= RENDER MODE ==================================================
		// Enables an render mode flag
		void Flags_Enable(RenderMode flag)		{ m_flags |= flag; }
		// Removes an render mode flag
		void Flags_Disable(RenderMode flag)		{ m_flags &= ~flag; }
		// Returns whether render mode flag is set
		bool Flags_IsSet(RenderMode flag)		{ return m_flags & flag; }
		//================================================================

		//= LINE RENDERING ==============================================================================================================
		void AddBoundigBox(const Math::BoundingBox& box, const Math::Vector4& color);
		void AddLine(const Math::Vector3& from, const Math::Vector3& to, const Math::Vector4& color) { AddLine(from, to, color, color); }
		void AddLine(const Math::Vector3& from, const Math::Vector3& to, const Math::Vector4& colorFrom, const Math::Vector4& colorTo);
		//===============================================================================================================================

		const std::shared_ptr<RHI_Device>& GetRHIDevice() { return m_rhiDevice; }
		static bool IsRendering()	{ return m_isRendering; }
		uint64_t GetFrameNum()		{ return m_frameNum; }
		Camera* GetCamera()			{ return m_camera; }

		//= Settings =============================
		// FXAA
		float m_fxaaSubPixel			= 1.75f;	// The amount of sub-pixel aliasing removal														- Default: 0.75f
		float m_fxaaEdgeThreshold		= 0.125f;	// Edge detection threshold. The minimum amount of local contrast required to apply algorithm.  - Default: 0.166f
		float m_fxaaEdgeThresholdMin	= 0.0312f;	// Darkness threshold. Trims the algorithm from processing darks								- Default: 0.0833f
		float m_bloomIntensity			= 0.2f;		// The intensity of the bloom
		float m_sharpenStrength			= 1.0f;		// Strength of the sharpening
		float m_sharpenClamp			= 0.35f;	// Limits maximum amount of sharpening a pixel receives											- Default: 0.035f
		//========================================

	private:
		void CreateRenderTextures(unsigned int width, unsigned int height);
		void SetGlobalBuffer(
			const Math::Matrix& mMVP			= Math::Matrix::Identity,
			unsigned int resolutionWidth		= Settings::Get().Resolution_GetWidth(),
			unsigned int resolutionHeight		= Settings::Get().Resolution_GetHeight(),
			float blur_sigma					= 0.0f,
			const Math::Vector2& blur_direction	= Math::Vector2::Zero
		);
		void Renderables_Acquire(const Variant& renderables);
		void Renderables_Sort(std::vector<Actor*>* renderables);

		//= PASSES ==============================================================================================================================================
		void Pass_DepthDirectionalLight(Light* directionalLight);
		void Pass_GBuffer();
		void Pass_PreLight(std::shared_ptr<RHI_RenderTexture>& texIn, std::shared_ptr<RHI_RenderTexture>& texOut, std::shared_ptr<RHI_RenderTexture>& texOut2);
		void Pass_Light(std::shared_ptr<RHI_RenderTexture>& texShadows, std::shared_ptr<RHI_RenderTexture>& texSSDO, std::shared_ptr<RHI_RenderTexture>& texOut);
		void Pass_PostLight(std::shared_ptr<RHI_RenderTexture>& texIn, std::shared_ptr<RHI_RenderTexture>& texOut);
		void Pass_TAA(std::shared_ptr<RHI_RenderTexture>& texIn, std::shared_ptr<RHI_RenderTexture>& texOut);
		void Pass_Transparent(std::shared_ptr<RHI_RenderTexture>& texOut);
		bool Pass_GBufferVisualize(std::shared_ptr<RHI_RenderTexture>& texOut);
		void Pass_Correction(std::shared_ptr<RHI_RenderTexture>& texIn, std::shared_ptr<RHI_RenderTexture>& texOut);
		void Pass_FXAA(std::shared_ptr<RHI_RenderTexture>& texIn, std::shared_ptr<RHI_RenderTexture>& texOut);
		void Pass_Sharpening(std::shared_ptr<RHI_RenderTexture>& texIn, std::shared_ptr<RHI_RenderTexture>& texOut);
		void Pass_ChromaticAberration(std::shared_ptr<RHI_RenderTexture>& texIn, std::shared_ptr<RHI_RenderTexture>& texOut);
		void Pass_Bloom(std::shared_ptr<RHI_RenderTexture>& texIn, std::shared_ptr<RHI_RenderTexture>& texOut);
		void Pass_BlurBox(std::shared_ptr<RHI_RenderTexture>& texIn, std::shared_ptr<RHI_RenderTexture>& texOut, float sigma);
		void Pass_BlurGaussian(std::shared_ptr<RHI_RenderTexture>& texIn, std::shared_ptr<RHI_RenderTexture>& texOut, float sigma);
		void Pass_BlurBilateralGaussian(std::shared_ptr<RHI_RenderTexture>& texIn, std::shared_ptr<RHI_RenderTexture>& texOut, float sigma, float pixelStride);
		void Pass_SSDO(std::shared_ptr<RHI_RenderTexture>& texOut);
		void Pass_ShadowMapping(std::shared_ptr<RHI_RenderTexture>& texOut, Light* inDirectionalLight);
		void Pass_Lines(std::shared_ptr<RHI_RenderTexture>& texOut);
		void Pass_Gizmos(std::shared_ptr<RHI_RenderTexture>& texOut);
		void Pass_PerformanceMetrics(std::shared_ptr<RHI_RenderTexture>& texOut);
		//=======================================================================================================================================================

		//= RENDER TEXTURES ===========================================
		// 1/1
		std::shared_ptr<RHI_RenderTexture> m_renderTexFull_Light;
		std::shared_ptr<RHI_RenderTexture> m_renderTexFull_TAA_Current;
		std::shared_ptr<RHI_RenderTexture> m_renderTexFull_TAA_History;
		std::shared_ptr<RHI_RenderTexture> m_renderTexFull_FinalFrame;
		// 1/2
		std::shared_ptr<RHI_RenderTexture> m_renderTexHalf_Shadows;
		std::shared_ptr<RHI_RenderTexture> m_renderTexHalf_SSDO;
		std::shared_ptr<RHI_RenderTexture> m_renderTexHalf_Spare;
		// 1/4
		std::shared_ptr<RHI_RenderTexture> m_renderTexQuarter_Blur1;
		std::shared_ptr<RHI_RenderTexture> m_renderTexQuarter_Blur2;
		//=============================================================

		//= SHADERS ===============================================
		std::shared_ptr<LightShader> m_shaderLight;
		std::shared_ptr<RHI_Shader> m_shaderLightDepth;
		std::shared_ptr<RHI_Shader> m_shaderLine;
		std::shared_ptr<RHI_Shader> m_shaderFont;
		std::shared_ptr<RHI_Shader> m_shaderTexture;
		std::shared_ptr<RHI_Shader> m_shaderFXAA;
		std::shared_ptr<RHI_Shader> m_shaderLuma;
		std::shared_ptr<RHI_Shader> m_shaderSSDO;
		std::shared_ptr<RHI_Shader> m_shaderTAA;
		std::shared_ptr<RHI_Shader> m_shaderShadowMapping;
		std::shared_ptr<RHI_Shader> m_shaderSharpening;
		std::shared_ptr<RHI_Shader> m_shaderChromaticAberration;
		std::shared_ptr<RHI_Shader> m_shaderBlurBox;
		std::shared_ptr<RHI_Shader> m_shaderBlurGaussian;
		std::shared_ptr<RHI_Shader> m_shaderBlurBilateralGaussian;
		std::shared_ptr<RHI_Shader> m_shaderBloom_Bright;
		std::shared_ptr<RHI_Shader> m_shaderBloom_BlurBlend;
		std::shared_ptr<RHI_Shader> m_shaderCorrection;
		std::shared_ptr<RHI_Shader> m_shaderTransformationGizmo;
		std::shared_ptr<RHI_Shader> m_shaderTransparent;
		//=========================================================

		//= SAMPLERS ===============================================
		std::shared_ptr<RHI_Sampler> m_samplerPointClampAlways;
		std::shared_ptr<RHI_Sampler> m_samplerPointClampGreater;
		std::shared_ptr<RHI_Sampler> m_samplerLinearClampGreater;
		std::shared_ptr<RHI_Sampler> m_samplerLinearWrapGreater;
		std::shared_ptr<RHI_Sampler> m_samplerLinearClampAlways;
		std::shared_ptr<RHI_Sampler> m_samplerBilinearClampAlways;
		std::shared_ptr<RHI_Sampler> m_samplerAnisotropicWrapAlways;
		//==========================================================

		//= PIPELINE STATES =============
		RHI_PipelineState m_pipelineLine;
		//===============================

		//= STANDARD TEXTURES ==================================
		std::shared_ptr<RHI_Texture> m_texNoiseNormal;
		std::shared_ptr<RHI_Texture> m_texWhite;
		std::shared_ptr<RHI_Texture> m_texBlack;
		std::shared_ptr<RHI_Texture> m_gizmoTexLightDirectional;
		std::shared_ptr<RHI_Texture> m_gizmoTexLightPoint;
		std::shared_ptr<RHI_Texture> m_gizmoTexLightSpot;
		//======================================================

		//= LINE RENDERING ==================================
		std::shared_ptr<RHI_VertexBuffer> m_lineVertexBuffer;
		unsigned int m_lineVertexCount = 0;
		std::vector<RHI_Vertex_PosCol> m_lineVertices;
		//===================================================

		//= MISC ========================================================
		std::shared_ptr<RHI_Device> m_rhiDevice;
		std::shared_ptr<RHI_Pipeline> m_rhiPipeline;
		std::unique_ptr<GBuffer> m_gbuffer;
		std::shared_ptr<RHI_Viewport> m_viewport;		
		std::unique_ptr<Rectangle> m_quad;
		std::unordered_map<RenderableType, std::vector<Actor*>> m_actors;
		Math::Matrix m_view;
		Math::Matrix m_viewBase;
		Math::Matrix m_projection;
		Math::Matrix m_projectionOrthographic;
		Math::Matrix m_viewProjection;
		Math::Matrix m_viewProjection_Orthographic;
		float m_nearPlane;
		float m_farPlane;
		Camera* m_camera;
		Light* GetLightDirectional();
		Skybox* GetSkybox();
		static bool m_isRendering;
		std::unique_ptr<Font> m_font;
		std::unique_ptr<Grid> m_grid;
		std::unique_ptr<Rectangle> m_gizmoRectLight;
		unsigned long m_flags;
		uint64_t m_frameNum;
		bool m_isOddFrame;		
		//===============================================================
		
		// Global buffer (holds what is need by almost every shader)
		struct ConstantBuffer_Global
		{
			Math::Matrix mMVP;
			Math::Matrix mView;
			Math::Matrix mProjection;	

			float camera_near;
			float camera_far;
			Math::Vector2 resolution;

			Math::Vector3 camera_position;
			float fxaa_subPixel;

			float fxaa_edgeThreshold;
			float fxaa_edgeThresholdMin;
			Math::Vector2 blur_direction;

			float blur_sigma;
			float bloom_intensity;
			float sharpen_strength;
			float sharpen_clamp;
		};
		std::shared_ptr<RHI_ConstantBuffer> m_bufferGlobal;
	};
}