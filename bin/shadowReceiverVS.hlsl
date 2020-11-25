//
cbuffer b0: register(b0) {
	row_major float4x4 ViewProjMatrix;
	row_major float4x4 ViewMatrix;
	float4 cameraPosition;
};

struct VS_OUTPUT {
	float4 position : SV_POSITION;
	float2 texCoord : TEXCOORD0;
	float3 worldPos : TEXCOORD1;
	float3 viewDir : TEXCOORD2;
	float3 normal : TEXCOORD3;
	float4 lightProjSpaceLokup : TEXCOORD4;
};

float4 CalcLightProjSpaceLookup(float4 projectSpace) 
{	
	//projectSpace.xy = (projectSpace.xy + projectSpace.w * 0.5);
#if 0
 	projectSpace.x =  projectSpace.x / projectSpace.w / 2.0f + 0.5f;
    projectSpace.y = -projectSpace.y / projectSpace.w / 2.0f + 0.5f;
#else
	//projectSpace.xy = (projectSpace.xy + projectSpace.w) * 0.5f;
	//projectSpace.xy /= projectSpace.z;
	//projectSpace.y = 1.0f - projectSpace.y;
#endif
	return projectSpace;
	//projectSpace.xy = projectSpace.xy / projectSpace.z;
	//float2 uv = (projectSpace.xy + 1.0f) * float2(0.5f, -0.5f);
	//return uv;
}

VS_OUTPUT VSmain(float4 Pos: POSITION, float3 Normal: NORMAL, float2 texCoord: TEXCOORD0)
{
	VS_OUTPUT Out;
	Out.position = mul(float4(Pos.xyz, 1.0f), ViewProjMatrix);
	Out.worldPos = mul(float4(Pos.xyz, 1.0f), ViewMatrix).xyz;
	Out.texCoord = texCoord;
	Out.viewDir = cameraPosition.xyz - Pos.xyz;
	Out.normal =  mul(Normal, (float3x3)ViewMatrix);
	Out.lightProjSpaceLokup = CalcLightProjSpaceLookup(Out.position);
	return Out;
}
