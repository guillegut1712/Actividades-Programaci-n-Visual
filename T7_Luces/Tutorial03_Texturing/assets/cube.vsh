cbuffer Constants
{
    float4x4 g_WorldViewProj;
    float4x4 g_World;         // Matriz de mundo para transformar normales
    float4x4 g_LightViewProj; // Matriz para proyección de sombras
};

cbuffer LightAttribs
{
    float4 g_LightDirection; // xyz - dirección, w - no usado
    float4 g_AmbientColor;   // Color ambiente
    float4 g_DiffuseColor;   // Color difuso
    float4 g_SpecularColor;  // Color especular
};

// Usar semantics tradicionales compatibles con vs_2_0
struct VSInput
{
    float3 Pos    : POSITION;     // Cambiado de ATTRIB0
    float2 UV     : TEXCOORD0;    // Cambiado de ATTRIB1
    float3 Normal : NORMAL;       // Cambiado de ATTRIB2
};

struct PSInput 
{ 
    float4 Pos        : SV_POSITION; 
    float2 UV         : TEXCOORD0;
    float3 Normal     : TEXCOORD1;
    float3 WorldPos   : TEXCOORD2;
    float4 LightSpacePos : TEXCOORD3;
};

void main(in  VSInput VSIn,
          out PSInput PSIn) 
{
    // Transformar posición al espacio de clip
    PSIn.Pos = mul(float4(VSIn.Pos, 1.0), g_WorldViewProj);
    
    // Pasar coordenadas UV
    PSIn.UV = VSIn.UV;
    
    // Transformar normal al espacio de mundo
    PSIn.Normal = normalize(mul(float4(VSIn.Normal, 0.0), g_World).xyz);
    
    // Calcular posición en espacio de mundo
    PSIn.WorldPos = mul(float4(VSIn.Pos, 1.0), g_World).xyz;
    
    // Calcular posición en espacio de luz para sombras
    PSIn.LightSpacePos = mul(float4(VSIn.Pos, 1.0), g_LightViewProj);
}