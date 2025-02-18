
#include "Tutorial02_Cube.hpp"
#include "MapHelper.hpp"
#include "GraphicsUtilities.h"
#include "ColorConversion.h"



#include <d3d11.h>
#include "Tutorial02_Cube.hpp"
#include "MapHelper.hpp"
#include "GraphicsUtilities.h"
#include "imgui.h"
#include "backends/imgui_impl_win32.h"
#include "backends/imgui_impl_dx11.h"

#include "RenderDeviceD3D11.h"
#include "DeviceContextD3D11.h"



#include "imgui.h"          // Cabecera principal de ImGui
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"

namespace Diligent
{

SampleBase* CreateSample()
{
    return new Tutorial02_Cube();
}

void Tutorial02_Cube::CreatePipelineState()
{

    GraphicsPipelineStateCreateInfo PSOCreateInfo;

    PSOCreateInfo.PSODesc.Name = "Cube PSO";
    PSOCreateInfo.PSODesc.PipelineType = PIPELINE_TYPE_GRAPHICS;
    PSOCreateInfo.GraphicsPipeline.NumRenderTargets             = 1;
    PSOCreateInfo.GraphicsPipeline.RTVFormats[0]                = m_pSwapChain->GetDesc().ColorBufferFormat;
    PSOCreateInfo.GraphicsPipeline.DSVFormat                    = m_pSwapChain->GetDesc().DepthBufferFormat;
    PSOCreateInfo.GraphicsPipeline.PrimitiveTopology            = PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    PSOCreateInfo.GraphicsPipeline.RasterizerDesc.CullMode      = CULL_MODE_NONE; // Deshabilitar culling
    PSOCreateInfo.GraphicsPipeline.DepthStencilDesc.DepthEnable = True;

    ShaderCreateInfo ShaderCI;

    ShaderCI.SourceLanguage = SHADER_SOURCE_LANGUAGE_HLSL;
    ShaderCI.Desc.UseCombinedTextureSamplers = true;
    ShaderCI.CompileFlags = SHADER_COMPILE_FLAG_PACK_MATRIX_ROW_MAJOR;

    ShaderMacro Macros[] = {{"CONVERT_PS_OUTPUT_TO_GAMMA", m_ConvertPSOutputToGamma ? "1" : "0"}};
    ShaderCI.Macros      = {Macros, _countof(Macros)};

    RefCntAutoPtr<IShaderSourceInputStreamFactory> pShaderSourceFactory;
    m_pEngineFactory->CreateDefaultShaderSourceStreamFactory(nullptr, &pShaderSourceFactory);
    ShaderCI.pShaderSourceStreamFactory = pShaderSourceFactory;
    // Create a vertex shader
    RefCntAutoPtr<IShader> pVS;
    {
        ShaderCI.Desc.ShaderType = SHADER_TYPE_VERTEX;
        ShaderCI.EntryPoint      = "main";
        ShaderCI.Desc.Name       = "Cube VS";
        ShaderCI.FilePath        = "cube.vsh";
        m_pDevice->CreateShader(ShaderCI, &pVS);
        // Create dynamic uniform buffer that will store our transformation matrix
        // Dynamic buffers can be frequently updated by the CPU
        BufferDesc CBDesc;
        CBDesc.Name           = "VS constants CB";
        CBDesc.Size           = sizeof(float4x4);
        CBDesc.Usage          = USAGE_DYNAMIC;
        CBDesc.BindFlags      = BIND_UNIFORM_BUFFER;
        CBDesc.CPUAccessFlags = CPU_ACCESS_WRITE;
        m_pDevice->CreateBuffer(CBDesc, nullptr, &m_VSConstants);
    }

    // Create a pixel shader
    RefCntAutoPtr<IShader> pPS;
    {
        ShaderCI.Desc.ShaderType = SHADER_TYPE_PIXEL;
        ShaderCI.EntryPoint      = "main";
        ShaderCI.Desc.Name       = "Cube PS";
        ShaderCI.FilePath        = "cube.psh";
        m_pDevice->CreateShader(ShaderCI, &pPS);
    }

    // clang-format off
    // Define vertex shader input layout
    LayoutElement LayoutElems[] =
    {
        // Attribute 0 - vertex position
        LayoutElement{0, 0, 3, VT_FLOAT32, False},
        // Attribute 1 - vertex color
        LayoutElement{1, 0, 4, VT_FLOAT32, False}
    };
    // clang-format on
    PSOCreateInfo.GraphicsPipeline.InputLayout.LayoutElements = LayoutElems;
    PSOCreateInfo.GraphicsPipeline.InputLayout.NumElements    = _countof(LayoutElems);
    PSOCreateInfo.pVS = pVS;
    PSOCreateInfo.pPS = pPS;
    PSOCreateInfo.PSODesc.ResourceLayout.DefaultVariableType = SHADER_RESOURCE_VARIABLE_TYPE_STATIC;

    m_pDevice->CreateGraphicsPipelineState(PSOCreateInfo, &m_pPSO);
    m_pPSO->GetStaticVariableByName(SHADER_TYPE_VERTEX, "Constants")->Set(m_VSConstants);
    m_pPSO->CreateShaderResourceBinding(&m_pSRB, true);
}

void Tutorial02_Cube::CreateVertexBuffer()
{
    // Layout of this structure matches the one we defined in the pipeline state
    struct Vertex
    {
        float3 pos;
        float4 color;
    };

    constexpr Vertex CubeVerts[8] =
        {
            {float3{-1, -1, -1}, float4{1, 0, 0, 1}},
            {float3{-1, +1, -1}, float4{0, 1, 0, 1}},
            {float3{+1, +1, -1}, float4{0, 0, 1, 1}},
            {float3{+1, -1, -1}, float4{1, 1, 1, 1}},

            {float3{-1, -1, +1}, float4{1, 1, 0, 1}},
            {float3{-1, +1, +1}, float4{0, 1, 1, 1}},
            {float3{+1, +1, +1}, float4{1, 0, 1, 1}},
            {float3{+1, -1, +1}, float4{0.2f, 0.2f, 0.2f, 1.f}},
        };

    // Create a vertex buffer that stores cube vertices
    BufferDesc VertBuffDesc;
    VertBuffDesc.Name      = "Cube vertex buffer";
    VertBuffDesc.Usage     = USAGE_IMMUTABLE;
    VertBuffDesc.BindFlags = BIND_VERTEX_BUFFER;
    VertBuffDesc.Size      = sizeof(CubeVerts);
    BufferData VBData;
    VBData.pData    = CubeVerts;
    VBData.DataSize = sizeof(CubeVerts);
    m_pDevice->CreateBuffer(VertBuffDesc, &VBData, &m_CubeVertexBuffer);
}

void Tutorial02_Cube::CreateIndexBuffer()
{
    // clang-format off
    constexpr Uint32 Indices[] =
    {
        2,0,1, 2,3,0,
        4,6,5, 4,7,6,
        0,7,4, 0,3,7,
        1,0,4, 1,4,5,
        1,5,2, 5,6,2,
        3,6,7, 3,2,6
    };
    // clang-format on

    BufferDesc IndBuffDesc;
    IndBuffDesc.Name      = "Cube index buffer";
    IndBuffDesc.Usage     = USAGE_IMMUTABLE;
    IndBuffDesc.BindFlags = BIND_INDEX_BUFFER;
    IndBuffDesc.Size      = sizeof(Indices);
    BufferData IBData;
    IBData.pData    = Indices;
    IBData.DataSize = sizeof(Indices);
    m_pDevice->CreateBuffer(IndBuffDesc, &IBData, &m_CubeIndexBuffer);
}







void Tutorial02_Cube::CreatePyramidBuffers()
{
    struct Vertex
    {
        float3 pos;
        float4 color;
    };

    // Pirámide de base triangular con dimensiones proporcionales al cubo (2x2x2)
    constexpr Vertex PyramidVerts[4] =
        {
            {float3{-1.0f, -1.0f, -1.0f}, float4{1, 0, 0, 1}}, // 0: Esquina trasera izquierda
            {float3{1.0f, -1.0f, -1.0f}, float4{0, 1, 0, 1}},  // 1: Esquina trasera derecha
            {float3{0.0f, -1.0f, 1.0f}, float4{0, 0, 1, 1}},   // 2: Esquina frontal
            {float3{0.0f, 1.0f, 0.0f}, float4{1, 1, 0, 1}},    // 3: Punta superior
        };

    // Índices para las caras de la pirámide triangular
    constexpr Uint32 PyramidIndices[] =
        {
            2, 1, 0, // Base triangular
            0, 1, 3, // Cara trasera
            1, 2, 3, // Cara derecha
            2, 0, 3  // Cara izquierda
        };

    // Crear buffer de vértices
    BufferDesc VertBuffDesc;
    VertBuffDesc.Name      = "Pyramid vertex buffer";
    VertBuffDesc.Usage     = USAGE_IMMUTABLE;
    VertBuffDesc.BindFlags = BIND_VERTEX_BUFFER;
    VertBuffDesc.Size      = sizeof(PyramidVerts);
    BufferData VBData;
    VBData.pData    = PyramidVerts;
    VBData.DataSize = sizeof(PyramidVerts);
    m_pDevice->CreateBuffer(VertBuffDesc, &VBData, &m_PyramidVertexBuffer);

    // Crear buffer de índices
    BufferDesc IndBuffDesc;
    IndBuffDesc.Name      = "Pyramid index buffer";
    IndBuffDesc.Usage     = USAGE_IMMUTABLE;
    IndBuffDesc.BindFlags = BIND_INDEX_BUFFER;
    IndBuffDesc.Size      = sizeof(PyramidIndices);
    BufferData IBData;
    IBData.pData    = PyramidIndices;
    IBData.DataSize = sizeof(PyramidIndices);
    m_pDevice->CreateBuffer(IndBuffDesc, &IBData, &m_PyramidIndexBuffer);
}



void Tutorial02_Cube::CreateRhombusBuffers()
{
    struct Vertex
    {
        float3 pos;
        float4 color;
    };

    // Vértices del rombo (dos pirámides base con alturas invertidas)
    constexpr Vertex RhombusVerts[6] =
        {
            {float3{-1.0f, 0.0f, -1.0f}, float4{1, 0, 0, 1}}, // 0: Esquina trasera izquierda
            {float3{1.0f, 0.0f, -1.0f}, float4{0, 1, 0, 1}},  // 1: Esquina trasera derecha
            {float3{0.0f, 1.0f, 0.0f}, float4{0, 0, 1, 1}},   // 2: Punta superior
            {float3{0.0f, -1.0f, 0.0f}, float4{1, 1, 0, 1}},  // 3: Punta inferior
            {float3{1.0f, 0.0f, 1.0f}, float4{0, 1, 1, 1}},   // 4: Esquina frontal derecha
            {float3{-1.0f, 0.0f, 1.0f}, float4{1, 0, 1, 1}},  // 5: Esquina frontal izquierda
        };

    // Índices para las caras del rombo
    constexpr Uint32 RhombusIndices[] =
        {
            0, 1, 2, // Cara superior trasera
            1, 4, 2, // Cara superior derecha
            4, 5, 2, // Cara superior frontal
            5, 0, 2, // Cara superior izquierda
            0, 1, 3, // Cara inferior trasera
            1, 4, 3, // Cara inferior derecha
            4, 5, 3, // Cara inferior frontal
            5, 0, 3  // Cara inferior izquierda
        };

    // Crear buffer de vértices
    BufferDesc VertBuffDesc;
    VertBuffDesc.Name      = "Rhombus vertex buffer";
    VertBuffDesc.Usage     = USAGE_IMMUTABLE;
    VertBuffDesc.BindFlags = BIND_VERTEX_BUFFER;
    VertBuffDesc.Size      = sizeof(RhombusVerts);
    BufferData VBData;
    VBData.pData    = RhombusVerts;
    VBData.DataSize = sizeof(RhombusVerts);
    m_pDevice->CreateBuffer(VertBuffDesc, &VBData, &m_RhombusVertexBuffer);

    // Crear buffer de índices
    BufferDesc IndBuffDesc;
    IndBuffDesc.Name      = "Rhombus index buffer";
    IndBuffDesc.Usage     = USAGE_IMMUTABLE;
    IndBuffDesc.BindFlags = BIND_INDEX_BUFFER;
    IndBuffDesc.Size      = sizeof(RhombusIndices);
    BufferData IBData;
    IBData.pData    = RhombusIndices;
    IBData.DataSize = sizeof(RhombusIndices);
    m_pDevice->CreateBuffer(IndBuffDesc, &IBData, &m_RhombusIndexBuffer);
}





void Tutorial02_Cube::CreateStarBuffers()
{
    struct Vertex
    {
        float3 pos;
        float4 color;
    };

    // Vértices de la estrella
    constexpr Vertex StarVerts[10] =
        {
            // Puntas de la estrella
            {float3{0.0f, 1.0f, 0.0f}, float4{1, 1, 0, 1}},
            {float3{0.0f, -1.0f, 0.0f}, float4{1, 1, 0, 1}},
            {float3{1.0f, 0.0f, 0.0f}, float4{1, 1, 0, 1}},
            {float3{-1.0f, 0.0f, 0.0f}, float4{1, 1, 0, 1}},
            {float3{0.0f, 0.0f, 1.0f}, float4{1, 1, 0, 1}},
            {float3{0.0f, 0.0f, -1.0f}, float4{1, 1, 0, 1}},
            {float3{0.5f, 0.5f, 0.5f}, float4{1, 0.5f, 0, 1}},
            {float3{-0.5f, 0.5f, 0.5f}, float4{1, 0.5f, 0, 1}},
            {float3{0.5f, -0.5f, 0.5f}, float4{1, 0.5f, 0, 1}},
            {float3{-0.5f, -0.5f, 0.5f}, float4{1, 0.5f, 0, 1}}};

    // Índices para las caras de la estrella
    constexpr Uint32 StarIndices[] =
        {
            0, 6, 2, // Cara superior derecha
            0, 7, 3, // Cara superior izquierda
            1, 8, 2, // Cara inferior derecha
            1, 9, 3, // Cara inferior izquierda
            4, 6, 7, // Cara frontal superior
            4, 8, 9, // Cara frontal inferior
            5, 6, 7, // Cara trasera superior
            5, 8, 9  // Cara trasera inferior
        };

    // Crear buffer de vértices
    BufferDesc VertBuffDesc;
    VertBuffDesc.Name      = "Star vertex buffer";
    VertBuffDesc.Usage     = USAGE_IMMUTABLE;
    VertBuffDesc.BindFlags = BIND_VERTEX_BUFFER;
    VertBuffDesc.Size      = sizeof(StarVerts);
    BufferData VBData;
    VBData.pData    = StarVerts;
    VBData.DataSize = sizeof(StarVerts);
    m_pDevice->CreateBuffer(VertBuffDesc, &VBData, &m_StarVertexBuffer);

    // Crear buffer de índices
    BufferDesc IndBuffDesc;
    IndBuffDesc.Name      = "Star index buffer";
    IndBuffDesc.Usage     = USAGE_IMMUTABLE;
    IndBuffDesc.BindFlags = BIND_INDEX_BUFFER;
    IndBuffDesc.Size      = sizeof(StarIndices);
    BufferData IBData;
    IBData.pData    = StarIndices;
    IBData.DataSize = sizeof(StarIndices);
    m_pDevice->CreateBuffer(IndBuffDesc, &IBData, &m_StarIndexBuffer);
}






void Tutorial02_Cube::CreateRectangleBuffers()
{
    struct Vertex
    {
        float3 pos;
        float4 color;
    };

    constexpr Vertex RectangleVerts[8] =
        {
            {float3{-0.1f, -0.5f, -0.1f}, float4{1, 0, 0, 1}},         // 0: Esquina inferior izquierda trasera
            {float3{0.1f, -0.5f, -0.1f}, float4{0, 1, 0, 1}},          // 1: Esquina inferior derecha trasera
            {float3{0.1f, 8.0f, -0.1f}, float4{0, 0, 1, 1}},           // 2: Esquina superior derecha trasera
            {float3{-0.1f, 8.0f, -0.1f}, float4{1, 1, 0, 1}},          // 3: Esquina superior izquierda trasera
            {float3{-0.1f, -0.5f, 0.1f}, float4{1, 0, 1, 1}},          // 4: Esquina inferior izquierda frontal
            {float3{0.1f, -0.5f, 0.1f}, float4{0, 1, 1, 1}},           // 5: Esquina inferior derecha frontal
            {float3{0.1f, 8.0f, 0.1f}, float4{1, 1, 1, 1}},            // 6: Esquina superior derecha frontal
            {float3{-0.1f, 8.0f, 0.1f}, float4{0.2f, 0.2f, 0.2f, 1.f}} // 7: Esquina superior izquierda frontal
        };

    constexpr Uint32 RectangleIndices[] =
        {
            0, 1, 2, 2, 3, 0, // Cara trasera
            4, 5, 6, 6, 7, 4, // Cara frontal
            0, 4, 7, 7, 3, 0, // Cara izquierda
            1, 5, 6, 6, 2, 1, // Cara derecha
            0, 1, 5, 5, 4, 0, // Cara inferior
            3, 2, 6, 6, 7, 3  // Cara superior
        };

    // Crear buffer de vértices
    BufferDesc VertBuffDesc;
    VertBuffDesc.Name      = "Rectangle vertex buffer";
    VertBuffDesc.Usage     = USAGE_IMMUTABLE;
    VertBuffDesc.BindFlags = BIND_VERTEX_BUFFER;
    VertBuffDesc.Size      = sizeof(RectangleVerts);
    BufferData VBData;
    VBData.pData    = RectangleVerts;
    VBData.DataSize = sizeof(RectangleVerts);
    m_pDevice->CreateBuffer(VertBuffDesc, &VBData, &m_RectangleVertexBuffer);

    // Crear buffer de índices
    BufferDesc IndBuffDesc;
    IndBuffDesc.Name      = "Rectangle index buffer";
    IndBuffDesc.Usage     = USAGE_IMMUTABLE;
    IndBuffDesc.BindFlags = BIND_INDEX_BUFFER;
    IndBuffDesc.Size      = sizeof(RectangleIndices);
    BufferData IBData;
    IBData.pData    = RectangleIndices;
    IBData.DataSize = sizeof(RectangleIndices);
    m_pDevice->CreateBuffer(IndBuffDesc, &IBData, &m_RectangleIndexBuffer);
}







void Tutorial02_Cube::CreateDiskBuffers()
{
    struct Vertex
    {
        float3 pos;
        float4 color;
    };

    const int   numSegments = 32;    // Segmentos
    const float radius      = 3.0f;  // Radio
    const float height      = 0.01f; // Altura

    std::vector<Vertex> DiskVerts;
    std::vector<Uint32> DiskIndices;

    DiskVerts.push_back({float3{0.0f, height, 0.0f}, float4{0.8f, 0.8f, 0.8f, 1.0f}}); // Centro

    for (int i = 0; i < numSegments; ++i)
    {
        float angle = 2.0f * PI_F * i / numSegments;
        float x     = radius * cos(angle);
        float z     = radius * sin(angle);
        DiskVerts.push_back({float3{x, height, z}, float4{0.8f, 0.8f, 0.8f, 1.0f}}); // Borde
    }

    for (int i = 1; i <= numSegments; ++i)
    {
        DiskIndices.push_back(0);                 
        DiskIndices.push_back(i);                   
        DiskIndices.push_back(i % numSegments + 1);
    }

    // Crear buffer de vértices
    BufferDesc VertBuffDesc;
    VertBuffDesc.Name      = "Disk vertex buffer";
    VertBuffDesc.Usage     = USAGE_IMMUTABLE;
    VertBuffDesc.BindFlags = BIND_VERTEX_BUFFER;
    VertBuffDesc.Size      = DiskVerts.size() * sizeof(Vertex);
    BufferData VBData;
    VBData.pData    = DiskVerts.data();
    VBData.DataSize = DiskVerts.size() * sizeof(Vertex);
    m_pDevice->CreateBuffer(VertBuffDesc, &VBData, &m_DiskVertexBuffer);

    // Crear buffer de índices
    BufferDesc IndBuffDesc;
    IndBuffDesc.Name      = "Disk index buffer";
    IndBuffDesc.Usage     = USAGE_IMMUTABLE;
    IndBuffDesc.BindFlags = BIND_INDEX_BUFFER;
    IndBuffDesc.Size      = DiskIndices.size() * sizeof(Uint32);
    BufferData IBData;
    IBData.pData    = DiskIndices.data();
    IBData.DataSize = DiskIndices.size() * sizeof(Uint32);
    m_pDevice->CreateBuffer(IndBuffDesc, &IBData, &m_DiskIndexBuffer);
}






void Tutorial02_Cube::UpdateViewMatrix()
{
    switch (vistaActual)
    {
        case 0:
            Vistas = float4x4::Translation(0.f, -10.0f, 50.0f);
            break;
        case 1:
            Vistas = float4x4::RotationX(-PI_F / 2.0f) * float4x4::Translation(0.f, 0.0f, 50.0f);
            break;
        case 2: 
            Vistas = float4x4::RotationX(PI_F / 2.0f) * float4x4::Translation(0.f, 0.0f, 50.0f);
            break;
        case 3:
            Vistas = float4x4::RotationY(-PI_F / 2.0f) * float4x4::Translation(0.0f, 0.0f, 50.0f);
            break;
        case 4:
            Vistas = float4x4::RotationY(PI_F / 2.0f) * float4x4::Translation(-0.0f, 0.0f, 50.0f);
            break;
        default:
            Vistas = float4x4::Identity();
            break;
    }
}


void Tutorial02_Cube::vistasRender()
{
    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Settings", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        const char* views[] = {"Norte", "Arriba", "Abajo", "Oeste", "Este"};
        if (ImGui::Combo("View", &vistaActual, views, IM_ARRAYSIZE(views)))
        {
            UpdateViewMatrix();
        }
    }
    ImGui::End();
}






void Tutorial02_Cube::Initialize(const SampleInitInfo& InitInfo)
{
    SampleBase::Initialize(InitInfo);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    ImGui::StyleColorsDark();
 

    CreatePipelineState();
    CreateVertexBuffer();
    CreateIndexBuffer();
    CreatePyramidBuffers();
    CreateRhombusBuffers();
    CreateStarBuffers();
    CreateRectangleBuffers();
    CreateDiskBuffers();
    vistaActual = 0;
    UpdateViewMatrix();
    vistasRender();
}






void Tutorial02_Cube::Render()
{


    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    auto* pRTV = m_pSwapChain->GetCurrentBackBufferRTV();
    auto* pDSV = m_pSwapChain->GetDepthBufferDSV();

    float4 ClearColor = {0.350f, 0.350f, 0.350f, 1.0f};
    m_pImmediateContext->ClearRenderTarget(pRTV, ClearColor.Data(), RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    m_pImmediateContext->ClearDepthStencil(pDSV, CLEAR_DEPTH_FLAG, 1.f, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    auto SrfPreTransform = GetSurfacePretransformMatrix(float3{0, 0, 1});
    auto Proj            = GetAdjustedProjectionMatrix(PI_F / 4.0f, 0.1f, 100.f);
    for (int i = 0; i < 13; ++i)
        m_WorldViewProjMatrix[i] = m_WorldViewProjMatrix[i] * Vistas * SrfPreTransform * Proj;



    // Disco
    {
        MapHelper<float4x4> CBConstants(m_pImmediateContext, m_VSConstants, MAP_WRITE, MAP_FLAG_DISCARD);
        *CBConstants = m_WorldViewProjMatrix[12];

        const Uint64 offset   = 0;
        IBuffer*     pBuffs[] = {m_DiskVertexBuffer};
        m_pImmediateContext->SetVertexBuffers(0, 1, pBuffs, &offset, RESOURCE_STATE_TRANSITION_MODE_TRANSITION, SET_VERTEX_BUFFERS_FLAG_RESET);
        m_pImmediateContext->SetIndexBuffer(m_DiskIndexBuffer, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
        m_pImmediateContext->SetPipelineState(m_pPSO);
        m_pImmediateContext->CommitShaderResources(m_pSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

        DrawIndexedAttribs DrawAttrs;
        DrawAttrs.IndexType  = VT_UINT32;
        DrawAttrs.NumIndices = 96;
        DrawAttrs.Flags      = DRAW_FLAG_VERIFY_ALL;
        m_pImmediateContext->DrawIndexed(DrawAttrs);
    }




    // Cubos
    for (int i = 0; i < 2; ++i)
    {
        MapHelper<float4x4> CBConstants(m_pImmediateContext, m_VSConstants, MAP_WRITE, MAP_FLAG_DISCARD);
        *CBConstants = m_WorldViewProjMatrix[i];

        const Uint64 offset   = 0;
        IBuffer*     pBuffs[] = {m_CubeVertexBuffer};
        m_pImmediateContext->SetVertexBuffers(0, 1, pBuffs, &offset, RESOURCE_STATE_TRANSITION_MODE_TRANSITION, SET_VERTEX_BUFFERS_FLAG_RESET);
        m_pImmediateContext->SetIndexBuffer(m_CubeIndexBuffer, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
        m_pImmediateContext->SetPipelineState(m_pPSO);
        m_pImmediateContext->CommitShaderResources(m_pSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

        DrawIndexedAttribs DrawAttrs;
        DrawAttrs.IndexType  = VT_UINT32;
        DrawAttrs.NumIndices = 36;
        DrawAttrs.Flags      = DRAW_FLAG_VERIFY_ALL;
        m_pImmediateContext->DrawIndexed(DrawAttrs);
    }

    // Piramides
    for (int i = 0; i < 2; ++i)
    {
        MapHelper<float4x4> CBConstants(m_pImmediateContext, m_VSConstants, MAP_WRITE, MAP_FLAG_DISCARD);
        *CBConstants = m_WorldViewProjMatrix[i + 2];

        const Uint64 offset   = 0;
        IBuffer*     pBuffs[] = {m_PyramidVertexBuffer};
        m_pImmediateContext->SetVertexBuffers(0, 1, pBuffs, &offset, RESOURCE_STATE_TRANSITION_MODE_TRANSITION, SET_VERTEX_BUFFERS_FLAG_RESET);
        m_pImmediateContext->SetIndexBuffer(m_PyramidIndexBuffer, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
        m_pImmediateContext->SetPipelineState(m_pPSO);
        m_pImmediateContext->CommitShaderResources(m_pSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

        DrawIndexedAttribs DrawAttrs;
        DrawAttrs.IndexType  = VT_UINT32;
        DrawAttrs.NumIndices = 18;
        DrawAttrs.Flags      = DRAW_FLAG_VERIFY_ALL;
        m_pImmediateContext->DrawIndexed(DrawAttrs);
    }


    // Rombos
    for (int i = 0; i < 2; ++i)
    {
        MapHelper<float4x4> CBConstants(m_pImmediateContext, m_VSConstants, MAP_WRITE, MAP_FLAG_DISCARD);
        *CBConstants = m_WorldViewProjMatrix[i + 4];

        const Uint64 offset   = 0;
        IBuffer*     pBuffs[] = {m_RhombusVertexBuffer};
        m_pImmediateContext->SetVertexBuffers(0, 1, pBuffs, &offset, RESOURCE_STATE_TRANSITION_MODE_TRANSITION, SET_VERTEX_BUFFERS_FLAG_RESET);
        m_pImmediateContext->SetIndexBuffer(m_RhombusIndexBuffer, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
        m_pImmediateContext->SetPipelineState(m_pPSO);
        m_pImmediateContext->CommitShaderResources(m_pSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

        DrawIndexedAttribs DrawAttrs;
        DrawAttrs.IndexType  = VT_UINT32;
        DrawAttrs.NumIndices = 24;
        DrawAttrs.Flags      = DRAW_FLAG_VERIFY_ALL;
        m_pImmediateContext->DrawIndexed(DrawAttrs);
    }



    // Estrellas
    for (int i = 0; i < 2; ++i)
    {
        MapHelper<float4x4> CBConstants(m_pImmediateContext, m_VSConstants, MAP_WRITE, MAP_FLAG_DISCARD);
        *CBConstants = m_WorldViewProjMatrix[i + 6];

        const Uint64 offset   = 0;
        IBuffer*     pBuffs[] = {m_StarVertexBuffer};
        m_pImmediateContext->SetVertexBuffers(0, 1, pBuffs, &offset, RESOURCE_STATE_TRANSITION_MODE_TRANSITION, SET_VERTEX_BUFFERS_FLAG_RESET);
        m_pImmediateContext->SetIndexBuffer(m_StarIndexBuffer, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
        m_pImmediateContext->SetPipelineState(m_pPSO);
        m_pImmediateContext->CommitShaderResources(m_pSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

        DrawIndexedAttribs DrawAttrs;
        DrawAttrs.IndexType  = VT_UINT32;
        DrawAttrs.NumIndices = 24; 
        DrawAttrs.Flags      = DRAW_FLAG_VERIFY_ALL;
        m_pImmediateContext->DrawIndexed(DrawAttrs);
    }






    // Rectangulos
    for (int i = 0; i < 4; ++i)
    {
        MapHelper<float4x4> CBConstants(m_pImmediateContext, m_VSConstants, MAP_WRITE, MAP_FLAG_DISCARD);
        *CBConstants = m_WorldViewProjMatrix[i + 6];

        const Uint64 offset   = 0;
        IBuffer*     pBuffs[] = {m_RectangleVertexBuffer};
        m_pImmediateContext->SetVertexBuffers(0, 1, pBuffs, &offset, RESOURCE_STATE_TRANSITION_MODE_TRANSITION, SET_VERTEX_BUFFERS_FLAG_RESET);
        m_pImmediateContext->SetIndexBuffer(m_RectangleIndexBuffer, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
        m_pImmediateContext->SetPipelineState(m_pPSO);
        m_pImmediateContext->CommitShaderResources(m_pSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

        DrawIndexedAttribs DrawAttrs;
        DrawAttrs.IndexType  = VT_UINT32;
        DrawAttrs.NumIndices = 36;
        DrawAttrs.Flags      = DRAW_FLAG_VERIFY_ALL;
        m_pImmediateContext->DrawIndexed(DrawAttrs);
    }


    
    
}















void Tutorial02_Cube::Update(double CurrTime, double ElapsedTime)
{
    SampleBase::Update(CurrTime, ElapsedTime);
    vistasRender();

    float4x4 GlobalRotation = float4x4::RotationY(static_cast<float>(CurrTime) * 1.0f);
    float4x4 ScaleMatrix    = float4x4::Scale(0.5f, 0.5f, 0.5f);

    // Cubos
    float4x4 TranslateLeft  = float4x4::Translation(-2.0f, 0.0f, 0.0f); // Norte
    float4x4 TranslateRight = float4x4::Translation(2.0f, 0.0f, 0.0f);  // Sur
    m_WorldViewProjMatrix[0] = ScaleMatrix * TranslateLeft * GlobalRotation;
    m_WorldViewProjMatrix[1] = ScaleMatrix * TranslateRight * GlobalRotation;

    // Piramides
    float4x4 TranslateFront = float4x4::Translation(0.0f, 0.0f, -2.0f); // Oeste
    float4x4 TranslateBack  = float4x4::Translation(0.0f, 0.0f, 2.0f);  // Este
    m_WorldViewProjMatrix[2] = ScaleMatrix * TranslateFront * GlobalRotation;
    m_WorldViewProjMatrix[3] = ScaleMatrix * TranslateBack * GlobalRotation;

    // Rombos
    float4x4 TranslateDownLeft  = float4x4::Translation(-2.0f, -2.0f, 0.0f); // Debajo del cubo izquierdo
    float4x4 TranslateDownRight = float4x4::Translation(2.0f, -2.0f, 0.0f);  // Debajo del cubo derecho
    m_WorldViewProjMatrix[4] = ScaleMatrix * TranslateDownLeft * GlobalRotation;
    m_WorldViewProjMatrix[5] = ScaleMatrix * TranslateDownRight * GlobalRotation;


    // Estrellas
    float4x4 TranslateStarLeft  = float4x4::Translation(0.0f, -2.0f, -2.0f); // Debajo de la pirámide izquierda (frente)
    float4x4 TranslateStarRight = float4x4::Translation(0.0f, -2.0f, 2.0f);  // Debajo de la pirámide derecha (atrás)
    m_WorldViewProjMatrix[6]    = ScaleMatrix * TranslateStarLeft * GlobalRotation;
    m_WorldViewProjMatrix[7]    = ScaleMatrix * TranslateStarRight * GlobalRotation;


    // Rectangulos
    float4x4 TranslateRectLeft  = float4x4::Translation(-2.0f, -2.0f, 0.0f);
    float4x4 TranslateRectRight = float4x4::Translation(2.0f, -2.0f, 0.0f);
    float4x4 TranslateRectUp    = float4x4::Translation(0.0f, -2.0f, -2.0f);
    float4x4 TranslateRectDown  = float4x4::Translation(0.0f, -2.0f, 2.0f);
    m_WorldViewProjMatrix[8]    = ScaleMatrix * TranslateRectLeft * GlobalRotation;
    m_WorldViewProjMatrix[9]    = ScaleMatrix * TranslateRectRight * GlobalRotation;
    m_WorldViewProjMatrix[10]    = ScaleMatrix * TranslateRectUp * GlobalRotation;
    m_WorldViewProjMatrix[11]    = ScaleMatrix * TranslateRectDown * GlobalRotation;

    // Disco
    float4x4 TranslateDisk   = float4x4::Translation(0.0f, 2.0f, 0.0f);
    m_WorldViewProjMatrix[12] = TranslateDisk * GlobalRotation;



    float4x4 View = float4x4::Translation(0.f, 0.0f, 10.0f); // Alejar la cámara

    auto SrfPreTransform = GetSurfacePretransformMatrix(float3{0, 0, 1});
    auto Proj            = GetAdjustedProjectionMatrix(PI_F / 4.0f, 0.1f, 100.f);

    for (int i = 0; i < 13; ++i)
        m_WorldViewProjMatrix[i] = m_WorldViewProjMatrix[i] * View * SrfPreTransform * Proj;


}






} // namespace Diligent