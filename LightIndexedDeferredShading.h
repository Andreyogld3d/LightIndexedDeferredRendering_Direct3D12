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

#pragma once

#include "DXSample.h"
#include "Camera.h"
#include <array>

#define USE_PLANE

using namespace DirectX;

// Note that while ComPtr is used to manage the lifetime of resources on the CPU,
// it has no understanding of the lifetime of resources on the GPU. Apps must account
// for the GPU lifetime of resources to avoid destroying objects that may still be
// referenced by the GPU.
// An example of this can be found in the class method: OnDestroy().
using Microsoft::WRL::ComPtr;

struct MeshData {
	ComPtr<ID3D12Resource> vb;
	ComPtr<ID3D12Resource> ib;
	D3D12_INDEX_BUFFER_VIEW ibView;
	D3D12_VERTEX_BUFFER_VIEW vbView;
	uint numFaces;
};

struct DescriptorSet {
	//std::array<>
};

class LightIndexedDeferredShading : public DXSample
{
public:
	static const uint maxLights = 255;
	LightIndexedDeferredShading(UINT width, UINT height, std::wstring name);

    virtual void OnInit();
    virtual void OnUpdate();
    virtual void OnRender();
    virtual void OnDestroy();

private:
	static const DXGI_FORMAT depthStencilFormat = DXGI_FORMAT_D32_FLOAT;
	//static const DXGI_FORMAT depthStencilFormat = DXGI_FORMAT_UNKNOWN;
    static const UINT FrameCount = 2;
    static const UINT TextureWidth = 256;
    static const UINT TextureHeight = 256;
    static const UINT TexturePixelSize = 4;    // The number of bytes used to represent a pixel in the texture.

    struct Vertex  {
        XMFLOAT3 position;
		XMFLOAT3 normal;
        XMFLOAT2 uv;
    };
	//
	struct CbData {
		Matrix4x4 m[2];
		Vector4D camPos;
	};
	// Light Description
	class Light {
	public:
		enum LightType {
			Point,
			Spot,
			Directional,
		};
		//
		Vector3D Position;
		//
		float Range;
		//
		Vector3D color;
		//
		LightType Type;
	};
	typedef std::array<Light, maxLights> Light_t;
	typedef std::array<Vector4D, maxLights> lightVector4Indices_t;
	typedef std::array<uint, maxLights> lightIndices_t;
	typedef std::array<D3D12_GPU_DESCRIPTOR_HANDLE, maxLights> D3D12_GPU_DESCRIPTOR_HANDLE_t;
	typedef std::array<Vector3D, maxLights> Vector3D_t;
	typedef lightVector4Indices_t lightPos_t;
	typedef std::array<float, maxLights> Float_t;

	struct LightInstanceData {
		Vector4D posRange;
		uint lightIndex;
	};
	struct CullingLightInfo {
		LightInstanceData instanceData;
		uint padding[3];
	};

	struct LightData {
		Matrix4x4 m;
		Vector4D lightData;
	};

	struct LightCullingData {
		// for lBufferCBVS
		D3D12_GPU_DESCRIPTOR_HANDLE_t lightCullingDescriptors;
		// output instance buffer
		ComPtr<ID3D12Resource> lightCullingInstanceBuffer;
		// output indirect buffer
		ComPtr<ID3D12Resource> lightCullingIndirectBuffer;
		//
		ComPtr<ID3D12Resource> lightCullingUpdateIndirectBuffer;
		// buffer with instance data
		ComPtr<ID3D12Resource> lightCullingDataBuffer;
		// ConstantBuffer for GPU Culling
		ComPtr<ID3D12Resource> lightCullingConstantBuffer;
		//
		D3D12_VERTEX_BUFFER_VIEW instanceVBView;
	};

	// 
	struct LightingData {
		//
		ComPtr<ID3D12PipelineState> lightSourcePipeline;
		ComPtr<ID3D12PipelineState> lightBufferPipeline;

		// GPU light culling
		ComPtr<ID3D12PipelineState> lightCullingPipeline;
		// RootSignature for GPU Clight culling
		ComPtr<ID3D12RootSignature> lightCullingRootSignature;
		// for execute indirect
		ComPtr<ID3D12CommandSignature> commandSignature;
		
		LightCullingData lightCullingData;

		LightCullingData lightCullingDataDebug;

		//
		ComPtr<ID3D12CommandQueue> computeCommandQueue;
		//
		ComPtr<ID3D12GraphicsCommandList> computeCommandList;
		//
		ComPtr<ID3D12CommandQueue> copyCommandQueue;
		//
		ComPtr<ID3D12GraphicsCommandList> copyCommandList;
		//
		ComPtr<ID3D12Fence> computeFence;
		//
		ComPtr<ID3D12Fence> copyFence;
		//
		HANDLE computeEvent;
		//
		HANDLE copyEvent;
		//
		UINT64 fence = 0;



		//
		ComPtr<ID3D12Resource> lightBufferRT;
		//
		D3D12_GPU_DESCRIPTOR_HANDLE lBufferRTDescriptor;
		//
		CD3DX12_CPU_DESCRIPTOR_HANDLE lBRTVHandle;
		//
		// constant buffer list of LightData 
		std::array<ComPtr<ID3D12Resource>, maxLights> lBufferCBVS;
		// constant buffer list of float4 light index
		std::array<ComPtr<ID3D12Resource>, maxLights> lBufferCBPS;
		// // constant buffer list of float4 light color
		std::array<ComPtr<ID3D12Resource>, maxLights> lSourceBufferCBVS;
		// for lBufferCBVS
		D3D12_GPU_DESCRIPTOR_HANDLE_t lBufferCBVSDescriptors;
		// for lBufferCBPS
		D3D12_GPU_DESCRIPTOR_HANDLE_t lBufferCBPSDescriptors;
		// for lSourceBufferCBVS
		D3D12_GPU_DESCRIPTOR_HANDLE_t lSourceCBVSDescriptors;
		// RootSignature for Light buffer pass
		ComPtr<ID3D12RootSignature> lightBufferRootSignature;
		// ConstantBuffer for all lights
		ComPtr<ID3D12Resource> lightBufferConstantBuffer;
		// for lightBufferConstantBuffer
		D3D12_GPU_DESCRIPTOR_HANDLE lightBufferDescriptor;
		//
		ComPtr<ID3D12GraphicsCommandList> rtCommandList;
		//
		Light_t lights;
		//
		Float_t lightsRanges;
		//
		lightPos_t lightsPositions;
		//
		Vector3D_t dirPosOffsets;
		//
		lightIndices_t lightIndices;
		//
		lightVector4Indices_t lightVector4Indices;
		//
		MeshData lightGeometryData;
	};
	CbData cbData;
    // Pipeline objects.
    CD3DX12_VIEWPORT m_viewport;
    CD3DX12_RECT m_scissorRect;
    ComPtr<IDXGISwapChain3> m_swapChain;
    ComPtr<ID3D12Device> m_device;
    ComPtr<ID3D12Resource> m_renderTargets[FrameCount];
	ComPtr<ID3D12Resource> m_depthStencil;
    ComPtr<ID3D12CommandAllocator> m_commandAllocator;
    ComPtr<ID3D12CommandQueue> m_commandQueue;
	ComPtr<ID3D12CommandAllocator> m_computeCommandAllocator;
	ComPtr<ID3D12CommandAllocator> m_copyCommandAllocator;
    ComPtr<ID3D12RootSignature> m_rootSignature;
	ComPtr<ID3D12RootSignature> m_depthRootSignature;
    ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
	ComPtr<ID3D12DescriptorHeap> m_dsvHeap;
    ComPtr<ID3D12DescriptorHeap> m_srvHeap;
    ComPtr<ID3D12PipelineState> m_pipelineState;
	ComPtr<ID3D12PipelineState> m_depthPipelineState;
    ComPtr<ID3D12GraphicsCommandList> m_commandList;
	D3D12_CPU_DESCRIPTOR_HANDLE m_cbDescriptors;
	ComPtr<ID3D12Resource> m_constantBuffer;
	D3D12_CPU_DESCRIPTOR_HANDLE m_srv_cbv_uav_descriptor;
	CD3DX12_CPU_DESCRIPTOR_HANDLE m_rtvHandle;
	CD3DX12_CPU_DESCRIPTOR_HANDLE m_dsvHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE m_GPUDescriptor;
	D3D12_GPU_DESCRIPTOR_HANDLE m_textureDescriptor;
	D3D12_GPU_DESCRIPTOR_HANDLE m_cBDescriptor;
	uint nFaces;
    UINT m_rtvDescriptorSize;

    // App resources.
    ComPtr<ID3D12Resource> m_vertexBuffer;
	ComPtr<ID3D12Resource> m_indexBuffer;
	D3D12_INDEX_BUFFER_VIEW ibView;
    D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;
    ComPtr<ID3D12Resource> m_texture;

    // Synchronization objects.
    UINT m_frameIndex;
    HANDLE m_fenceEvent;
    ComPtr<ID3D12Fence> m_fence;
    UINT64 m_fenceValue;

	Camera camera;
	Vector2D MousePosition = Vector2D::Zero();
	Vector2D DMousePosition = Vector2D::Zero();

	LightingData m_lightingData;
	
	void InitLightCullingData(LightCullingData& lightCullingData);
	ID3D12Resource* CreateConstantBuffer(size_t size);
	void GeneratePointLights(const Vector3D& start, const Vector3D& end, const Vector2D& RadiusRange);
	void InitGPULightCullng();
	void InitLightingSystem();
	void InitCamera();
    void LoadPipeline();
    void LoadAssets();
    std::vector<UINT8> GenerateTextureData();
	void drawToLightBuffer();
	void drawLightsSources();
	void CullLights(LightCullingData& lightCullingData, float R = 0.0f);
    void PopulateCommandList();
    void WaitForPreviousFrame();
	virtual void OnKeyDown(UINT8 /*key*/);
};
