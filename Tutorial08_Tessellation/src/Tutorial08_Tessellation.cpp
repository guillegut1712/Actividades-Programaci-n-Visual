#include "Tutorial08_Tessellation.hpp"
#include "MapHelper.hpp"
#include "GraphicsUtilities.h"
#include "TextureUtilities.h"
#include "ColorConversion.h"
#include "ShaderMacroHelper.hpp"
#include "imgui.h"
#include <random>
#include <algorithm>

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
    unsigned int NumHorzBlocks; // Number of blocks along the horizontal edge
    unsigned int NumVertBlocks; // Number of blocks along the horizontal edge
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

    // Nuevos parámetros para el shader de terreno
    float4 GrassColor;
    float4 RockColor;
    float  GrassSlopeThreshold;
    float  GrassBlendAmount;
};

} // namespace

void Tutorial08_Tessellation::CreatePipelineStates()
{
    const bool bWireframeSupported = m_pDevice->GetDeviceInfo().Features.GeometryShaders;

    // Pipeline state object encompasses configuration of all GPU stages

    GraphicsPipelineStateCreateInfo PSOCreateInfo;

    // Pipeline state name is used by the engine to report issues.
    // It is always a good idea to give objects descriptive names.
    PSOCreateInfo.PSODesc.Name = "Terrain PSO";

    // This is a graphics pipeline
    PSOCreateInfo.PSODesc.PipelineType = PIPELINE_TYPE_GRAPHICS;

    // clang-format off
    // This tutorial will render to a single render target
    PSOCreateInfo.GraphicsPipeline.NumRenderTargets             = 1;
    // Set render target format which is the format of the swap chain's color buffer
    PSOCreateInfo.GraphicsPipeline.RTVFormats[0]                = m_pSwapChain->GetDesc().ColorBufferFormat;
    // Set depth buffer format which is the format of the swap chain's back buffer
    PSOCreateInfo.GraphicsPipeline.DSVFormat                    = m_pSwapChain->GetDesc().DepthBufferFormat;
    // Primitive topology type defines what kind of primitives will be rendered by this pipeline state
    PSOCreateInfo.GraphicsPipeline.PrimitiveTopology            = PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST;
    // Cull back faces. For some reason, in OpenGL the order is reversed
    PSOCreateInfo.GraphicsPipeline.RasterizerDesc.CullMode      = m_pDevice->GetDeviceInfo().IsGLDevice() ? CULL_MODE_FRONT : CULL_MODE_BACK;
    // Enable depth testing
    PSOCreateInfo.GraphicsPipeline.DepthStencilDesc.DepthEnable = True;
    // clang-format on

    // Create dynamic uniform buffer that will store shader constants
    CreateUniformBuffer(m_pDevice, sizeof(GlobalConstants), "Global shader constants CB", &m_ShaderConstants);

    ShaderCreateInfo ShaderCI;
    // Tell the system that the shader source code is in HLSL.
    // For OpenGL, the engine will convert this into GLSL under the hood.
    ShaderCI.SourceLanguage = SHADER_SOURCE_LANGUAGE_HLSL;

    // OpenGL backend requires emulated combined HLSL texture samplers (g_Texture + g_Texture_sampler combination)
    ShaderCI.Desc.UseCombinedTextureSamplers = true;

    // Pack matrices in row-major order
    ShaderCI.CompileFlags = SHADER_COMPILE_FLAG_PACK_MATRIX_ROW_MAJOR;

    // Define shader macros
    ShaderMacroHelper Macros;
    Macros.Add("BLOCK_SIZE", m_BlockSize);

    // Presentation engine always expects input in gamma space. Normally, pixel shader output is
    // converted from linear to gamma space by the GPU. However, some platforms (e.g. Android in GLES mode,
    // or Emscripten in WebGL mode) do not support gamma-correction. In this case the application
    // has to do the conversion manually.
    Macros.Add("CONVERT_PS_OUTPUT_TO_GAMMA", m_ConvertPSOutputToGamma);

    ShaderCI.Macros = Macros;

    // Create a shader source stream factory to load shaders from files.
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

    // Define variable type that will be used by default
    PSOCreateInfo.PSODesc.ResourceLayout.DefaultVariableType = SHADER_RESOURCE_VARIABLE_TYPE_STATIC;

    // clang-format off
    ShaderResourceVariableDesc Vars[] = 
    {
        {SHADER_TYPE_HULL | SHADER_TYPE_DOMAIN,  "g_HeightMap", SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE},
        {SHADER_TYPE_PIXEL,                      "g_Texture",   SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE}
    };
    // clang-format on
    PSOCreateInfo.PSODesc.ResourceLayout.Variables    = Vars;
    PSOCreateInfo.PSODesc.ResourceLayout.NumVariables = _countof(Vars);

    // clang-format off
    // Define immutable sampler for g_HeightMap and g_Texture. Immutable samplers should be used whenever possible
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
    // clang-format on
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
            // clang-format off
            m_pPSO[i]->GetStaticVariableByName(SHADER_TYPE_VERTEX, "VSConstants")->Set(m_ShaderConstants);
            m_pPSO[i]->GetStaticVariableByName(SHADER_TYPE_HULL,   "HSConstants")->Set(m_ShaderConstants);
            m_pPSO[i]->GetStaticVariableByName(SHADER_TYPE_DOMAIN, "DSConstants")->Set(m_ShaderConstants);
            // clang-format on
        }
    }
    if (m_pPSO[1])
    {
        // clang-format off
        m_pPSO[1]->GetStaticVariableByName(SHADER_TYPE_GEOMETRY, "GSConstants")->Set(m_ShaderConstants);
        m_pPSO[1]->GetStaticVariableByName(SHADER_TYPE_PIXEL,    "PSConstants")->Set(m_ShaderConstants);
        // clang-format on
    }
}

void Tutorial08_Tessellation::LoadTextures()
{
    {
        // Load height map texture
        TextureLoadInfo loadInfo;
        loadInfo.IsSRGB = false;
        loadInfo.Name   = "Terrain height map";
        CreateTextureFromFile("ps_height_1k.png", loadInfo, m_pDevice, &m_pHeightMap);

        // Get shader resource view from the texture
        m_HeightMapSRV = m_pHeightMap->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE);

        // Get height map dimensions
        const auto& HMDesc = m_pHeightMap->GetDesc();
        m_HeightMapWidth   = HMDesc.Width;
        m_HeightMapHeight  = HMDesc.Height;

        // Inicializar datos para la erosión
        InitializeErosion();
    }

    {
        // Load color map texture
        TextureLoadInfo loadInfo;
        loadInfo.IsSRGB = true;
        loadInfo.Name   = "Terrain color map";
        CreateTextureFromFile("ps_texture_2k.png", loadInfo, m_pDevice, &m_pColorMap);

        // Get shader resource view from the texture
        m_ColorMapSRV = m_pColorMap->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE);
    }

    // Since we are using mutable variable, we must create a shader resource binding object
    // http://diligentgraphics.com/2016/03/23/resource-binding-model-in-diligent-engine-2-0/
    for (size_t i = 0; i < _countof(m_pPSO); ++i)
    {
        if (m_pPSO[i])
        {
            m_pPSO[i]->CreateShaderResourceBinding(&m_SRB[i], true);
            // Set texture SRV in the SRB
            // clang-format off
            m_SRB[i]->GetVariableByName(SHADER_TYPE_PIXEL,  "g_Texture")->Set(m_ColorMapSRV);
            m_SRB[i]->GetVariableByName(SHADER_TYPE_DOMAIN, "g_HeightMap")->Set(m_HeightMapSRV);
            m_SRB[i]->GetVariableByName(SHADER_TYPE_HULL,   "g_HeightMap")->Set(m_HeightMapSRV);
            // clang-format on
        }
    }
}

void Tutorial08_Tessellation::InitializeErosion()
{
    // Inicializar con valores por defecto
    m_HeightData.resize(m_HeightMapWidth * m_HeightMapHeight, 0.5f); // Mitad de la altura como predeterminado
    m_OriginalHeightData.resize(m_HeightMapWidth * m_HeightMapHeight, 0.5f);
    m_ErosionChangeMap.resize(m_HeightMapWidth * m_HeightMapHeight, 0.0f);
    // Extraer datos del heightmap para procesamiento de erosión
    TextureSubResData subresData;
    Box               box;
    box.MaxX = m_HeightMapWidth;
    box.MaxY = m_HeightMapHeight;

    std::vector<uint8_t> heightMapData(m_HeightMapWidth * m_HeightMapHeight * 4);
    subresData.pData  = heightMapData.data();
    subresData.Stride = m_HeightMapWidth * 4;

    // Declarar mappedData como variable
    MappedTextureSubresource mappedData;
    m_pImmediateContext->MapTextureSubresource(m_pHeightMap, 0, 0, MAP_READ, MAP_FLAG_DO_NOT_WAIT, &box, mappedData);

    // Resto del código con acceso directo a mappedData (sin el operador ->)
    for (unsigned int y = 0; y < m_HeightMapHeight; ++y)
    {
        for (unsigned int x = 0; x < m_HeightMapWidth; ++x)
        {
            for (unsigned int c = 0; c < 4; ++c)
            {
                unsigned int srcOffset = y * mappedData.Stride + x * 4 + c;
                unsigned int dstOffset = y * m_HeightMapWidth * 4 + x * 4 + c;

                // Verificación de límites
                if (srcOffset < mappedData.Stride * m_HeightMapHeight && dstOffset < heightMapData.size())
                {
                    heightMapData[dstOffset] = *((uint8_t*)mappedData.pData + srcOffset);
                }
            }
        }
    }

    m_pImmediateContext->UnmapTextureSubresource(m_pHeightMap, 0, 0);

    // Convertir a float para mejor procesamiento
    m_HeightData.resize(m_HeightMapWidth * m_HeightMapHeight);
    m_OriginalHeightData.resize(m_HeightMapWidth * m_HeightMapHeight);
    m_ErosionChangeMap.resize(m_HeightMapWidth * m_HeightMapHeight, 0.0f);

    for (unsigned int i = 0; i < m_HeightMapWidth * m_HeightMapHeight; ++i)
    {
        // Asumiendo que el heightmap es de formato R8G8B8A8, tomamos el canal R como altura
        m_HeightData[i]         = static_cast<float>(heightMapData[i * 4]) / 255.0f;
        m_OriginalHeightData[i] = m_HeightData[i];
    }
}

float Tutorial08_Tessellation::SampleHeightmap(float x, float y)
{
    // Interpolación bilineal para obtener la altura en una posición arbitraria
    int x0 = static_cast<int>(x);
    int y0 = static_cast<int>(y);
    int x1 = x0 + 1;
    int y1 = y0 + 1;

    // Asegurar que las coordenadas estén dentro de los límites
    x0 = std::max(0, std::min(x0, static_cast<int>(m_HeightMapWidth) - 1));
    y0 = std::max(0, std::min(y0, static_cast<int>(m_HeightMapHeight) - 1));
    x1 = std::max(0, std::min(x1, static_cast<int>(m_HeightMapWidth) - 1));
    y1 = std::max(0, std::min(y1, static_cast<int>(m_HeightMapHeight) - 1));

    // Calcular factores de interpolación
    float tx = x - static_cast<float>(x0);
    float ty = y - static_cast<float>(y0);

    // Obtener alturas en los cuatro puntos de la cuadrícula
    float h00 = m_HeightData[y0 * m_HeightMapWidth + x0];
    float h10 = m_HeightData[y0 * m_HeightMapWidth + x1];
    float h01 = m_HeightData[y1 * m_HeightMapWidth + x0];
    float h11 = m_HeightData[y1 * m_HeightMapWidth + x1];

    // Interpolar bilinealmente
    float h0 = (1.0f - tx) * h00 + tx * h10;
    float h1 = (1.0f - tx) * h01 + tx * h11;
    return (1.0f - ty) * h0 + ty * h1;
}

void Tutorial08_Tessellation::ModifyHeightmap(float x, float y, float delta, float radius)
{
    // Modificar el heightmap dentro de un radio usando un kernel gaussiano
    int r  = static_cast<int>(radius);
    int x0 = static_cast<int>(x) - r;
    int y0 = static_cast<int>(y) - r;
    int x1 = static_cast<int>(x) + r;
    int y1 = static_cast<int>(y) + r;

    // Asegurar que las coordenadas estén dentro de los límites
    x0 = std::max(0, x0);
    y0 = std::max(0, y0);
    x1 = std::min(x1, static_cast<int>(m_HeightMapWidth) - 1);
    y1 = std::min(y1, static_cast<int>(m_HeightMapHeight) - 1);

    float radiusSq = radius * radius;

    // Aplicar modificación con caída gaussiana
    for (int cy = y0; cy <= y1; cy++)
    {
        for (int cx = x0; cx <= x1; cx++)
        {
            float dx     = x - static_cast<float>(cx);
            float dy     = y - static_cast<float>(cy);
            float distSq = dx * dx + dy * dy;

            if (distSq < radiusSq)
            {
                // Calcular peso basado en la distancia (kernel gaussiano)
                float weight = std::exp(-distSq / (2.0f * radiusSq / 9.0f));

                // Modificar el heightmap
                int index = cy * m_HeightMapWidth + cx;
                m_ErosionChangeMap[index] += delta * weight;
            }
        }
    }
}

void Tutorial08_Tessellation::SimulateErosion(int dropletCount)
{
    std::random_device                    rd;
    std::mt19937                          gen(rd());
    std::uniform_real_distribution<float> posDistX(0, static_cast<float>(m_HeightMapWidth - 1));
    std::uniform_real_distribution<float> posDistY(0, static_cast<float>(m_HeightMapHeight - 1));
    std::uniform_real_distribution<float> dirDist(-1.0f, 1.0f);

    // Realizar la simulación para el número de gotas especificado
    for (int i = 0; i < dropletCount; ++i)
    {
        // Crear una nueva gota en una posición aleatoria
        Droplet droplet;
        droplet.position  = float2(posDistX(gen), posDistY(gen));
        droplet.direction = float2(0, 0);
        droplet.speed     = 0.0f;
        droplet.water     = 1.0f;
        droplet.sediment  = 0.0f;

        // Simular el movimiento de la gota hasta que se evapore o alcance el máximo de pasos
        for (int lifetime = 0; lifetime < m_ErosionParams.maxPath; ++lifetime)
        {
            // Obtener coordenadas enteras y fraccionales
            int   xi = static_cast<int>(droplet.position.x);
            int   yi = static_cast<int>(droplet.position.y);
            float xf = droplet.position.x - xi;
            float yf = droplet.position.y - yi;

            // Salir si la gota está fuera del mapa
            if (xi < 0 || xi >= static_cast<int>(m_HeightMapWidth - 1) ||
                yi < 0 || yi >= static_cast<int>(m_HeightMapHeight - 1))
            {
                break;
            }

            // Calcular gradiente usando muestras de altura
            float h00 = m_HeightData[yi * m_HeightMapWidth + xi];
            float h10 = m_HeightData[yi * m_HeightMapWidth + xi + 1];
            float h01 = m_HeightData[(yi + 1) * m_HeightMapWidth + xi];
            float h11 = m_HeightData[(yi + 1) * m_HeightMapWidth + xi + 1];

            // Calcular gradiente utilizando interpolación bilineal
            float2 gradient;
            gradient.x = ((h10 - h00) * (1.0f - yf) + (h11 - h01) * yf);
            gradient.y = ((h01 - h00) * (1.0f - xf) + (h11 - h10) * xf);

            // Actualizar dirección basada en el gradiente y la inercia
            if (length(gradient) <= FLT_EPSILON)
            {
                // Si el gradiente es casi cero, elegir una dirección aleatoria
                droplet.direction = normalize(float2(dirDist(gen), dirDist(gen)));
            }
            else
            {
                // Combinar la dirección actual con el gradiente negativo (cuesta abajo)
                gradient          = -normalize(gradient);
                droplet.direction = normalize(
                    droplet.direction * m_ErosionParams.inertia +
                    gradient * (1.0f - m_ErosionParams.inertia));
            }

            // Calcular la nueva posición
            float2 newPos = droplet.position + droplet.direction;

            // Obtener alturas
            float oldHeight  = SampleHeightmap(droplet.position.x, droplet.position.y);
            float newHeight  = SampleHeightmap(newPos.x, newPos.y);
            float heightDiff = newHeight - oldHeight;

            // Determinar si deposita o erosiona
            if (heightDiff > 0)
            {
                // Si va cuesta arriba, deposita sedimento para rellenar
                float depositAmount = std::min(heightDiff, droplet.sediment);
                ModifyHeightmap(droplet.position.x, droplet.position.y, depositAmount, 1.0f);
                droplet.sediment -= depositAmount;
                droplet.speed = 0.0f;
            }
            else
            {
                // Calcular capacidad de transporte
                float slope    = std::max(-heightDiff, m_ErosionParams.minSlope);
                float capacity = slope * droplet.speed * droplet.water * m_ErosionParams.capacity;

                // Decidir entre depositar o erosionar
                if (droplet.sediment > capacity)
                {
                    // Depositar exceso de sedimento
                    float depositAmount = (droplet.sediment - capacity) * m_ErosionParams.deposition;
                    ModifyHeightmap(droplet.position.x, droplet.position.y, depositAmount, 1.0f);
                    droplet.sediment -= depositAmount;
                }
                else
                {
                    // Erosionar el terreno
                    float erosionAmount = std::min(
                        (capacity - droplet.sediment) * m_ErosionParams.erosion,
                        -heightDiff // No erosionar más que la diferencia de altura
                    );

                    // Distribuir la erosión en un radio
                    ModifyHeightmap(droplet.position.x, droplet.position.y, -erosionAmount, m_ErosionParams.radius);
                    droplet.sediment += erosionAmount;
                }
            }

            // Actualizar velocidad y posición
            droplet.speed    = sqrt(droplet.speed * droplet.speed + std::fabs(heightDiff) * m_ErosionParams.gravity);
            droplet.position = newPos;

            // Reducir el agua por evaporación
            droplet.water *= (1.0f - m_ErosionParams.evaporation);

            // Si se ha evaporado toda el agua, terminar la simulación
            if (droplet.water < 0.01f)
                break;
        }
    }
}

void Tutorial08_Tessellation::ApplyErosionChanges()
{
    // Aplicar los cambios de erosión al heightmap
    for (unsigned int i = 0; i < m_HeightMapWidth * m_HeightMapHeight; ++i)
    {
        m_HeightData[i] += m_ErosionChangeMap[i];
        // Asegurar que la altura esté en el rango [0, 1]
        m_HeightData[i] = std::max(0.0f, std::min(1.0f, m_HeightData[i]));
    }

    // Limpiar el mapa de cambios
    std::fill(m_ErosionChangeMap.begin(), m_ErosionChangeMap.end(), 0.0f);

    // Actualizar la textura de altura
    CreateUpdatedHeightMap();
}

void Tutorial08_Tessellation::SaveErosionState()
{
    // Guardar una copia del estado actual para poder revertir
    std::copy(m_HeightData.begin(), m_HeightData.end(), m_OriginalHeightData.begin());
}

void Tutorial08_Tessellation::RevertErosion()
{
    // Restaurar los datos originales del heightmap
    std::copy(m_OriginalHeightData.begin(), m_OriginalHeightData.end(), m_HeightData.begin());
    std::fill(m_ErosionChangeMap.begin(), m_ErosionChangeMap.end(), 0.0f);

    // Actualizar la textura de altura
    CreateUpdatedHeightMap();

    // Resetear contador de iteraciones
    m_ErosionIterationCount = 0;
}

void Tutorial08_Tessellation::CreateUpdatedHeightMap()
{
    // Crear una nueva textura con los datos de altura modificados
    TextureDesc TexDesc;
    TexDesc.Name      = "Updated Height Map";
    TexDesc.Type      = RESOURCE_DIM_TEX_2D;
    TexDesc.Width     = m_HeightMapWidth;
    TexDesc.Height    = m_HeightMapHeight;
    TexDesc.Format    = TEX_FORMAT_R8_UNORM;
    TexDesc.BindFlags = BIND_SHADER_RESOURCE;
    TexDesc.Usage     = USAGE_DEFAULT;

    // Crear vector para almacenar datos de altura
    std::vector<uint8_t> heightMapBytes(m_HeightMapWidth * m_HeightMapHeight);

    // Crear un patrón de prueba con gradiente para asegurar visibilidad
    for (unsigned int y = 0; y < m_HeightMapHeight; ++y)
    {
        for (unsigned int x = 0; x < m_HeightMapWidth; ++x)
        {
            float normalizedX = static_cast<float>(x) / m_HeightMapWidth;
            float normalizedY = static_cast<float>(y) / m_HeightMapHeight;

            // Usar los datos existentes pero asegurar que no sean totalmente negros
            unsigned int idx    = y * m_HeightMapWidth + x;
            float        height = (idx < m_HeightData.size()) ? m_HeightData[idx] : 0.0f;

            // Si la altura es muy baja, usar un gradiente de prueba
            if (height < 0.05f)
            {
                height = 0.1f + (normalizedX + normalizedY) * 0.4f;
            }

            // Convertir a byte
            heightMapBytes[y * m_HeightMapWidth + x] = static_cast<uint8_t>(height * 255.0f);
        }
    }

    TextureSubResData SubresData;
    SubresData.pData  = heightMapBytes.data();
    SubresData.Stride = m_HeightMapWidth;

    TextureData TexData;
    TexData.NumSubresources = 1;
    TexData.pSubResources   = &SubresData;

    RefCntAutoPtr<ITexture> pNewHeightMap;
    m_pDevice->CreateTexture(TexDesc, &TexData, &pNewHeightMap);

    // Actualizar el SRV para usar la nueva textura
    m_HeightMapSRV = pNewHeightMap->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE);

    // Actualizar los SRBs para usar la nueva textura
    for (size_t i = 0; i < _countof(m_SRB); ++i)
    {
        if (m_SRB[i])
        {
            m_SRB[i]->GetVariableByName(SHADER_TYPE_DOMAIN, "g_HeightMap")->Set(m_HeightMapSRV);
            m_SRB[i]->GetVariableByName(SHADER_TYPE_HULL, "g_HeightMap")->Set(m_HeightMapSRV);
        }
    }

    // Reemplazar la textura anterior con la nueva
    m_pHeightMap = pNewHeightMap;
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
        ImGui::SliderFloat("Tess density", &m_TessDensity, 1.f, 32.f);
        ImGui::SliderFloat("Distance", &m_Distance, 1.f, 20.f);

        // Información de depuración
        ImGui::Text("HeightMap Size: %dx%d", m_HeightMapWidth, m_HeightMapHeight);
        ImGui::Text("HeightData Size: %zu", m_HeightData.size());

        // Mostrar muestras de alturas
        if (!m_HeightData.empty())
        {
            ImGui::Text("Sample Heights [0]: %.3f", m_HeightData[0]);
            size_t middle = m_HeightData.size() / 2;
            if (middle < m_HeightData.size())
                ImGui::Text("Sample Heights [mid]: %.3f", m_HeightData[middle]);
        }

        // Nueva sección de UI para erosión
        if (ImGui::CollapsingHeader("Erosión Hidráulica", ImGuiTreeNodeFlags_DefaultOpen))
        {
            if (ImGui::Button("Aplicar erosión"))
            {
                // Guardar estado antes de aplicar nuevos cambios
                if (m_ErosionIterationCount == 0)
                    SaveErosionState();

                // Simular erosión
                SimulateErosion(m_ErosionParams.dropletCount);
                ApplyErosionChanges();
                m_ErosionIterationCount++;
            }

            ImGui::SameLine();

            if (ImGui::Button("Revertir"))
            {
                RevertErosion();
            }

            ImGui::Text("Iteraciones: %d", m_ErosionIterationCount);

            ImGui::SliderFloat("Inertia", &m_ErosionParams.inertia, 0.0f, 1.0f);
            ImGui::SliderFloat("Capacidad sedimento", &m_ErosionParams.capacity, 1.0f, 16.0f);
            ImGui::SliderFloat("Velocidad deposición", &m_ErosionParams.deposition, 0.0f, 1.0f);
            ImGui::SliderFloat("Velocidad erosión", &m_ErosionParams.erosion, 0.0f, 1.0f);
            ImGui::SliderFloat("Evaporación", &m_ErosionParams.evaporation, 0.0f, 0.1f);
            ImGui::SliderFloat("Pendiente mínima", &m_ErosionParams.minSlope, 0.0001f, 0.05f);
            ImGui::SliderFloat("Gravedad", &m_ErosionParams.gravity, 1.0f, 20.0f);
            ImGui::SliderFloat("Radio erosión", &m_ErosionParams.radius, 1.0f, 8.0f);
            ImGui::SliderInt("Longitud máx. camino", &m_ErosionParams.maxPath, 16, 256);
            ImGui::SliderInt("Gotas por iteración", &m_ErosionParams.dropletCount, 1000, 50000);
        }

        // UI para el color y parámetros del terreno
        if (ImGui::CollapsingHeader("Apariencia del Terreno", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::ColorEdit4("Color de pasto", &m_GrassColor.x);
            ImGui::ColorEdit4("Color de roca", &m_RockColor.x);
            ImGui::SliderFloat("Umbral de pendiente", &m_GrassSlopeThreshold, 0.0f, 1.0f);
            ImGui::SliderFloat("Cantidad de mezcla", &m_GrassBlendAmount, 0.0f, 1.0f);
        }
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
        // If manual gamma correction is required, we need to clear the render target with sRGB color
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

        Consts->LengthScale = 10.f;
        Consts->HeightScale = Consts->LengthScale / 25.f;

        Consts->WorldView     = m_WorldViewMatrix;
        Consts->WorldViewProj = m_WorldViewProjMatrix;

        Consts->TessDensity          = m_TessDensity;
        Consts->AdaptiveTessellation = m_AdaptiveTessellation ? 1 : 0;

        const auto& SCDesc   = m_pSwapChain->GetDesc();
        Consts->ViewportSize = float4(static_cast<float>(SCDesc.Width), static_cast<float>(SCDesc.Height), 1.f / static_cast<float>(SCDesc.Width), 1.f / static_cast<float>(SCDesc.Height));

        Consts->LineWidth = 3.0f;

        // Añadir valores predeterminados para el color (para depuración)
        // Usar valores brillantes para asegurar que se vean
        Consts->GrassColor = float4(0.0f, 1.0f, 0.0f, 1.0f); // Verde brillante por defecto
        Consts->RockColor  = float4(0.5f, 0.5f, 0.5f, 1.0f); // Gris por defecto

        // Si los valores de usuario están definidos, usarlos
        if (m_GrassColor.x > 0.0f || m_GrassColor.y > 0.0f || m_GrassColor.z > 0.0f)
        {
            Consts->GrassColor = m_GrassColor;
        }

        if (m_RockColor.x > 0.0f || m_RockColor.y > 0.0f || m_RockColor.z > 0.0f)
        {
            Consts->RockColor = m_RockColor;
        }

        // Usar valores por defecto para los umbrales si no están inicializados
        Consts->GrassSlopeThreshold = m_GrassSlopeThreshold > 0.0f ?
            m_GrassSlopeThreshold :
            0.5f;

        Consts->GrassBlendAmount = m_GrassBlendAmount > 0.0f ?
            m_GrassBlendAmount :
            0.5f;
    }

    // Set the pipeline state
    m_pImmediateContext->SetPipelineState(m_pPSO[m_Wireframe ? 1 : 0]);
    // Commit shader resources. RESOURCE_STATE_TRANSITION_MODE_TRANSITION mode
    // makes sure that resources are transitioned to required states.
    m_pImmediateContext->CommitShaderResources(m_SRB[m_Wireframe ? 1 : 0], RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    DrawAttribs DrawAttrs;
    DrawAttrs.NumVertices = NumHorzBlocks * NumVertBlocks;
    DrawAttrs.Flags       = DRAW_FLAG_VERIFY_ALL;
    m_pImmediateContext->Draw(DrawAttrs);
}

void Tutorial08_Tessellation::Update(double CurrTime, double ElapsedTime)
{
    SampleBase::Update(CurrTime, ElapsedTime);
    UpdateUI();

    // Set world view matrix
    if (m_Animate)
    {
        m_RotationAngle += static_cast<float>(ElapsedTime) * 0.2f;
        if (m_RotationAngle > PI_F * 2.f)
            m_RotationAngle -= PI_F * 2.f;
    }

    float4x4 ModelMatrix = float4x4::RotationY(m_RotationAngle) * float4x4::RotationX(-PI_F * 0.1f);
    // Camera is at (0, 0, -m_Distance) looking along Z axis
    float4x4 ViewMatrix = float4x4::Translation(0.f, 0.0f, m_Distance);

    // Get pretransform matrix that rotates the scene according the surface orientation
    auto SrfPreTransform = GetSurfacePretransformMatrix(float3{0, 0, 1});

    // Get projection matrix adjusted to the current screen orientation
    auto Proj = GetAdjustedProjectionMatrix(PI_F / 4.0f, 0.1f, 1000.f);

    m_WorldViewMatrix = ModelMatrix * ViewMatrix * SrfPreTransform;

    // Compute world-view-projection matrix
    m_WorldViewProjMatrix = m_WorldViewMatrix * Proj;
}

} // namespace Diligent