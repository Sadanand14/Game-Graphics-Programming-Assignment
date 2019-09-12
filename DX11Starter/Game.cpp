#include "Game.h"
#include "Vertex.h"
#include "d3d11.h"
#include "Mesh.h"
#include "Camera.h"
#include <iostream>
#include "Lights.h"

#include <filesystem>
#include <sstream>
#include <vector>

// For the DirectX Math library
using namespace DirectX;
namespace fs = std::experimental::filesystem;

// --------------------------------------------------------
// Constructor
//
// DXCore (base class) constructor will set up underlying fields.
// DirectX itself, and our window, are not ready yet!
//
// hInstance - the application's OS-level handle (unique ID)
// --------------------------------------------------------

std::wstring stringStream2wstring(std::stringstream& strs);

Game::Game(HINSTANCE hInstance)
	: DXCore(
		hInstance,		// The application's handle
		"DirectX Game",	   	// Text for the window's title bar
		1280,			// Width of the window's client area
		720,			// Height of the window's client area
		true)			// Show extra stats (fps) in title bar?
{
	// Initialize fields
	camera = new Camera((float)width, (float)height);
	vertexBuffer = 0;
	indexBuffer = 0;
	vertexShader = 0;
	pixelShader = 0;
	
#if defined(DEBUG) || defined(_DEBUG)
	// Do we want a console window?  Probably only in debug mode
	CreateConsoleWindow(500, 120, 32, 120);
	printf("Console window created successfully.  Feel free to printf() here.\n");
#endif

}

// --------------------------------------------------------
// Destructor - Clean up anything our game has created:
//  - Release all DirectX objects created here
//  - Delete any objects to prevent memory leaks
// --------------------------------------------------------
Game::~Game()
{
	if (skyRS != nullptr) skyRS->Release();
	if (skyDS != nullptr)skyDS->Release();

	
	// Release any (and all!) DirectX objects
	// we've made in the Game class
	if (vertexBuffer!= nullptr) { vertexBuffer->Release(); }
	if (indexBuffer != nullptr) { indexBuffer->Release(); }
	if (WaterVertexBuffer != nullptr) WaterVertexBuffer->Release();
	if (WaterIndexBuffer != nullptr) WaterIndexBuffer->Release();

	for (auto& m : entityList) { delete m; }
	for (auto&& m : meshMap) { delete m.second; }
	for (auto&& m : texMap) { delete m.second; }
	// Delete our simple shader objects, which
	// will clean up their own internal DirectX stuff

	if (camera != nullptr) delete camera;

	if (pixelShader != nullptr) delete pixelShader;
	if (vertexShader != nullptr) delete vertexShader;
	if (SkyVS != nullptr) delete SkyVS;
	if (SkyPS != nullptr) delete SkyPS;
	if (waterShaderVS != nullptr) delete waterShaderVS;
	if (waterShaderPS != nullptr) delete waterShaderPS;

	if (material != nullptr) delete material;

	entityList.clear();
	meshMap.clear();
	texMap.clear();
}

// --------------------------------------------------------
// Called once per program, after DirectX and the window
// are initialized but before the game loop.
// --------------------------------------------------------
void Game::Init()
{
	// Helper methods for loading shaders, creating some basic
	// geometry to draw and some simple camera matrices.
	//  - You'll be expanding and/or replacing these later
	LoadShaders();
	LoadModelDirectory();
	LoadTextureDirectory();
	CreateWaterMesh();
	CreateMatrices();
	CreateBasicGeometry();
	AddLighting();

	D3D11_RASTERIZER_DESC rd = {};
	rd.CullMode = D3D11_CULL_FRONT;
	rd.FillMode = D3D11_FILL_SOLID;
	rd.DepthClipEnable = true;
	device->CreateRasterizerState(&rd, &skyRS);

	D3D11_DEPTH_STENCIL_DESC dd = {};
	dd.DepthEnable = true;
	dd.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	dd.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
	device->CreateDepthStencilState(&dd, &skyDS);
	
	// Tell the input assembler stage of the pipeline what kind of
	// geometric primitives (points, lines or triangles) we want to draw.  
	// Essentially: "What kind of shape should the GPU draw with our data?"
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void Game::AddLighting() 
{
	//intitalizing the directional light structure defined in game.h
	light1.AmbientColor.x = 0.1f;
	light1.AmbientColor.y = 0.1f;
	light1.AmbientColor.z = 0.1f;
	light1.AmbientColor.w = 1.0f;

	light1.DiffuseColor.x = 0.0f;
	light1.DiffuseColor.y = 0.0f;
	light1.DiffuseColor.z = 1.0f;
	light1.DiffuseColor.w = 1.0f;

	light1.Direction.x = 1.0f;
	light1.Direction.y = -1.0f;
	light1.Direction.z = 0.0f;

	light2.AmbientColor.x = 0.1f;
	light2.AmbientColor.y = 0.1f;
	light2.AmbientColor.z = 0.1f;
	light2.AmbientColor.w = 1.0f;

	light2.DiffuseColor.x = 1.0f;
	light2.DiffuseColor.y = 0.0f;
	light2.DiffuseColor.z = 0.0f;
	light2.DiffuseColor.w = 1.0f;

	light2.Direction.x = -1.0f;
	light2.Direction.y = 1.0f;
	light2.Direction.z = 0.0f;
}

void Game::LoadShaders()
{
	vertexShader = new SimpleVertexShader(device, context);
	vertexShader->LoadShaderFile(L"VertexShader.cso");

	pixelShader = new SimplePixelShader(device, context);
	pixelShader->LoadShaderFile(L"PixelShader.cso");

	SkyVS = new SimpleVertexShader(device, context);
	SkyVS->LoadShaderFile(L"SkyboxVS.cso");

	SkyPS = new SimplePixelShader(device, context);
	SkyPS->LoadShaderFile(L"SkyboxPS.cso");

	waterShaderVS = new SimpleVertexShader(device, context);
	waterShaderVS->LoadShaderFile(L"WaterShaderVS.cso");

	waterShaderPS = new SimplePixelShader(device, context);
	waterShaderPS->LoadShaderFile(L"WaterShaderPS.cso");
}

void Game::LoadModelDirectory() 
{
	std::stringstream ss;
	std::string s,path;
	std::string ModelPath = "Models";
	unsigned int strlength = ModelPath.length() + 1;
	for (const auto& entry : fs::directory_iterator(ModelPath)) 
	{
		ss << entry.path();
		s = ss.str();
		ss.str(std::string());
		ss.clear();
		path = s.substr(strlength);
		ss << ModelPath << "/" << path;
		meshMap[path.substr(0, path.find("."))] =new Mesh(ss.str().c_str(), device);
		ss.str(std::string());
		ss.clear();
	}

}

void Game::CreateWaterMesh() 
{
	WaterVertex Current;
	WaterVertex* vbw = new WaterVertex[400 * 400];
	for (unsigned int i = 0; i < 400; i++) 
	{	
		for (unsigned int j = 0; j < 400; j++) 
		{
			Current = WaterVertex();
			Current.Position = XMFLOAT3(i, 0, j);
			Current.Normal = XMFLOAT3(0, 1, 0);
			Current.UV = XMFLOAT2(0, 0);
			Current.Tangent = XMFLOAT3(0, 0, 0);
			vbw[i * 400 + j] = Current;
		}
	}

	UINT* ibw = new UINT[6*399*399];
	int index = 0;
	for (unsigned int i = 0; i < 399; i++)
	{
		for (unsigned int j = 0; j < 399; j++)
		{
			ibw[index++] = i*400 + j ;
			ibw[index++] = i*400 + (j+1);
			ibw[index++] = (i+1)*400 + j ;
			ibw[index++] = i * 400 + (j + 1);
			ibw[index++] = (i+1)*400 + (j+1);
			ibw[index++] = (i + 1) * 400 + j;
		}
	}

	
	D3D11_BUFFER_DESC vbd;
	vbd.Usage = D3D11_USAGE_IMMUTABLE;
	vbd.ByteWidth = sizeof(WaterVertex) * 400*400;
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.CPUAccessFlags = 0;
	vbd.MiscFlags = 0;
	vbd.StructureByteStride = 0;
	D3D11_SUBRESOURCE_DATA initialVertexData;
	initialVertexData.pSysMem = &vbw[0];
	device->CreateBuffer(&vbd, &initialVertexData, &WaterVertexBuffer);

	delete[] vbw;
	//creating buffer for the indices
	D3D11_BUFFER_DESC ibd;
	ibd.Usage = D3D11_USAGE_IMMUTABLE;
	ibd.ByteWidth = sizeof(UINT) * 6*399*399;
	ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibd.CPUAccessFlags = 0;
	ibd.MiscFlags = 0;
	ibd.StructureByteStride = 0;
	D3D11_SUBRESOURCE_DATA initialIndexData;
	initialIndexData.pSysMem = &ibw[0];
	device->CreateBuffer(&ibd, &initialIndexData, &WaterIndexBuffer);
	delete[] ibw;

	XMMATRIX trans = XMMatrixTranslation(0.0f, 0.0f, 0.0f);
	XMMATRIX rot = XMMatrixRotationRollPitchYaw(0.0f, 0.0f, 0.0f);
	XMMATRIX scale = XMMatrixScaling(1.0f, 1.0f, 1.0f);
	XMMATRIX waterMatrix = XMMatrixMultiply(XMMatrixMultiply(scale, rot), trans);
	XMStoreFloat4x4(&WaterMatrix, XMMatrixTranspose(waterMatrix));

}

void Game::LoadTextureDirectory() 
{
	std::stringstream ss;
	std::string s, path;
	std::wstring ws;
	std::string texturePath = "Textures";
	unsigned int strlength = texturePath.length()+1;
	for(const auto& entry : fs::directory_iterator(texturePath))
	{
		ss << entry.path();
		s = ss.str();
		ss.str(std::string());
		ss.clear();
		path = s.substr(strlength);
		ss << texturePath << "/" << path;
		texMap[path.substr(0, path.find("."))] = new Texture(stringStream2wstring(ss), device, context);
		ss.str(std::string());
		ss.clear();
	}
}


// --------------------------------------------------------
// Initializes the matrices necessary to represent our geometry's 
// transformations and our 3D camera
// --------------------------------------------------------
void Game::CreateMatrices()
{
	XMMATRIX W = XMMatrixIdentity();
	XMStoreFloat4x4(&worldMatrix, XMMatrixTranspose(W)); // Transpose for HLSL!


	XMVECTOR pos = XMVectorSet(0, 0, -5, 0);
	XMVECTOR dir = XMVectorSet(0, 0, 1, 0);
	XMVECTOR up = XMVectorSet(0, 1, 0, 0);
	XMMATRIX V = XMMatrixLookToLH(
		pos,     // The position of the "camera"
		dir,     // Direction the camera is looking
		up);     // "Up" direction in 3D space (prevents roll)
	XMStoreFloat4x4(&viewMatrix, XMMatrixTranspose(V)); // Transpose for HLSL!

	XMMATRIX P = XMMatrixPerspectiveFovLH(
		0.25f * 3.1415926535f,		// Field of View Angle
		(float)width / height,		// Aspect ratio
		0.1f,						// Near clip plane distance
		100.0f);					// Far clip plane distance
	XMStoreFloat4x4(&projectionMatrix, XMMatrixTranspose(P)); // Transpose for HLSL!
}


void Game::CreateBasicGeometry()
{
	material = new Materials(vertexShader, pixelShader);//had to create a dummy material so compiler wont throw an error

	XMMATRIX trans = XMMatrixTranslation(0.0f, 0.0f, 0.0f);
	XMMATRIX rot = XMMatrixRotationRollPitchYaw(0.0f, 0.0f, 0.0f);
	XMMATRIX scale = XMMatrixScaling(1.0f, 1.0f, 1.0f);
	
	entityList.push_back(new Entity(trans, rot, scale, "cube", material));

	trans = XMMatrixTranslation(2.0f, 0.0f, 0.0f);
	rot = XMMatrixRotationRollPitchYaw(0.0f, 0.0f, 0.0f);
	scale = XMMatrixScaling(0.5f, 0.5f, 0.5f);

	entityList.push_back(new Entity(trans, rot, scale, "sphere", material));

}


// --------------------------------------------------------
// Handle resizing DirectX "stuff" to match the new window size.
// For instance, updating our projection matrix's aspect ratio.
// --------------------------------------------------------
void Game::OnResize()
{
	// Handle base-level DX resize stuff
	DXCore::OnResize();

	// Update our projection matrix since the window size changed
	XMMATRIX P = XMMatrixPerspectiveFovLH(
		0.25f * 3.1415926535f,	// Field of View Angle
		(float)width / height,	// Aspect ratio
		0.1f,				  	// Near clip plane distance
		100.0f);			  	// Far clip plane distance
	XMStoreFloat4x4(&projectionMatrix, XMMatrixTranspose(P)); // Transpose for HLSL!
}

// --------------------------------------------------------
// Update your game here - user input, move objects, AI, etc.
// --------------------------------------------------------
void Game::Update(float deltaTime, float totalTime)
{
	XMMATRIX rot = XMMatrixRotationRollPitchYaw(0.0f,totalTime, totalTime);
	//entityList[0].SetRot(rot);

	rot = XMMatrixRotationRollPitchYaw(totalTime, 0.0f, totalTime);
	//entityList[1].SetRot(rot);
	
	camera->Update(deltaTime);
	// Quit if the escape key is pressed
	if (GetAsyncKeyState(VK_ESCAPE))
		Quit();
	
	camera->Update(deltaTime);
}

// --------------------------------------------------------
// Clear the screen, redraw everything, present to the user
// --------------------------------------------------------
void Game::Draw(float deltaTime, float totalTime)
{
	// Background color (Cornflower Blue in this case) for clearing
	const float color[4] = { 0.4f, 0.6f, 0.75f, 0.0f };

	// Clear the render target and depth buffer (erases what's on the screen)
	//  - Do this ONCE PER FRAME
	//  - At the beginning of Draw (before drawing *anything*)
	context->ClearRenderTargetView(backBufferRTV, color);
	context->ClearDepthStencilView(
		depthStencilView,
		D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL,
		1.0f,
		0);

	//Looped all the sequences for loading the worldmatrix as well as loading the index and vertex buffers to the 
	//GPU using a vector of entities.

	for (int i = 0; i < entityList.size(); i++) {
		XMStoreFloat4x4(&worldMatrix, XMMatrixTranspose(entityList[i]->GetWM()));
		vertexShader->SetMatrix4x4("world", worldMatrix);
		vertexShader->SetMatrix4x4("view", camera->GetView());
		vertexShader->SetMatrix4x4("projection", camera->GetProjection());
		pixelShader->SetData("Light1", &light1, sizeof(DirectionalLight));
		pixelShader->SetData("Light2", &light2, sizeof(DirectionalLight));
		pixelShader->SetShaderResourceView("Texture", texMap["crate"]->GetSRV());
		pixelShader->SetSamplerState("BasicSampler",Texture::m_sampler);
		vertexShader->CopyAllBufferData();
		pixelShader->CopyAllBufferData();
		vertexShader->SetShader();
		pixelShader->SetShader();
		UINT stride = sizeof(Vertex);
		UINT offset = 0;
		ID3D11Buffer *v1Buffer = meshMap[entityList[i]->GetTitle()]->GetVertexBuffer();
		ID3D11Buffer *i1Buffer = meshMap[entityList[i]->GetTitle()]->GetIndexBuffer();
		context->IASetVertexBuffers(0, 1, &v1Buffer, &stride, &offset);
		context->IASetIndexBuffer(i1Buffer, DXGI_FORMAT_R32_UINT, 0);
		int indicesCount1 = meshMap[entityList[i]->GetTitle()]->GetIndexCount();
		context->DrawIndexed(indicesCount1, 0, 0);
	}
	DrawWater();

	RenderSky();
	
	swapChain->Present(0, 0);
}


void Game::DrawWater()
{
	UINT stride = sizeof(WaterVertex);
	UINT offset = 0;
	context->IAGetVertexBuffers(0, 1, &WaterVertexBuffer, &stride, &offset);
	context->IASetIndexBuffer(WaterIndexBuffer, DXGI_FORMAT_R32_UINT, 0);

	waterShaderVS->SetMatrix4x4("world", WaterMatrix);
	waterShaderVS->SetMatrix4x4("view", camera->GetView());
	waterShaderVS->SetMatrix4x4("projection", camera->GetProjection());
	waterShaderVS->CopyAllBufferData();
	waterShaderVS->SetShader();

	waterShaderPS->CopyAllBufferData();
	waterShaderPS->SetShader();
	
	context->DrawIndexed(6 * 399 * 399, 0, 0);
}

void Game::RenderSky() 
{
	Mesh* skymesh = meshMap["cube"];
	ID3D11Buffer* vb = skymesh->GetVertexBuffer();
	ID3D11Buffer* ib = skymesh->GetIndexBuffer();

	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	context->IASetVertexBuffers(0, 1, &vb, &stride, &offset);
	context->IASetIndexBuffer(ib, DXGI_FORMAT_R32_UINT, 0);

	SkyVS->SetMatrix4x4("view", camera->GetView());
	SkyVS->SetMatrix4x4("projection", camera->GetProjection());
	SkyVS->CopyAllBufferData();
	SkyVS->SetShader();
	
	SkyPS->SetShaderResourceView("sky",texMap["SunnyCubeMap"]->GetSRV());
	SkyPS->SetSamplerState("BasicSampler", Texture::m_sampler);
	SkyPS->CopyAllBufferData();
	SkyPS->SetShader();

	context->RSSetState(skyRS);
	context->OMSetDepthStencilState(skyDS, 0);

	context->DrawIndexed(skymesh->GetIndexCount(), 0, 0);

	context->RSSetState(0);
	context->OMSetDepthStencilState(0, 0);
}


#pragma region Mouse Input

// --------------------------------------------------------
// Helper method for mouse clicking.  We get this information
// from the OS-level messages anyway, so these helpers have
// been created to provide basic mouse input if you want it.
// --------------------------------------------------------
void Game::OnMouseDown(WPARAM buttonState, int x, int y)
{
	// Add any custom code here...
	
	// Save the previous mouse position, so we have it for the future
	prevMousePos.x = x;
	prevMousePos.y = y;

	// Caputure the mouse so we keep getting mouse move
	// events even if the mouse leaves the window.  we'll be
	// releasing the capture once a mouse button is released
	SetCapture(hWnd);
}

// --------------------------------------------------------
// Helper method for mouse release
// --------------------------------------------------------
void Game::OnMouseUp(WPARAM buttonState, int x, int y)
{
	// Add any custom code here...

	// We don't care about the tracking the cursor outside
	// the window anymore (we're not dragging if the mouse is up)
	ReleaseCapture();
}

// --------------------------------------------------------
// Helper method for mouse movement.  We only get this message
// if the mouse is currently over the window, or if we're 
// currently capturing the mouse.
// --------------------------------------------------------
void Game::OnMouseMove(WPARAM buttonState, int x, int y)
{
	// Add any custom code here...
	if (buttonState & 0x0001)
	{
		float xDiff = (x - prevMousePos.x) * 0.005f;
		float yDiff = (y - prevMousePos.y) * 0.005f;
		camera->Rotate(yDiff, xDiff);
	}
	// Save the previous mouse position, so we have it for the future
	prevMousePos.x = x;
	prevMousePos.y = y;
}

// --------------------------------------------------------
// Helper method for mouse wheel scrolling.  
// WheelDelta may be positive or negative, depending 
// on the direction of the scroll
// --------------------------------------------------------
void Game::OnMouseWheel(float wheelDelta, int x, int y)
{
	// Add any custom code here...
}

std::wstring stringStream2wstring(std::stringstream& strs)
{
	std::string str = strs.str();
	typedef std::codecvt_utf8<wchar_t> convert_type;
	std::wstring_convert<convert_type, wchar_t> converter;
	return converter.from_bytes(str);
}
#pragma endregion