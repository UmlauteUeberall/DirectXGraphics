cbuffer Light : register(b0)
{
	float4 AmbientColor;
	float4 DiffuseColor;
	float4 SpecularColor;
	float3 LightDir;
	float filler;
	float3 CameraPos;
}

struct PixelShaderInput
{
	float4 pos : SV_POSITION;
	float3 normal : NORMAL;
	float4 color : COLOR;
	float4 posWorld : POSITION1;

};

float4 SimplePixelShader(PixelShaderInput _in) : SV_TARGET
{
	float3 ambient = _in.color.xyz * AmbientColor.xyz * AmbientColor.a;

	// saturate - clampt werte im Bereich 0-1
	// normalize - ver�ndert die L�nge eines Vektors auf 1
	// cross - Kreuzprodukt float4 cross(float4 _a, float4 _b) gibt einen Vektor zur�ck im rechten Winkel auf beiden Inputs, 
	//				die L�nge ist die Fl�che des aufgespannten Parallelograms
	// dot - Punktprodukt float dot(float4 _a, float4 _b) gibt bei normalisierten Vektoren werte von -1 bis 1 aus, 
	//				beinhaltet den Innenwinkel

	float3 diffuse = saturate(_in.color.xyz 
								* DiffuseColor.xyz
								* dot(normalize(-LightDir), normalize(_in.normal)) 
								* DiffuseColor.a);

	// Hilfsvektor ist der durchschnitt aus Lichtrichtung und Punkt zu Kamera
	// dieser wird mit der normalen verglichen um bei �bereinstimmung viel zu gl�nzen
	float3 halfVector = normalize(normalize(CameraPos - _in.posWorld.xyz) + normalize(-LightDir));
	
	float dotP = dot(halfVector, normalize(_in.normal));
	dotP = pow(dotP, 100);

	float3 specular = saturate(SpecularColor.xyz * dotP * SpecularColor.w);

	//return float4(_in.normal,1);

	return float4(saturate(ambient + diffuse + specular), _in.color.a);
}