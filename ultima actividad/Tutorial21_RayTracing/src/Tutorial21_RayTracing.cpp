#include "Tutorial21_RayTracing.hpp"
#include "MapHelper.hpp"
#include "GraphicsTypesX.hpp"
#include "GraphicsUtilities.h"
#include "TextureUtilities.h"
#include "ShaderMacroHelper.hpp"
#include "imgui.h"
#include "ImGuiUtils.hpp"
#include "AdvancedMath.hpp"
#include "PlatformMisc.hpp"
#include <random>

namespace Diligent
{

SampleBase* CreateSample()
{
    return new Tutorial21_RayTracing();
}

void Tutorial21_RayTracing::CreateGraphicsPSO()
{

    GraphicsPipelineStateCreateInfo PSOCreateInfo;

    PSOCreateInfo.PSODesc.Name         = "Image blit PSO";
    PSOCreateInfo.PSODesc.PipelineType = PIPELINE_TYPE_GRAPHICS;

    PSOCreateInfo.GraphicsPipeline.NumRenderTargets             = 1;
    PSOCreateInfo.GraphicsPipeline.RTVFormats[0]                = m_pSwapChain->GetDesc().ColorBufferFormat;
    PSOCreateInfo.GraphicsPipeline.PrimitiveTopology            = PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
    PSOCreateInfo.GraphicsPipeline.RasterizerDesc.CullMode      = CULL_MODE_NONE;
    PSOCreateInfo.GraphicsPipeline.DepthStencilDesc.DepthEnable = False;

    ShaderCreateInfo ShaderCI;
    ShaderCI.SourceLanguage = SHADER_SOURCE_LANGUAGE_HLSL;
    ShaderCI.ShaderCompiler = SHADER_COMPILER_DXC;
    ShaderCI.CompileFlags   = SHADER_COMPILE_FLAG_PACK_MATRIX_ROW_MAJOR;
    RefCntAutoPtr<IShaderSourceInputStreamFactory> pShaderSourceFactory;
    m_pEngineFactory->CreateDefaultShaderSourceStreamFactory(nullptr, &pShaderSourceFactory);
    ShaderCI.pShaderSourceStreamFactory = pShaderSourceFactory;

    RefCntAutoPtr<IShader> pVS;
    {
        ShaderCI.Desc.ShaderType = SHADER_TYPE_VERTEX;
        ShaderCI.EntryPoint      = "main";
        ShaderCI.Desc.Name       = "Image blit VS";
        ShaderCI.FilePath        = "ImageBlit.vsh";
        m_pDevice->CreateShader(ShaderCI, &pVS);
        VERIFY_EXPR(pVS != nullptr);
    }

    RefCntAutoPtr<IShader> pPS;
    {
        ShaderCI.Desc.ShaderType = SHADER_TYPE_PIXEL;
        ShaderCI.EntryPoint      = "main";
        ShaderCI.Desc.Name       = "Image blit PS";
        ShaderCI.FilePath        = "ImageBlit.psh";
        m_pDevice->CreateShader(ShaderCI, &pPS);
        VERIFY_EXPR(pPS != nullptr);
    }

    PSOCreateInfo.pVS = pVS;
    PSOCreateInfo.pPS = pPS;

    PSOCreateInfo.PSODesc.ResourceLayout.DefaultVariableType = SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC;

    m_pDevice->CreateGraphicsPipelineState(PSOCreateInfo, &m_pImageBlitPSO);
    VERIFY_EXPR(m_pImageBlitPSO != nullptr);

    m_pImageBlitPSO->CreateShaderResourceBinding(&m_pImageBlitSRB, true);
    VERIFY_EXPR(m_pImageBlitSRB != nullptr);
}

void Tutorial21_RayTracing::CreateRayTracingPSO()
{
    m_MaxRecursionDepth = std::min(m_MaxRecursionDepth, m_pDevice->GetAdapterInfo().RayTracing.MaxRecursionDepth);
    RayTracingPipelineStateCreateInfoX PSOCreateInfo;

    PSOCreateInfo.PSODesc.Name         = "Ray tracing PSO";
    PSOCreateInfo.PSODesc.PipelineType = PIPELINE_TYPE_RAY_TRACING;

    ShaderMacroHelper Macros;
    Macros.AddShaderMacro("NUM_TEXTURES", NumTextures);

    ShaderCreateInfo ShaderCI;
    ShaderCI.Desc.UseCombinedTextureSamplers = false;
    ShaderCI.Macros                          = Macros;
    ShaderCI.ShaderCompiler                  = SHADER_COMPILER_DXC;
    ShaderCI.CompileFlags                    = SHADER_COMPILE_FLAG_PACK_MATRIX_ROW_MAJOR;
    ShaderCI.HLSLVersion                     = {6, 3};
    ShaderCI.SourceLanguage                  = SHADER_SOURCE_LANGUAGE_HLSL;

    RefCntAutoPtr<IShaderSourceInputStreamFactory> pShaderSourceFactory;
    m_pEngineFactory->CreateDefaultShaderSourceStreamFactory(nullptr, &pShaderSourceFactory);
    ShaderCI.pShaderSourceStreamFactory = pShaderSourceFactory;

    RefCntAutoPtr<IShader> pRayGen;
    {
        ShaderCI.Desc.ShaderType = SHADER_TYPE_RAY_GEN;
        ShaderCI.Desc.Name       = "Ray tracing RG";
        ShaderCI.FilePath        = "RayTrace.rgen";
        ShaderCI.EntryPoint      = "main";
        m_pDevice->CreateShader(ShaderCI, &pRayGen);
        VERIFY_EXPR(pRayGen != nullptr);
    }

    RefCntAutoPtr<IShader> pPrimaryMiss, pShadowMiss;
    {
        ShaderCI.Desc.ShaderType = SHADER_TYPE_RAY_MISS;
        ShaderCI.Desc.Name       = "Primary ray miss shader";
        ShaderCI.FilePath        = "PrimaryMiss.rmiss";
        ShaderCI.EntryPoint      = "main";
        m_pDevice->CreateShader(ShaderCI, &pPrimaryMiss);
        VERIFY_EXPR(pPrimaryMiss != nullptr);

        ShaderCI.Desc.Name  = "Shadow ray miss shader";
        ShaderCI.FilePath   = "ShadowMiss.rmiss";
        ShaderCI.EntryPoint = "main";
        m_pDevice->CreateShader(ShaderCI, &pShadowMiss);
        VERIFY_EXPR(pShadowMiss != nullptr);
    }

    RefCntAutoPtr<IShader> pCubePrimaryHit, pGroundHit, pGlassPrimaryHit, pSpherePrimaryHit, pSphereSolida, pSphereGlass;
    {
        ShaderCI.Desc.ShaderType = SHADER_TYPE_RAY_CLOSEST_HIT;
        ShaderCI.Desc.Name       = "Cube primary ray closest hit shader";
        ShaderCI.FilePath        = "CubePrimaryHit.rchit";
        ShaderCI.EntryPoint      = "main";
        m_pDevice->CreateShader(ShaderCI, &pCubePrimaryHit);
        VERIFY_EXPR(pCubePrimaryHit != nullptr);

        ShaderCI.Desc.Name  = "Ground primary ray closest hit shader";
        ShaderCI.FilePath   = "Ground.rchit";
        ShaderCI.EntryPoint = "main";
        m_pDevice->CreateShader(ShaderCI, &pGroundHit);
        VERIFY_EXPR(pGroundHit != nullptr);

        ShaderCI.Desc.Name  = "Glass primary ray closest hit shader";
        ShaderCI.FilePath   = "GlassPrimaryHit.rchit";
        ShaderCI.EntryPoint = "main";
        m_pDevice->CreateShader(ShaderCI, &pGlassPrimaryHit);
        VERIFY_EXPR(pGlassPrimaryHit != nullptr);

        ShaderCI.Desc.Name  = "Sphere primary ray closest hit shader";
        ShaderCI.FilePath   = "SpherePrimaryHit.rchit";
        ShaderCI.EntryPoint = "main";
        m_pDevice->CreateShader(ShaderCI, &pSpherePrimaryHit);
        VERIFY_EXPR(pSpherePrimaryHit != nullptr);

        ShaderCI.Desc.Name  = "Sphere primary closest hit shader";
        ShaderCI.FilePath   = "SphereSolida.rchit";
        ShaderCI.EntryPoint = "main";
        m_pDevice->CreateShader(ShaderCI, &pSphereSolida);
        VERIFY_EXPR(pSphereSolida != nullptr);

        ShaderCI.Desc.Name  = "Sphere primary hit shader";
        ShaderCI.FilePath   = "SphereGlass.rchit";
        ShaderCI.EntryPoint = "main";
        m_pDevice->CreateShader(ShaderCI, &pSphereGlass);
        VERIFY_EXPR(pSphereGlass != nullptr);
    }

    RefCntAutoPtr<IShader> pSphereIntersection;
    {
        ShaderCI.Desc.ShaderType = SHADER_TYPE_RAY_INTERSECTION;
        ShaderCI.Desc.Name       = "Sphere intersection shader";
        ShaderCI.FilePath        = "SphereIntersection.rint";
        ShaderCI.EntryPoint      = "main";
        m_pDevice->CreateShader(ShaderCI, &pSphereIntersection);
        VERIFY_EXPR(pSphereIntersection != nullptr);
    }

    PSOCreateInfo.AddGeneralShader("Main", pRayGen);
    PSOCreateInfo.AddGeneralShader("PrimaryMiss", pPrimaryMiss);
    PSOCreateInfo.AddGeneralShader("ShadowMiss", pShadowMiss);
    PSOCreateInfo.AddTriangleHitShader("CubePrimaryHit", pCubePrimaryHit);
    PSOCreateInfo.AddTriangleHitShader("GroundHit", pGroundHit);
    PSOCreateInfo.AddTriangleHitShader("GlassPrimaryHit", pGlassPrimaryHit);
    PSOCreateInfo.AddProceduralHitShader("SpherePrimaryHit", pSphereIntersection, pSpherePrimaryHit);
    PSOCreateInfo.AddProceduralHitShader("SphereShadowHit", pSphereIntersection);
    PSOCreateInfo.AddProceduralHitShader("SphereSolida", pSphereIntersection, pSphereSolida);
    PSOCreateInfo.AddProceduralHitShader("SphereGlass", pSphereIntersection, pSphereGlass);
    PSOCreateInfo.RayTracingPipeline.MaxRecursionDepth = static_cast<Uint8>(m_MaxRecursionDepth);
    PSOCreateInfo.RayTracingPipeline.ShaderRecordSize  = 0;

    PSOCreateInfo.MaxAttributeSize = std::max<Uint32>(sizeof(/* BuiltInTriangleIntersectionAttributes */ float2), sizeof(HLSL::ProceduralGeomIntersectionAttribs));
    PSOCreateInfo.MaxPayloadSize   = std::max<Uint32>(sizeof(HLSL::PrimaryRayPayload), sizeof(HLSL::ShadowRayPayload));
    SamplerDesc SamLinearWrapDesc{
        FILTER_TYPE_LINEAR, FILTER_TYPE_LINEAR, FILTER_TYPE_LINEAR,
        TEXTURE_ADDRESS_WRAP, TEXTURE_ADDRESS_WRAP, TEXTURE_ADDRESS_WRAP //
    };

    PipelineResourceLayoutDescX ResourceLayout;
    ResourceLayout.DefaultVariableType = SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE;
    ResourceLayout.AddImmutableSampler(SHADER_TYPE_RAY_CLOSEST_HIT, "g_SamLinearWrap", SamLinearWrapDesc);
    ResourceLayout
        .AddVariable(SHADER_TYPE_RAY_GEN | SHADER_TYPE_RAY_MISS | SHADER_TYPE_RAY_CLOSEST_HIT, "g_ConstantsCB", SHADER_RESOURCE_VARIABLE_TYPE_STATIC)
        .AddVariable(SHADER_TYPE_RAY_GEN, "g_ColorBuffer", SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC);

    PSOCreateInfo.PSODesc.ResourceLayout = ResourceLayout;

    m_pDevice->CreateRayTracingPipelineState(PSOCreateInfo, &m_pRayTracingPSO);
    VERIFY_EXPR(m_pRayTracingPSO != nullptr);
    m_pRayTracingPSO->GetStaticVariableByName(SHADER_TYPE_RAY_GEN, "g_ConstantsCB")->Set(m_ConstantsCB);
    m_pRayTracingPSO->GetStaticVariableByName(SHADER_TYPE_RAY_MISS, "g_ConstantsCB")->Set(m_ConstantsCB);
    m_pRayTracingPSO->GetStaticVariableByName(SHADER_TYPE_RAY_CLOSEST_HIT, "g_ConstantsCB")->Set(m_ConstantsCB);
    m_pRayTracingPSO->CreateShaderResourceBinding(&m_pRayTracingSRB, true);
    VERIFY_EXPR(m_pRayTracingSRB != nullptr);
}

void Tutorial21_RayTracing::LoadTextures()
{
    IDeviceObject*          pTexSRVs[NumTextures] = {};
    RefCntAutoPtr<ITexture> pTex[NumTextures];
    StateTransitionDesc     Barriers[NumTextures];
    for (int tex = 0; tex < NumTextures; ++tex)
    {
        TextureLoadInfo loadInfo;
        loadInfo.IsSRGB = true;
        std::stringstream FileNameSS;
        FileNameSS << "DGLogo" << tex << ".png";
        auto FileName = FileNameSS.str();
        CreateTextureFromFile(FileName.c_str(), loadInfo, m_pDevice, &pTex[tex]);

        auto* pTextureSRV = pTex[tex]->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE);
        pTexSRVs[tex]     = pTextureSRV;
        Barriers[tex]     = StateTransitionDesc{pTex[tex], RESOURCE_STATE_UNKNOWN, RESOURCE_STATE_SHADER_RESOURCE, STATE_TRANSITION_FLAG_UPDATE_STATE};
    }
    m_pImmediateContext->TransitionResourceStates(_countof(Barriers), Barriers);

    m_pRayTracingSRB->GetVariableByName(SHADER_TYPE_RAY_CLOSEST_HIT, "g_CubeTextures")->SetArray(pTexSRVs, 0, NumTextures);
    RefCntAutoPtr<ITexture> pGroundTex;
    CreateTextureFromFile("Ground.jpg", TextureLoadInfo{}, m_pDevice, &pGroundTex);

    m_pRayTracingSRB->GetVariableByName(SHADER_TYPE_RAY_CLOSEST_HIT, "g_GroundTexture")->Set(pGroundTex->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE));
}

void Tutorial21_RayTracing::CreateCubeBLAS(float cubeSize, RefCntAutoPtr<IBottomLevelAS>& OutBLAS)
{
    RefCntAutoPtr<IDataBlob> pCubeVerts;
    RefCntAutoPtr<IDataBlob> pCubeIndices;
    GeometryPrimitiveInfo    CubeGeoInfo;
    CreateGeometryPrimitive(CubeGeometryPrimitiveAttributes{cubeSize, GEOMETRY_PRIMITIVE_VERTEX_FLAG_ALL}, &pCubeVerts, &pCubeIndices, &CubeGeoInfo);

    struct CubeVertex
    {
        float3 Pos;
        float3 Normal;
        float2 UV;
    };
    VERIFY_EXPR(CubeGeoInfo.VertexSize == sizeof(CubeVertex));
    const CubeVertex* pVerts   = pCubeVerts->GetConstDataPtr<CubeVertex>();
    const Uint32*     pIndices = pCubeIndices->GetConstDataPtr<Uint32>();

    if (!m_CubeAttribsCB)
    {
        HLSL::CubeAttribs Attribs;
        for (Uint32 v = 0; v < CubeGeoInfo.NumVertices; ++v)
        {
            Attribs.UVs[v]     = {pVerts[v].UV, 0, 0};
            Attribs.Normals[v] = pVerts[v].Normal;
        }

        for (Uint32 i = 0; i < CubeGeoInfo.NumIndices; i += 3)
        {
            const Uint32* TriIdx{&pIndices[i]};
            Attribs.Primitives[i / 3] = uint4{TriIdx[0], TriIdx[1], TriIdx[2], 0};
        }

        BufferDesc BuffDesc;
        BuffDesc.Name      = "Cube Attribs";
        BuffDesc.Usage     = USAGE_IMMUTABLE;
        BuffDesc.BindFlags = BIND_UNIFORM_BUFFER;
        BuffDesc.Size      = sizeof(Attribs);

        BufferData BufData = {&Attribs, BuffDesc.Size};

        m_pDevice->CreateBuffer(BuffDesc, &BufData, &m_CubeAttribsCB);
        VERIFY_EXPR(m_CubeAttribsCB != nullptr);

        m_pRayTracingSRB->GetVariableByName(SHADER_TYPE_RAY_CLOSEST_HIT, "g_CubeAttribsCB")->Set(m_CubeAttribsCB);
    }

    RefCntAutoPtr<IBuffer>             pCubeVertexBuffer;
    RefCntAutoPtr<IBuffer>             pCubeIndexBuffer;
    GeometryPrimitiveBuffersCreateInfo CubeBuffersCI;
    CubeBuffersCI.VertexBufferBindFlags = BIND_RAY_TRACING;
    CubeBuffersCI.IndexBufferBindFlags  = BIND_RAY_TRACING;
    CreateGeometryPrimitiveBuffers(m_pDevice, CubeGeometryPrimitiveAttributes{cubeSize, GEOMETRY_PRIMITIVE_VERTEX_FLAG_POSITION},
                                   &CubeBuffersCI, &pCubeVertexBuffer, &pCubeIndexBuffer);

    BLASTriangleDesc Triangles;
    Triangles.GeometryName         = "Cube";
    Triangles.MaxVertexCount       = CubeGeoInfo.NumVertices;
    Triangles.VertexValueType      = VT_FLOAT32;
    Triangles.VertexComponentCount = 3;
    Triangles.MaxPrimitiveCount    = CubeGeoInfo.NumIndices / 3;
    Triangles.IndexType            = VT_UINT32;

    BottomLevelASDesc ASDesc;
    ASDesc.Name          = (cubeSize == 2.f ? "Cube BLAS" : "SmallCube BLAS");
    ASDesc.Flags         = RAYTRACING_BUILD_AS_PREFER_FAST_TRACE;
    ASDesc.pTriangles    = &Triangles;
    ASDesc.TriangleCount = 1;

    m_pDevice->CreateBLAS(ASDesc, &OutBLAS);
    VERIFY_EXPR(OutBLAS != nullptr);

    RefCntAutoPtr<IBuffer> pScratchBuffer;
    {
        BufferDesc BuffDesc;
        BuffDesc.Name      = "BLAS Scratch Buffer";
        BuffDesc.Usage     = USAGE_DEFAULT;
        BuffDesc.BindFlags = BIND_RAY_TRACING;
        BuffDesc.Size      = OutBLAS->GetScratchBufferSizes().Build;

        m_pDevice->CreateBuffer(BuffDesc, nullptr, &pScratchBuffer);
        VERIFY_EXPR(pScratchBuffer != nullptr);
    }


    BLASBuildTriangleData TriangleData;
    TriangleData.GeometryName         = Triangles.GeometryName;
    TriangleData.pVertexBuffer        = pCubeVertexBuffer;
    TriangleData.VertexStride         = sizeof(float3);
    TriangleData.VertexCount          = Triangles.MaxVertexCount;
    TriangleData.VertexValueType      = Triangles.VertexValueType;
    TriangleData.VertexComponentCount = Triangles.VertexComponentCount;
    TriangleData.pIndexBuffer         = pCubeIndexBuffer;
    TriangleData.PrimitiveCount       = Triangles.MaxPrimitiveCount;
    TriangleData.IndexType            = Triangles.IndexType;
    TriangleData.Flags                = RAYTRACING_GEOMETRY_FLAG_OPAQUE;

    BuildBLASAttribs Attribs;
    Attribs.pBLAS                       = OutBLAS;
    Attribs.pTriangleData               = &TriangleData;
    Attribs.TriangleDataCount           = 1;
    Attribs.pScratchBuffer              = pScratchBuffer;
    Attribs.BLASTransitionMode          = RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
    Attribs.GeometryTransitionMode      = RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
    Attribs.ScratchBufferTransitionMode = RESOURCE_STATE_TRANSITION_MODE_TRANSITION;

    m_pImmediateContext->BuildBLAS(Attribs);
}

void Tutorial21_RayTracing::CreateProceduralBLAS()
{
    static_assert(sizeof(HLSL::BoxAttribs) % 16 == 0, "BoxAttribs must be aligned by 16 bytes");

    const HLSL::BoxAttribs Boxes[] = {
        HLSL::BoxAttribs{-3.0f, -3.0f, -3.0f, 3.0f, 3.0f, 3.0f},
        HLSL::BoxAttribs{-0.6f, -0.6f, -0.6f, 0.6f, 0.6f, 0.6f}};

    {
        BufferData BufData = {Boxes, sizeof(Boxes)};
        BufferDesc BuffDesc;
        BuffDesc.Name              = "AABB Buffer";
        BuffDesc.Usage             = USAGE_IMMUTABLE;
        BuffDesc.BindFlags         = BIND_RAY_TRACING | BIND_SHADER_RESOURCE;
        BuffDesc.Size              = sizeof(Boxes);
        BuffDesc.ElementByteStride = sizeof(Boxes[0]);
        BuffDesc.Mode              = BUFFER_MODE_STRUCTURED;

        m_pDevice->CreateBuffer(BuffDesc, &BufData, &m_BoxAttribsCB);
        VERIFY_EXPR(m_BoxAttribsCB != nullptr);

        m_pRayTracingSRB->GetVariableByName(SHADER_TYPE_RAY_INTERSECTION, "g_BoxAttribs")->Set(m_BoxAttribsCB->GetDefaultView(BUFFER_VIEW_SHADER_RESOURCE));
    }

    {

        BLASBoundingBoxDesc BoxInfo;
        {
            BoxInfo.GeometryName = "Box";
            BoxInfo.MaxBoxCount  = 1;

            BottomLevelASDesc ASDesc;
            ASDesc.Name     = "Procedural BLAS";
            ASDesc.Flags    = RAYTRACING_BUILD_AS_PREFER_FAST_TRACE;
            ASDesc.pBoxes   = &BoxInfo;
            ASDesc.BoxCount = 1;

            m_pDevice->CreateBLAS(ASDesc, &m_pProceduralBLAS);
            VERIFY_EXPR(m_pProceduralBLAS != nullptr);
        }

        RefCntAutoPtr<IBuffer> pScratchBuffer;
        {
            BufferDesc BuffDesc;
            BuffDesc.Name      = "BLAS Scratch Buffer";
            BuffDesc.Usage     = USAGE_DEFAULT;
            BuffDesc.BindFlags = BIND_RAY_TRACING;
            BuffDesc.Size      = m_pProceduralBLAS->GetScratchBufferSizes().Build;

            m_pDevice->CreateBuffer(BuffDesc, nullptr, &pScratchBuffer);
            VERIFY_EXPR(pScratchBuffer != nullptr);
        }

        BLASBuildBoundingBoxData BoxData;
        BoxData.GeometryName = BoxInfo.GeometryName;
        BoxData.BoxCount     = 1;
        BoxData.BoxStride    = sizeof(Boxes[0]);
        BoxData.pBoxBuffer   = m_BoxAttribsCB;

        BuildBLASAttribs Attribs;
        Attribs.pBLAS                       = m_pProceduralBLAS;
        Attribs.pBoxData                    = &BoxData;
        Attribs.BoxDataCount                = 1;
        Attribs.pScratchBuffer              = pScratchBuffer;
        Attribs.BLASTransitionMode          = RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
        Attribs.GeometryTransitionMode      = RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
        Attribs.ScratchBufferTransitionMode = RESOURCE_STATE_TRANSITION_MODE_TRANSITION;

        m_pImmediateContext->BuildBLAS(Attribs);
    }
}

void Tutorial21_RayTracing::UpdateTLAS()
{
    static constexpr int BaseCount    = 4;
    static constexpr int TotalObjects = BaseCount + MaxSmallSpheres + 3 + MaxSmallCubes; 
    bool                 NeedUpdate   = true;

    static std::vector<float3> smallSpherePositions;
    if (smallSpherePositions.empty())
    {
        smallSpherePositions.reserve(MaxSmallSpheres);
        std::mt19937                          rng(42);
        std::uniform_real_distribution<float> distRadial(8.0f, 20.0f);
        std::uniform_real_distribution<float> distAngle(0.0f, 2.0f * PI_F);
        std::uniform_real_distribution<float> distY(-0.5f, 0.5f);

        for (int i = 0; i < MaxSmallSpheres; ++i)
        {
            int   sector          = i % 5;
            float sectorAngleBase = sector * (2.0f * PI_F / 5.0f);
            float sectorWidth     = 0.8f * (2.0f * PI_F / 5.0f);

            float angle  = sectorAngleBase + distAngle(rng) * sectorWidth;
            float radius = distRadial(rng);
            radius += (i % 3) * 1.5f;

            float x = radius * cosf(angle);
            float z = radius * sinf(angle);
            float y = -5.0f + distY(rng);

            bool tooClose = false;
            for (size_t j = 0; j < smallSpherePositions.size() && j < 10; ++j)
            {
                float3 existing = smallSpherePositions[smallSpherePositions.size() - 1 - j];
                float  distSq   = (x - existing.x) * (x - existing.x) +
                    (z - existing.z) * (z - existing.z);
                if (distSq < 4.0f)
                {
                    tooClose = true;
                    break;
                }
            }

            if (tooClose && smallSpherePositions.size() > 0)
            {
                radius += 3.0f;
                x = radius * cosf(angle + 0.2f);
                z = radius * sinf(angle + 0.2f);
            }

            smallSpherePositions.push_back(float3{x, y, z});
        }
    }

    static std::vector<float3> smallCubePositions;
    if (smallCubePositions.empty())
    {
        smallCubePositions.reserve(MaxSmallCubes);
        std::mt19937                          rng(24); 
        std::uniform_real_distribution<float> distRadial(8.0f, 22.0f);
        std::uniform_real_distribution<float> distAngle(0.0f, 2.0f * PI_F);
        std::uniform_real_distribution<float> distY(-0.7f, 0.7f);

        for (int i = 0; i < MaxSmallCubes; ++i)
        {
            int   sector          = i % 5;
            float sectorAngleBase = sector * (2.0f * PI_F / 5.0f) + PI_F / 5.0f; 
            float sectorWidth     = 0.8f * (2.0f * PI_F / 5.0f);

            float angle  = sectorAngleBase + distAngle(rng) * sectorWidth;
            float radius = distRadial(rng);
            radius += (i % 4) * 1.2f;

            float x = radius * cosf(angle);
            float z = radius * sinf(angle);
            float y = -5.0f + distY(rng);

            bool tooClose = false;
            for (size_t j = 0; j < smallCubePositions.size() && j < 10; ++j)
            {
                float3 existing = smallCubePositions[smallCubePositions.size() - 1 - j];
                float  distSq   = (x - existing.x) * (x - existing.x) +
                    (z - existing.z) * (z - existing.z);
                if (distSq < 5.0f)
                {
                    tooClose = true;
                    break;
                }
            }

            if (tooClose && smallCubePositions.size() > 0)
            {
                radius += 3.5f;
                x = radius * cosf(angle + 0.3f);
                z = radius * sinf(angle + 0.3f);
            }

            smallCubePositions.push_back(float3{x, y, z});
        }
    }


    if (!m_pTLAS)
    {
        TopLevelASDesc TLASDesc;
        TLASDesc.Name             = "TLAS";
        TLASDesc.MaxInstanceCount = TotalObjects;
        TLASDesc.Flags            = RAYTRACING_BUILD_AS_ALLOW_UPDATE | RAYTRACING_BUILD_AS_PREFER_FAST_TRACE;

        m_pDevice->CreateTLAS(TLASDesc, &m_pTLAS);
        VERIFY_EXPR(m_pTLAS != nullptr);

        NeedUpdate = false;

        m_pRayTracingSRB->GetVariableByName(SHADER_TYPE_RAY_GEN, "g_TLAS")->Set(m_pTLAS);
        m_pRayTracingSRB->GetVariableByName(SHADER_TYPE_RAY_CLOSEST_HIT, "g_TLAS")->Set(m_pTLAS);
    }

    if (!m_ScratchBuffer)
    {
        BufferDesc BuffDesc;
        BuffDesc.Name      = "TLAS Scratch Buffer";
        BuffDesc.Usage     = USAGE_DEFAULT;
        BuffDesc.BindFlags = BIND_RAY_TRACING;
        BuffDesc.Size      = std::max(m_pTLAS->GetScratchBufferSizes().Build, m_pTLAS->GetScratchBufferSizes().Update);

        m_pDevice->CreateBuffer(BuffDesc, nullptr, &m_ScratchBuffer);
        VERIFY_EXPR(m_ScratchBuffer != nullptr);
    }

    if (!m_InstanceBuffer)
    {
        BufferDesc BuffDesc;
        BuffDesc.Name      = "TLAS Instance Buffer";
        BuffDesc.Usage     = USAGE_DEFAULT;
        BuffDesc.BindFlags = BIND_RAY_TRACING;
        BuffDesc.Size      = TLAS_INSTANCE_DATA_SIZE * TotalObjects;

        m_pDevice->CreateBuffer(BuffDesc, nullptr, &m_InstanceBuffer);
        VERIFY_EXPR(m_InstanceBuffer != nullptr);
    }

    TLASBuildInstanceData* Instances = new TLASBuildInstanceData[TotalObjects]();


    struct CubeInstanceData
    {
        float3 BasePos;
        float  TimeOffset;
    } CubeInstData[] =
        {
            {float3{1, 1, 1}, 0.00f},
            {float3{2, 0, -1}, 0.53f},
            {float3{-1, 1, 2}, 1.27f},
            {float3{-2, 0, -1}, 4.16f}};

    static_assert(_countof(CubeInstData) == NumCubes, "Cube instance data array size mismatch");

    const auto AnimateOpaqueCube = [&](TLASBuildInstanceData& Dst) //
    {
        float  t     = sin(m_AnimationTime * PI_F * 0.5f) + CubeInstData[Dst.CustomId].TimeOffset;
        float3 Pos   = CubeInstData[Dst.CustomId].BasePos * 2.0f + float3(sin(t * 1.13f), sin(t * 0.77f), sin(t * 2.15f)) * 0.5f;
        float  angle = 0.1f * PI_F * (m_AnimationTime + CubeInstData[Dst.CustomId].TimeOffset * 2.0f);

        if (!m_EnableCubes[Dst.CustomId])
            Dst.Mask = 0;

        Dst.Transform.SetTranslation(Pos.x, -Pos.y, Pos.z);
        Dst.Transform.SetRotation(float3x3::RotationY(angle).Data());
    };


    Instances[0].InstanceName = "Ground Instance";
    Instances[0].pBLAS        = m_pCubeBLAS;
    Instances[0].Mask         = OPAQUE_GEOM_MASK;
    Instances[0].Transform.SetRotation(float3x3::Scale(100.0f, 0.1f, 100.0f).Data());
    Instances[0].Transform.SetTranslation(0.0f, -6.0f, 0.0f);


    const char* largeSphereNames[] = {
        "Sphere Instance",
        "Sphere Instance 2",
        "Sphere Instance 3"};


    float3 largeSpherePositions[] = {
        {0.0f, -3.0f, 0.0f},  
        {-7.5f, -3.0f, -2.5f}, 
        {7.5f, -3.0f, -2.5f}   
    };

    const char* largeCubeNames[] = {
        "Cube Instance Large 1",
        "Cube Instance Large 2",
        "Cube Instance Large 3"};


    float3 largeCubePositions[] = {
        {-6.0f, -3.0f, 8.0f}, 
        {0.0f, -3.0f, 10.0f}, 
        {6.0f, -3.0f, 8.0f} 
    };


    for (int k = 0; k < 3; ++k)
    {
        int   idx  = 1 + k;
        auto& inst = Instances[idx];

        inst.InstanceName = largeSphereNames[k];
        inst.CustomId     = 0;
        inst.pBLAS        = m_pProceduralBLAS;
        inst.Mask         = OPAQUE_GEOM_MASK;
        inst.Transform.SetTranslation(
            largeSpherePositions[k].x,
            largeSpherePositions[k].y,
            largeSpherePositions[k].z);
    }


    static std::string smallSphereNames[MaxSmallSpheres];

    for (int i = 0; i < MaxSmallSpheres; ++i)
    {
        int   idx  = 4 + i;
        auto& inst = Instances[idx];

        smallSphereNames[i] = "SmallSphere" + std::to_string(i);
        inst.InstanceName   = smallSphereNames[i].c_str();
        inst.pBLAS          = m_pProceduralBLAS;
        inst.CustomId       = 1;
        inst.Mask           = (i < m_NumScatterSpheres ? OPAQUE_GEOM_MASK : 0u);

        auto pos = smallSpherePositions[i];
        inst.Transform.SetTranslation(pos.x, pos.y, pos.z);
    }


    for (int k = 0; k < 3; ++k)
    {
        int   idx  = 4 + MaxSmallSpheres + k;
        auto& inst = Instances[idx];

        inst.InstanceName = largeCubeNames[k];
        inst.pBLAS        = m_pCubeBLAS;
        inst.Mask         = OPAQUE_GEOM_MASK;


        float scale = 1.5f;
        inst.Transform.SetRotation(float3x3::Scale(scale, scale, scale).Data());
        inst.Transform.SetTranslation(
            largeCubePositions[k].x,
            largeCubePositions[k].y,
            largeCubePositions[k].z);
    }

    static std::string smallCubeNames[MaxSmallCubes];

    for (int i = 0; i < MaxSmallCubes; ++i)
    {
        int   idx  = 4 + MaxSmallSpheres + 3 + i;
        auto& inst = Instances[idx];

        smallCubeNames[i] = "SmallCube" + std::to_string(i);
        inst.InstanceName = smallCubeNames[i].c_str();
        inst.pBLAS        = m_pSmallCubeBLAS;
        inst.Mask         = (i < m_NumSmallCubes ? OPAQUE_GEOM_MASK : 0u);

        float scale     = 0.3f + (i % 5) * 0.05f; 
        float rotationX = i * 0.2f;               
        float rotationY = i * 0.3f;              
        float rotationZ = i * 0.1f;               

        float3x3 rotMatrix = float3x3::RotationY(rotationY) *
            float3x3::RotationX(rotationX) *
            float3x3::RotationZ(rotationZ) *
            float3x3::Scale(scale, scale, scale);
        inst.Transform.SetRotation(rotMatrix.Data());

        if (i < smallCubePositions.size())
        {
            auto pos = smallCubePositions[i];
            inst.Transform.SetTranslation(pos.x, pos.y, pos.z);
        }
    }


    BuildTLASAttribs Attribs;
    Attribs.pTLAS                        = m_pTLAS;
    Attribs.Update                       = NeedUpdate;
    Attribs.pScratchBuffer               = m_ScratchBuffer;
    Attribs.pInstanceBuffer              = m_InstanceBuffer;
    Attribs.pInstances                   = Instances;
    Attribs.InstanceCount                = TotalObjects; 
    Attribs.BindingMode                  = HIT_GROUP_BINDING_MODE_PER_INSTANCE;
    Attribs.HitGroupStride               = HIT_GROUP_STRIDE;
    Attribs.TLASTransitionMode           = RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
    Attribs.BLASTransitionMode           = RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
    Attribs.InstanceBufferTransitionMode = RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
    Attribs.ScratchBufferTransitionMode  = RESOURCE_STATE_TRANSITION_MODE_TRANSITION;

    m_pImmediateContext->BuildTLAS(Attribs);


    delete[] Instances;
}

void Tutorial21_RayTracing::CreateSBT()
{
    ShaderBindingTableDesc SBTDesc;
    SBTDesc.Name = "SBT";
    SBTDesc.pPSO = m_pRayTracingPSO;

    m_pDevice->CreateSBT(SBTDesc, &m_pSBT);
    VERIFY_EXPR(m_pSBT != nullptr);


    m_pSBT->BindRayGenShader("Main");
    m_pSBT->BindMissShader("PrimaryMiss", PRIMARY_RAY_INDEX);
    m_pSBT->BindMissShader("ShadowMiss", SHADOW_RAY_INDEX);

    m_pSBT->BindHitGroupForInstance(m_pTLAS, "Ground Instance", PRIMARY_RAY_INDEX, "GroundHit");
    m_pSBT->BindHitGroupForInstance(m_pTLAS, "Sphere Instance", PRIMARY_RAY_INDEX, "SphereGlass");
    m_pSBT->BindHitGroupForInstance(m_pTLAS, "Sphere Instance 2", PRIMARY_RAY_INDEX, "SphereSolida");
    m_pSBT->BindHitGroupForInstance(m_pTLAS, "Sphere Instance 3", PRIMARY_RAY_INDEX, "SpherePrimaryHit");

    m_pSBT->BindHitGroupForInstance(m_pTLAS, "Cube Instance Large 1", PRIMARY_RAY_INDEX, "GroundHit");        
    m_pSBT->BindHitGroupForInstance(m_pTLAS, "Cube Instance Large 2", PRIMARY_RAY_INDEX, "GlassPrimaryHit");  
    m_pSBT->BindHitGroupForInstance(m_pTLAS, "Cube Instance Large 3", PRIMARY_RAY_INDEX, "SpherePrimaryHit"); 


 
    static constexpr int               NumSmallSpheres = MaxSmallSpheres;
    std::mt19937                       generator(std::random_device{}());
    std::uniform_int_distribution<int> distribution(0, 9);

    for (int i = 0; i < NumSmallSpheres; ++i)
    {
        std::string instanceName = "SmallSphere" + std::to_string(i);

       
        const char* shaderName;
        int         shaderSelector = (i % 5); 
        if (shaderSelector < 2)
        {
            shaderName = "SphereSolida";
        }
        else if (shaderSelector < 4)
        {
            shaderName = "SpherePrimaryHit";
        }
        else
        {
            shaderName = "SphereGlass";
        }

        m_pSBT->BindHitGroupForInstance(m_pTLAS, instanceName.c_str(), PRIMARY_RAY_INDEX, shaderName);
    }

 
    for (int i = 0; i < MaxSmallCubes; ++i)
    {
        std::string instanceName = "SmallCube" + std::to_string(i);


        const char* shaderName;
        int         shaderSelector = (i % 3);
        if (shaderSelector == 0)
        {
            shaderName = "GroundHit"; 
        }
        else if (shaderSelector == 1)
        {
            shaderName = "GlassPrimaryHit"; 
        }
        else
        {
            shaderName = "SpherePrimaryHit"; 
        }

        m_pSBT->BindHitGroupForInstance(m_pTLAS, instanceName.c_str(), PRIMARY_RAY_INDEX, shaderName);
    }

    m_pSBT->BindHitGroupForTLAS(m_pTLAS, SHADOW_RAY_INDEX, nullptr);

    
    for (int k = 1; k <= 3; ++k)
    {
        std::string name = "Sphere Instance";
        if (k > 1)
            name += " " + std::to_string(k);

        m_pSBT->BindHitGroupForInstance(
            m_pTLAS,
            name.c_str(),
            SHADOW_RAY_INDEX,
            "SphereShadowHit");
    }

    int sphereCounter = 0;
    while (sphereCounter < NumSmallSpheres)
    {
        std::string sphereName = "SmallSphere" + std::to_string(sphereCounter);

        m_pSBT->BindHitGroupForInstance(
            m_pTLAS,
            sphereName.c_str(),
            SHADOW_RAY_INDEX,
            "SphereShadowHit");

        ++sphereCounter;
    }

    for (int k = 1; k <= 3; ++k)
    {
        std::string name = "Cube Instance Large " + std::to_string(k);
        m_pSBT->BindHitGroupForInstance(m_pTLAS, name.c_str(), SHADOW_RAY_INDEX, nullptr);
    }

    for (int i = 0; i < MaxSmallCubes; ++i)
    {
        std::string cubeName = "SmallCube" + std::to_string(i);
        m_pSBT->BindHitGroupForInstance(m_pTLAS, cubeName.c_str(), SHADOW_RAY_INDEX, nullptr);
    }

    m_pImmediateContext->UpdateSBT(m_pSBT);
}

void Tutorial21_RayTracing::Initialize(const SampleInitInfo& InitInfo)
{
    SampleBase::Initialize(InitInfo);

    if ((m_pDevice->GetAdapterInfo().RayTracing.CapFlags & RAY_TRACING_CAP_FLAG_STANDALONE_SHADERS) == 0)
    {
        UNSUPPORTED("Ray tracing shaders are not supported by device");
        return;
    }

    m_NumScatterSpheres = 50;
    m_NumSmallCubes     = 30;
    m_ScatterSpherePositions.clear();
    m_ScatterSphereNames.clear();
    m_SmallCubePositions.clear();
    m_SmallCubeNames.clear();

    BufferDesc BuffDesc;
    BuffDesc.Name      = "Constant buffer";
    BuffDesc.Size      = sizeof(m_Constants);
    BuffDesc.Usage     = USAGE_DEFAULT;
    BuffDesc.BindFlags = BIND_UNIFORM_BUFFER;

    m_pDevice->CreateBuffer(BuffDesc, nullptr, &m_ConstantsCB);
    VERIFY_EXPR(m_ConstantsCB != nullptr);

    CreateGraphicsPSO();
    CreateRayTracingPSO();
    LoadTextures();
    CreateCubeBLAS(2.0f, m_pCubeBLAS);     
    CreateCubeBLAS(0.5f, m_pSmallCubeBLAS);
    CreateProceduralBLAS();
    UpdateTLAS();
    CreateSBT();

    m_Camera.SetPos(float3(25.f, -0.5f, -8.f));
    m_Camera.SetRotation(1.25f, -0.1f);
    m_Camera.SetRotationSpeed(0.005f);
    m_Camera.SetMoveSpeed(5.f);
    m_Camera.SetSpeedUpScales(5.f, 10.f);


    {
        m_Constants.ClipPlanes                = float2{0.1f, 100.0f};
        m_Constants.ShadowPCF                 = 1;
        m_Constants.MaxRecursion              = std::min(Uint32{6}, m_MaxRecursionDepth);
        m_Constants.SphereReflectionColorMask = {0.65f, 0.85f, 0.95f};
        m_Constants.SphereReflectionBlur      = 3;
        m_Constants.GlassReflectionColorMask  = {0.22f, 0.83f, 0.93f};
        m_Constants.GlassAbsorption           = 0.5f;
        m_Constants.GlassMaterialColor        = {0.33f, 0.93f, 0.29f};
        m_Constants.GlassIndexOfRefraction    = {1.5f, 1.02f};
        m_Constants.GlassEnableDispersion     = 0;
        m_Constants.DispersionSamples[0]      = {0.140000f, 0.000000f, 0.266667f, 0.53f};
        m_Constants.DispersionSamples[1]      = {0.130031f, 0.037556f, 0.612267f, 0.25f};
        m_Constants.DispersionSamples[2]      = {0.100123f, 0.213556f, 0.785067f, 0.16f};
        m_Constants.DispersionSamples[3]      = {0.050277f, 0.533556f, 0.785067f, 0.00f};
        m_Constants.DispersionSamples[4]      = {0.000000f, 0.843297f, 0.619682f, 0.13f};
        m_Constants.DispersionSamples[5]      = {0.000000f, 0.927410f, 0.431834f, 0.38f};
        m_Constants.DispersionSamples[6]      = {0.000000f, 0.972325f, 0.270893f, 0.27f};
        m_Constants.DispersionSamples[7]      = {0.000000f, 0.978042f, 0.136858f, 0.19f};
        m_Constants.DispersionSamples[8]      = {0.324000f, 0.944560f, 0.029730f, 0.47f};
        m_Constants.DispersionSamples[9]      = {0.777600f, 0.871879f, 0.000000f, 0.64f};
        m_Constants.DispersionSamples[10]     = {0.972000f, 0.762222f, 0.000000f, 0.77f};
        m_Constants.DispersionSamples[11]     = {0.971835f, 0.482222f, 0.000000f, 0.62f};
        m_Constants.DispersionSamples[12]     = {0.886744f, 0.202222f, 0.000000f, 0.73f};
        m_Constants.DispersionSamples[13]     = {0.715967f, 0.000000f, 0.000000f, 0.68f};
        m_Constants.DispersionSamples[14]     = {0.459920f, 0.000000f, 0.000000f, 0.91f};
        m_Constants.DispersionSamples[15]     = {0.218000f, 0.000000f, 0.000000f, 0.99f};
        m_Constants.DispersionSampleCount     = 4;
        m_Constants.AmbientColor              = float4(1.f, 1.f, 1.f, 0.f) * 0.015f;
        m_Constants.LightPos[0]               = {10.00f, +9.0f, +2.00f, 0.f};
        m_Constants.LightColor[0]             = {0.90f, +0.9f, +1.00f, 0.f};
        m_Constants.LightPos[1]               = {-5.00f, +6.0f, -8.00f, 0.f};
        m_Constants.LightColor[1]             = {1.00f, +0.8f, +0.70f, 0.f};
        m_Constants.DiscPoints[0]             = {+0.0f, +0.0f, +0.9f, -0.9f};
        m_Constants.DiscPoints[1]             = {-0.8f, +1.0f, -1.1f, -0.8f};
        m_Constants.DiscPoints[2]             = {+1.5f, +1.2f, -2.1f, +0.7f};
        m_Constants.DiscPoints[3]             = {+0.1f, -2.2f, -0.2f, +2.4f};
        m_Constants.DiscPoints[4]             = {+2.4f, -0.3f, -3.0f, +2.8f};
        m_Constants.DiscPoints[5]             = {+2.0f, -2.6f, +0.7f, +3.5f};
        m_Constants.DiscPoints[6]             = {-3.2f, -1.6f, +3.4f, +2.2f};
        m_Constants.DiscPoints[7]             = {-1.8f, -3.2f, -1.1f, +3.6f};
    }
    static_assert(sizeof(HLSL::Constants) % 16 == 0, "must be aligned by 16 bytes");
}

void Tutorial21_RayTracing::ModifyEngineInitInfo(const ModifyEngineInitInfoAttribs& Attribs)
{
    SampleBase::ModifyEngineInitInfo(Attribs);
    Attribs.EngineCI.Features.RayTracing = DEVICE_FEATURE_STATE_ENABLED;
}

void Tutorial21_RayTracing::Render()
{
    UpdateTLAS();

    {
        float3 CameraWorldPos = float3::MakeVector(m_Camera.GetWorldMatrix()[3]);
        auto   CameraViewProj = m_Camera.GetViewMatrix() * m_Camera.GetProjMatrix();

        m_Constants.CameraPos   = float4{CameraWorldPos, 1.0f};
        m_Constants.InvViewProj = CameraViewProj.Inverse();

        m_pImmediateContext->UpdateBuffer(m_ConstantsCB, 0, sizeof(m_Constants), &m_Constants, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    }

    {
        m_pRayTracingSRB->GetVariableByName(SHADER_TYPE_RAY_GEN, "g_ColorBuffer")->Set(m_pColorRT->GetDefaultView(TEXTURE_VIEW_UNORDERED_ACCESS));

        m_pImmediateContext->SetPipelineState(m_pRayTracingPSO);
        m_pImmediateContext->CommitShaderResources(m_pRayTracingSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

        TraceRaysAttribs Attribs;
        Attribs.DimensionX = m_pColorRT->GetDesc().Width;
        Attribs.DimensionY = m_pColorRT->GetDesc().Height;
        Attribs.pSBT       = m_pSBT;

        m_pImmediateContext->TraceRays(Attribs);
    }

    {
        m_pImageBlitSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_Texture")->Set(m_pColorRT->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE));

        auto* pRTV = m_pSwapChain->GetCurrentBackBufferRTV();
        m_pImmediateContext->SetRenderTargets(1, &pRTV, nullptr, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

        m_pImmediateContext->SetPipelineState(m_pImageBlitPSO);
        m_pImmediateContext->CommitShaderResources(m_pImageBlitSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

        m_pImmediateContext->Draw(DrawAttribs{3, DRAW_FLAG_VERIFY_ALL});
    }
}

void Tutorial21_RayTracing::Update(double CurrTime, double ElapsedTime)
{
    SampleBase::Update(CurrTime, ElapsedTime);
    UpdateUI();

    if (m_Animate)
    {
        m_AnimationTime += static_cast<float>(std::min(m_MaxAnimationTimeDelta, ElapsedTime));
    }

    m_Camera.Update(m_InputController, static_cast<float>(ElapsedTime));

    auto oldPos = m_Camera.GetPos();
    if (oldPos.y < -5.7f)
    {
        oldPos.y = -5.7f;
        m_Camera.SetPos(oldPos);
        m_Camera.Update(m_InputController, 0.f);
    }
}

void Tutorial21_RayTracing::WindowResize(Uint32 Width, Uint32 Height)
{
    if (Width == 0 || Height == 0)
        return;

    float AspectRatio = static_cast<float>(Width) / static_cast<float>(Height);
    m_Camera.SetProjAttribs(m_Constants.ClipPlanes.x, m_Constants.ClipPlanes.y, AspectRatio, PI_F / 4.f,
                            m_pSwapChain->GetDesc().PreTransform, m_pDevice->GetDeviceInfo().NDC.MinZ == -1);


    if (m_pColorRT != nullptr &&
        m_pColorRT->GetDesc().Width == Width &&
        m_pColorRT->GetDesc().Height == Height)
        return;

    m_pColorRT = nullptr;

    TextureDesc RTDesc       = {};
    RTDesc.Name              = "Color buffer";
    RTDesc.Type              = RESOURCE_DIM_TEX_2D;
    RTDesc.Width             = Width;
    RTDesc.Height            = Height;
    RTDesc.BindFlags         = BIND_UNORDERED_ACCESS | BIND_SHADER_RESOURCE;
    RTDesc.ClearValue.Format = m_ColorBufferFormat;
    RTDesc.Format            = m_ColorBufferFormat;

    m_pDevice->CreateTexture(RTDesc, nullptr, &m_pColorRT);
}

void Tutorial21_RayTracing::UpdateUI()
{
    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Configuración de Objetos", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("Control de Objetos");


        if (ImGui::SliderInt("Cantidad de Esferas", &m_NumScatterSpheres, 0, MaxSmallSpheres))
        { 
        }

  
        if (ImGui::SliderInt("Cantidad de Cubos", &m_NumSmallCubes, 0, MaxSmallCubes))
        {
        }

        ImGui::Separator();
        ImGui::Text("Instrucciones: Use WASD para mover la cámara");
        ImGui::End();
    }
}
} 