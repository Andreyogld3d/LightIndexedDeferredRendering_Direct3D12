//*********************************************************
//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
//*********************************************************

#ifndef layout
#define layout(x)
#endif

#ifndef NUM_LIGHTS
#define NUM_LIGHTS 255
#endif

#define MULTIPLE_LIGHTS

struct PSInput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
	float3 worldPos : TEXCOORD1;
	float3 viewDir : TEXCOORD2;
	float3 normal : TEXCOORD3;
	float4 lightProjSpaceLokup : TEXCOORD4;
};


cbuffer b0: register(b0) {
	row_major float4x4 ModelViewProjection;
	row_major float4x4 ViewMatrix;
	float4 cameraPosition;
};

PSInput VSmain(in float4 position : POSITION, in float3 Normal : NORMAL, in float2 uv : TEXCOORD)
{
    PSInput result;
   result.position = mul(position, ModelViewProjection);
    result.uv = uv;
	result.worldPos = mul(float4(position.xyz, 1.0f), ViewMatrix).xyz;
	result.normal = Normal;
	result.viewDir = cameraPosition.xyz - position.xyz;
	result.lightProjSpaceLokup = result.position;
    return result;
}

struct PSInputDepth
{
	float4 position : SV_POSITION;
};

PSInputDepth VSMainDepth(in float4 position : POSITION)
{
	PSInputDepth result;
	result.position = mul(position, ModelViewProjection);
	return result;
}

Texture2D g_texture : register(t0);
SamplerState g_sampler : register(s0);

#ifdef MULTIPLE_LIGHTS
layout (binding = 4) Texture2D BitPlane : register(t1);
layout(binding = 4) SamplerState sBitPlane : register(s1);

struct Light {
	float4 posRange;
	float4 colorLightType;
};

layout(binding = 5) cbuffer b0: register(b0) {
	Light lights[NUM_LIGHTS];
};

float4 GetLightIndexImpl(Texture2D BitPlane, SamplerState sBitPlane, float2 uv) {
	float4 packedLight = BitPlane.Sample(sBitPlane, uv);
	float4 unpackConst = float4(4.0, 16.0, 64.0, 256.0) / 256.0;
	float4 floorValues = ceil(packedLight * 254.5);
	float4 lightIndex;
	[unroll]
	for(int i = 0; i < 4; i++) {
		packedLight = floorValues * 0.25;
		floorValues = floor(packedLight);
		float4 fracParts = packedLight - floorValues;
		lightIndex[i] = dot(fracParts, unpackConst);
	}
	return lightIndex;
}

#define GetLightIndex(tex, uv) GetLightIndexImpl(tex, s##tex, uv)

float4 CalculateLighting(float4 Color, float3 worldPos, float3 Normal, float3 viewDir, float4 lightIndex)
{
	float3 ambient_color = float3(0.0f, 0.0f, 0.0f);
    float3 color = float3(0.0f, 0.0f, 0.0f);
    
    float3 n = normalize(Normal);
    float3 v = normalize(viewDir);
// NO_LIGHT_BUFFER standart Forward lighting 
//#define NO_LIGHT_BUFFER 
#define ORDER_LIGHT_FIX
#ifndef NO_LIGHT_BUFFER
    for (int i = 0; i < 4; ++i)
    {                   
#ifdef ORDER_LIGHT_FIX
		float lIndex = ceil(NUM_LIGHTS - 256.0f * lightIndex[i]);
#else
		float lIndex = 255.0f * lightIndex[i];
#endif
		Light light = lights[int(lIndex)]; 
#else
    for (int i = 0; i < NUM_LIGHTS; ++i)
    {        		
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

#endif

float2 CalcLightProjSpaceLookup(float4 projectSpace) 
{	
	float2 uv = projectSpace.xy / projectSpace.w; // -projectSpace.w
	uv = uv * float2(0.5f, - 0.5f) + 0.5f;  
	return uv;
}

float4 psMain(PSInput input) : SV_TARGET
{
	float4 Color = g_texture.Sample(g_sampler, input.uv);
#ifdef MULTIPLE_LIGHTS
	float2 projectSpace = CalcLightProjSpaceLookup(input.lightProjSpaceLokup);

	float3 Normal = normalize(input.normal);
	float3 viewDir = normalize(input.viewDir);

	float4 lightIndex = GetLightIndex(BitPlane, projectSpace);
	float4 Albedo = CalculateLighting(Color, input.worldPos, Normal, viewDir, lightIndex);
	if (Albedo.x != 0.0f || Albedo.y != 0.0f || Albedo.z != 0.0f) {
		Color *= Albedo;
	}
#endif
    return Color;
}
