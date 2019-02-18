#include "Game.h"
#include "Vertex.h"
#include "d3d11.h"
#include "Mesh.h"
#include <vector>
#include "Camera.h"
#include <iostream>
#include "Lights.h"


// For the DirectX Math library
using namespace DirectX;

// --------------------------------------------------------
// Constructor
//
// DXCore (base class) constructor will set up underlying fields.
// DirectX itself, and our window, are not ready yet!
//
// hInstance - the application's OS-level handle (unique ID)
// --------------------------------------------------------
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
	entityList.clear();
	// Release any (and all!) DirectX objects
	// we've made in the Game class
	if (vertexBuffer) { vertexBuffer->Release(); }
	if (indexBuffer) { indexBuffer->Release(); }
	//delete mesh1, mesh2, mesh3, mesh4,entity1, entity2, entity3, entity4, entity5, &entityList;
	if (mesh1 != nullptr)
		delete mesh1;
	if (mesh2 != nullptr)
		delete mesh2;
	if (mesh3 != nullptr)
		delete mesh3;
	if (mesh4 != nullptr)
		delete mesh4;
	if (material != nullptr)
		delete material;
	// Delete our simple shader objects, which
	// will clean up their own internal DirectX stuff
	delete vertexShader;
	delete pixelShader;
	delete camera;
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
	CreateMatrices();
	CreateBasicGeometry();

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

	// Tell the input assembler stage of the pipeline what kind of
	// geometric primitives (points, lines or triangles) we want to draw.  
	// Essentially: "What kind of shape should the GPU draw with our data?"
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

// --------------------------------------------------------
// Loads shaders from compiled shader object (.cso) files using
// my SimpleShader wrapper for DirectX shader manipulation.
// - SimpleShader provides helpful methods for sending
//   data to individual variables on the GPU
// --------------------------------------------------------
void Game::LoadShaders()
{
	vertexShader = new SimpleVertexShader(device, context);
	vertexShader->LoadShaderFile(L"VertexShader.cso");

	pixelShader = new SimplePixelShader(device, context);
	pixelShader->LoadShaderFile(L"PixelShader.cso");
}



// --------------------------------------------------------
// Initializes the matrices necessary to represent our geometry's 
// transformations and our 3D camera
// --------------------------------------------------------
void Game::CreateMatrices()
{
	// Set up world matrix
	// - In an actual game, each object will need one of these and they should
	//    update when/if the object moves (every frame)
	// - You'll notice a "transpose" happening below, which is redundant for
	//    an identity matrix.  This is just to show that HLSL expects a different
	//    matrix (column major vs row major) than the DirectX Math library
	XMMATRIX W = XMMatrixIdentity();
	XMStoreFloat4x4(&worldMatrix, XMMatrixTranspose(W)); // Transpose for HLSL!

	// Create the View matrix
	// - In an actual game, recreate this matrix every time the camera 
	//    moves (potentially every frame)
	// - We're using the LOOK TO function, which takes the position of the
	//    camera and the direction vector along which to look (as well as "up")
	// - Another option is the LOOK AT function, to look towards a specific
	//    point in 3D space
	XMVECTOR pos = XMVectorSet(0, 0, -5, 0);
	XMVECTOR dir = XMVectorSet(0, 0, 1, 0);
	XMVECTOR up = XMVectorSet(0, 1, 0, 0);
	XMMATRIX V = XMMatrixLookToLH(
		pos,     // The position of the "camera"
		dir,     // Direction the camera is looking
		up);     // "Up" direction in 3D space (prevents roll)
	XMStoreFloat4x4(&viewMatrix, XMMatrixTranspose(V)); // Transpose for HLSL!

	// Create the Projection matrix
	// - This should match the window's aspect ratio, and also update anytime
	//    the window resizes (which is already happening in OnResize() below)
	XMMATRIX P = XMMatrixPerspectiveFovLH(
		0.25f * 3.1415926535f,		// Field of View Angle
		(float)width / height,		// Aspect ratio
		0.1f,						// Near clip plane distance
		100.0f);					// Far clip plane distance
	XMStoreFloat4x4(&projectionMatrix, XMMatrixTranspose(P)); // Transpose for HLSL!
}


// --------------------------------------------------------
// Creates the geometry we're going to draw - a single triangle for now
// --------------------------------------------------------

//Doing linear interpolation between

void Game::CreateBasicGeometry()
{
	// Create some temporary variables to represent colors
	// - Not necessary, just makes things more readable
	XMFLOAT4 red = XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f);
	XMFLOAT4 green = XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f);
	XMFLOAT4 blue = XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f);

	//Defined 3 separate meshes
	Vertex vertices1[] =
	{
		{XMFLOAT3(-1.0f, +1.0f, +0.0f),XMFLOAT3(0.0f,0.0f,-1.0f),XMFLOAT2(0.0f,0.0f)},//0
		{XMFLOAT3(+1.0f, 1.0f, +0.0f),XMFLOAT3(0.0f,0.0f,-1.0f),XMFLOAT2(0.0f,0.0f)},//1
		{XMFLOAT3(0.0f, 0.0f, +0.0f),XMFLOAT3(0.0f,0.0f,-1.0f),XMFLOAT2(0.0f,0.0f)},//2
		{XMFLOAT3(-1.0f, -1.0f, +0.0f),XMFLOAT3(0.0f,0.0f,-1.0f),XMFLOAT2(0.0f,0.0f)},//3
		{XMFLOAT3(1.0f, -1.0f, +0.0f),XMFLOAT3(0.0f,0.0f,-1.0f),XMFLOAT2(0.0f,0.0f)},//4
	};

	UINT indices1[] = {0,1,2,3,2,4,0,2,3,1,4,2};

	mesh1 = new Mesh(vertices1, indices1, 5, 12, device);

	Vertex vertices2[] =
	{
		{XMFLOAT3(-1.5f, +0.0f, +0.0f),XMFLOAT3(0.0f,0.0f,-1.0f),XMFLOAT2(0.0f,0.0f)},//0
		{XMFLOAT3(-1.0f, +1.0f, +0.0f),XMFLOAT3(0.0f,0.0f,-1.0f),XMFLOAT2(0.0f,0.0f)},//1
		{XMFLOAT3(-0.5f, +0.0f, +0.0f),XMFLOAT3(0.0f,0.0f,-1.0f),XMFLOAT2(0.0f,0.0f)},//2
		{XMFLOAT3(0.0f, -1.0f, +1.0f),XMFLOAT3(0.0f,0.0f,-1.0f),XMFLOAT2(0.0f,0.0f)},//3
		{XMFLOAT3(0.5f, 0.0f, 0.0f),XMFLOAT3(0.0f,0.0f,-1.0f),XMFLOAT2(0.0f,0.0f)},//4
		{XMFLOAT3(1.0f, +1.0f, +0.0f),XMFLOAT3(0.0f,0.0f,-1.0f),XMFLOAT2(0.0f,0.0f)},//5
		{XMFLOAT3(1.5f, 0.0f, +0.0f),XMFLOAT3(0.0f,0.0f,-1.0f),XMFLOAT2(0.0f,0.0f)},//6
	};

	UINT indices2[] = { 0,1,2,2,4,3,4,5,6 };

	mesh2 = new Mesh(vertices2, indices2, 7, 9, device);
	
	Vertex vertices3[] =
	{
		{XMFLOAT3(-1.0f, 0.0f, 0.0f),XMFLOAT3(0.0f,0.0f,-1.0f),XMFLOAT2(0.0f,0.0f)},//0
		{XMFLOAT3(1.0f, 0.0f, 0.0f),XMFLOAT3(0.0f,0.0f,-1.0f),XMFLOAT2(0.0f,0.0f)},//1
		{XMFLOAT3(0.0f, 3.0f, 0.0f),XMFLOAT3(0.0f,0.0f,-1.0f),XMFLOAT2(0.0f,0.0f)},//2
		{XMFLOAT3(0.0f, -3.0f, 0.0f),XMFLOAT3(0.0f,0.0f,-1.0f),XMFLOAT2(0.0f,0.0f)},//3
	};

	UINT indices3[] = { 0,2,1,1,3,0 };

	mesh3 = new Mesh(vertices3, indices3, 4, 6, device);

	material = new Materials(vertexShader, pixelShader);//had to create a dummy material so compiler wont throw an error

	XMMATRIX trans = XMMatrixTranslation(0.0f, 0.0f, 0.0f);
	XMMATRIX rot = XMMatrixRotationRollPitchYaw(0.0f, 0.0f, 0.0f);
	XMMATRIX scale = XMMatrixScaling(1.0f, 1.0f, 1.0f);
	
	mesh4 = new Mesh("helix.obj", device);
	entityList.push_back(Entity(trans, rot, scale, mesh4, material));
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
	entityList[0].SetRot(rot);
	camera->Update(deltaTime);
	// Quit if the escape key is pressed
	if (GetAsyncKeyState(VK_ESCAPE))
		Quit();
	
	if (GetAsyncKeyState('W')) 
	{
		camera->MoveForward();
	}
	if (GetAsyncKeyState('S'))
	{
		camera->MoveBackward();
	}
	if (GetAsyncKeyState('A')) 
	{
		camera->MoveLeft();
	}
	if (GetAsyncKeyState('D'))
	{
		camera->MoveRight();
	}
	if (GetAsyncKeyState(VK_SPACE))
	{
		camera->MoveUpward();
	}
	if (GetAsyncKeyState('X'))
	{
		camera->MoveDownward();
	}
	if (GetAsyncKeyState('V')) 
	{
		camera->RotateUp();
	}
	if (GetAsyncKeyState('C')) 
	{
		camera->RotateDown();
	}
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
		XMStoreFloat4x4(&worldMatrix, XMMatrixTranspose(entityList[i].GetWM()));
		vertexShader->SetMatrix4x4("world", worldMatrix);
		vertexShader->SetMatrix4x4("view", camera->GetView());
		vertexShader->SetMatrix4x4("projection", camera->GetProjection());
		pixelShader->SetData("Light1", &light1, sizeof(DirectionalLight));
		pixelShader->SetData("Light2", &light2, sizeof(DirectionalLight));
		vertexShader->CopyAllBufferData();
		pixelShader->CopyAllBufferData();
		vertexShader->SetShader();
		pixelShader->SetShader();
		UINT stride = sizeof(Vertex);
		UINT offset = 0;
		ID3D11Buffer *v1Buffer = entityList[i].GetMesh()->GetVertexBuffer();
		ID3D11Buffer *i1Buffer = entityList[i].GetMesh()->GetIndexBuffer();
		context->IASetVertexBuffers(0, 1, &v1Buffer, &stride, &offset);
		context->IASetIndexBuffer(i1Buffer, DXGI_FORMAT_R32_UINT, 0);
		int indicesCount1 = entityList[i].GetMesh()->GetIndexCount();
		context->DrawIndexed(indicesCount1, 0, 0);
	}

	swapChain->Present(0, 0);
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
	if (buttonState & 0x0001) 
	{
		camera->RotateLeft();
	}
	
	if (buttonState & 0x0002) 
	{
		camera->RotateRight();
	}
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
#pragma endregion