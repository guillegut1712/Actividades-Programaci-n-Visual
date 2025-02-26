
#include <random>
#define NOMINMAX
#include <Windows.h>
#include "Tutorial03_Texturing.hpp"
#include "MapHelper.hpp"
#include "GraphicsUtilities.h"
#include "TextureUtilities.h"
#include "ColorConversion.h"
#include "../../Common/src/TexturedCube.hpp"
#include "imgui.h"


namespace Diligent
{

    SampleBase* CreateSample()
    {
        return new Tutorial03_Texturing();
    }

    void Tutorial03_Texturing::CreatePipelineState()
    {
        LayoutElement LayoutElems[] =
        {
            LayoutElement{0, 0, 3, VT_FLOAT32, False},
            LayoutElement{1, 0, 2, VT_FLOAT32, False},
            LayoutElement{2, 1, 4, VT_FLOAT32, False, INPUT_ELEMENT_FREQUENCY_PER_INSTANCE},
            LayoutElement{3, 1, 4, VT_FLOAT32, False, INPUT_ELEMENT_FREQUENCY_PER_INSTANCE},
            LayoutElement{4, 1, 4, VT_FLOAT32, False, INPUT_ELEMENT_FREQUENCY_PER_INSTANCE},
            LayoutElement{5, 1, 4, VT_FLOAT32, False, INPUT_ELEMENT_FREQUENCY_PER_INSTANCE}
        };
 
        RefCntAutoPtr<IShaderSourceInputStreamFactory> pShaderSourceFactory;
        m_pEngineFactory->CreateDefaultShaderSourceStreamFactory(nullptr, &pShaderSourceFactory);

        TexturedCube::CreatePSOInfo CubePsoCI;
        CubePsoCI.pDevice                = m_pDevice;
        CubePsoCI.RTVFormat              = m_pSwapChain->GetDesc().ColorBufferFormat;
        CubePsoCI.DSVFormat              = m_pSwapChain->GetDesc().DepthBufferFormat;
        CubePsoCI.pShaderSourceFactory   = pShaderSourceFactory;
        CubePsoCI.VSFilePath             = "cube_inst.vsh";
        CubePsoCI.PSFilePath             = "cube_inst.psh";
        CubePsoCI.ExtraLayoutElements    = LayoutElems;
        CubePsoCI.NumExtraLayoutElements = _countof(LayoutElems);

        m_pPSO = TexturedCube::CreatePipelineState(CubePsoCI, m_ConvertPSOutputToGamma);
        CreateUniformBuffer(m_pDevice, sizeof(float4x4) * 2, "VS constants CB", &m_VSConstants);
        m_pPSO->GetStaticVariableByName(SHADER_TYPE_VERTEX, "Constants")->Set(m_VSConstants);
        m_pPSO->CreateShaderResourceBinding(&m_SRB, true);
    }

    void Tutorial03_Texturing::CreateInstanceBuffer()
    {
        BufferDesc InstBuffDesc;
        InstBuffDesc.Name = "Instance data buffer";
        InstBuffDesc.Usage     = USAGE_DEFAULT;
        InstBuffDesc.BindFlags = BIND_VERTEX_BUFFER;
        InstBuffDesc.Size      = sizeof(float4x4) * MaxInstances;
        m_pDevice->CreateBuffer(InstBuffDesc, nullptr, &m_InstanceBuffer);
        PopulateInstanceBuffer();
    }


    bool Tutorial03_Texturing::HandleNativeMessage(const void* pNativeMsgData)
    {
        const MSG* pMsg = reinterpret_cast<const MSG*>(pNativeMsgData);
        if (pMsg->message == WM_MOUSEMOVE || pMsg->message == WM_LBUTTONDOWN || pMsg->message == WM_LBUTTONUP || pMsg->message == WM_MOUSEWHEEL)
        {
            int  x          = LOWORD(pMsg->lParam);
            int  y          = HIWORD(pMsg->lParam);
            bool buttonDown = (pMsg->message == WM_LBUTTONDOWN);
            bool buttonUp   = (pMsg->message == WM_LBUTTONUP);
            int  wheel      = 0;
            if (pMsg->message == WM_MOUSEWHEEL)
            {
                wheel = GET_WHEEL_DELTA_WPARAM(pMsg->wParam) / WHEEL_DELTA;
            }
            HandleMouseEvent(x, y, buttonDown, buttonUp, wheel);
            return true; 
        }
        return false;
    }

    void Tutorial03_Texturing::UpdateCameraMatrices()
    {
        VistaPan = float4x4::Translation(CamaraPan.PanOffset.x, -CamaraPan.PanOffset.y, 0.0f) *
            float4x4::Scale(CamaraPan.Zoom, CamaraPan.Zoom, CamaraPan.Zoom) *
            float4x4::RotationX(-0.8f) *
            float4x4::Translation(0.f, 0.f, 20.0f);

        VistaOrb = float4x4::Translation(0.0f, 0.0f, -CamaraOrb.OrbitDistance + 2.0f * std::sin(CamaraOrb.OrbitAngleY)) *
            float4x4::RotationX(CamaraOrb.OrbitAngleX) *
            float4x4::RotationY(CamaraOrb.OrbitAngleY) *
            float4x4::Scale(1.0f, -1.0f, 1.0f);
    }

    void Tutorial03_Texturing::HandleMouseEvent(int x, int y, bool buttonDown, bool buttonUp, int wheel)
    {
        const auto& SCDesc     = m_pSwapChain->GetDesc();
        float       screenPosX = static_cast<float>(x) / SCDesc.Width;

        int windowIdx = -1;
        if (screenPosX < 1.0f / 2.0f)
            windowIdx = 0;
        else
            windowIdx = 1;

        if (buttonDown)
        {
            m_MouseCaptured = true;
            m_ActiveWindow  = windowIdx;
            m_LastMousePos  = float2(static_cast<float>(x), static_cast<float>(y));
        }
        else if (buttonUp)
        {
            m_MouseCaptured = false;
            m_ActiveWindow  = -1;
        }

        if (m_MouseCaptured)
        {
            float2 currentPos = float2(static_cast<float>(x), static_cast<float>(y));
            float2 delta      = currentPos - m_LastMousePos;

            switch (m_ActiveWindow)
            {
                case 0:                                           
                    CamaraPan.PanOffset.x += delta.x * 0.005f; 
                    CamaraPan.PanOffset.y -= delta.y * 0.005f;
                    break;

                case 1:                                          
                    CamaraOrb.OrbitAngleY -= delta.x * 0.01f;
                    CamaraOrb.OrbitAngleX -= delta.y * 0.01f; 
                    break;
            }
            m_LastMousePos = currentPos;
        }

        if (wheel != 0)
        {
            const float wheelValue = static_cast<float>(wheel);
            auto adjustZoomWindow1 = [](float zoomDelta, float currentZoom) 
            {
                float newZoom = currentZoom + zoomDelta;
                return newZoom < 0.1f ? 0.1f : (newZoom > 5.0f ? 5.0f : newZoom);
            };
            auto adjustDistanceWindow2 = [](float distanceDelta, float currentDistance)
            {
                float newDistance = currentDistance - distanceDelta;
                return newDistance < 5.0f ? 5.0f : (newDistance > 40.0f ? 40.0f : newDistance);
            };
            auto adjustZoomWindow3 = [](float zoomFactor, float currentZoom)
            {
                float newZoom = currentZoom * zoomFactor;
                return newZoom < 0.001f ? 0.001f : (newZoom > 0.1f ? 0.1f : newZoom);
            };

            switch (windowIdx)
            {
                case 0:
                    CamaraPan.Zoom = adjustZoomWindow1(wheelValue * 0.1f, CamaraPan.Zoom);
                    break;
                case 1:
                    CamaraOrb.OrbitDistance = adjustDistanceWindow2(wheelValue * 1.0f, CamaraOrb.OrbitDistance);
                    break;
            }
        }
    }

    void Tutorial03_Texturing::Initialize(const SampleInitInfo& InitInfo)
    {
        SampleBase::Initialize(InitInfo);
        CreatePipelineState();
        m_CubeVertexBuffer = TexturedCube::CreateVertexBuffer(m_pDevice, GEOMETRY_PRIMITIVE_VERTEX_FLAG_POS_TEX);
        m_CubeIndexBuffer  = TexturedCube::CreateIndexBuffer(m_pDevice);
        m_TextureSRV       = TexturedCube::LoadTexture(m_pDevice, "DGLogo.png")->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE);
        m_SRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_Texture")->Set(m_TextureSRV);
        CreateInstanceBuffer();
        VistaPan = float4x4::RotationX(-0.8f) * float4x4::Translation(0.f, 0.f, 20.0f);
        VistaOrb = float4x4::RotationX(-0.8f) * float4x4::Translation(0.f, 0.f, 20.0f);
        CamaraPan.Zoom = 1.0f;
        CamaraOrb.OrbitAngleX   = 3.0f; 
        CamaraOrb.OrbitAngleY   = 0.0f;
        CamaraOrb.OrbitDistance = 20.0f;
    }

    void Tutorial03_Texturing::UpdateUI()
    {
        UpdateCameraMatrices();

        ImGui::SetNextWindowPos(ImVec2(0, 600), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(750, 150), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Paneo y Zoom", nullptr))
        {
            ImGui::Text("Controles:");
            ImGui::SliderFloat("Control en X", &CamaraPan.PanOffset.x, -10.0f, 10.0f);
            ImGui::SliderFloat("Control en Y", &CamaraPan.PanOffset.y, -10.0f, 10.0f);
            ImGui::SliderFloat("Control de Zoom", &CamaraPan.Zoom, 0.1f, 5.0f);
        }
        ImGui::End();

        ImGui::SetNextWindowPos(ImVec2(770, 600), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(750, 150), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Orbit", nullptr))
        {
            ImGui::Text("Controles");
            ImGui::SliderFloat("Control en X", &CamaraOrb.OrbitAngleX, -PI_F, PI_F);
            ImGui::SliderFloat("Control en Y", &CamaraOrb.OrbitAngleY, -PI_F, PI_F);
            ImGui::SliderFloat("Control de Zoom", &CamaraOrb.OrbitDistance, 5.0f, 40.0f);
        }
        ImGui::End();
    }

    void Tutorial03_Texturing::PopulateInstanceBuffer()
    {
        std::vector<float4x4> InstanceData(MaxInstances);
        int                   instId = 0;
        static float mainRotation       = 0.0f;
        static float firstTierRotation  = 0.0f;
        static float secondTierRotation = 0.0f;
        mainRotation += 0.003f;      
        firstTierRotation += 0.003f;  
        secondTierRotation += 0.0005f; 
        float4x4 baseMatrix    = float4x4::Scale(1.6f, 0.1f, 1.6f) * float4x4::Translation(0.0f, 4.8f, 0.0f);
        InstanceData[instId++] = baseMatrix;
        float4x4 mainRotMatrix     = float4x4::RotationY(mainRotation);
        float4x4 firstLevelMatrix  = mainRotMatrix * float4x4::RotationY(firstTierRotation);
        float4x4 secondLevelMatrix = firstLevelMatrix * float4x4::RotationY(secondTierRotation);
        float4x4 centerPoleMatrix = float4x4::Scale(0.1f, 1.0f, 0.1f) * float4x4::Translation(0.0f, 3.65f, 0.0f);
        InstanceData[instId++]    = centerPoleMatrix;

        float4x4 horizontalArm1 = float4x4::Scale(3.6f, 0.1f, 0.1f) * float4x4::Translation(0.0f, 2.6f, 0.0f) * firstLevelMatrix;
        float4x4 horizontalArm2 = float4x4::Scale(0.1f, 0.1f, 3.6f) * float4x4::Translation(0.0f, 2.6f, 0.0f) * firstLevelMatrix;
        InstanceData[instId++] = horizontalArm1;
        InstanceData[instId++] = horizontalArm2;

        float4x4 cubePositions[] = {
            float4x4::Translation(3.0f, 2.0f, 0.0f),
            float4x4::Translation(-3.0f, 2.0f, 0.0f),
            float4x4::Translation(0.0f, 2.0f, 3.0f),
            float4x4::Translation(0.0f, 2.0f, -3.0f)};

        for (const auto& pos : cubePositions)
        {
            float4x4 cubeMatrix    = float4x4::Scale(0.6f, 0.6f, 0.6f) * pos * firstLevelMatrix;
            InstanceData[instId++] = cubeMatrix;
        }

    
        float4x4 verticalConnectors[] = {
            float4x4::Scale(0.1f, 0.85f, 0.1f) * float4x4::Translation(0.0f, 0.85f, 3.0f),
            float4x4::Scale(0.1f, 0.85f, 0.1f) * float4x4::Translation(0.0f, 0.85f, -3.0f),
            float4x4::Scale(0.1f, 0.85f, 0.1f) * float4x4::Translation(3.0f, 0.85f, 0.0f),
            float4x4::Scale(0.1f, 0.85f, 0.1f) * float4x4::Translation(-3.0f, 0.85f, 0.0f)};

        for (const auto& connector : verticalConnectors)
        {
            InstanceData[instId++] = connector * secondLevelMatrix;
        }

        float4x4 secondLevelArms[] = {
            float4x4::Scale(2.0f, 0.1f, 0.1f) * float4x4::Translation(0.0f, 0.2f, 3.0f),
            float4x4::Scale(2.0f, 0.1f, 0.1f) * float4x4::Translation(0.0f, 0.2f, -3.0f),
            float4x4::Scale(0.1f, 0.1f, 2.0f) * float4x4::Translation(3.0f, 0.2f, 0.0f),
            float4x4::Scale(0.1f, 0.1f, 2.0f) * float4x4::Translation(-3.0f, 0.2f, 0.0f)};

        for (const auto& arm : secondLevelArms)
        {
            InstanceData[instId++] = arm * secondLevelMatrix;
        }

        float4x4 secondTierPositions[] =
        {
            float4x4::Translation(1.0f, -0.4f, 3.0f),
            float4x4::Translation(-1.0f, -0.4f, 3.0f),
            float4x4::Translation(1.0f, -0.4f, -3.0f),
            float4x4::Translation(-1.0f, -0.4f, -3.0f),
            float4x4::Translation(3.0f, -0.4f, 1.0f),
            float4x4::Translation(3.0f, -0.4f, -1.0f),
            float4x4::Translation(-3.0f, -0.4f, 1.0f),
            float4x4::Translation(-3.0f, -0.4f, -1.0f)
        };

        for (const auto& pos : secondTierPositions)
        {
            float4x4 cubeMatrix    = float4x4::Scale(0.6f, 0.6f, 0.6f) * pos * secondLevelMatrix;
            InstanceData[instId++] = cubeMatrix;
        }

        Uint32 DataSize = static_cast<Uint32>(sizeof(InstanceData[0]) * instId);
        m_pImmediateContext->UpdateBuffer(m_InstanceBuffer, 0, DataSize, InstanceData.data(), RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    }

    void Tutorial03_Texturing::Update(double CurrTime, double ElapsedTime)
    {
        SampleBase::Update(CurrTime, ElapsedTime);
        UpdateUI();
        auto SrfPreTransform = GetSurfacePretransformMatrix(float3{0, 0, 1});
        auto Proj = GetAdjustedProjectionMatrix(PI_F / 4.0f, 0.1f, 100.f);
        m_ViewProjMatrix = VistaPan * SrfPreTransform * Proj;
        m_RotationMatrix = float4x4::RotationY(static_cast<float>(CurrTime) * 0.f) * float4x4::RotationX(-static_cast<float>(CurrTime) * 0.f);
    }

    void Tutorial03_Texturing::Render()
    {
        auto* pRTV = m_pSwapChain->GetCurrentBackBufferRTV();
        auto* pDSV = m_pSwapChain->GetDepthBufferDSV();
        PopulateInstanceBuffer();
        float4 ClearColor = {0.350f, 0.350f, 0.350f, 1.0f};

        if (m_ConvertPSOutputToGamma)
        {
            ClearColor = LinearToSRGB(ClearColor);
        }

        m_pImmediateContext->ClearRenderTarget(pRTV, ClearColor.Data(), RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
        m_pImmediateContext->ClearDepthStencil(pDSV, CLEAR_DEPTH_FLAG, 1.f, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
        auto SrfPreTransform = GetSurfacePretransformMatrix(float3{0, 0, 1});
        auto Proj = GetAdjustedProjectionMatrix(PI_F / 4.0f, 0.1f, 100.f);
        const auto& SCDesc = m_pSwapChain->GetDesc();

        Viewport Viewports[3];
        Viewports[0].TopLeftX = 0;
        Viewports[0].TopLeftY = 0;
        Viewports[0].Width    = SCDesc.Width / 2;
        Viewports[0].Height   = SCDesc.Height;
        Viewports[0].MinDepth = 0;
        Viewports[0].MaxDepth = 1;
        Viewports[1].TopLeftX = SCDesc.Width / 2;
        Viewports[1].TopLeftY = 0;
        Viewports[1].Width    = SCDesc.Width / 2;
        Viewports[1].Height   = SCDesc.Height;
        Viewports[1].MinDepth = 0;
        Viewports[1].MaxDepth = 1;

        for (int viewIdx = 0; viewIdx < 3; viewIdx++)
        {
            m_pImmediateContext->SetViewports(1, &Viewports[viewIdx], SCDesc.Width, SCDesc.Height);
            float4x4 CurrentView;
            switch (viewIdx)
            {
                case 0: CurrentView = VistaPan; break;
                case 1: CurrentView = VistaOrb; break;
            }

            float4x4 ViewProj = CurrentView * SrfPreTransform * Proj;
            {
                MapHelper<float4x4> CBConstants(m_pImmediateContext, m_VSConstants, MAP_WRITE, MAP_FLAG_DISCARD);
                CBConstants[0] = ViewProj;
                CBConstants[1] = m_RotationMatrix;
            }

            const Uint64 offsets[] = {0, 0};
            IBuffer*     pBuffs[]  = {m_CubeVertexBuffer, m_InstanceBuffer};
            m_pImmediateContext->SetVertexBuffers(0, _countof(pBuffs), pBuffs, offsets, RESOURCE_STATE_TRANSITION_MODE_TRANSITION, SET_VERTEX_BUFFERS_FLAG_RESET);
            m_pImmediateContext->SetIndexBuffer(m_CubeIndexBuffer, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
            m_pImmediateContext->SetPipelineState(m_pPSO);
            m_pImmediateContext->CommitShaderResources(m_SRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
            DrawIndexedAttribs DrawAttrs;
            DrawAttrs.IndexType    = VT_UINT32;
            DrawAttrs.NumIndices   = 36;
            DrawAttrs.NumInstances = 24;
            DrawAttrs.Flags        = DRAW_FLAG_VERIFY_ALL;
            m_pImmediateContext->DrawIndexed(DrawAttrs);
        }
    }

} 