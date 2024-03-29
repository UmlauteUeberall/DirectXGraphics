Texture2D controllTex : register(t0);
Texture2D RTex : register(t1);
Texture2D GTex : register(t2);
Texture2D BTex : register(t3);
Texture2D ATex : register(t4);
Texture2D NormalMap : register(t5);

SamplerState controllSampler : register(s0);
SamplerState RSampler : register(s1);
SamplerState GSampler : register(s2);
SamplerState BSampler : register(s3);
SamplerState ASampler : register(s4);
SamplerState NormalSampler : register(s5);

cbuffer Light : register(b0)
{
	float4 AmbientColor;
	float4 DiffuseColor;
	float4 SpecularColor;
	float3 LightDir;
	float filler;
	float3 CameraPos;
}

cbuffer Terrain : register(b1)
{
	float4 terrainST;
}

struct PixelShaderInput
{
	float4 pos : SV_POSITION;
	float3 normal : NORMAL;
	float4 color : COLOR;
	float4 posWorld : POSITION1;
	float2 uv : TEXCOORD;
};

float4 SplatMapPixelShader(PixelShaderInput _in) : SV_TARGET
{
	float4 controllCol = controllTex.Sample(controllSampler, _in.uv);

	float3 weight = abs(NormalMap.Sample(NormalSampler, _in.uv * 2 * terrainST.xy + terrainST.zw));
	weight = normalize(pow(weight, 10));

	float3 colRxy = RTex.Sample(RSampler, _in.posWorld.xy * terrainST.xy * 8 + terrainST.zw * 23).rgb;
	float3 colRxz = RTex.Sample(RSampler, _in.posWorld.xz * terrainST.xy * 8 + terrainST.zw * 23).rgb;
	float3 colRyz = RTex.Sample(RSampler, _in.posWorld.yz * terrainST.xy * 8 + terrainST.zw * 23).rgb;
	float3 colR = colRxz * weight.y + colRxy * weight.z + colRyz * weight.x;
	
	float3 colGxy = GTex.Sample(GSampler, _in.posWorld.xy * terrainST.xy * 11 + terrainST.zw * 23).rgb;
	float3 colGxz = GTex.Sample(GSampler, _in.posWorld.xz * terrainST.xy * 11 + terrainST.zw * 23).rgb;
	float3 colGyz = GTex.Sample(GSampler, _in.posWorld.yz * terrainST.xy * 11 + terrainST.zw * 23).rgb;
	float3 colG = colGxz * weight.y + colGxy * weight.z + colGyz * weight.x;
	
	float3 colBxy = BTex.Sample(BSampler, _in.posWorld.xy * terrainST.xy * 17 + terrainST.zw * 23).rgb;
	float3 colBxz = BTex.Sample(BSampler, _in.posWorld.xz * terrainST.xy * 17 + terrainST.zw * 23).rgb;
	float3 colByz = BTex.Sample(BSampler, _in.posWorld.yz * terrainST.xy * 17 + terrainST.zw * 23).rgb;
	float3 colB = colBxz * weight.y + colBxy * weight.z + colByz * weight.x;
	
	float3 colAxy = ATex.Sample(ASampler, _in.posWorld.xy * terrainST.xy * 13 + terrainST.zw * 23).rgb;
	float3 colAxz = ATex.Sample(ASampler, _in.posWorld.xz * terrainST.xy * 13 + terrainST.zw * 23).rgb;
	float3 colAyz = ATex.Sample(ASampler, _in.posWorld.yz * terrainST.xy * 13 + terrainST.zw * 23).rgb;
	float3 colA = colAxz * weight.y + colAxy * weight.z + colAyz * weight.x;


	float3 col = _in.color * (colR * controllCol.r * controllCol.r +
								colG * controllCol.g * controllCol.g +
								colB * controllCol.b * controllCol.b/* +*/)
					/ (controllCol.r * controllCol.r + controllCol.g * controllCol.g+ controllCol.b * controllCol.b);
				//				colA * controllCol.a * controllCol.a) 
				//	/ (controllCol.r * controllCol.r+ controllCol.g * controllCol.g + controllCol.b * controllCol.b+ controllCol.a * controllCol.a);

	return float4(col, 1);

	float3 ambient = col.xyz * AmbientColor.xyz * AmbientColor.a;

	// saturate - clampt werte im Bereich 0-1
	// normalize - ver�ndert die L�nge eines Vektors auf 1
	// cross - Kreuzprodukt float4 cross(float4 _a, float4 _b) gibt einen Vektor zur�ck im rechten Winkel auf beiden Inputs, 
	//				die L�nge ist die Fl�che des aufgespannten Parallelograms
	// dot - Punktprodukt float dot(float4 _a, float4 _b) gibt bei normalisierten Vektoren werte von -1 bis 1 aus, 
	//				beinhaltet den Innenwinkel

	float3 diffuse = saturate(col.xyz
								* DiffuseColor.xyz
								* dot(normalize(-LightDir), normalize(_in.normal))
								* DiffuseColor.a);

	// Hilfsvektor ist der durchschnitt aus Lichtrichtung und Punkt zu Kamera
	// dieser wird mit der normalen verglichen um bei �bereinstimmung viel zu gl�nzen
	float3 halfVector = normalize(normalize(CameraPos - _in.posWorld.xyz) + normalize(-LightDir));

	float dotP = dot(halfVector, normalize(_in.normal));
	dotP = pow(dotP, 100);

	float3 specular = saturate(SpecularColor.xyz * dotP * SpecularColor.w);


	return float4(saturate(ambient + diffuse + specular), _in.color.a);
}