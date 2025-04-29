// archivo hpp

#pragma once

#include "FirstPersonCamera.hpp"
#include "SampleBase.hpp"
#include "BasicMath.hpp"
#include <vector>
#include <random>

namespace Diligent
{

namespace HLSL
{
#include "../assets/structures.fxh"
}

class Tutorial21_RayTracing final : public SampleBase
{
public:
    virtual const Char* GetSampleName() const override final { return "Tutorial21: Ray tracing"; }
    virtual void ModifyEngineInitInfo(const ModifyEngineInitInfoAttribs& Attribs) override final;
    virtual void Initialize(const SampleInitInfo& InitInfo) override final;
    virtual void Update(double CurrTime, double ElapsedTime) override final;
    virtual void WindowResize(Uint32 Width, Uint32 Height) override final;
    virtual void Render() override final;

private:
    void CreateRayTracingPSO();
    void CreateGraphicsPSO();
    void CreateCubeBLAS();
    void CreateProceduralBLAS();
    void UpdateTLAS();
    void CreateSBT();
    void LoadTextures();
    void UpdateUI();

    static constexpr int NumTextures = 4;
    static constexpr int NumCubes    = 4;

    RefCntAutoPtr<IBuffer> m_CubeAttribsCB;
    RefCntAutoPtr<IBuffer> m_BoxAttribsCB;
    RefCntAutoPtr<IBuffer> m_ConstantsCB;
    RefCntAutoPtr<IPipelineState>         m_pRayTracingPSO;
    RefCntAutoPtr<IShaderResourceBinding> m_pRayTracingSRB;
    RefCntAutoPtr<IPipelineState>         m_pImageBlitPSO;
    RefCntAutoPtr<IShaderResourceBinding> m_pImageBlitSRB;
    RefCntAutoPtr<IBottomLevelAS>      m_pCubeBLAS;
    RefCntAutoPtr<IBottomLevelAS>      m_pProceduralBLAS;
    RefCntAutoPtr<ITopLevelAS>         m_pTLAS;
    RefCntAutoPtr<IBuffer>             m_InstanceBuffer;
    RefCntAutoPtr<IBuffer>             m_ScratchBuffer;
    RefCntAutoPtr<IShaderBindingTable> m_pSBT;

    Uint32                   m_MaxRecursionDepth     = 8;
    int                      m_NumScatterSpheres     = 20;
    static constexpr int     MaxSmallSpheres         = 200;
    const double             m_MaxAnimationTimeDelta = 1.0 / 60.0;
    float                    m_AnimationTime         = 0.0f;
    float                    m_DispersionFactor      = 0.1f;
    bool                     m_EnableCubes[NumCubes] = {true, true, true, true};
    bool                     m_Animate               = true;
    HLSL::Constants          m_Constants             = {};
    static std::vector<float3> smallSpherePositions;
    std::vector<std::string> m_ScatterSphereNames;
    std::vector<float3>        m_ScatterSpherePositions;
    std::mt19937               m_Rng{std::random_device{}()};

    FirstPersonCamera m_Camera;
    TEXTURE_FORMAT          m_ColorBufferFormat = TEX_FORMAT_RGBA8_UNORM;
    RefCntAutoPtr<ITexture> m_pColorRT;
};

}