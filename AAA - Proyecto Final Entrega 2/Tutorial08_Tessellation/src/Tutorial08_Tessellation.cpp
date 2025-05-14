#include "Tutorial08_Tessellation.hpp"
#include "MapHelper.hpp"
#include "GraphicsUtilities.h"
#include "TextureUtilities.h"
#include "ColorConversion.h"
#include "ShaderMacroHelper.hpp"
#include "imgui.h"

namespace Diligent
{

SampleBase* CreateSample()
{
    return new Tutorial08_Tessellation();
}

namespace
{

struct GlobalConstants
{
    unsigned int NumHorzBlocks;
    unsigned int NumVertBlocks;
    float        fNumHorzBlocks;
    float        fNumVertBlocks;

    float fBlockSize;
    float LengthScale;
    float HeightScale;
    float LineWidth;

    float  TessDensity;
    int    AdaptiveTessellation;
    float2 Dummy2;

    float4x4 WorldView;
    float4x4 WorldViewProj;
    float4   ViewportSize;
};

} // namespace

void Tutorial08_Tessellation::CreatePipelineStates()
{
    const bool bWireframeSupported = m_pDevice->GetDeviceInfo().Features.GeometryShaders;

    GraphicsPipelineStateCreateInfo PSOCreateInfo;
    PSOCreateInfo.PSODesc.Name = "Terrain PSO";
    PSOCreateInfo.PSODesc.PipelineType = PIPELINE_TYPE_GRAPHICS;
    PSOCreateInfo.GraphicsPipeline.NumRenderTargets             = 1;
    PSOCreateInfo.GraphicsPipeline.RTVFormats[0]                = m_pSwapChain->GetDesc().ColorBufferFormat;
    PSOCreateInfo.GraphicsPipeline.DSVFormat                    = m_pSwapChain->GetDesc().DepthBufferFormat;
    PSOCreateInfo.GraphicsPipeline.PrimitiveTopology            = PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST;
    PSOCreateInfo.GraphicsPipeline.RasterizerDesc.CullMode      = m_pDevice->GetDeviceInfo().IsGLDevice() ? CULL_MODE_FRONT : CULL_MODE_BACK;
    PSOCreateInfo.GraphicsPipeline.DepthStencilDesc.DepthEnable = True;

    CreateUniformBuffer(m_pDevice, sizeof(GlobalConstants), "Global shader constants CB", &m_ShaderConstants);

    ShaderCreateInfo ShaderCI;
    ShaderCI.SourceLanguage = SHADER_SOURCE_LANGUAGE_HLSL;
    ShaderCI.Desc.UseCombinedTextureSamplers = true;
    ShaderCI.CompileFlags = SHADER_COMPILE_FLAG_PACK_MATRIX_ROW_MAJOR;


    ShaderMacroHelper Macros;
    Macros.Add("BLOCK_SIZE", m_BlockSize);
    Macros.Add("CONVERT_PS_OUTPUT_TO_GAMMA", m_ConvertPSOutputToGamma);

    ShaderCI.Macros = Macros;

    RefCntAutoPtr<IShaderSourceInputStreamFactory> pShaderSourceFactory;
    m_pEngineFactory->CreateDefaultShaderSourceStreamFactory(nullptr, &pShaderSourceFactory);
    ShaderCI.pShaderSourceStreamFactory = pShaderSourceFactory;

    // Create a vertex shader
    RefCntAutoPtr<IShader> pVS;
    {
        ShaderCI.Desc.ShaderType = SHADER_TYPE_VERTEX;
        ShaderCI.EntryPoint      = "TerrainVS";
        ShaderCI.Desc.Name       = "Terrain VS";
        ShaderCI.FilePath        = "terrain.vsh";

        m_pDevice->CreateShader(ShaderCI, &pVS);
    }


    // Create a geometry shader
    RefCntAutoPtr<IShader> pGS;
    if (bWireframeSupported)
    {
        ShaderCI.Desc.ShaderType = SHADER_TYPE_GEOMETRY;
        ShaderCI.EntryPoint      = "TerrainGS";
        ShaderCI.Desc.Name       = "Terrain GS";
        ShaderCI.FilePath        = "terrain.gsh";

        m_pDevice->CreateShader(ShaderCI, &pGS);
    }

    // Create a hull shader
    RefCntAutoPtr<IShader> pHS;
    {
        ShaderCI.Desc.ShaderType = SHADER_TYPE_HULL;
        ShaderCI.EntryPoint      = "TerrainHS";
        ShaderCI.Desc.Name       = "Terrain HS";
        ShaderCI.FilePath        = "terrain.hsh";

        m_pDevice->CreateShader(ShaderCI, &pHS);
    }

    // Create a domain shader
    RefCntAutoPtr<IShader> pDS;
    {
        ShaderCI.Desc.ShaderType = SHADER_TYPE_DOMAIN;
        ShaderCI.EntryPoint      = "TerrainDS";
        ShaderCI.Desc.Name       = "Terrain DS";
        ShaderCI.FilePath        = "terrain.dsh";

        m_pDevice->CreateShader(ShaderCI, &pDS);
    }

    // Create a pixel shader
    RefCntAutoPtr<IShader> pPS, pWirePS;
    {
        ShaderCI.Desc.ShaderType = SHADER_TYPE_PIXEL;
        ShaderCI.EntryPoint      = "TerrainPS";
        ShaderCI.Desc.Name       = "Terrain PS";
        ShaderCI.FilePath        = "terrain.psh";

        m_pDevice->CreateShader(ShaderCI, &pPS);

        if (bWireframeSupported)
        {
            ShaderCI.EntryPoint = "WireTerrainPS";
            ShaderCI.Desc.Name  = "Wireframe Terrain PS";
            ShaderCI.FilePath   = "terrain_wire.psh";

            m_pDevice->CreateShader(ShaderCI, &pWirePS);
        }
    }

    PSOCreateInfo.pVS = pVS;
    PSOCreateInfo.pHS = pHS;
    PSOCreateInfo.pDS = pDS;
    PSOCreateInfo.pPS = pPS;
    PSOCreateInfo.PSODesc.ResourceLayout.DefaultVariableType = SHADER_RESOURCE_VARIABLE_TYPE_STATIC;

    ShaderResourceVariableDesc Vars[] = 
    {
        {SHADER_TYPE_HULL | SHADER_TYPE_DOMAIN,  "g_HeightMap", SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE},
        {SHADER_TYPE_PIXEL,                      "g_Texture",   SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE}
    };

    PSOCreateInfo.PSODesc.ResourceLayout.Variables    = Vars;
    PSOCreateInfo.PSODesc.ResourceLayout.NumVariables = _countof(Vars);

    SamplerDesc SamLinearClampDesc
    {
        FILTER_TYPE_LINEAR, FILTER_TYPE_LINEAR, FILTER_TYPE_LINEAR, 
        TEXTURE_ADDRESS_CLAMP, TEXTURE_ADDRESS_CLAMP, TEXTURE_ADDRESS_CLAMP
    };
    ImmutableSamplerDesc ImtblSamplers[] = 
    {
        {SHADER_TYPE_HULL | SHADER_TYPE_DOMAIN, "g_HeightMap", SamLinearClampDesc},
        {SHADER_TYPE_PIXEL,                     "g_Texture",   SamLinearClampDesc}
    };

    PSOCreateInfo.PSODesc.ResourceLayout.ImmutableSamplers    = ImtblSamplers;
    PSOCreateInfo.PSODesc.ResourceLayout.NumImmutableSamplers = _countof(ImtblSamplers);

    m_pDevice->CreateGraphicsPipelineState(PSOCreateInfo, &m_pPSO[0]);

    if (bWireframeSupported)
    {
        PSOCreateInfo.pGS = pGS;
        PSOCreateInfo.pPS = pWirePS;
        m_pDevice->CreateGraphicsPipelineState(PSOCreateInfo, &m_pPSO[1]);
    }

    for (Uint32 i = 0; i < _countof(m_pPSO); ++i)
    {
        if (m_pPSO[i])
        {
            m_pPSO[i]->GetStaticVariableByName(SHADER_TYPE_VERTEX, "VSConstants")->Set(m_ShaderConstants);
            m_pPSO[i]->GetStaticVariableByName(SHADER_TYPE_HULL,   "HSConstants")->Set(m_ShaderConstants);
            m_pPSO[i]->GetStaticVariableByName(SHADER_TYPE_DOMAIN, "DSConstants")->Set(m_ShaderConstants);
        }
    }
    if (m_pPSO[1])
    {
        m_pPSO[1]->GetStaticVariableByName(SHADER_TYPE_GEOMETRY, "GSConstants")->Set(m_ShaderConstants);
        m_pPSO[1]->GetStaticVariableByName(SHADER_TYPE_PIXEL,    "PSConstants")->Set(m_ShaderConstants);
    }
}

void Tutorial08_Tessellation::LoadTextures()
{
    {
        // Load texture
        TextureLoadInfo loadInfo;
        loadInfo.IsSRGB = false;
        loadInfo.Name   = "Terrain height map";
        RefCntAutoPtr<ITexture> HeightMap;
        CreateTextureFromFile("ps_height_1k.png", loadInfo, m_pDevice, &HeightMap);
        const auto& HMDesc = HeightMap->GetDesc();
        m_HeightMapWidth   = HMDesc.Width;
        m_HeightMapHeight  = HMDesc.Height;
        m_HeightMapSRV = HeightMap->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE);
    }

    {
        TextureLoadInfo loadInfo;
        loadInfo.IsSRGB = true;
        loadInfo.Name   = "Terrain color map";
        RefCntAutoPtr<ITexture> ColorMap;
        CreateTextureFromFile("ps_texture_2k.png", loadInfo, m_pDevice, &ColorMap);
        m_ColorMapSRV = ColorMap->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE);
    }


    for (size_t i = 0; i < _countof(m_pPSO); ++i)
    {
        if (m_pPSO[i])
        {
            m_pPSO[i]->CreateShaderResourceBinding(&m_SRB[i], true);
            m_SRB[i]->GetVariableByName(SHADER_TYPE_PIXEL,  "g_Texture")->Set(m_ColorMapSRV);
            m_SRB[i]->GetVariableByName(SHADER_TYPE_DOMAIN, "g_HeightMap")->Set(m_HeightMapSRV);
            m_SRB[i]->GetVariableByName(SHADER_TYPE_HULL,   "g_HeightMap")->Set(m_HeightMapSRV);
        }
    }
}

void Tutorial08_Tessellation::UpdateUI()
{
    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Settings", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Checkbox("Animate", &m_Animate);
        ImGui::Checkbox("Adaptive tessellation", &m_AdaptiveTessellation);
        if (m_pPSO[1])
            ImGui::Checkbox("Wireframe", &m_Wireframe);

        ImGui::Separator();
        ImGui::Text("Erosion Simulation");

        if (ImGui::SliderFloat("Erosion Level", &m_ErosionLevel, 0.0f, 1.0f))
        {
            // Apply erosion effects manually when slider changes
            if (!m_SimulatingErosion)
                ApplyErosionEffects();
        }

        ImGui::SliderFloat("Erosion Speed", &m_ErosionSpeed, 0.05f, 1.0f);

        // Erosion simulation buttons
        if (!m_SimulatingErosion)
        {
            if (ImGui::Button("Simular Erosion"))
            {
                m_SimulatingErosion = true;
                m_ErosionLevel      = 0.0f;
                ApplyErosionEffects();
            }
        }
        else
        {
            if (ImGui::Button("Detener Simulacion"))
            {
                m_SimulatingErosion = false;
            }
            // Show erosion progress
            ImGui::ProgressBar(m_ErosionLevel, ImVec2(0.0f, 0.0f));
            ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
            ImGui::Text("%.1f%%", m_ErosionLevel * 100.0f);
        }

        ImGui::Separator();
        ImGui::Text("Manual Controls");

        ImGui::SliderFloat("Tess density", &m_TessDensity, 1.f, 64.f);
        ImGui::SliderInt("Block size", &m_BlockSize, 16, 128);
        ImGui::SliderFloat("Height scale", &m_HeightScale, 1.0f, 25.0f);
        ImGui::SliderFloat("Length scale", &m_LengthScale, 5.0f, 15.0f);
        ImGui::SliderFloat("Distance", &m_Distance, 1.f, 20.f);
    }
    ImGui::End();
}

void Tutorial08_Tessellation::ModifyEngineInitInfo(const ModifyEngineInitInfoAttribs& Attribs)
{
    SampleBase::ModifyEngineInitInfo(Attribs);

    Attribs.EngineCI.Features.Tessellation    = DEVICE_FEATURE_STATE_ENABLED;
    Attribs.EngineCI.Features.GeometryShaders = DEVICE_FEATURE_STATE_OPTIONAL;
}

void Tutorial08_Tessellation::Initialize(const SampleInitInfo& InitInfo)
{
    SampleBase::Initialize(InitInfo);

    CreatePipelineStates();
    LoadTextures();
}

// Render a frame
void Tutorial08_Tessellation::Render()
{
    auto* pRTV = m_pSwapChain->GetCurrentBackBufferRTV();
    auto* pDSV = m_pSwapChain->GetDepthBufferDSV();
    // Clear the back buffer
    float4 ClearColor = {0.350f, 0.350f, 0.350f, 1.0f};
    if (m_ConvertPSOutputToGamma)
    {
        ClearColor = LinearToSRGB(ClearColor);
    }
    m_pImmediateContext->ClearRenderTarget(pRTV, ClearColor.Data(), RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    m_pImmediateContext->ClearDepthStencil(pDSV, CLEAR_DEPTH_FLAG, 1.f, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    unsigned int NumHorzBlocks = m_HeightMapWidth / m_BlockSize;
    unsigned int NumVertBlocks = m_HeightMapHeight / m_BlockSize;
    {
        // Map the buffer and write rendering data
        MapHelper<GlobalConstants> Consts(m_pImmediateContext, m_ShaderConstants, MAP_WRITE, MAP_FLAG_DISCARD);
        Consts->fBlockSize     = static_cast<float>(m_BlockSize);
        Consts->NumHorzBlocks  = NumHorzBlocks;
        Consts->NumVertBlocks  = NumVertBlocks;
        Consts->fNumHorzBlocks = static_cast<float>(NumHorzBlocks);
        Consts->fNumVertBlocks = static_cast<float>(NumVertBlocks);

        Consts->LengthScale = m_LengthScale;
        Consts->HeightScale = Consts->LengthScale / m_HeightScale;

        Consts->WorldView     = m_WorldViewMatrix;
        Consts->WorldViewProj = m_WorldViewProjMatrix;

        Consts->TessDensity          = m_TessDensity;
        Consts->AdaptiveTessellation = m_AdaptiveTessellation ? 1 : 0;

        const auto& SCDesc   = m_pSwapChain->GetDesc();
        Consts->ViewportSize = float4(static_cast<float>(SCDesc.Width), static_cast<float>(SCDesc.Height), 1.f / static_cast<float>(SCDesc.Width), 1.f / static_cast<float>(SCDesc.Height));

        Consts->LineWidth = 3.0f;
    }


    // Set the pipeline state
    m_pImmediateContext->SetPipelineState(m_pPSO[m_Wireframe ? 1 : 0]);
    m_pImmediateContext->CommitShaderResources(m_SRB[m_Wireframe ? 1 : 0], RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    DrawAttribs DrawAttrs;
    DrawAttrs.NumVertices = NumHorzBlocks * NumVertBlocks;
    DrawAttrs.Flags       = DRAW_FLAG_VERIFY_ALL;
    m_pImmediateContext->Draw(DrawAttrs);
}

void Tutorial08_Tessellation::Update(double CurrTime, double ElapsedTime)
{
    SampleBase::Update(CurrTime, ElapsedTime);

    // Update erosion simulation
    if (m_SimulatingErosion)
    {
        m_ErosionLevel += static_cast<float>(ElapsedTime) * m_ErosionSpeed;

        if (m_ErosionLevel >= 1.0f)
        {
            m_ErosionLevel      = 1.0f;
            m_SimulatingErosion = false;
        }

        ApplyErosionEffects();
    }

    UpdateUI();

    if (m_Animate)
    {
        m_RotationAngle += static_cast<float>(ElapsedTime) * 0.2f;
        if (m_RotationAngle > PI_F * 2.f)
            m_RotationAngle -= PI_F * 2.f;
    }

    float4x4 ModelMatrix = float4x4::RotationY(m_RotationAngle) * float4x4::RotationX(-PI_F * 0.1f);
    float4x4 ViewMatrix = float4x4::Translation(0.f, 0.0f, m_Distance);

    auto SrfPreTransform = GetSurfacePretransformMatrix(float3{0, 0, 1});
    auto Proj = GetAdjustedProjectionMatrix(PI_F / 4.0f, 0.1f, 1000.f);

    m_WorldViewMatrix = ModelMatrix * ViewMatrix * SrfPreTransform;
    m_WorldViewProjMatrix = m_WorldViewMatrix * Proj;
}

void Tutorial08_Tessellation::ApplyErosionEffects()
{
    // Store original values for reference
    static const float originalHeightScale = 5.0f;
    static const float originalTessDensity = 32.0f;
    static const int   originalBlockSize   = 32;
    static const float originalLengthScale = 10.0f;

    float erosionFactor = m_ErosionLevel;

    // Apply erosion effects:

    m_HeightScale = originalHeightScale + (erosionFactor * 15.0f);
    m_TessDensity = originalTessDensity + (erosionFactor * 24.0f);
    m_BlockSize = static_cast<int>(originalBlockSize - (erosionFactor * 16));
    m_LengthScale = originalLengthScale - (erosionFactor * 1.0f);

    m_HeightScale = std::max(1.0f, std::min(25.0f, m_HeightScale));
    m_TessDensity = std::max(1.0f, std::min(64.0f, m_TessDensity));
    m_BlockSize   = std::max(16, std::min(128, m_BlockSize));
    m_LengthScale = std::max(5.0f, std::min(15.0f, m_LengthScale));
}

} // namespace Diligent