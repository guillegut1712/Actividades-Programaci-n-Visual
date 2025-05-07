#pragma once

#include "SampleBase.hpp"
#include "BasicMath.hpp"
#include <vector>

namespace Diligent
{

// Estructura para el manejo de una gota de agua en la simulación de erosión
struct Droplet
{
    float2 position;  // Posición x,z en el mapa
    float2 direction; // Dirección normalizada de movimiento
    float  speed;     // Velocidad actual
    float  water;     // Cantidad de agua
    float  sediment;  // Cantidad de sedimento transportado
};

// Parámetros para la simulación de erosión
struct ErosionParams
{
    float inertia      = 0.3f;  // Persistencia de la dirección (0-1)
    float capacity     = 8.0f;  // Capacidad de sedimento
    float deposition   = 0.2f;  // Velocidad de deposición (0-1)
    float erosion      = 0.7f;  // Velocidad de erosión (0-1)
    float evaporation  = 0.02f; // Tasa de evaporación (0-1)
    float minSlope     = 0.01f; // Pendiente mínima para la erosión
    float gravity      = 10.0f; // Fuerza de gravedad
    float radius       = 4.0f;  // Radio de erosión
    int   maxPath      = 64;    // Pasos máximos por gota
    int   dropletCount = 10000; // Número de gotas por ronda
};

class Tutorial08_Tessellation final : public SampleBase
{
public:
    virtual void ModifyEngineInitInfo(const ModifyEngineInitInfoAttribs& Attribs) override final;
    virtual void Initialize(const SampleInitInfo& InitInfo) override final;

    virtual void Render() override final;
    virtual void Update(double CurrTime, double ElapsedTime) override final;

    virtual const Char* GetSampleName() const override final { return "Tutorial08: Terrain with Erosion"; }

private:
    void CreatePipelineStates();
    void LoadTextures();
    void UpdateUI();

    // Nuevos métodos para la erosión
    void  InitializeErosion();
    void  SimulateErosion(int dropletCount);
    void  CreateUpdatedHeightMap();
    float SampleHeightmap(float x, float y);
    void  ModifyHeightmap(float x, float y, float delta, float radius);
    void  ApplyErosionChanges();
    void  RevertErosion();
    void  SaveErosionState();

    RefCntAutoPtr<IPipelineState>         m_pPSO[2];
    RefCntAutoPtr<IShaderResourceBinding> m_SRB[2];
    RefCntAutoPtr<IBuffer>                m_ShaderConstants;
    RefCntAutoPtr<ITextureView>           m_HeightMapSRV;
    RefCntAutoPtr<ITextureView>           m_ColorMapSRV;
    RefCntAutoPtr<ITexture>               m_pHeightMap;
    RefCntAutoPtr<ITexture>               m_pColorMap;

    float4x4 m_WorldViewProjMatrix;
    float4x4 m_WorldViewMatrix;

    bool  m_Animate              = true;
    bool  m_Wireframe            = false;
    float m_RotationAngle        = 0;
    float m_TessDensity          = 32;
    float m_Distance             = 10.f;
    bool  m_AdaptiveTessellation = true;
    int   m_BlockSize            = 32;

    unsigned int m_HeightMapWidth  = 0;
    unsigned int m_HeightMapHeight = 0;

    // Nuevas variables para erosión
    std::vector<float> m_OriginalHeightData;        // Copia del mapa de altura original
    std::vector<float> m_HeightData;                // Datos de altura actuales
    std::vector<float> m_ErosionChangeMap;          // Mapa de cambios por erosión
    ErosionParams      m_ErosionParams;             // Parámetros de erosión
    int                m_ErosionIterationCount = 0; // Contador de iteraciones

    // Variables para apariencia del terreno
    float4 m_GrassColor          = float4(0.3f, 0.9f, 0.3f, 1.0f);
    float4 m_RockColor           = float4(0.5f, 0.5f, 0.5f, 1.0f);
    float  m_GrassSlopeThreshold = 0.5f;
    float  m_GrassBlendAmount    = 0.5f;
};

} // namespace Diligent