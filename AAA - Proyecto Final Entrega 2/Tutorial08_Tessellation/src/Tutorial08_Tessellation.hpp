#pragma once

#include "SampleBase.hpp"
#include "BasicMath.hpp"

namespace Diligent
{

class Tutorial08_Tessellation final : public SampleBase
{
public:
    virtual void ModifyEngineInitInfo(const ModifyEngineInitInfoAttribs& Attribs) override final;
    virtual void Initialize(const SampleInitInfo& InitInfo) override final;

    virtual void Render() override final;
    virtual void Update(double CurrTime, double ElapsedTime) override final;

    virtual const Char* GetSampleName() const override final { return "Tutorial08: Tessellation"; }

private:
    void CreatePipelineStates();
    void LoadTextures();
    void UpdateUI();
    void ApplyErosionEffects();

    RefCntAutoPtr<IPipelineState>         m_pPSO[2];
    RefCntAutoPtr<IShaderResourceBinding> m_SRB[2];
    RefCntAutoPtr<IBuffer>                m_ShaderConstants;
    RefCntAutoPtr<ITextureView>           m_HeightMapSRV;
    RefCntAutoPtr<ITextureView>           m_ColorMapSRV;

    float4x4 m_WorldViewProjMatrix;
    float4x4 m_WorldViewMatrix;

    bool  m_Animate              = true;
    bool  m_Wireframe            = false;
    float m_RotationAngle        = 0;
    float m_TessDensity          = 32;
    float m_Distance             = 10.f;
    bool  m_AdaptiveTessellation = true;
    int   m_BlockSize            = 32;
    float m_HeightScale          = 5.0f;
    float m_LengthScale          = 10.0f;
    float m_ErosionLevel         = 0.0f;

    bool  m_SimulatingErosion = false;
    float m_ErosionSpeed      = 0.1f;

    unsigned int m_HeightMapWidth  = 0;
    unsigned int m_HeightMapHeight = 0;
};

} // namespace Diligent