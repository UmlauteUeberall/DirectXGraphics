#pragma once

enum CB_Type
{
	CB_APPLICATION,			// F�r Daten die sich nie �ndern, zb CameraToScreen-Matrix
	CB_FRAME,				// F�r Daten die sich einmal pro Frame �ndern, WorlfToCamera-Matrix
	CB_OBJECT,				// F�r Daten die sich bei jedem Objekt �ndern, ObjectToWorld-Matrix
	CB_LIGHT,				// F�r Lichtberechnung
	CB_TERRAIN,				// F�r Terrainbespa�ung, extra Daten f�r das Terrain
	
	NumConstantBuffers
};

struct SDirectXSettings
{
	// Device Swap Chain	- sorgt f�r Austausch des Bildes
	ID3D11Device* m_device = nullptr;						// Softwarerepr�sentation der Grafikkarte
	ID3D11DeviceContext* m_deviceContext = nullptr;			// Direkter zugriff auf die Grafikkarte
	IDXGISwapChain* m_swapChain = nullptr;

	// Render Target View - hier wird reingezeichnet
	ID3D11RenderTargetView* m_renderTargetView = nullptr;

	// Depth Stencil
	ID3D11Texture2D* m_depthStencilBuffer = nullptr;
	ID3D11DepthStencilView* m_depthStencilView = nullptr;
	ID3D11DepthStencilState* m_depthStencilState = nullptr;
	ID3D11RasterizerState* m_rasterrizerStateSolid = nullptr;
	ID3D11RasterizerState* m_rasterrizerStateWireframe = nullptr;

	ID3D11RasterizerState* m_currentRasterrizerState = nullptr;

	D3D11_VIEWPORT m_viewPort;
	ID3D11Buffer* m_constantBuffers[NumConstantBuffers];

	ID3D11VertexShader* m_simpleVertexShader;
	ID3D11InputLayout* m_simpleInputLayout;
	ID3D11PixelShader* m_simplePixelShader;

	ID3D11VertexShader* m_texturedVertexShader;
	ID3D11InputLayout* m_texturedInputLayout;
	ID3D11PixelShader* m_texturedPixelShader;

	ID3D11VertexShader* m_splatMapVertexShader;
	ID3D11InputLayout* m_splatMapInputLayout;
	ID3D11PixelShader* m_splatMapPixelShader;

	ID3D11VertexShader* m_osnmVertexShader;
	ID3D11InputLayout* m_osnmInputLayout;
	ID3D11PixelShader* m_osnmPixelShader;

	ID3D11VertexShader* m_triplanarVertexShader;
	ID3D11InputLayout* m_triplanarInputLayout;
	ID3D11PixelShader* m_triplanarPixelShader;

	ID3D11VertexShader* m_skyboxVertexShader;
	ID3D11InputLayout* m_skyboxInputLayout;
	ID3D11PixelShader* m_skyboxPixelShader;
};
