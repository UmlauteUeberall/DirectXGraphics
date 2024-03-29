#pragma once
#include "WindowSettings.h"
#include "DirectXSettings.h"
#include "ConstantBuffer.h"
#include "Entity.h"
#include "ContentManager.h"
#include "InputManager.h"
#include "AssetManager.h"

#define WDS (*(CGame::Get()->GetWindowSettings()))
#define DXS (*(CGame::Get()->GetDirectXSettings()))
#define CTM (*(CGame::Get()->GetContentManager()))
#define IPM (*(CGame::Get()->GetInputManager()))
#define ASM (*(CGame::Get()->GetAssetManager()))

#define FAILHR(errorcode) if (FAILED(hr)) { return errorcode; }

class CGame
{
private:
	CGame();
public:
	~CGame();


public:
	static CGame* Get()
	{
		static CGame* instance = new CGame();	// Diese Zeile wird nur beim ersten Mal ausgef�hrt
		return instance;						// Diese immer
	}

	// Beschreibt wie der Speicher allokiert werden soll, hier wird daf�r gesorgt 
	// dass die adresse durch 16 teilbar ist (Notwendig sp�ter f�r Kommunikation mit der Grafikkarte)
	static void* operator new(size_t _size)
	{
		return _aligned_malloc(_size, 16);
	}

	static void operator delete(void* _memory)
	{
		_aligned_free(_memory);
	}

private:
	SWindowSettings m_windowSettings;
	SDirectXSettings m_directXSettings;
	CContentManager m_contentManager;
	CInputManager m_inputManager;
	CAssetManager m_assetManager;

	XMFLOAT3 m_camPos;
	XMFLOAT3 m_camRot;

	SStandardConstantBuffer m_applicationConstantBuffer;
	SStandardConstantBuffer m_frameConstantBuffer;
	SLightConstantBuffer m_lightConstantBuffer;
	STerrainConstantBuffer m_terrainConstantBuffer;

	bool m_isRunning;

public:
	int Initialize(HINSTANCE _hInstance);
	int Run();
	void Finalize();

	inline SWindowSettings* GetWindowSettings() { return &m_windowSettings; }
	inline SDirectXSettings* GetDirectXSettings() { return &m_directXSettings; }
	inline CContentManager* GetContentManager() { return &m_contentManager; }
	inline CInputManager* GetInputManager() { return &m_inputManager; }
	inline CAssetManager* GetAssetManager() { return &m_assetManager; }


	void SwitchRasterizerState();

	const float MOVEMENT_SPEED = 4;
	const float ROTATION_SPEED = 4;

private:
	int InitApplication(HINSTANCE _hInstance);
	int InitDirectX();
	int InitConstantBuffers();
	int LoadLevel();

	int CreateSimpleShader();
	int CreateTexturedShader();
	int CreateSplatMapShader();
	int CreateOSNMShader();
	int CreateTriplanarShader();
	int CreateSkyboxShader();

	void ClearBackBuffer(const float _clearColor[4], float _clearDepth, UINT8 _clearStencil);

	void Update(float _deltaTime);
	void Render();


};

