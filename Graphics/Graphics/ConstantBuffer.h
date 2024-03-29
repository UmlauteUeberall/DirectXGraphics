#pragma once

struct SStandardConstantBuffer
{
	XMMATRIX m_matrix;
};

struct SLightConstantBuffer
{
	XMFLOAT4 AmbientColor;

	XMFLOAT4 DiffuseColor;
	XMFLOAT4 SpecularColor;
	
	XMFLOAT3 LightDir;
	float LightDirPadding;

	XMFLOAT3 CameraPos;
	float CameraPosPadding;
};

struct STerrainConstantBuffer
{
	// S cale XY
	// T ransform ZW
	XMFLOAT4 m_TerrainST;
};