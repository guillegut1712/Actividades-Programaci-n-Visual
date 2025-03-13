// Agregar estos includes adicionales
#include "Tutorial03_Texturing.hpp"
#include "MapHelper.hpp"
#include "GraphicsUtilities.h"
#include "TextureUtilities.h"
#include "ColorConversion.h"
#include "imgui.h"

namespace Diligent
{

// Método original CreateSample sin cambios


    SampleBase* CreateSample()
{
    return new Tutorial03_Texturing();
}



    void Tutorial03_Texturing::CreateVertexBuffer()
{
    // Layout of this structure matches the one we defined in the pipeline state
    struct Vertex
    {
        float3 pos;
        float2 uv;
        float3 normal; // Añadido para iluminación
    };

    // Define cube vertex data with normales
    Vertex CubeVerts[] =
        {
            // Cara frontal (z negativa)
            {float3{-1, -1, -1}, float2{0, 1}, float3{0, 0, -1}},
            {float3{-1, +1, -1}, float2{0, 0}, float3{0, 0, -1}},
            {float3{+1, +1, -1}, float2{1, 0}, float3{0, 0, -1}},
            {float3{+1, -1, -1}, float2{1, 1}, float3{0, 0, -1}},

            // Cara inferior (y negativa)
            {float3{-1, -1, -1}, float2{0, 1}, float3{0, -1, 0}},
            {float3{-1, -1, +1}, float2{0, 0}, float3{0, -1, 0}},
            {float3{+1, -1, +1}, float2{1, 0}, float3{0, -1, 0}},
            {float3{+1, -1, -1}, float2{1, 1}, float3{0, -1, 0}},

            // Cara derecha (x positiva)
            {float3{+1, -1, -1}, float2{0, 1}, float3{1, 0, 0}},
            {float3{+1, -1, +1}, float2{1, 1}, float3{1, 0, 0}},
            {float3{+1, +1, +1}, float2{1, 0}, float3{1, 0, 0}},
            {float3{+1, +1, -1}, float2{0, 0}, float3{1, 0, 0}},

            // Cara superior (y positiva)
            {float3{+1, +1, -1}, float2{0, 1}, float3{0, 1, 0}},
            {float3{+1, +1, +1}, float2{0, 0}, float3{0, 1, 0}},
            {float3{-1, +1, +1}, float2{1, 0}, float3{0, 1, 0}},
            {float3{-1, +1, -1}, float2{1, 1}, float3{0, 1, 0}},

            // Cara izquierda (x negativa)
            {float3{-1, +1, -1}, float2{1, 0}, float3{-1, 0, 0}},
            {float3{-1, +1, +1}, float2{0, 0}, float3{-1, 0, 0}},
            {float3{-1, -1, +1}, float2{0, 1}, float3{-1, 0, 0}},
            {float3{-1, -1, -1}, float2{1, 1}, float3{-1, 0, 0}},

            // Cara trasera (z positiva)
            {float3{-1, -1, +1}, float2{1, 1}, float3{0, 0, 1}},
            {float3{+1, -1, +1}, float2{0, 1}, float3{0, 0, 1}},
            {float3{+1, +1, +1}, float2{0, 0}, float3{0, 0, 1}},
            {float3{-1, +1, +1}, float2{1, 0}, float3{0, 0, 1}}};

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



    void Tutorial03_Texturing::CreateIndexBuffer()
{
    // clang-format off
    constexpr Uint32 Indices[] =
    {
        2,0,1,    2,3,0,
        4,6,5,    4,7,6,
        8,10,9,   8,11,10,
        12,14,13, 12,15,14,
        16,18,17, 16,19,18,
        20,21,22, 20,22,23
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


    // Implementación manual de producto vectorial (si lo necesitas)
float3 cross(const float3& a, const float3& b)
{
    return float3(
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x);
}

// Implementación manual de producto escalar (si lo necesitas)
float dot(const float3& a, const float3& b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}



void Tutorial03_Texturing::CreatePipelineState()
{




    ShaderCreateInfo ShaderCI;
    ShaderCI.SourceLanguage                  = SHADER_SOURCE_LANGUAGE_HLSL;
    ShaderCI.Desc.UseCombinedTextureSamplers = true;
    ShaderCI.CompileFlags                    = SHADER_COMPILE_FLAG_PACK_MATRIX_ROW_MAJOR;


    // Código original... con algunas modificaciones:

    // Modificar el shader para soportar iluminación
    ShaderMacro Macros[] = {
        {"CONVERT_PS_OUTPUT_TO_GAMMA", m_ConvertPSOutputToGamma ? "1" : "0"},
        {"ENABLE_SHADOWS", "1"}};
    ShaderCI.Macros = {Macros, _countof(Macros)};

    // Modificar los layout elements para incluir normales
    LayoutElement LayoutElems[] =
        {
            // Attribute 0 - vertex position
            LayoutElement{0, 0, 3, VT_FLOAT32, False},
            // Attribute 1 - texture coordinates
            LayoutElement{1, 0, 2, VT_FLOAT32, False},
            // Attribute 2 - normal
            LayoutElement{2, 0, 3, VT_FLOAT32, False}};

    // Agregar variables para las sombras y luces
    ShaderResourceVariableDesc Vars[] =
        {
            {SHADER_TYPE_PIXEL, "g_Texture", SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE},
            {SHADER_TYPE_PIXEL, "g_ShadowMap", SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE},
            {SHADER_TYPE_VERTEX, "g_LightAttribs", SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE}};

    // Resto del código original...


}

// Método para crear el shadow map
void Tutorial03_Texturing::CreateShadowMap()
{
    TextureDesc ShadowMapDesc;
    ShadowMapDesc.Name      = "Shadow map texture";
    ShadowMapDesc.Type      = RESOURCE_DIM_TEX_2D;
    ShadowMapDesc.Width     = 1024;
    ShadowMapDesc.Height    = 1024;
    ShadowMapDesc.Format    = TEX_FORMAT_D32_FLOAT;
    ShadowMapDesc.BindFlags = BIND_DEPTH_STENCIL | BIND_SHADER_RESOURCE;

    m_pDevice->CreateTexture(ShadowMapDesc, nullptr, &m_ShadowMap);
    m_ShadowMapSRV = m_ShadowMap->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE);
    m_ShadowMapRTV = m_ShadowMap->GetDefaultView(TEXTURE_VIEW_DEPTH_STENCIL);

    // Crear pipeline state para renderizar el shadow map
    GraphicsPipelineStateCreateInfo PSOCreateInfo;
    PSOCreateInfo.PSODesc.Name         = "Shadow Map PSO";
    PSOCreateInfo.PSODesc.PipelineType = PIPELINE_TYPE_GRAPHICS;

    // Configurar pipeline para render de sombras
    PSOCreateInfo.GraphicsPipeline.NumRenderTargets             = 0;
    PSOCreateInfo.GraphicsPipeline.DSVFormat                    = TEX_FORMAT_D32_FLOAT;
    PSOCreateInfo.GraphicsPipeline.PrimitiveTopology            = PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    PSOCreateInfo.GraphicsPipeline.RasterizerDesc.CullMode      = CULL_MODE_BACK;
    PSOCreateInfo.GraphicsPipeline.DepthStencilDesc.DepthEnable = True;

    // Crear shader para el shadow map
    ShaderCreateInfo ShaderCI;
    ShaderCI.SourceLanguage                  = SHADER_SOURCE_LANGUAGE_HLSL;
    ShaderCI.Desc.UseCombinedTextureSamplers = true;
    ShaderCI.CompileFlags                    = SHADER_COMPILE_FLAG_PACK_MATRIX_ROW_MAJOR;

    RefCntAutoPtr<IShaderSourceInputStreamFactory> pShaderSourceFactory;
    m_pEngineFactory->CreateDefaultShaderSourceStreamFactory(nullptr, &pShaderSourceFactory);
    ShaderCI.pShaderSourceStreamFactory = pShaderSourceFactory;

    // Vertex shader para el shadow map
    RefCntAutoPtr<IShader> pShadowMapVS;
    ShaderCI.Desc.ShaderType = SHADER_TYPE_VERTEX;
    ShaderCI.EntryPoint      = "main";
    ShaderCI.Desc.Name       = "Shadow Map VS";
    ShaderCI.FilePath        = "shadow_map.vsh";
    m_pDevice->CreateShader(ShaderCI, &pShadowMapVS);

    // Definir el input layout
    LayoutElement ShadowLayoutElems[] =
        {
            LayoutElement{0, 0, 3, VT_FLOAT32, False}};

    PSOCreateInfo.pVS                                         = pShadowMapVS;
    PSOCreateInfo.GraphicsPipeline.InputLayout.LayoutElements = ShadowLayoutElems;
    PSOCreateInfo.GraphicsPipeline.InputLayout.NumElements    = _countof(ShadowLayoutElems);

    m_pDevice->CreateGraphicsPipelineState(PSOCreateInfo, &m_pShadowMapPSO);
    if (!m_pShadowMapPSO)
    {
        LOG_ERROR("Failed to create shadow map PSO!");
        return;
    }
    m_pShadowMapPSO->CreateShaderResourceBinding(&m_ShadowMapSRB, true);
}

void Tutorial03_Texturing::CreateFloorGeometry()
{
    // Definir vertices para el piso
    struct Vertex
    {
        float3 pos;
        float2 uv;
        float3 normal;
    };

    // Crear piso como un plano grande
    Vertex FloorVerts[] = {
        {float3{-10.0f, -1.0f, -10.0f}, float2{0.0f, 0.0f}, float3{0.0f, 1.0f, 0.0f}},
        {float3{-10.0f, -1.0f, 10.0f}, float2{0.0f, 10.0f}, float3{0.0f, 1.0f, 0.0f}},
        {float3{10.0f, -1.0f, 10.0f}, float2{10.0f, 10.0f}, float3{0.0f, 1.0f, 0.0f}},
        {float3{10.0f, -1.0f, -10.0f}, float2{10.0f, 0.0f}, float3{0.0f, 1.0f, 0.0f}}};

    // Crear buffer del piso
    BufferDesc VertBuffDesc;
    VertBuffDesc.Name      = "Floor vertex buffer";
    VertBuffDesc.Usage     = USAGE_IMMUTABLE;
    VertBuffDesc.BindFlags = BIND_VERTEX_BUFFER;
    VertBuffDesc.Size      = sizeof(FloorVerts);
    BufferData VBData;
    VBData.pData    = FloorVerts;
    VBData.DataSize = sizeof(FloorVerts);
    m_pDevice->CreateBuffer(VertBuffDesc, &VBData, &m_FloorVertexBuffer);

    // Índices para el piso
    Uint32 FloorIndices[] = {
        0, 1, 2,
        0, 2, 3};

    BufferDesc IndBuffDesc;
    IndBuffDesc.Name      = "Floor index buffer";
    IndBuffDesc.Usage     = USAGE_IMMUTABLE;
    IndBuffDesc.BindFlags = BIND_INDEX_BUFFER;
    IndBuffDesc.Size      = sizeof(FloorIndices);
    BufferData IBData;
    IBData.pData    = FloorIndices;
    IBData.DataSize = sizeof(FloorIndices);
    m_pDevice->CreateBuffer(IndBuffDesc, &IBData, &m_FloorIndexBuffer);
}

void Tutorial03_Texturing::LoadTextures()
{

    // Código original para la textura del cubo
    TextureLoadInfo loadInfo;
    loadInfo.IsSRGB = true;
    RefCntAutoPtr<ITexture> Tex;
    CreateTextureFromFile("DGLogo.png", loadInfo, m_pDevice, &Tex);
    m_TextureSRV = Tex->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE);
    m_SRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_Texture")->Set(m_TextureSRV);

    // Cargar textura para el piso
    RefCntAutoPtr<ITexture> FloorTex;
    CreateTextureFromFile("floor.png", loadInfo, m_pDevice, &FloorTex);
    m_FloorTextureSRV = FloorTex->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE);

    // Crear SRB para el piso
    m_pPSO->CreateShaderResourceBinding(&m_FloorSRB, true);
    m_FloorSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_Texture")->Set(m_FloorTextureSRV);
    m_FloorSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_ShadowMap")->Set(m_ShadowMapSRV);
}

void Tutorial03_Texturing::SetupLights()
{
    // Configurar atributos de la luz
    m_LightAttribs.Direction = float4(m_LightDirection, 0.0f);
    m_LightAttribs.Ambient   = float4(0.1f, 0.1f, 0.1f, 1.0f);
    m_LightAttribs.Diffuse   = float4(0.8f, 0.8f, 0.8f, 1.0f);
    m_LightAttribs.Specular  = float4(0.5f, 0.5f, 0.5f, 1.0f);

    // Crear buffer para las propiedades de la luz
    BufferDesc LightBuffDesc;
    LightBuffDesc.Name           = "Light attributes buffer";
    LightBuffDesc.Usage          = USAGE_DYNAMIC;
    LightBuffDesc.BindFlags      = BIND_UNIFORM_BUFFER;
    LightBuffDesc.CPUAccessFlags = CPU_ACCESS_WRITE;
    LightBuffDesc.Size           = sizeof(LightAttribs);

    m_pDevice->CreateBuffer(LightBuffDesc, nullptr, &m_LightBuffer);

    // Asociar el buffer a los SRBs
    m_SRB->GetVariableByName(SHADER_TYPE_VERTEX, "g_LightAttribs")->Set(m_LightBuffer);
    m_FloorSRB->GetVariableByName(SHADER_TYPE_VERTEX, "g_LightAttribs")->Set(m_LightBuffer);
}



void Tutorial03_Texturing::UpdateCamera(double CurrTime, double ElapsedTime)
{
    // Rotación automática de la cámara si está activada
    if (m_AutoRotateCamera)
    {
        m_CameraYaw += static_cast<float>(ElapsedTime) * m_CameraRotationSpeed;

        // Mantener el ángulo entre 0 y 2π
        if (m_CameraYaw > 2.0f * PI_F)
            m_CameraYaw -= 2.0f * PI_F;
    }

    // Calcular posición de la cámara basada en los ángulos y distancia
    m_CameraPos.x = m_CameraDistance * std::sin(m_CameraYaw) * std::cos(m_CameraPitch);
    m_CameraPos.y = m_CameraDistance * std::sin(m_CameraPitch) + 1.0f; // +1.0f para elevar ligeramente la cámara
    m_CameraPos.z = m_CameraDistance * std::cos(m_CameraYaw) * std::cos(m_CameraPitch);
}

void Tutorial03_Texturing::RenderShadowMap()
{

    double CurrentTime = m_CurrTime;
    // Guardar el estado actual del contexto
    auto* pOrigRTV = m_pSwapChain->GetCurrentBackBufferRTV();
    auto* pOrigDSV = m_pSwapChain->GetDepthBufferDSV();

    // Calcular posición de la luz (desde la dirección contraria)
    float3 LightPos = -m_LightDirection * 10.0f;

    // Construir matriz de vista para la luz manualmente
    float4x4 LightView;
    LightView._11 = 1.0f;
    LightView._12 = 0.0f;
    LightView._13 = 0.0f;
    LightView._14 = 0.0f;
    LightView._21 = 0.0f;
    LightView._22 = 1.0f;
    LightView._23 = 0.0f;
    LightView._24 = 0.0f;
    LightView._31 = 0.0f;
    LightView._32 = 0.0f;
    LightView._33 = 1.0f;
    LightView._34 = 0.0f;
    LightView._41 = -LightPos.x;
    LightView._42 = -LightPos.y;
    LightView._43 = -LightPos.z;
    LightView._44 = 1.0f;

    // O si está disponible, usar:
    // float4x4 LightView = float4x4::LookAtLH(LightPos, float3(0.0f, 0.0f, 0.0f), float3(0.0f, 1.0f, 0.0f));

    // Matriz de proyección ortográfica para las sombras
    

    // Implementación manual de matriz de proyección ortográfica
    float4x4 LightProj;
    float    Width  = 15.0f;
    float    Height = 15.0f;
    float    NearZ  = 0.1f;
    float    FarZ   = 30.0f;

    // Llenar la matriz
    LightProj._11 = 2.0f / Width;
    LightProj._12 = 0.0f;
    LightProj._13 = 0.0f;
    LightProj._14 = 0.0f;

    LightProj._21 = 0.0f;
    LightProj._22 = 2.0f / Height;
    LightProj._23 = 0.0f;
    LightProj._24 = 0.0f;

    LightProj._31 = 0.0f;
    LightProj._32 = 0.0f;
    LightProj._33 = 1.0f / (FarZ - NearZ);
    LightProj._34 = 0.0f;

    LightProj._41 = 0.0f;
    LightProj._42 = 0.0f;
    LightProj._43 = -NearZ / (FarZ - NearZ);
    LightProj._44 = 1.0f;



    m_LightViewProjMatrix = LightView * LightProj;

    // Resto del código para renderizar el shadow map...
}

void Tutorial03_Texturing::Initialize(const SampleInitInfo& InitInfo)
{
    SampleBase::Initialize(InitInfo);

    // Crear el buffer de constantes VS primero
    BufferDesc VSCBDesc;
    VSCBDesc.Name           = "VS constants CB";
    VSCBDesc.Size           = sizeof(float4x4);
    VSCBDesc.Usage          = USAGE_DYNAMIC;
    VSCBDesc.BindFlags      = BIND_UNIFORM_BUFFER;
    VSCBDesc.CPUAccessFlags = CPU_ACCESS_WRITE;
    m_pDevice->CreateBuffer(VSCBDesc, nullptr, &m_VSConstants);

    // Primero crea el pipeline state que creará el m_SRB
    CreatePipelineState();

    // Verifica que el SRB existe después de crear el PSO
    if (!m_SRB)
    {
        LOG_ERROR("SRB was not properly created in CreatePipelineState!");
        return;
    }

    // Luego crea los buffers y geometría
    CreateVertexBuffer();
    CreateIndexBuffer();
    CreateFloorGeometry();
    CreateShadowMap();

    // Finalmente carga las texturas que dependen del SRB
    LoadTextures();
    SetupLights();
}

// Render a frame
void Tutorial03_Texturing::Render()
{
    // Renderizar primero el shadow map
    RenderShadowMap();

    // Continuar con el rendering normal
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

    // Actualizar buffer de luz
    {
        MapHelper<LightAttribs> LightData(m_pImmediateContext, m_LightBuffer, MAP_WRITE, MAP_FLAG_DISCARD);
        *LightData = m_LightAttribs;
    }

    // Renderizar el cubo con iluminación
    {
        MapHelper<float4x4> CBConstants(m_pImmediateContext, m_VSConstants, MAP_WRITE, MAP_FLAG_DISCARD);

        float4x4 Model = float4x4::RotationY(static_cast<float>(m_CurrTime) * 1.0f) * float4x4::RotationX(-PI_F * 0.1f);


        // Implementación manual de matriz de vista
        float4x4 View;
        float3   zaxis = normalize(m_CameraTarget - m_CameraPos);
        float3   xaxis = normalize(cross(m_CameraUp, zaxis));
        float3   yaxis = cross(zaxis, xaxis);

        View._11 = xaxis.x;
        View._12 = yaxis.x;
        View._13 = zaxis.x;
        View._14 = 0.0f;
        View._21 = xaxis.y;
        View._22 = yaxis.y;
        View._23 = zaxis.y;
        View._24 = 0.0f;
        View._31 = xaxis.z;
        View._32 = yaxis.z;
        View._33 = zaxis.z;
        View._34 = 0.0f;
        View._41 = -dot(xaxis, m_CameraPos);
        View._42 = -dot(yaxis, m_CameraPos);
        View._43 = -dot(zaxis, m_CameraPos);
        View._44 = 1.0f;


        // Implementación manual de matriz de proyección perspectiva
        float4x4 Proj;
        float    AspectRatio = float(m_pSwapChain->GetDesc().Width) / float(m_pSwapChain->GetDesc().Height);
        float    FovY        = PI_F / 4.0f;
        float    NearZ       = 0.1f;
        float    FarZ        = 100.0f;
        float    tanHalfFovy = tan(FovY / 2.0f);

        Proj._11 = 1.0f / (AspectRatio * tanHalfFovy);
        Proj._12 = 0.0f;
        Proj._13 = 0.0f;
        Proj._14 = 0.0f;

        Proj._21 = 0.0f;
        Proj._22 = 1.0f / tanHalfFovy;
        Proj._23 = 0.0f;
        Proj._24 = 0.0f;

        Proj._31 = 0.0f;
        Proj._32 = 0.0f;
        Proj._33 = FarZ / (FarZ - NearZ);
        Proj._34 = 1.0f;

        Proj._41 = 0.0f;
        Proj._42 = 0.0f;
        Proj._43 = -NearZ * FarZ / (FarZ - NearZ);
        Proj._44 = 0.0f;
        
        *CBConstants              = Model * View * Proj;
    }

    const Uint64 offset   = 0;
    IBuffer*     pBuffs[] = {m_CubeVertexBuffer};
    m_pImmediateContext->SetVertexBuffers(0, 1, pBuffs, &offset, RESOURCE_STATE_TRANSITION_MODE_TRANSITION, SET_VERTEX_BUFFERS_FLAG_RESET);
    m_pImmediateContext->SetIndexBuffer(m_CubeIndexBuffer, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    m_pImmediateContext->SetPipelineState(m_pPSO);
    m_pImmediateContext->CommitShaderResources(m_SRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    DrawIndexedAttribs DrawAttrs;
    DrawAttrs.IndexType  = VT_UINT32;
    DrawAttrs.NumIndices = 36;
    DrawAttrs.Flags      = DRAW_FLAG_VERIFY_ALL;
    m_pImmediateContext->DrawIndexed(DrawAttrs);

    // Renderizar el piso con iluminación y sombras
    {
        MapHelper<float4x4> CBConstants(m_pImmediateContext, m_VSConstants, MAP_WRITE, MAP_FLAG_DISCARD);
        // Implementación manual de matriz de vista
        float4x4 View;
        float3   zaxis = normalize(m_CameraTarget - m_CameraPos);
        float3   xaxis = normalize(cross(m_CameraUp, zaxis));
        float3   yaxis = cross(zaxis, xaxis);

        View._11                 = xaxis.x;
        View._12                 = yaxis.x;
        View._13                 = zaxis.x;
        View._14                 = 0.0f;
        View._21                 = xaxis.y;
        View._22                 = yaxis.y;
        View._23                 = zaxis.y;
        View._24                 = 0.0f;
        View._31                 = xaxis.z;
        View._32                 = yaxis.z;
        View._33                 = zaxis.z;
        View._34                 = 0.0f;
        View._41                 = -dot(xaxis, m_CameraPos);
        View._42                 = -dot(yaxis, m_CameraPos);
        View._43                 = -dot(zaxis, m_CameraPos);
        View._44                 = 1.0f;


        // Implementación manual de matriz de proyección perspectiva
        float4x4 Proj;
        float    AspectRatio = float(m_pSwapChain->GetDesc().Width) / float(m_pSwapChain->GetDesc().Height);
        float    FovY        = PI_F / 4.0f;
        float    NearZ       = 0.1f;
        float    FarZ        = 100.0f;
        float    tanHalfFovy = tan(FovY / 2.0f);

        Proj._11 = 1.0f / (AspectRatio * tanHalfFovy);
        Proj._12 = 0.0f;
        Proj._13 = 0.0f;
        Proj._14 = 0.0f;

        Proj._21 = 0.0f;
        Proj._22 = 1.0f / tanHalfFovy;
        Proj._23 = 0.0f;
        Proj._24 = 0.0f;

        Proj._31 = 0.0f;
        Proj._32 = 0.0f;
        Proj._33 = FarZ / (FarZ - NearZ);
        Proj._34 = 1.0f;

        Proj._41 = 0.0f;
        Proj._42 = 0.0f;
        Proj._43 = -NearZ * FarZ / (FarZ - NearZ);
        Proj._44 = 0.0f;
        
        *CBConstants             = View * Proj;
    }

    pBuffs[0] = m_FloorVertexBuffer;
    m_pImmediateContext->SetVertexBuffers(0, 1, pBuffs, &offset, RESOURCE_STATE_TRANSITION_MODE_TRANSITION, SET_VERTEX_BUFFERS_FLAG_RESET);
    m_pImmediateContext->SetIndexBuffer(m_FloorIndexBuffer, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    m_pImmediateContext->CommitShaderResources(m_FloorSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    DrawAttrs.NumIndices = 6;
    m_pImmediateContext->DrawIndexed(DrawAttrs);

    // Renderizar UI para control de luz
    ImGui::Begin("Light Control");
    ImGui::SliderFloat3("Light Direction", &m_LightDirection.x, -1.0f, 1.0f);
    ImGui::SliderFloat4("Ambient Color", &m_LightAttribs.Ambient.x, 0.0f, 1.0f);
    ImGui::SliderFloat4("Diffuse Color", &m_LightAttribs.Diffuse.x, 0.0f, 1.0f);
    ImGui::SliderFloat4("Specular Color", &m_LightAttribs.Specular.x, 0.0f, 1.0f);
    ImGui::End();
}

void Tutorial03_Texturing::Update(double CurrTime, double ElapsedTime)
{
    SampleBase::Update(CurrTime, ElapsedTime);

    // Guardar el tiempo actual
    m_CurrTime = CurrTime;

    // Actualizar la cámara con el tiempo transcurrido
    UpdateCamera(CurrTime, ElapsedTime);

    // Normalizar dirección de luz
    m_LightDirection         = normalize(m_LightDirection);
    m_LightAttribs.Direction = float4(m_LightDirection, 0.0f);

    // Aplicar rotación al cubo
    float4x4 CubeModelTransform = float4x4::RotationY(static_cast<float>(CurrTime) * 1.0f) * float4x4::RotationX(-PI_F * 0.1f);

    // Calcular vista desde la posición de la cámara
    float4x4 View = float4x4::Translation(0.0f, 0.0f, 0.0f);
    View._41      = -m_CameraPos.x;
    View._42      = -m_CameraPos.y;
    View._43      = -m_CameraPos.z;

    // En lugar de ViewFromPosDir, construimos la matriz de vista manualmente o usamos otra función
    // que esté disponible en tu versión de Diligent Engine

    // Obtener matriz de proyección
    // Implementación manual de matriz de proyección perspectiva
    float4x4 Proj;
    float    AspectRatio = float(m_pSwapChain->GetDesc().Width) / float(m_pSwapChain->GetDesc().Height);
    float    FovY        = PI_F / 4.0f;
    float    NearZ       = 0.1f;
    float    FarZ        = 100.0f;
    float    tanHalfFovy = tan(FovY / 2.0f);

    Proj._11 = 1.0f / (AspectRatio * tanHalfFovy);
    Proj._12 = 0.0f;
    Proj._13 = 0.0f;
    Proj._14 = 0.0f;

    Proj._21 = 0.0f;
    Proj._22 = 1.0f / tanHalfFovy;
    Proj._23 = 0.0f;
    Proj._24 = 0.0f;

    Proj._31 = 0.0f;
    Proj._32 = 0.0f;
    Proj._33 = FarZ / (FarZ - NearZ);
    Proj._34 = 1.0f;

    Proj._41 = 0.0f;
    Proj._42 = 0.0f;
    Proj._43 = -NearZ * FarZ / (FarZ - NearZ);
    Proj._44 = 0.0f;

    // Compute world-view-projection matrix
    m_WorldViewProjMatrix = CubeModelTransform * View * Proj;
}

} // namespace Diligent