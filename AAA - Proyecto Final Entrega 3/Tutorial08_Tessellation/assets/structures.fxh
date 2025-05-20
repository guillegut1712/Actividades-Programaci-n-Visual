struct TerrainVSOut
{
    float2 BlockOffset : BLOCK_OFFSET;
};

struct TerrainHSOut
{
    float2 BlockOffset : BLOCK_OFFSET;
};

struct TerrainHSConstFuncOut
{
    float Edges[4]  : SV_TessFactor;
    float Inside[2] : SV_InsideTessFactor;
};

struct TerrainDSOut
{
    float4 Pos : SV_Position;
    float3 WorldPos : POSITION;  // Posici�n en espacio de mundo para c�lculos de iluminaci�n
    float2 uv : TEX_COORD;
};

struct TerrainGSOut
{
    TerrainDSOut DSOut;
    float3 DistToEdges : DIST_TO_EDGES;
};

struct GlobalConstants
{
    uint NumHorzBlocks; // Number of blocks along the horizontal edge
    uint NumVertBlocks; // Number of blocks along the horizontal edge
    float fNumHorzBlocks;
    float fNumVertBlocks;

    float fBlockSize;
    float LengthScale;
    float HeightScale;
    float LineWidth;

    float TessDensity;
    int AdaptiveTessellation;
    float ErosionLevel;     // Nivel actual de erosi�n 
    float Dummy;            // Para alineaci�n

    float4x4 WorldView;
    float4x4 WorldViewProj;
    float4x4 World;        // Matriz de mundo para c�lculos de iluminaci�n
    float4 ViewportSize;
};