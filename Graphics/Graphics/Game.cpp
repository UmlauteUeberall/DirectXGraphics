#include "GraphicsPCH.h"
#include "Game.h"
#include "Cube.h"
#include "Oktaeder.h"
#include "Sphere.h"
#include "Helper.h"
#include "TexturedPlane.h"
#include "TexturedSphere.h"
#include "Button2D.h"
#include "Text2D.h"
#include "Terrain.h"
#include "OSNMCube.h"
#include "TriplanarOktaeder.h"
#include "TriplanarSphere.h"
#include "MorphCube.h"
#include "Skybox.h"

LRESULT CALLBACK WndProc(HWND _hwnd, UINT _message, WPARAM _wparam, LPARAM _lparam);

CGame::CGame()
{
}

CGame::~CGame()
{
}

int CGame::Initialize(HINSTANCE _hInstance)
{
	if (!XMVerifyCPUSupport())
	{
		MessageBox(nullptr, L"Failed to load DirectXMathLibrary", L"Error", MB_OK);
		return -1;
	}

	// Windows Fenster erstellen
	int returnValue = InitApplication(_hInstance);
	if (FAILED(returnValue))
	{
		MessageBox(nullptr, L"Failed to create Window", L"Error", MB_OK);
		return returnValue;
	}

	// DirectX initialisieren
	returnValue = InitDirectX();
	if (FAILED(returnValue))
	{
		MessageBox(nullptr, L"Failed to initialize DirectX", L"Error", MB_OK);
		return returnValue;
	}

	// DirectX initialisieren
	returnValue = InitConstantBuffers();
	if (FAILED(returnValue))
	{
		MessageBox(nullptr, L"Failed to initialize Constant Buffers", L"Error", MB_OK);
		return returnValue;
	}

	CTM.Initialize();

	returnValue = CreateSimpleShader();
	if (FAILED(returnValue))
	{
		MessageBox(nullptr, L"Could not create Simple shader", L"Error", MB_OK);
		return returnValue;
	}

	returnValue = CreateTexturedShader();
	if (FAILED(returnValue))
	{
		MessageBox(nullptr, L"Could not create Textured shader", L"Error", MB_OK);
		return returnValue;
	}

	returnValue = CreateSplatMapShader();
	if (FAILED(returnValue))
	{
		MessageBox(nullptr, L"Could not create SplatMap shader", L"Error", MB_OK);
		return returnValue;
	}

	returnValue = CreateOSNMShader();
	if (FAILED(returnValue))
	{
		MessageBox(nullptr, L"Could not create OSNM shader", L"Error", MB_OK);
		return returnValue;
	}

	returnValue = CreateTriplanarShader();
	if (FAILED(returnValue))
	{
		MessageBox(nullptr, L"Could not create Triplanar shader", L"Error", MB_OK);
		return returnValue;
	}

	returnValue = CreateSkyboxShader();
	if (FAILED(returnValue))
	{
		MessageBox(nullptr, L"Could not create Skybox shader", L"Error", MB_OK);
		return returnValue;
	}

	returnValue = m_inputManager.InitDirectInput(_hInstance);
	if (FAILED(returnValue))
	{
		MessageBox(nullptr, L"Could not create Direct Input", L"Error", MB_OK);
		return returnValue;
	}
	LoadLevel();

	m_isRunning = true;

	return 0;
}

int CGame::Run()
{
	MSG msg = { 0 };

	static DWORD prevTime = timeGetTime();
	static const float targetFrameRate = 30.0f;
	static const float maxTimeStep = 1.0f / targetFrameRate;
	static DWORD currentTime;
	float deltaTime;

	while (m_isRunning && msg.message != WM_QUIT)
	{
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			currentTime = timeGetTime();
			deltaTime = (currentTime - prevTime) * 0.001f;		// Angabe in Sekunden

			deltaTime = std::min<float>(deltaTime, maxTimeStep);
			prevTime = currentTime;

			Update(deltaTime);
			Render();
		}
	}

	return 0;
}

void CGame::Finalize()
{
	CTM.Finalize();
	ASM.CleanUp();
}

void CGame::SwitchRasterizerState()
{
	if (m_directXSettings.m_currentRasterrizerState == m_directXSettings.m_rasterrizerStateSolid)
	{
		m_directXSettings.m_currentRasterrizerState = m_directXSettings.m_rasterrizerStateWireframe;
	}
	else
	{
		m_directXSettings.m_currentRasterrizerState = m_directXSettings.m_rasterrizerStateSolid;

	}
}

int CGame::InitApplication(HINSTANCE _hInstance)
{
	WNDCLASSEX wndClass = { 0 };
	//ZeroMemory(&wndClass, sizeof(WNDCLASSEX)); // oldschool weg um Speicher zu leeren

	wndClass.cbSize = sizeof(WNDCLASSEX);
	wndClass.style = CS_HREDRAW | CS_VREDRAW;
	wndClass.lpfnWndProc = WndProc;
	wndClass.hInstance = _hInstance;
	wndClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wndClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 9);
	wndClass.lpszClassName = m_windowSettings.m_WindowClassName;

	if (!RegisterClassEx(&wndClass))
	{
		return -2;
	}

	RECT windowRect = { 0,0, m_windowSettings.m_WindowWidth, m_windowSettings.m_WindowHeigth };
	if (!m_windowSettings.m_Fullscreen)
	{
		AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, false);
	}

	m_windowSettings.m_WindowHandle = CreateWindowA(m_windowSettings.m_WindowClassNameShort,
		m_windowSettings.m_WindowName,
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		windowRect.right - windowRect.left,
		windowRect.bottom - windowRect.top,
		nullptr,
		nullptr,
		_hInstance,
		nullptr);

	if (!m_windowSettings.m_WindowHandle)
	{
		return -3;
	}

	ShowWindow(m_windowSettings.m_WindowHandle, 10);
	UpdateWindow(m_windowSettings.m_WindowHandle);

	return 0;
}

int CGame::InitDirectX()
{
	RECT clientRect;
	GetClientRect(m_windowSettings.m_WindowHandle, &clientRect);

	unsigned long clientWidth = clientRect.right - clientRect.left;
	unsigned long clientHeight = clientRect.bottom - clientRect.top;

	DXGI_SWAP_CHAIN_DESC swapChainDesc = { 0 };
	swapChainDesc.BufferCount = 1;
	swapChainDesc.BufferDesc.Width = clientWidth;
	swapChainDesc.BufferDesc.Height = clientHeight;
	swapChainDesc.BufferDesc.RefreshRate.Numerator = 0;
	swapChainDesc.BufferDesc.RefreshRate.Denominator = 0;
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.OutputWindow = m_windowSettings.m_WindowHandle;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SampleDesc.Quality = 0;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	swapChainDesc.Windowed = true;


	unsigned int createDeviceFlags = 0;

#if _DEBUG
	createDeviceFlags = D3D11_CREATE_DEVICE_DEBUG;
#endif

	D3D_FEATURE_LEVEL featureLevels[]
	{
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,
		D3D_FEATURE_LEVEL_9_3,
		D3D_FEATURE_LEVEL_9_2,
		D3D_FEATURE_LEVEL_9_1,
	};

	D3D_FEATURE_LEVEL featureLevel;
	HRESULT hr = D3D11CreateDeviceAndSwapChain(nullptr,
		D3D_DRIVER_TYPE_HARDWARE,
		nullptr,
		createDeviceFlags,
		featureLevels,
		_countof(featureLevels),
		D3D11_SDK_VERSION,
		&swapChainDesc,
		&m_directXSettings.m_swapChain,
		&m_directXSettings.m_device,
		&featureLevel,
		&m_directXSettings.m_deviceContext);

	if (FAILED(hr))
	{
		return -10;
	}

	ID3D11Texture2D* backbuffer;
	hr = m_directXSettings.m_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backbuffer);
	if (FAILED(hr))
	{
		return -11;
	}

	hr = m_directXSettings.m_device->CreateRenderTargetView(backbuffer, nullptr, &m_directXSettings.m_renderTargetView);
	if (FAILED(hr))
	{
		return -12;
	}

	SafeRelease(backbuffer);

	D3D11_TEXTURE2D_DESC depthStencilBufferDesc = { 0 };
	depthStencilBufferDesc.ArraySize = 1;
	depthStencilBufferDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	depthStencilBufferDesc.CPUAccessFlags = 0;
	depthStencilBufferDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;		// 24 bit Tiefe, 8 bit Stencil
	depthStencilBufferDesc.Height = clientHeight;
	depthStencilBufferDesc.Width = clientWidth;
	depthStencilBufferDesc.MipLevels = 1;
	depthStencilBufferDesc.SampleDesc.Count = 1;
	depthStencilBufferDesc.SampleDesc.Quality = 0;
	depthStencilBufferDesc.Usage = D3D11_USAGE_DEFAULT;

	hr = m_directXSettings.m_device->CreateTexture2D(&depthStencilBufferDesc,
		nullptr,
		&m_directXSettings.m_depthStencilBuffer);

	if (FAILED(hr))
	{
		return -13;
	}

	hr = m_directXSettings.m_device->CreateDepthStencilView(m_directXSettings.m_depthStencilBuffer,
		nullptr,
		&m_directXSettings.m_depthStencilView);

	if (FAILED(hr))
	{
		return -14;
	}

	D3D11_DEPTH_STENCIL_DESC depthStencilDesc = { 0 };
	depthStencilDesc.DepthEnable = true;
	depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS;		// Nahe Objekte nehmen, ferne wegwerfen
	depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	depthStencilDesc.StencilEnable = false;

	hr = m_directXSettings.m_device->CreateDepthStencilState(&depthStencilDesc, &m_directXSettings.m_depthStencilState);
	if (FAILED(hr))
	{
		return -15;
	}

	// Rasterizer Macht Vektor zu Pixel
	D3D11_RASTERIZER_DESC rasterdesc;
	ZeroMemory(&rasterdesc, sizeof(D3D11_RASTERIZER_DESC));
	rasterdesc.AntialiasedLineEnable = false;
	rasterdesc.FillMode = D3D11_FILL_SOLID;		// Komplette Dreiecke zeigen, rumspielen!
	rasterdesc.CullMode = D3D11_CULL_BACK;		// R�ckseiten wegschneiden
	rasterdesc.DepthBias = 0;
	rasterdesc.DepthBiasClamp = 0.0f;
	rasterdesc.DepthClipEnable = true;
	rasterdesc.FrontCounterClockwise = false;		// Dreiecke im Uhrzeigersinn zeigen nach vorne
	rasterdesc.MultisampleEnable = false;
	rasterdesc.ScissorEnable = false;
	rasterdesc.SlopeScaledDepthBias = 0.0f;

	hr = m_directXSettings.m_device->CreateRasterizerState(&rasterdesc, &m_directXSettings.m_rasterrizerStateSolid);
	if (FAILED(hr))
	{
		return -16;
	}

	rasterdesc.FillMode = D3D11_FILL_WIREFRAME;
	hr = m_directXSettings.m_device->CreateRasterizerState(&rasterdesc, &m_directXSettings.m_rasterrizerStateWireframe);
	if (FAILED(hr))
	{
		return -17;
	}
	m_directXSettings.m_currentRasterrizerState = m_directXSettings.m_rasterrizerStateSolid;

	m_directXSettings.m_viewPort.Width = clientWidth;
	m_directXSettings.m_viewPort.Height = clientHeight;
	m_directXSettings.m_viewPort.TopLeftX = 0.0f;
	m_directXSettings.m_viewPort.TopLeftY = 0.0f;
	m_directXSettings.m_viewPort.MinDepth = 0.0f;
	m_directXSettings.m_viewPort.MaxDepth = 1.0f;


	return 0;
}

int CGame::InitConstantBuffers()
{
	D3D11_BUFFER_DESC constantBuffer = { 0 };
	constantBuffer.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	constantBuffer.ByteWidth = sizeof(SStandardConstantBuffer);
	constantBuffer.Usage = D3D11_USAGE_DEFAULT;

	HRESULT hr = m_directXSettings.m_device->CreateBuffer(&constantBuffer, nullptr, &m_directXSettings.m_constantBuffers[CB_APPLICATION]);
	FAILHR(-40);
	hr = m_directXSettings.m_device->CreateBuffer(&constantBuffer, nullptr, &m_directXSettings.m_constantBuffers[CB_FRAME]);
	FAILHR(-41);
	hr = m_directXSettings.m_device->CreateBuffer(&constantBuffer, nullptr, &m_directXSettings.m_constantBuffers[CB_OBJECT]);
	FAILHR(-42);

	constantBuffer.ByteWidth = sizeof(SLightConstantBuffer);
	hr = m_directXSettings.m_device->CreateBuffer(&constantBuffer, nullptr, &m_directXSettings.m_constantBuffers[CB_LIGHT]);
	FAILHR(-43);

	constantBuffer.ByteWidth = sizeof(STerrainConstantBuffer);
	hr = m_directXSettings.m_device->CreateBuffer(&constantBuffer, nullptr, &m_directXSettings.m_constantBuffers[CB_TERRAIN]);
	FAILHR(-44);

	m_terrainConstantBuffer.m_TerrainST = XMFLOAT4(1, 1, 0, 0);

	RECT clientRect;
	GetClientRect(m_windowSettings.m_WindowHandle, &clientRect);
	float clientHeight = clientRect.bottom - clientRect.top;
	float clientWidth = clientRect.right - clientRect.left;


	m_applicationConstantBuffer.m_matrix = XMMatrixPerspectiveFovLH(
		XMConvertToRadians(60),
		clientWidth / clientHeight,
		0.1f,
		100.0f);

	m_directXSettings.m_deviceContext->UpdateSubresource(m_directXSettings.m_constantBuffers[CB_APPLICATION],
		0, nullptr, &m_applicationConstantBuffer, 0, 0);

	m_camPos = XMFLOAT3(0, 0, -5);
	//m_camPos = XMFLOAT3(0, 5, -5);
	//m_camRot = XMFLOAT3(45, 0, 0);


	return 0;
}

void ClickTest(CButton2D* _caller)
{
	CTM.RemoveEntity(_caller);
}

int CGame::LoadLevel()
{
	/*
	CTM.AddEntity(new CCube(XMFLOAT3(0, 0, 5)));

	for (int i = 5; i >= 0; i--)
	{
		CTM.AddEntity(new COktaeder(XMFLOAT4(1, 1, 1, 1), XMFLOAT3(i - 2, 0, 0)));

	}
	*/

	//CTM.AddEntity(new CSphere(XMFLOAT4(1,0,1, 1), 40, 60));
	CTM.AddEntity(new CTexturedSphere(L"world.png", 32, 24, XMFLOAT3(2, 0,2)));

	//CTM.AddEntity(new CTexturedPlane(L"DirectX-11-Rendering-Pipeline.png"));
	//for (int i = 0; i < 20; i++)
	//{
	//	CTM.AddEntity(new CTexturedSphere(L"world.png", 32, 24, XMFLOAT3(i - 9.5f, 0,0)));
	//}
	//CTM.AddEntity(new CButton2D(XMFLOAT2(0,0), L"Button.png", ClickTest));
	//CTM.AddEntity(new CText2D(L"Close Window", XMFLOAT2(40, 40)));

	CTM.AddEntity(new CTerrain(L"Controll.jpg",
		L"Grass.png",	// R
		L"Stone.png",	// G
		L"Sand.png",	// B
		L"Water.png",	// A
		L"Height.png",
		L"normalmap.png",
		100,
		100,
		XMFLOAT3(-2, -2, -2)));

	CTM.AddEntity(new COSNMCube(L"CubeTex.png", L"OSNMNormal.png", XMFLOAT3(4, 0, 0)));

	CTM.AddEntity(new CTriplanarOktaeder(L"brick-wall.jpg", XMFLOAT3(-4, 0, 0)));
	CTM.AddEntity(new CTriplanarSphere(L"brick-wall.jpg", 24, 24,XMFLOAT3(-4.5f, 0, 0)));
	CTM.AddEntity(new CTriplanarOktaeder(L"brick-wall.jpg", XMFLOAT3(-5, 0, 0)));
	CTM.AddEntity(new CTriplanarOktaeder(L"tiles.png", XMFLOAT3(0, 0, 0)));

	CTM.AddEntity(new CSkybox(L"skybox.png", L"skybox2.png"));
	CTM.AddEntity(new CMorphCube(10, XMFLOAT3(2, 0, 0)));

	return 0;
}

int CGame::CreateSimpleShader()
{
	ID3DBlob* shaderBlob;

#if _DEBUG
	LPCWSTR compiledShaderName = L"SimpleVertexShader_d.cso";
#else
	LPCWSTR compiledShaderName = L"SimpleVertexShader.cso";
#endif

	HRESULT hr = D3DReadFileToBlob(compiledShaderName, &shaderBlob);
	FAILHR(-50);

	hr = m_directXSettings.m_device->CreateVertexShader(shaderBlob->GetBufferPointer(),
		shaderBlob->GetBufferSize(), nullptr, &m_directXSettings.m_simpleVertexShader);
	FAILHR(-51);

	D3D11_INPUT_ELEMENT_DESC vertexLayoutDesc[] =
	{
		{
			"POSITION",						// Semantic - Identifikation im Shader
			0,								// Semantic index, falls es mehr als eins von diesem Typen vorhanden ist
			DXGI_FORMAT_R32G32B32_FLOAT,	// Float3
			0,								// Falls mehr als ein VertexShader vorhanden ist
			offsetof(SVertexPosColor, Position),
			D3D11_INPUT_PER_VERTEX_DATA,	// Werte einzeln f�r jeden Vertex nacheinander �bergeben
			0
		},
		{
			"NORMAL",						// Semantic - Identifikation im Shader
			0,								// Semantic index, falls es mehr als eins von diesem Typen vorhanden ist
			DXGI_FORMAT_R32G32B32_FLOAT,	// Float3
			0,								// Falls mehr als ein VertexShader vorhanden ist
			offsetof(SVertexPosColor, Normal),
			D3D11_INPUT_PER_VERTEX_DATA,	// Werte einzeln f�r jeden Vertex nacheinander �bergeben
			0
		},
		{
			"COLOR",						// Semantic - Identifikation im Shader
			0,								// Semantic index, falls es mehr als eins von diesem Typen vorhanden ist
			DXGI_FORMAT_R32G32B32A32_FLOAT,	// Float4
			0,								// Falls mehr als ein VertexShader vorhanden ist
			offsetof(SVertexPosColor, Color),
			D3D11_INPUT_PER_VERTEX_DATA,	// Werte einzeln f�r jeden Vertex nacheinander �bergeben
			0
		}
	};

	hr = m_directXSettings.m_device->CreateInputLayout(vertexLayoutDesc,
		_countof(vertexLayoutDesc),
		shaderBlob->GetBufferPointer(),
		shaderBlob->GetBufferSize(),
		&m_directXSettings.m_simpleInputLayout);
	FAILHR(-52);


#if _DEBUG
	compiledShaderName = L"SimplePixelShader_d.cso";
#else
	compiledShaderName = L"SimplePixelShader.cso";
#endif
	hr = D3DReadFileToBlob(compiledShaderName, &shaderBlob);
	FAILHR(-53);

	hr = m_directXSettings.m_device->CreatePixelShader(shaderBlob->GetBufferPointer(),
		shaderBlob->GetBufferSize(),
		nullptr,
		&m_directXSettings.m_simplePixelShader);
	FAILHR(-54);

	return 0;
}

int CGame::CreateTexturedShader()
{
	ID3DBlob* shaderBlob;

#if _DEBUG
	LPCWSTR compiledShaderName = L"TexturedVertexShader_d.cso";
#else
	LPCWSTR compiledShaderName = L"TexturedVertexShader.cso";
#endif

	HRESULT hr = D3DReadFileToBlob(compiledShaderName, &shaderBlob);
	FAILHR(-55);

	hr = m_directXSettings.m_device->CreateVertexShader(shaderBlob->GetBufferPointer(),
		shaderBlob->GetBufferSize(), nullptr, &m_directXSettings.m_texturedVertexShader);
	FAILHR(-56);

	D3D11_INPUT_ELEMENT_DESC vertexLayoutDesc[] =
	{
		{
			"POSITION",						// Semantic - Identifikation im Shader
			0,								// Semantic index, falls es mehr als eins von diesem Typen vorhanden ist
			DXGI_FORMAT_R32G32B32_FLOAT,	// Float3
			0,								// Falls mehr als ein VertexShader vorhanden ist
			offsetof(SVertexPosColor, Position),
			D3D11_INPUT_PER_VERTEX_DATA,	// Werte einzeln f�r jeden Vertex nacheinander �bergeben
			0
		},
		{
			"NORMAL",						// Semantic - Identifikation im Shader
			0,								// Semantic index, falls es mehr als eins von diesem Typen vorhanden ist
			DXGI_FORMAT_R32G32B32_FLOAT,	// Float3
			0,								// Falls mehr als ein VertexShader vorhanden ist
			offsetof(SVertexPosColor, Normal),
			D3D11_INPUT_PER_VERTEX_DATA,	// Werte einzeln f�r jeden Vertex nacheinander �bergeben
			0
		},
		{
			"COLOR",						// Semantic - Identifikation im Shader
			0,								// Semantic index, falls es mehr als eins von diesem Typen vorhanden ist
			DXGI_FORMAT_R32G32B32A32_FLOAT,	// Float4
			0,								// Falls mehr als ein VertexShader vorhanden ist
			offsetof(SVertexPosColor, Color),
			D3D11_INPUT_PER_VERTEX_DATA,	// Werte einzeln f�r jeden Vertex nacheinander �bergeben
			0
		},
		{
			"TEXCOORD",						// Semantic - Identifikation im Shader
			0,								// Semantic index, falls es mehr als eins von diesem Typen vorhanden ist
			DXGI_FORMAT_R32G32_FLOAT,		// Float2
			0,								// Falls mehr als ein VertexShader vorhanden ist
			offsetof(SVertexPosColor, UV),
			D3D11_INPUT_PER_VERTEX_DATA,	// Werte einzeln f�r jeden Vertex nacheinander �bergeben
			0
		}
	};

	hr = m_directXSettings.m_device->CreateInputLayout(vertexLayoutDesc,
		_countof(vertexLayoutDesc),
		shaderBlob->GetBufferPointer(),
		shaderBlob->GetBufferSize(),
		&m_directXSettings.m_texturedInputLayout);
	FAILHR(-57);


#if _DEBUG
	compiledShaderName = L"TexturedPixelShader_d.cso";
#else
	compiledShaderName = L"TexturedPixelShader.cso";
#endif
	hr = D3DReadFileToBlob(compiledShaderName, &shaderBlob);
	FAILHR(-58);

	hr = m_directXSettings.m_device->CreatePixelShader(shaderBlob->GetBufferPointer(),
		shaderBlob->GetBufferSize(),
		nullptr,
		&m_directXSettings.m_texturedPixelShader);
	FAILHR(-59);

	return 0;
}

int CGame::CreateSplatMapShader()
{
	ID3DBlob* shaderBlob;

#if _DEBUG
	LPCWSTR compiledShaderName = L"SplatMapVertexShader_d.cso";
#else
	LPCWSTR compiledShaderName = L"SplatMapVertexShader.cso";
#endif

	HRESULT hr = D3DReadFileToBlob(compiledShaderName, &shaderBlob);
	FAILHR(-60);

	hr = m_directXSettings.m_device->CreateVertexShader(shaderBlob->GetBufferPointer(),
		shaderBlob->GetBufferSize(), nullptr, &m_directXSettings.m_splatMapVertexShader);
	FAILHR(-61);

	D3D11_INPUT_ELEMENT_DESC vertexLayoutDesc[] =
	{
		{
			"POSITION",						// Semantic - Identifikation im Shader
			0,								// Semantic index, falls es mehr als eins von diesem Typen vorhanden ist
			DXGI_FORMAT_R32G32B32_FLOAT,	// Float3
			0,								// Falls mehr als ein VertexShader vorhanden ist
			offsetof(SVertexPosColor, Position),
			D3D11_INPUT_PER_VERTEX_DATA,	// Werte einzeln f�r jeden Vertex nacheinander �bergeben
			0
		},
		{
			"NORMAL",						// Semantic - Identifikation im Shader
			0,								// Semantic index, falls es mehr als eins von diesem Typen vorhanden ist
			DXGI_FORMAT_R32G32B32_FLOAT,	// Float3
			0,								// Falls mehr als ein VertexShader vorhanden ist
			offsetof(SVertexPosColor, Normal),
			D3D11_INPUT_PER_VERTEX_DATA,	// Werte einzeln f�r jeden Vertex nacheinander �bergeben
			0
		},
		{
			"COLOR",						// Semantic - Identifikation im Shader
			0,								// Semantic index, falls es mehr als eins von diesem Typen vorhanden ist
			DXGI_FORMAT_R32G32B32A32_FLOAT,	// Float4
			0,								// Falls mehr als ein VertexShader vorhanden ist
			offsetof(SVertexPosColor, Color),
			D3D11_INPUT_PER_VERTEX_DATA,	// Werte einzeln f�r jeden Vertex nacheinander �bergeben
			0
		},
		{
			"TEXCOORD",						// Semantic - Identifikation im Shader
			0,								// Semantic index, falls es mehr als eins von diesem Typen vorhanden ist
			DXGI_FORMAT_R32G32_FLOAT,		// Float2
			0,								// Falls mehr als ein VertexShader vorhanden ist
			offsetof(SVertexPosColor, UV),
			D3D11_INPUT_PER_VERTEX_DATA,	// Werte einzeln f�r jeden Vertex nacheinander �bergeben
			0
		}
	};

	hr = m_directXSettings.m_device->CreateInputLayout(vertexLayoutDesc,
		_countof(vertexLayoutDesc),
		shaderBlob->GetBufferPointer(),
		shaderBlob->GetBufferSize(),
		&m_directXSettings.m_splatMapInputLayout);
	FAILHR(-62);


#if _DEBUG
	compiledShaderName = L"SplatMapPixelShader_d.cso";
#else
	compiledShaderName = L"SplatMapPixelShader.cso";
#endif
	hr = D3DReadFileToBlob(compiledShaderName, &shaderBlob);
	FAILHR(-63);

	hr = m_directXSettings.m_device->CreatePixelShader(shaderBlob->GetBufferPointer(),
		shaderBlob->GetBufferSize(),
		nullptr,
		&m_directXSettings.m_splatMapPixelShader);
	FAILHR(-64);

	return 0;
}

int CGame::CreateOSNMShader()
{
	ID3DBlob* shaderBlob;

#if _DEBUG
	LPCWSTR compiledShaderName = L"OSNMVertexShader_d.cso";
#else
	LPCWSTR compiledShaderName = L"OSNMVertexShader.cso";
#endif

	HRESULT hr = D3DReadFileToBlob(compiledShaderName, &shaderBlob);
	FAILHR(-65);

	hr = m_directXSettings.m_device->CreateVertexShader(shaderBlob->GetBufferPointer(),
		shaderBlob->GetBufferSize(), nullptr, &m_directXSettings.m_osnmVertexShader);
	FAILHR(-66);

	D3D11_INPUT_ELEMENT_DESC vertexLayoutDesc[] =
	{
		{
			"POSITION",						// Semantic - Identifikation im Shader
			0,								// Semantic index, falls es mehr als eins von diesem Typen vorhanden ist
			DXGI_FORMAT_R32G32B32_FLOAT,	// Float3
			0,								// Falls mehr als ein VertexShader vorhanden ist
			offsetof(SVertexPosColor, Position),
			D3D11_INPUT_PER_VERTEX_DATA,	// Werte einzeln f�r jeden Vertex nacheinander �bergeben
			0
		},
		{
			"NORMAL",						// Semantic - Identifikation im Shader
			0,								// Semantic index, falls es mehr als eins von diesem Typen vorhanden ist
			DXGI_FORMAT_R32G32B32_FLOAT,	// Float3
			0,								// Falls mehr als ein VertexShader vorhanden ist
			offsetof(SVertexPosColor, Normal),
			D3D11_INPUT_PER_VERTEX_DATA,	// Werte einzeln f�r jeden Vertex nacheinander �bergeben
			0
		},
		{
			"COLOR",						// Semantic - Identifikation im Shader
			0,								// Semantic index, falls es mehr als eins von diesem Typen vorhanden ist
			DXGI_FORMAT_R32G32B32A32_FLOAT,	// Float4
			0,								// Falls mehr als ein VertexShader vorhanden ist
			offsetof(SVertexPosColor, Color),
			D3D11_INPUT_PER_VERTEX_DATA,	// Werte einzeln f�r jeden Vertex nacheinander �bergeben
			0
		},
		{
			"TEXCOORD",						// Semantic - Identifikation im Shader
			0,								// Semantic index, falls es mehr als eins von diesem Typen vorhanden ist
			DXGI_FORMAT_R32G32_FLOAT,		// Float2
			0,								// Falls mehr als ein VertexShader vorhanden ist
			offsetof(SVertexPosColor, UV),
			D3D11_INPUT_PER_VERTEX_DATA,	// Werte einzeln f�r jeden Vertex nacheinander �bergeben
			0
		}
	};

	hr = m_directXSettings.m_device->CreateInputLayout(vertexLayoutDesc,
		_countof(vertexLayoutDesc),
		shaderBlob->GetBufferPointer(),
		shaderBlob->GetBufferSize(),
		&m_directXSettings.m_osnmInputLayout);
	FAILHR(-67);


#if _DEBUG
	compiledShaderName = L"OSNMPixelShader_d.cso";
#else
	compiledShaderName = L"OSNMPixelShader.cso";
#endif
	hr = D3DReadFileToBlob(compiledShaderName, &shaderBlob);
	FAILHR(-68);

	hr = m_directXSettings.m_device->CreatePixelShader(shaderBlob->GetBufferPointer(),
		shaderBlob->GetBufferSize(),
		nullptr,
		&m_directXSettings.m_osnmPixelShader);
	FAILHR(-69);

	return 0;
}

int CGame::CreateTriplanarShader()
{
	ID3DBlob* shaderBlob;

#if _DEBUG
	LPCWSTR compiledShaderName = L"TriplanarVertexShader_d.cso";
#else
	LPCWSTR compiledShaderName = L"TriplanarVertexShader.cso";
#endif

	HRESULT hr = D3DReadFileToBlob(compiledShaderName, &shaderBlob);
	FAILHR(-70);

	hr = m_directXSettings.m_device->CreateVertexShader(shaderBlob->GetBufferPointer(),
		shaderBlob->GetBufferSize(), nullptr, &m_directXSettings.m_triplanarVertexShader);
	FAILHR(-71);

	D3D11_INPUT_ELEMENT_DESC vertexLayoutDesc[] =
	{
		{
			"POSITION",						// Semantic - Identifikation im Shader
			0,								// Semantic index, falls es mehr als eins von diesem Typen vorhanden ist
			DXGI_FORMAT_R32G32B32_FLOAT,	// Float3
			0,								// Falls mehr als ein VertexShader vorhanden ist
			offsetof(SVertexPosColor, Position),
			D3D11_INPUT_PER_VERTEX_DATA,	// Werte einzeln f�r jeden Vertex nacheinander �bergeben
			0
		},
		{
			"NORMAL",						// Semantic - Identifikation im Shader
			0,								// Semantic index, falls es mehr als eins von diesem Typen vorhanden ist
			DXGI_FORMAT_R32G32B32_FLOAT,	// Float3
			0,								// Falls mehr als ein VertexShader vorhanden ist
			offsetof(SVertexPosColor, Normal),
			D3D11_INPUT_PER_VERTEX_DATA,	// Werte einzeln f�r jeden Vertex nacheinander �bergeben
			0
		}
	};

	hr = m_directXSettings.m_device->CreateInputLayout(vertexLayoutDesc,
		_countof(vertexLayoutDesc),
		shaderBlob->GetBufferPointer(),
		shaderBlob->GetBufferSize(),
		&m_directXSettings.m_triplanarInputLayout);
	FAILHR(-72);


#if _DEBUG
	compiledShaderName = L"TriplanarPixelShader_d.cso";
#else
	compiledShaderName = L"TriplanarPixelShader.cso";
#endif
	hr = D3DReadFileToBlob(compiledShaderName, &shaderBlob);
	FAILHR(-73);

	hr = m_directXSettings.m_device->CreatePixelShader(shaderBlob->GetBufferPointer(),
		shaderBlob->GetBufferSize(),
		nullptr,
		&m_directXSettings.m_triplanarPixelShader);
	FAILHR(-74);

	return 0;
}

int CGame::CreateSkyboxShader()
{
	ID3DBlob* shaderBlob;

#if _DEBUG
	LPCWSTR compiledShaderName = L"SkyboxVertexShader_d.cso";
#else
	LPCWSTR compiledShaderName = L"SkyboxVertexShader.cso";
#endif

	HRESULT hr = D3DReadFileToBlob(compiledShaderName, &shaderBlob);
	FAILHR(-75);

	hr = m_directXSettings.m_device->CreateVertexShader(shaderBlob->GetBufferPointer(),
		shaderBlob->GetBufferSize(), nullptr, &m_directXSettings.m_skyboxVertexShader);
	FAILHR(-76);

	D3D11_INPUT_ELEMENT_DESC vertexLayoutDesc[] =
	{
		{
			"POSITION",						// Semantic - Identifikation im Shader
			0,								// Semantic index, falls es mehr als eins von diesem Typen vorhanden ist
			DXGI_FORMAT_R32G32B32_FLOAT,	// Float3
			0,								// Falls mehr als ein VertexShader vorhanden ist
			offsetof(SVertexPosColor, Position),
			D3D11_INPUT_PER_VERTEX_DATA,	// Werte einzeln f�r jeden Vertex nacheinander �bergeben
			0
		},
		{
			"COLOR",						// Semantic - Identifikation im Shader
			0,								// Semantic index, falls es mehr als eins von diesem Typen vorhanden ist
			DXGI_FORMAT_R32G32B32A32_FLOAT,	// Float4
			0,								// Falls mehr als ein VertexShader vorhanden ist
			offsetof(SVertexPosColor, Color),
			D3D11_INPUT_PER_VERTEX_DATA,	// Werte einzeln f�r jeden Vertex nacheinander �bergeben
			0
		},
		{
			"TEXCOORD",						// Semantic - Identifikation im Shader
			0,								// Semantic index, falls es mehr als eins von diesem Typen vorhanden ist
			DXGI_FORMAT_R32G32_FLOAT,		// Float2
			0,								// Falls mehr als ein VertexShader vorhanden ist
			offsetof(SVertexPosColor, UV),
			D3D11_INPUT_PER_VERTEX_DATA,	// Werte einzeln f�r jeden Vertex nacheinander �bergeben
			0
		}
	};

	hr = m_directXSettings.m_device->CreateInputLayout(vertexLayoutDesc,
		_countof(vertexLayoutDesc),
		shaderBlob->GetBufferPointer(),
		shaderBlob->GetBufferSize(),
		&m_directXSettings.m_skyboxInputLayout);
	FAILHR(-77);


#if _DEBUG
	compiledShaderName = L"SkyboxPixelShader_d.cso";
#else
	compiledShaderName = L"SkyboxPixelShader.cso";
#endif
	hr = D3DReadFileToBlob(compiledShaderName, &shaderBlob);
	FAILHR(-78);

	hr = m_directXSettings.m_device->CreatePixelShader(shaderBlob->GetBufferPointer(),
		shaderBlob->GetBufferSize(),
		nullptr,
		&m_directXSettings.m_skyboxPixelShader);
	FAILHR(-79);

	return 0;
}

void CGame::ClearBackBuffer(const float _clearColor[4], float _clearDepth, UINT8 _clearStencil)
{
	m_directXSettings.m_deviceContext->ClearRenderTargetView(m_directXSettings.m_renderTargetView,
		_clearColor);
	m_directXSettings.m_deviceContext->ClearDepthStencilView(m_directXSettings.m_depthStencilView,
		D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL,
		_clearDepth,
		_clearStencil);
}

void CGame::Update(float _deltaTime)
{
	m_inputManager.DetectInput();

	if (m_inputManager.GetKeyDown(DIK_ESCAPE))
	{
		m_isRunning = false;
	}

	if (m_inputManager.GetKeyDown(DIK_U))
	{
		SwitchRasterizerState();
	}

	XMVECTOR f = { 0, 0, 1,0 };
	XMVECTOR r = { 1, 0, 0,0 };
	XMVECTOR u = { 0, 1, 0,0 };

	XMMATRIX m = XMMatrixRotationRollPitchYaw(XMConvertToRadians(m_camRot.x),
		XMConvertToRadians(m_camRot.y),
		XMConvertToRadians(m_camRot.z));

	f = XMVector3Transform(f, m);
	r = XMVector3Transform(r, m);
	u = XMVector3Transform(u, m);

	XMFLOAT3 forward = XMFLOAT3(f.m128_f32[0], f.m128_f32[1], f.m128_f32[2]);
	XMFLOAT3 right = XMFLOAT3(r.m128_f32[0], r.m128_f32[1], r.m128_f32[2]);
	XMFLOAT3 up = XMFLOAT3(u.m128_f32[0], u.m128_f32[1], u.m128_f32[2]);

	XMFLOAT3 camMovement = XMFLOAT3(0, 0, 0);

	if (m_inputManager.GetKey(DIK_W))
	{
		camMovement = camMovement + forward;
	}
	if (m_inputManager.GetKey(DIK_S))
	{
		camMovement = camMovement - forward;
	}
	if (m_inputManager.GetKey(DIK_A))
	{
		camMovement = camMovement - right;
	}
	if (m_inputManager.GetKey(DIK_D))
	{
		camMovement = camMovement + right;
	}
	if (m_inputManager.GetKey(DIK_Q))
	{
		camMovement = camMovement - up;
	}
	if (m_inputManager.GetKey(DIK_E))
	{
		camMovement = camMovement + up;
	}

	m_camPos = m_camPos + (camMovement * _deltaTime * MOVEMENT_SPEED);

	if (m_inputManager.GetMouseKey(1))
	{
		XMFLOAT2 mouseDelta = m_inputManager.GetMouseMovement();
		m_camRot.x += mouseDelta.y * _deltaTime * ROTATION_SPEED;
		m_camRot.y += mouseDelta.x * _deltaTime * ROTATION_SPEED;
	}

	m_contentManager.Update(_deltaTime);

	//m_terrainConstantBuffer.m_TerrainST = XMFLOxAT4(1, 1,
	//	m_terrainConstantBuffer.m_TerrainST.z + _deltaTime * 0.01f,
	//	m_terrainConstantBuffer.m_TerrainST.w + _deltaTime * 0.02f);
}

void CGame::Render()
{
	// Hardware Check
	assert(m_directXSettings.m_device);
	assert(m_directXSettings.m_deviceContext);

	// Backbuffer clear
	ClearBackBuffer(Colors::Navy, 1.0f, 0);

	XMMATRIX rotation = XMMatrixRotationRollPitchYaw(XMConvertToRadians(m_camRot.x),
		XMConvertToRadians(m_camRot.y),
		XMConvertToRadians(m_camRot.z));

	XMMATRIX position = XMMatrixTranslation(m_camPos.x, m_camPos.y, m_camPos.z);

	m_frameConstantBuffer.m_matrix = XMMatrixInverse(nullptr, XMMatrixMultiply(rotation, position));

	m_directXSettings.m_deviceContext->UpdateSubresource(m_directXSettings.m_constantBuffers[CB_FRAME],
		0, nullptr, &m_frameConstantBuffer, 0, 0);

	m_lightConstantBuffer.AmbientColor = XMFLOAT4(0.1f, 0.1f, 0.1f, 1);
	m_lightConstantBuffer.DiffuseColor = XMFLOAT4(0.8f, 0.8f, 0.8f, 1);
	m_lightConstantBuffer.SpecularColor = XMFLOAT4(1.0f, 1.0f, 1.0f, 1);
	m_lightConstantBuffer.CameraPos = m_camPos;
	m_lightConstantBuffer.LightDir = XMFLOAT3(0.1f, -1.0f, 1.0f);

	m_directXSettings.m_deviceContext->UpdateSubresource(m_directXSettings.m_constantBuffers[CB_LIGHT],
		0, nullptr, &m_lightConstantBuffer, 0, 0);

	m_directXSettings.m_deviceContext->UpdateSubresource(m_directXSettings.m_constantBuffers[CB_TERRAIN],
		0, nullptr, &m_terrainConstantBuffer, 0, 0);

	m_contentManager.Render();

	m_directXSettings.m_swapChain->Present(1, 0);
}

LRESULT CALLBACK WndProc(HWND _hwnd, UINT _message, WPARAM _wparam, LPARAM _lparam)
{
	PAINTSTRUCT ps;
	HDC hdc;

	switch (_message)
	{
	case WM_PAINT:
		hdc = BeginPaint(_hwnd, &ps);
		EndPaint(_hwnd, &ps);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(_hwnd, _message, _wparam, _lparam);
	}
	return 0;
}
