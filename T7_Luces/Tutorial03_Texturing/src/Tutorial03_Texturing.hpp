#pragma once

#include "SampleBase.hpp"
#include "BasicMath.hpp"

namespace Diligent
{

class Tutorial03_Texturing final : public SampleBase
{
public:
    virtual void Initialize(const SampleInitInfo& InitInfo) override final;

    virtual void        Render() override final;
    virtual void        Update(double CurrTime, double ElapsedTime) override final;
    virtual const Char* GetSampleName() const override final { return "Tutorial03: Texturing with Shadows"; }

private:
    void CreatePipelineState();
    void CreateVertexBuffer();
    void CreateIndexBuffer();
    void CreateFloorGeometry();
    void LoadTextures();
    void CreateShadowMap();
    void RenderShadowMap();
    void SetupLights();
    void UpdateCamera(double CurrTime, double ElapsedTime);

    // Original objects
    RefCntAutoPtr<IPipelineState>         m_pPSO;
    RefCntAutoPtr<IBuffer>                m_CubeVertexBuffer;
    RefCntAutoPtr<IBuffer>                m_CubeIndexBuffer;
    RefCntAutoPtr<IBuffer>                m_VSConstants;
    RefCntAutoPtr<ITextureView>           m_TextureSRV;
    RefCntAutoPtr<IShaderResourceBinding> m_SRB;
    float4x4                              m_WorldViewProjMatrix;

    // New objects for the enhanced features
    RefCntAutoPtr<IBuffer>                m_FloorVertexBuffer;
    RefCntAutoPtr<IBuffer>                m_FloorIndexBuffer;
    RefCntAutoPtr<IPipelineState>         m_pShadowMapPSO;
    RefCntAutoPtr<ITexture>               m_ShadowMap;
    RefCntAutoPtr<ITextureView>           m_ShadowMapSRV;
    RefCntAutoPtr<ITextureView>           m_ShadowMapRTV;
    RefCntAutoPtr<IShaderResourceBinding> m_ShadowMapSRB;
    RefCntAutoPtr<ITextureView>           m_FloorTextureSRV;
    RefCntAutoPtr<IShaderResourceBinding> m_FloorSRB;
    RefCntAutoPtr<IBuffer>                m_LightBuffer;


    double m_CurrTime = 0.0; 

    // Camera and scene settings
    float3   m_CameraPos           = float3(0.0f, 2.0f, -5.0f);
    float3   m_CameraTarget        = float3(0.0f, 0.0f, 0.0f);
    float3   m_CameraUp            = float3(0.0f, 1.0f, 0.0f);
    float    m_CameraYaw           = 0.0f;
    float    m_CameraPitch         = 0.0f;
    float    m_CameraDistance      = 5.0f;
    float    m_CameraRotationSpeed = 0.3f; // Velocidad de rotación automática
    bool     m_AutoRotateCamera    = true; // Flag para rotación automática
    float3   m_LightDirection      = normalize(float3(0.5f, -1.0f, 0.3f));
    float4x4 m_LightViewProjMatrix;

    struct LightAttribs
    {
        float4 Direction;
        float4 Ambient;
        float4 Diffuse;
        float4 Specular;
    };

    LightAttribs m_LightAttribs;
};

} // namespace Diligent