//
#define LIGHT
#define SIMPLE_POINT_LIGHT

#ifndef NUM_LIGHTS
#define NUM_LIGHTS 255
#endif

#define DeclTex2D(tex, r) Texture2D tex:register(t##r);SamplerState s##tex:register(s##r)
#define tex2D(tex, uv) tex.Sample(s##tex, uv)

#if 0
uniform sampler2D tex1 : register(s0);
uniform sampler2D tex2 : register(s1);
uniform sampler2D BitPlane : register(s2);
#else
DeclTex2D(tex1, 0);
DeclTex2D(BitPlane, 1);
#endif

struct Light {
	float4 posRange;
	float4 colorLightType;
};

uniform Light lights[NUM_LIGHTS];

struct VS_OUTPUT {
	float4 position : SV_POSITION;
	float2 texCoord : TEXCOORD0;
	float3 worldPos : TEXCOORD1;
	float3 viewDir : TEXCOORD2;
	float3 normal : TEXCOORD3;
	float4 lightProjSpaceLokup : TEXCOORD4;
};

float4 GetLightIndexImpl(Texture2D BitPlane, SamplerState sBitPlane, float2 uv) {
	float4 packedLight = tex2D(BitPlane, uv);
    // Set depending on the texture size
	const float4 unpackConst = float4(4.0f, 16.0f, 64.0f, 256.0f) / 256.0f;
    // Expand out to the 0..255 range (ceil to avoid precision errors)
	float4 floorValues = ceil(packedLight * 254.5f);
	float4 lightIndex;
	for(int i = 0; i < 4; i++) {
		packedLight = floorValues * 0.25f;
		floorValues = floor(packedLight);
		float4 fracParts = packedLight - floorValues;
		lightIndex[i] = dot(fracParts, unpackConst);
	}
	return 256.0f * lightIndex;//!!! Bug: original Demo 255
}

#define GetLightIndex(tex, uv) GetLightIndexImpl(tex, s##tex, uv)

float4 CalculateLighting(float4 Color, float3 worldPos, float3 Normal, float3 viewDir, float4 lightIndex)
{
	float3 ambient_color = float3(0.3f, 0.3f, 0.3f);
    float3 color = float3(0.0f, 0.0f, 0.0f);
    
    float3 n = normalize(Normal);
    float3 v = normalize(viewDir);
// NO_LIGHT_BUFFER standart Forward lighting 
//#define NO_LIGHT_BUFFER 
#ifndef NO_LIGHT_BUFFER
    for (int i = 0; i < 4; ++i)
#else
    for (int i = 0; i < 255; ++i)
#endif
    {                   
#ifndef NO_LIGHT_BUFFER
		Light light = lights[lightIndex[i]];
#else
		Light light = lights[i];
#endif
        float3 l = (light.posRange.xyz - worldPos) * light.posRange.w;
        float atten = saturate(1.0f - dot(l, l));
        l = normalize(l);
        float3 h = normalize(l + v);
        
        float nDotL = saturate(dot(n, l));
        float nDotH = saturate(dot(n, h));
        float power = (nDotL == 0.0f) ? 0.0f : pow(nDotH, 16.0f);
        
        color += (light.colorLightType.xyz * nDotL * atten) + power * atten;
    }
	color += ambient_color;
	return float4(color.xyz, Color.a);
}

float2 CalcLightProjSpaceLookup(float4 projectSpace) 
{	
	float2 uv = projectSpace.xy / projectSpace.w; // -projectSpace.w
	uv = uv * float2(0.5f, - 0.5f) + 0.5f;  
	return uv;
}


float4 psMain(in VS_OUTPUT In) : SV_TARGET 
{  
	float4 Color = tex2D(tex1, In.texCoord);

	float3 Normal = normalize(In.normal);
	float3 viewDir = normalize(In.viewDir);

	float2 projectSpace = CalcLightProjSpaceLookup(In.lightProjSpaceLokup);

	float4 lightIndex = GetLightIndex(BitPlane, projectSpace);
	float4 Albedo = CalculateLighting(Color, In.worldPos, Normal, viewDir, lightIndex);

	//Color.xyz += Albedo.xyz;
	return Color * Albedo + 0.01f;
}
