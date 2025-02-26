
#pragma once
#include <string>
#include <vector>
#include "SampleBase.hpp"
#include "BasicMath.hpp"

namespace Diligent
{

    class Tutorial03_Texturing final : public SampleBase
    {
        public:
            virtual void Initialize(const SampleInitInfo& InitInfo) override final;
            virtual void Render() override final;
            virtual void Update(double CurrTime, double ElapsedTime) override final;
            virtual bool HandleNativeMessage(const void* pNativeMsgData) override final;
            virtual const Char* GetSampleName() const override final { return "Tutorial03_Texturing"; }

        private:
            void CreatePipelineState();
            void CreateInstanceBuffer();
            void UpdateUI();
            void PopulateInstanceBuffer();
            void UpdateCameraMatrices();
            void HandleMouseEvent(int x, int y, bool buttonDown, bool buttonUp, int wheel);

            bool                 m_MouseCaptured = false;
            int                  m_ActiveWindow  = -1;
            int                  m_GridSize   = 5;
            static constexpr int MaxGridSize  = 32;
            static constexpr int MaxInstances = MaxGridSize * MaxGridSize * MaxGridSize;

            
            float2 m_LastMousePos  = {0.0f, 0.0f};
            struct CameraParams
            {
                float2 PanOffset     = {0.0f, 0.0f};
                float  Zoom          = 1.0f;
                float  OrbitAngleX   = -0.8f;
                float  OrbitAngleY   = 0.0f;
                float  OrbitDistance = 20.0f;
                float3 Position = {0.0f, 0.0f, 20.0f};
                float  RotX     = 0.0f;
                float  RotY     = 0.0f;
                float  RotZ     = 0.0f;
                float  ViewZoom = 0.01f;
            };

            RefCntAutoPtr<IPipelineState>         m_pPSO;
            RefCntAutoPtr<IBuffer>                m_CubeVertexBuffer;
            RefCntAutoPtr<IBuffer>                m_CubeIndexBuffer;
            RefCntAutoPtr<IBuffer>                m_InstanceBuffer;
            RefCntAutoPtr<IBuffer>                m_InstanceColorBuffDesc;
            RefCntAutoPtr<IBuffer>                m_VSConstants;
            RefCntAutoPtr<ITextureView>           m_TextureSRV;
            RefCntAutoPtr<IShaderResourceBinding> m_SRB;

            CameraParams CamaraPan;
            CameraParams CamaraOrb;

            float4x4 VistaPan;
            float4x4 VistaOrb;
            float4x4 m_ViewProjMatrix;
            float4x4 m_RotationMatrix;
            
    };

} 