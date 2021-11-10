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

#include "stdafx.h"
#include "LightIndexedDeferredRendering.h"
#include "Timer.h"

//#define GPU_CULLING

//#define USE_PERSPECTIVE_RIGHT_HANDLED

namespace {
	struct MeshParameters;

	INLINE static int Rand()
	{
		return rand();
	}
	template <typename T1, typename T2>
	INLINE static T1 Lerp(const T1& a, const T1& b, T2 s)
	{
		return a + static_cast<T1>(s * (b - a));
	}
	template <typename T>
	INLINE static T Rand(T Min, T Max)
	{
		float t = Rand() * (1.0f / RAND_MAX);
		return Lerp(Min, Max, t);
	}
	static const float coord = 200.0f;
#if defined(_DEBUG)
	// Enable better shader debugging with the graphics debugging tools.
	static const UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
	static const UINT compileFlags = 0;
#endif

	// Standart vertex(textured vertex with Normal)
	struct Vertex {
		//
		static const uint NormalOffset = sizeof(Vector3D);
		//
		static const uint TexCoordOffset = 2 * sizeof(Vector3D);
		// vertex position
		Vector3D position;
		// normal
		Vector3D normal;
		// tex coords
		Vector2D tvert;
	};
	Timer timer;
	float emulationTime = 0.1f;

	static const uint numLights = LightIndexedDeferredRendering::maxLights;

	void outError(ID3DBlob* ppErrorMsgs)
	{
		if (!ppErrorMsgs) {
			return;
		}
		OutputDebugStringA("Compilation error\n");
		OutputDebugStringA(static_cast<const char *>(ppErrorMsgs->GetBufferPointer()));
		OutputDebugStringA("\n");
		ppErrorMsgs->Release();
	}
	INLINE static void TransformCoord(Vector3D& v, const Matrix4x4& mat)
	{
		v = v * mat;
		v += mat.GetPosition();
	}
	void UpdateBuffer(ID3D12Resource* buffer, const void* data, size_t size)
	{
		void* pData = nullptr;
		CD3DX12_RANGE readRange(0, 0);        // We do not intend to read from this resource on the CPU.
		ThrowIfFailed(buffer->Map(0, &readRange, &pData));
		memcpy(pData, data, size);
		buffer->Unmap(0, nullptr);
	}
	void InitMeshData(ID3D12Device* pDevice, MeshData& meshData, const void* vertices, size_t vertexSize, const void* indices, size_t indicesSize, uint stride = static_cast<uint>(sizeof(Vector3D)))
	{
		ThrowIfFailed(pDevice->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(vertexSize),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&meshData.vb)));
		UpdateBuffer(meshData.vb.Get(), vertices, vertexSize);

		meshData.vbView.BufferLocation = meshData.vb->GetGPUVirtualAddress();
		meshData.vbView.StrideInBytes = stride;
		meshData.vbView.SizeInBytes = static_cast<UINT>(vertexSize);

		ThrowIfFailed(pDevice->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(indicesSize),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&meshData.ib)));
		UpdateBuffer(meshData.ib.Get(), indices, indicesSize);

		meshData.ibView.BufferLocation = meshData.ib->GetGPUVirtualAddress();
		meshData.ibView.SizeInBytes = static_cast<UINT>(indicesSize);
		meshData.ibView.Format = DXGI_FORMAT_R16_UINT;
	}
	void ResetCommandList(ID3D12GraphicsCommandList* commandList, ID3D12CommandAllocator* commandAllocator)
	{
		assert(commandList && "NULL Pointer");
		assert(commandAllocator && "NULL Pointer");
		// Command list allocators can only be reset when the associated 
		// command lists have finished execution on the GPU; apps should use 
		// fences to determine GPU execution progress.
		ThrowIfFailed(commandAllocator->Reset());
		// However, when ExecuteCommandList() is called on a particular command 
		// list, that command list can then be reset at any time and must be before 
		// re-recording.
		ThrowIfFailed(commandList->Reset(commandAllocator, NULL));
	}
	//
	void CreateSphere(ID3D12Device* pDevice, MeshData& meshData, uint numRings, uint numSectors, float Radius)
	{
		uint numTriangles = (numRings + 1) * numSectors * 2;
		//
		uint numVertex = (numRings + 1) * numSectors + 2;
		uint numVertices = numVertex;
		meshData.numFaces = numTriangles;

		assert(numVertex < USHRT_MAX && "Out Of Range");
		const uint indicesSize = numTriangles * 3 * sizeof(ushort);

		uint vertexFormatSize = sizeof(Vector3D);
		size_t vertexSize = numVertices * vertexFormatSize;
		size_t totalSize = vertexSize + indicesSize;
		std::vector<ubyte> bufferData;
		bufferData.resize(totalSize);
		ubyte* data = bufferData.data();

		Vector3D& pos = *reinterpret_cast<Vector3D *>(data);
		//
		// last index of last vertex
		uint last = numVertex - 1; // Soutch 
		const Vector3D firstPosition = Vector3D(0.0f, Radius, 0.0f);
		const Vector3D lastPosition = Vector3D(0.0f, -Radius, 0.0f);
		// North
		pos = firstPosition;
		Vector3D& lastPos = reinterpret_cast<Vector3D *>(data)[last];
		lastPos = lastPosition;
		const float PI = static_cast<float>(Pi);
		//
		float da = PI / (numRings + 2.0f);
		float db = 2.0f * PI / numSectors;
		float af = PI - da / 2.0f;
		float bf = 2.0f * PI - db / 2.0f;
		//
		uint n = 1;
		//
		for (float a = da; a < af; a += da) {
			// 
			float y = Radius * cosf(a);
			// 
			float xz = Radius * sinf(a);
			//
			float rRadius = 1.0f / Radius;
			// 
			for (float b = 0.0; b < bf; n++, b += db) {
				//
				float x = xz * sinf(b);
				float z = xz * cosf(b);
				size_t Index = n * vertexFormatSize;
				Vector3D& position = *reinterpret_cast<Vector3D *>(&data[Index]);
				assert(Index + vertexFormatSize < bufferData.size() && "Out Of Range");
				position = Vector3D(x, y, z);
			}
		}
		//
		struct Face {
			ushort i1;
			ushort i2;
			ushort i3;
		};
		Face* t = reinterpret_cast<Face *>(&bufferData[vertexSize]);
		memset(t, 0, indicesSize);
		// num sectors
		for (n = 0; n < numSectors; n += 3) {
			//
			t[n].i1 = 0;
			//
			t[n].i2 = n + 1;
			//
			t[n].i3 = n == numSectors - 1 ? 1 : n + 2;
			//
			t[numTriangles - numSectors + n].i1 = numVertex - 1;
			t[numTriangles - numSectors + n].i2 = numVertex - 2 - n;
			t[numTriangles - numSectors + n].i3 = numVertex - 2 - ((1 + n) % numSectors);
		}
		//
		int k = 1;
		// numSectors
		//S' n = numSectors;
		for (uint i = 0; i < numRings; i++, k += numSectors) {
			for (uint j = 0; j < numSectors; j++, n += 2) {
				//
				t[n].i1 = k + j;
				//
				t[n].i2 = k + numSectors + j;
				//
				t[n].i3 = k + numSectors + ((j + 1) % numSectors);
				//
				t[n + 1].i1 = t[n].i1;
				t[n + 1].i2 = t[n].i3;
				t[n + 1].i3 = k + ((j + 1) % numSectors);
			}
		}

		InitMeshData(pDevice, meshData, bufferData.data(), vertexSize, t, indicesSize);
	}
	//
	void CreatePlane(ID3D12Device* pDevice, MeshData& meshData, uint width, uint height, uint stepX, uint stepZ, const Vector3D& normal, float d)
	{
		assert(width && "Invalid Value");
		assert(height && "Invalid Value");
		//
		float tu = 1.0f / width;
		float tv = 1.0f / height;
		assert(stepX && "Invalid Value");
		assert(stepZ && "Invalid Value");

		assert(!(width % stepX) && "Invalid Value");
		assert(!(height % stepZ) && "Invalid Value");

		uint StepWidth = width / stepX + 1;
		uint StepHeight = height / stepZ + 1;
		// total vertexs
		uint numVertexs = StepWidth * StepHeight;
		// size of vertex data
		uint VertexDataSize = sizeof(Vertex) * numVertexs;
		uint numPolygons = 2 * (StepWidth - 1) * (StepHeight - 1);
		meshData.numFaces = numPolygons;
		uint NumIndices = numPolygons * 3;
		// size of index data
		uint IndexDataSize = NumIndices * sizeof(short);
		std::vector<ubyte> bufferData;
		size_t totalSize = IndexDataSize + VertexDataSize;
		bufferData.resize(totalSize);
		ubyte* data = bufferData.data();
		Vertex* vertexs = reinterpret_cast<Vertex *>(data);
		memset(vertexs, 0, VertexDataSize);
		ushort* indices = reinterpret_cast<ushort *>(data + VertexDataSize);
		assert(vertexs && "NULL Pointer");
		assert(indices && "NULL Pointer");
		uint vertexIndex = 0;
		uint numIndices = 0;
		// Fill Vertex and indices
		int halfHeight = height >> 1;
		int halfWidth = width >> 1;
		for (int i = -halfHeight; i <= halfHeight; i += stepX) {
			for (int j = -halfWidth; j <= halfWidth; j += stepZ) {
				assert(vertexIndex < numVertexs && "Out Of Range");
				Vertex& v = vertexs[vertexIndex];
				v.position = Vector3D(static_cast<float>(i), 0.0f, static_cast<float>(j));
				v.tvert.x = tv * (i + halfHeight);
				v.tvert.y = tu * (j + halfWidth);
				vertexIndex++;
			}
		}
		uint vertexHOffset = 0;
		uint kOffset = height / stepZ + 1;
		vertexIndex = 0;
		//uint Step = (stepZ % 2) ? 0 : 1;
		for (uint i = 0; i < height; i += stepZ) {
			vertexIndex = vertexHOffset;
			for (uint j = 0; j < width; j += stepX) {

				assert(numIndices < NumIndices && "Out Of Range");
				indices[numIndices] = vertexIndex;

				assert(numIndices + 1 < NumIndices && "Out Of Range");
				indices[numIndices + 1] = vertexIndex + StepHeight;

				assert(numIndices + 2 < NumIndices && "Out Of Range");
				indices[numIndices + 2] = vertexIndex + StepHeight + 1;

				assert(numIndices + 3 < NumIndices && "Out Of Range");
				indices[numIndices + 3] = vertexIndex + StepHeight + 1;

				assert(numIndices + 4 < NumIndices && "Out Of Range");
				indices[numIndices + 4] = vertexIndex + 1;

				assert(numIndices + 5 < NumIndices && "Out Of Range");
				indices[numIndices + 5] = vertexIndex;

				numIndices += 6;
				vertexIndex++;
			}
			vertexHOffset += kOffset;
		}
		//MemoryUtils::CheckMemory();
		numIndices = 3 * numPolygons;
		for (uint i = 0; i < numIndices; i += 3) {
			// 1 index
			ushort index0 = indices[i + 0];
			// 2 index
			ushort index1 = indices[i + 1];
			// 3 index
			ushort index2 = indices[i + 2];
			assert(index0 < numVertexs && "Out Of Range");
			Vertex& V0 = vertexs[index0];
			assert(index1 < numVertexs && "Out Of Range");
			Vertex& V1 = vertexs[index1];
			assert(index2 < numVertexs && "Out Of Range");
			Vertex& V2 = vertexs[index2];
			const Vector3D& normal = Vector3D::Y();
			V0.normal = normal;
			V1.normal = normal;
			V2.normal = normal;
		}
		InitMeshData(pDevice, meshData, bufferData.data(), VertexDataSize, indices, IndexDataSize, sizeof(Vertex));
		//for(size_t i = 0; i < numVertexs; ++i) {
		//	Vector3D& normal = vertexs[i].normal;
		//	if (normal.y < 0.0f) {
		//		normal.y = - normal.y;
		//	}
		//	//assert(vertices[i].normal.y >= 0.0f);
		//	normal.Normalize();
		//}
	}

}

LightIndexedDeferredRendering::LightIndexedDeferredRendering(UINT width, UINT height, std::wstring name) :
    DXSample(width, height, name),
    m_frameIndex(0),
    m_viewport(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height)),
    m_scissorRect(0, 0, static_cast<LONG>(width), static_cast<LONG>(height)),
    m_rtvDescriptorSize(0)
{
}

void PerspectiveMatrix(float* Matrix, float fov, float aspect, float zNear, float zFar)
{
	assert(fov > 0 && "Invalid Value");
	assert(aspect > 0 && "Invalid Value");
	assert(zFar > zNear && "Invalid Value");
	assert(zNear != zFar && "Invalid Value");

	float yScale = 1.0f / tanf(degrad * fov * 0.5f);
	Matrix[0] = yScale / aspect;
	Matrix[1] = 0.0f;
	Matrix[2] = 0.0f;
	Matrix[3] = 0.0f;

	Matrix[4] = 0.0f;
	Matrix[5] = yScale;
	Matrix[6] = 0.0f;
	Matrix[7] = 0.0f;

	Matrix[8] = 0.0f;
	Matrix[9] = 0.0f;
	Matrix[10] = zFar / (zNear - zFar);// new code change
	Matrix[11] = -1.0f;	 // new code change

	Matrix[12] = 0.0f;
	Matrix[13] = 0.0f;
	Matrix[14] = zNear * zFar / (zNear - zFar);
	Matrix[15] = 0.0f;
}

void LightIndexedDeferredRendering::InitCamera()
{
	camera.SetAspect(m_viewport.Width / m_viewport.Height);
	Matrix4x4& m = camera.GetProjectionMatrix();
#ifndef USE_PERSPECTIVE_RIGHT_HANDLED
	m.PerspectiveFovDirect3D(camera.GetFov(), camera.GetAspect(), camera.GetzNear(), camera.GetzFar());
#else
	PerspectiveMatrix(m, camera.GetFov(), camera.GetAspect(), camera.GetzNear(), camera.GetzFar());
#endif
	camera.SetPosition(Vector3D(0.0f, 200.0f, 0.0f));
	camera.EnableFly(true);
}

void LightIndexedDeferredRendering::OnInit()
{
	timer.Init();
	InitCamera();
    LoadPipeline();
    LoadAssets();
}

ID3D12Resource* LightIndexedDeferredRendering::CreateConstantBuffer(size_t size)
{
	ID3D12Resource* constantBuffer = nullptr;
	// CB size is required to be 256-byte aligned.
	size = (size + 255) & ~255;
	ThrowIfFailed(m_device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(size),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&constantBuffer)));

	// Describe and create a constant buffer view.
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.BufferLocation = constantBuffer->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = static_cast<UINT>(size);
	uint descriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_device->CreateConstantBufferView(&cbvDesc, m_srv_cbv_uav_descriptor);
	m_srv_cbv_uav_descriptor.ptr += descriptorSize;
	return constantBuffer;
}

void LightIndexedDeferredRendering::GeneratePointLights(const Vector3D& start, const Vector3D& end, const Vector2D& RadiusRange)
{
	srand(static_cast<uint>(time(nullptr)));
	auto& lights = m_lightingData.lights;
#if 0
	const Light lightsList[] = {
			{ {2, 10, 0 }, 20, {0.0, 1.0, 0.0 }, Light::Point },
			{ {0, 10, 0 }, 20, { 0.0, 1.0, 0.0 }, Light::Point },
			{ {-10, 10, 5 }, 20, { 1.0, 0.0, 0.0 }, Light::Point }
	};
	lights[0] = { {2, 10, 0 }, 20, { 0.0, 0.0, 1.0 }, Light::Point };
	lights[1] = { {0, 10, 0 }, 20, { 0.0, 1.0, 0.0 }, Light::Point };
	lights[2] = { {-10, 10, 5 }, 120, { 1.0, 0.0, 0.0 }, Light::Point };
	for (size_t i = 0; i < countof(lightsList); ++i) {
		Light& light = lights[i];
		light = lightsList[i];
	}
#else
	for (uint i = 0; i < m_lightingData.numLights; ++i) {
		Light& light = lights[i];
		light.Type = Light::Point;
		light.Range = Rand(RadiusRange.x, RadiusRange.y);
		light.Position.x = static_cast<float>(Rand(start.x, end.x));
		light.Position.y = static_cast<float>(Rand(start.y, end.y));
		light.Position.z = static_cast<float>(Rand(start.z, end.z));
		float r = Rand(0.0f, 1.0f);
		float g = Rand(0.0f, 1.0f);
		float b = Rand(0.0f, 1.0f);
		light.color = Vector3D(r, g, b);
	}
#endif
	const Vector3D off(5.0f, 5.0f, 0.0f);
	const Vector3D Xdir = off;
	const Vector3D Zdir = -off;
	for (uint i = 0; i < m_lightingData.numLights; ++i) {
		Light& light = lights[i];
		m_lightingData.lightsRanges[i] = light.Range;
		if (i % 2) {
			m_lightingData.dirPosOffsets[i] = Xdir;
		}
		else {
			m_lightingData.dirPosOffsets[i] = Zdir;
		}
		m_lightingData.lightsPositions[i] = Vector4D(light.Position.x, light.Position.y, light.Position.z, light.Range);
		light.Range = 1.0f / light.Range;
	}
}

void LightIndexedDeferredRendering::InitLightCullingData(LightCullingData& lightCullingData)
{
	uint instanceLightSize = sizeof(LightInstanceData) * m_lightingData.numLights;
	ThrowIfFailed(m_device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(instanceLightSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS),
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		nullptr,
		IID_PPV_ARGS(&lightCullingData.lightCullingInstanceBuffer)));

	lightCullingData.instanceVBView.BufferLocation = lightCullingData.lightCullingInstanceBuffer->GetGPUVirtualAddress();
	lightCullingData.instanceVBView.StrideInBytes = static_cast<UINT>(sizeof(LightInstanceData));
	lightCullingData.instanceVBView.SizeInBytes = instanceLightSize;

	UINT descriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	lightCullingData.lightCullingDescriptors[0] = m_GPUDescriptor; // Instance Buffer
	m_GPUDescriptor.ptr += descriptorSize;

	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc;

	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
	uavDesc.Buffer.FirstElement = 0;
	uavDesc.Buffer.CounterOffsetInBytes = 0;
	uavDesc.Format = DXGI_FORMAT_R32_TYPELESS;
	uavDesc.Buffer.NumElements = instanceLightSize / 4;
	uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_RAW;
	uavDesc.Buffer.StructureByteStride = 0;

	m_device->CreateUnorderedAccessView(lightCullingData.lightCullingInstanceBuffer.Get(), nullptr, &uavDesc, m_srv_cbv_uav_descriptor);
	m_srv_cbv_uav_descriptor.ptr += descriptorSize;

	ThrowIfFailed(m_device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(sizeof(D3D12_DRAW_INDEXED_ARGUMENTS), D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS),
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		nullptr,
		IID_PPV_ARGS(&lightCullingData.lightCullingIndirectBuffer)));

	lightCullingData.lightCullingDescriptors[1] = m_GPUDescriptor; // Indirect Buffer
	m_GPUDescriptor.ptr += descriptorSize;

	uavDesc.Buffer.NumElements = sizeof(D3D12_DRAW_INDEXED_ARGUMENTS) / 4;

	m_device->CreateUnorderedAccessView(lightCullingData.lightCullingIndirectBuffer.Get(), nullptr, &uavDesc, m_srv_cbv_uav_descriptor);
	m_srv_cbv_uav_descriptor.ptr += descriptorSize;

	ThrowIfFailed(m_device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(sizeof(D3D12_DRAW_INDEXED_ARGUMENTS)),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&lightCullingData.lightCullingUpdateIndirectBuffer)));

	ThrowIfFailed(m_device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(sizeof(CullingLightInfo) * m_lightingData.numLights),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&lightCullingData.lightCullingDataBuffer)));

	lightCullingData.lightCullingDescriptors[2] = m_GPUDescriptor; // InstanceData Buffers
	m_GPUDescriptor.ptr += descriptorSize;

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc;

	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	srvDesc.Buffer.FirstElement = 0;

	srvDesc.Format = DXGI_FORMAT_R32_TYPELESS;
	srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;
	srvDesc.Buffer.NumElements = instanceLightSize / 4;
	srvDesc.Buffer.StructureByteStride = 0;

	m_device->CreateShaderResourceView(lightCullingData.lightCullingDataBuffer.Get(), &srvDesc, m_srv_cbv_uav_descriptor);
	m_srv_cbv_uav_descriptor.ptr += descriptorSize;

	lightCullingData.lightCullingDescriptors[3] = m_GPUDescriptor; // CB
	lightCullingData.lightCullingConstantBuffer.Attach(CreateConstantBuffer(sizeof(Frustum)));
	m_GPUDescriptor.ptr += descriptorSize;
}

void LightIndexedDeferredRendering::InitGPULightCullng()
{
	ThrowIfFailed(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COMPUTE, IID_PPV_ARGS(&m_computeCommandAllocator)));
	ThrowIfFailed(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_COMPUTE, m_computeCommandAllocator.Get(), nullptr, IID_PPV_ARGS(&m_lightingData.computeCommandList)));
	ThrowIfFailed(m_lightingData.computeCommandList->Close());

	ThrowIfFailed(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COPY, IID_PPV_ARGS(&m_copyCommandAllocator)));
	ThrowIfFailed(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_COPY, m_copyCommandAllocator.Get(), nullptr, IID_PPV_ARGS(&m_lightingData.copyCommandList)));
	ThrowIfFailed(m_lightingData.copyCommandList->Close());

	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_COMPUTE;
	ThrowIfFailed(m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_lightingData.computeCommandQueue)));

	const char* lightCullingShaderCodeHLSL = R"(
		struct LightInstanceData {
			float4 InstanceData;
			uint lightIndex;
		};
		struct CullingLightInfo {
			float4 InstanceData;
			uint lightIndex;
			uint padding[3];
		};
		struct DrawIndexedIndirectCommand {
			uint indexCount;
			uint instanceCount;
			uint firstIndex;
			int  vertexOffset;
			uint firstInstance;
		};
		static const uint numPlanes = 6;
		struct Frustum {
			float4 planes[numPlanes];
		};
		RWByteAddressBuffer instanceVB : register(u0);
		RWByteAddressBuffer numObjects : register(u1);
		StructuredBuffer<CullingLightInfo> cullingLightInfo : register(t0);
		cbuffer CBuffer : register(b0) {
			Frustum frustum;
		};

		inline float SignedDistanceToPoint(const float4 plane, const float3 p) {
			return dot(plane.xyz, p) + plane.w;
		}

		inline bool SphereInFrustum(const float4 boundingSphere) {
		[unroll]
		bool res = true;
		for (uint i = 0; i < numPlanes; ++i) {
			res = SignedDistanceToPoint(frustum.planes[i], boundingSphere.xyz) > -boundingSphere.w;
			if (!res) {
				break;
			}
		}
		return res;
		}

		[numthreads(1, 1, 1)]
		void CullLights(uint3 DTid : SV_DispatchThreadID) {
			static const uint float4Size = 16;
			static const uint InstanceGPUDataSize = float4Size + 4;
			if (!SphereInFrustum(cullingLightInfo[DTid.x].InstanceData)) {
				return;
			}
			uint bufferIndex = 0;
			numObjects.InterlockedAdd(4u, 1u, bufferIndex);
			bufferIndex *= InstanceGPUDataSize;
			instanceVB.Store4(bufferIndex, asuint(cullingLightInfo[DTid.x].InstanceData));
			instanceVB.Store(bufferIndex + float4Size, cullingLightInfo[DTid.x].lightIndex);
		}
	)";
	ID3DBlob* ppErrorMsgs = nullptr;
	ID3DBlob* computeShader = nullptr;
	HRESULT hr = D3DCompile(lightCullingShaderCodeHLSL, strlen(lightCullingShaderCodeHLSL), nullptr, nullptr, nullptr, "CullLights", "cs_5_0", compileFlags, 0, &computeShader, &ppErrorMsgs);

	InitLightCullingData(m_lightingData.lightCullingData);
	InitLightCullingData(m_lightingData.lightCullingDataDebug);

	CD3DX12_ROOT_PARAMETER1 rootParameters[3];
	CD3DX12_DESCRIPTOR_RANGE1 ranges[4];
	ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);
	ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 1, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);
	rootParameters[0].InitAsDescriptorTable(2, &ranges[0], D3D12_SHADER_VISIBILITY_ALL);

	ranges[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);
	rootParameters[1].InitAsDescriptorTable(1, &ranges[2], D3D12_SHADER_VISIBILITY_ALL);

	ranges[3].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);
	rootParameters[2].InitAsDescriptorTable(1, &ranges[3], D3D12_SHADER_VISIBILITY_ALL);

	D3D12_ROOT_SIGNATURE_FLAGS flags = 	D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS | D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS | 
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS | D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;

	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
	rootSignatureDesc.Init_1_1(static_cast<uint>(std::size(rootParameters)), rootParameters, 0, nullptr, flags);

	ComPtr<ID3DBlob> signature;
	ComPtr<ID3DBlob> error;
	ThrowIfFailed(D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1_1, &signature, &error));
	ThrowIfFailed(m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_lightingData.lightCullingRootSignature)));

	D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {};
	desc.CS = { computeShader->GetBufferPointer(), computeShader->GetBufferSize()};
	desc.pRootSignature = m_lightingData.lightCullingRootSignature.Get();
	ThrowIfFailed(m_device->CreateComputePipelineState(&desc, IID_PPV_ARGS(&m_lightingData.lightCullingPipeline)));

	D3D12_INDIRECT_ARGUMENT_DESC indirectArgDesc = {};
	indirectArgDesc.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED;
	D3D12_COMMAND_SIGNATURE_DESC commandSignatureDesc;
	commandSignatureDesc.NodeMask = 0;
	commandSignatureDesc.pArgumentDescs = &indirectArgDesc;
	commandSignatureDesc.ByteStride = sizeof(D3D12_DRAW_INDEXED_ARGUMENTS);
	commandSignatureDesc.NumArgumentDescs = 1;
	ThrowIfFailed(m_device->CreateCommandSignature(&commandSignatureDesc, nullptr, IID_PPV_ARGS(&m_lightingData.commandSignature)));

	// Describe and create the command queue.
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_COPY;

	ThrowIfFailed(m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_lightingData.copyCommandQueue)));
	
	ThrowIfFailed(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_lightingData.computeFence)));
	m_lightingData.fence = 1;
	// Create an event handle to use for frame synchronization.
	m_lightingData.computeEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);

	ThrowIfFailed(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_lightingData.copyFence)));
	//m_lightingData.fence = 1;
	// Create an event handle to use for frame synchronization.
	m_lightingData.copyEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
}

void LightIndexedDeferredRendering::InitLightingSystem()
{
	UINT descriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	const D3D12_HEAP_PROPERTIES heapProperties = { D3D12_HEAP_TYPE_DEFAULT, D3D12_CPU_PAGE_PROPERTY_UNKNOWN, D3D12_MEMORY_POOL_UNKNOWN, 1, 1 };

	const D3D12_CLEAR_VALUE OptimizedClearValue_RGBA = {
		DXGI_FORMAT_R8G8B8A8_UNORM, {0.0f, 0.0f, 0.0f, 0.0f}
	};

	HRESULT hr = m_device->CreateCommittedResource(&heapProperties,
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM, static_cast<UINT>(m_viewport.Width),
			static_cast<UINT>(m_viewport.Height), 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET),
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		&OptimizedClearValue_RGBA,
		IID_PPV_ARGS(&m_lightingData.lightBufferRT));
	m_lightingData.lightBufferRT->SetName(L"LightBufferRT");

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	m_device->CreateShaderResourceView(m_lightingData.lightBufferRT.Get(), &srvDesc, m_srv_cbv_uav_descriptor);
	m_srv_cbv_uav_descriptor.ptr += descriptorSize;

	m_lightingData.lBufferRTDescriptor = m_GPUDescriptor;
	m_GPUDescriptor.ptr += descriptorSize;

	m_lightingData.lBRTVHandle = m_rtvHandle;
	m_device->CreateRenderTargetView(m_lightingData.lightBufferRT.Get(), nullptr, m_lightingData.lBRTVHandle);
	m_rtvHandle.Offset(1, m_rtvDescriptorSize);

	m_lightingData.lightBufferConstantBuffer.Attach(CreateConstantBuffer(255 * sizeof(Light)));
	m_lightingData.lightBufferDescriptor = m_GPUDescriptor;
	m_GPUDescriptor.ptr += descriptorSize;

	// Create the command list.
	ThrowIfFailed(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocator.Get(), nullptr, IID_PPV_ARGS(&m_lightingData.rtCommandList)));
	// Close the command list and execute it to begin the initial GPU setup.
	ThrowIfFailed(m_lightingData.rtCommandList->Close());

#ifdef	GPU_CULLING

	const char* hlslVS = R"(
	cbuffer CBuffer : register(b0) {
		row_major float4x4 ViewProjMatrix;
	};
	struct PS {
		float4 position : SV_POSITION;
		float4 color : TEXCOORD0;
	};
	PS vsMain(in float4 position : POSITION, in float4 v0 : TRANSFORM0, in float4 v1 : TRANSFORM1) {
		PS Out;
		Out.position = mul(float4(v0.xyz + position.xyz * v0.w, 1.0f), ViewProjMatrix);
		Out.color = v1;
		return Out;
	};)";

	const char* hlslPS = R"(
	struct PS {
		float4 position : SV_POSITION;
		float4 color : TEXCOORD0;
	};
	float4 psMain(in PS ps) : SV_TARGET {
		return ps.color;
	})";

#else
	const char* hlslVS =
		"cbuffer CBuffer : register(b0) {"
		"	row_major float4x4 ViewProjMatrix;"
		"	float4 LightData;"
		"};"
		"struct PS {"
		"	float4 position : SV_POSITION;"
		"};"
		"PS vsMain(in float4 position : POSITION) {"
		"PS Out;"
		"Out.position = mul(float4(LightData.xyz + position.xyz * LightData.w, 1.0f), ViewProjMatrix);"
		"return Out;"
		"}";

	//
	const char* hlslPS =
		"cbuffer CBuffer : register(b0) {"
		"	float4 LightIndex;"
		"};"
		"struct PS {"
		"	float4 position : SV_POSITION;"
		"};"
		"float4 psMain(in PS ps) : SV_TARGET {"
		"	return LightIndex;"
		"}";

#endif

	ID3DBlob* ppErrorMsgs = nullptr;
	ID3DBlob* vertexShader = nullptr;
	hr = D3DCompile(hlslVS, strlen(hlslVS), nullptr, nullptr, nullptr, "vsMain", "vs_5_0", compileFlags, 0, &vertexShader, &ppErrorMsgs);
	outError(ppErrorMsgs);
	ID3DBlob* pixelShader = nullptr;
	hr = D3DCompile(hlslPS, strlen(hlslPS), nullptr, nullptr, nullptr, "psMain", "ps_5_0", compileFlags, 0, &pixelShader, &ppErrorMsgs);
	outError(ppErrorMsgs);

	CD3DX12_ROOT_PARAMETER1 rootParameters[2];
	CD3DX12_DESCRIPTOR_RANGE1 ranges[2];
	ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);
	rootParameters[0].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_VERTEX);

	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
#ifdef	GPU_CULLING
	rootSignatureDesc.Init_1_1(1, rootParameters, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
#else
	ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);
	rootParameters[1].InitAsDescriptorTable(1, &ranges[1], D3D12_SHADER_VISIBILITY_PIXEL);

	rootSignatureDesc.Init_1_1(_countof(rootParameters), rootParameters, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
#endif
	ComPtr<ID3DBlob> signature;
	ComPtr<ID3DBlob> error;
	ThrowIfFailed(D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1_1, &signature, &error));
	ThrowIfFailed(m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_lightingData.lightBufferRootSignature)));
	
	// Define the vertex input layout.
	D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
#ifdef	GPU_CULLING
		{ "TRANSFORM", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 0, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
		{ "TRANSFORM", 1, DXGI_FORMAT_R8G8B8A8_UNORM, 1, sizeof(float) * 4, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 }
#endif
	};

	// Describe and create the graphics pipeline state object (PSO).
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
	psoDesc.pRootSignature = m_lightingData.lightBufferRootSignature.Get();
	psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader);
	psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader);
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState.DepthEnable = TRUE;
	psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.DSVFormat = depthStencilFormat;
	psoDesc.SampleDesc.Count = 1;
	ThrowIfFailed(m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_lightingData.lightSourcePipeline)));

	D3D12_RENDER_TARGET_BLEND_DESC& RenderTarget = psoDesc.BlendState.RenderTarget[0];
	RenderTarget.BlendEnable = TRUE;
	RenderTarget.SrcBlend = D3D12_BLEND_ONE;
	RenderTarget.DestBlend = D3D12_BLEND_BLEND_FACTOR;
	RenderTarget.SrcBlendAlpha = D3D12_BLEND_ONE;
	RenderTarget.DestBlendAlpha = D3D12_BLEND_BLEND_FACTOR;
	psoDesc.RasterizerState.FrontCounterClockwise = TRUE;
	psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_GREATER_EQUAL;
	ThrowIfFailed(m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_lightingData.lightBufferPipeline)));

	const float lCoords = 0.9f * coord;
	const float minR = m_lightingData.radiuseRange.x;
	const float maxR = m_lightingData.radiuseRange.y;
	GeneratePointLights(Vector3D(-lCoords, minR, -lCoords), Vector3D(lCoords, maxR / 2, lCoords), Vector2D(minR, maxR));

	for (int lightIndex = m_lightingData.numLights - 1; lightIndex >= 0; --lightIndex) {
		Vector4D& outColor = m_lightingData.lightVector4Indices[lightIndex];
		// Set the light index color 
		ubyte convertColor = static_cast<ubyte>(lightIndex);
		ubyte redBit = (convertColor & (0x3 << 0)) << 6;
		ubyte greenBit = (convertColor & (0x3 << 2)) << 4;
		ubyte blueBit = (convertColor & (0x3 << 4)) << 2;
		ubyte alphaBit = (convertColor & (0x3 << 6)) << 0;
		outColor = Vector4D(redBit, greenBit, blueBit, alphaBit);

		const float divisor = 255.0f;
		outColor /= divisor;

		m_lightingData.lBufferCBVS[lightIndex].Attach(CreateConstantBuffer(sizeof(Matrix4x4) + sizeof(Vector4D)));
		m_lightingData.lBufferCBPS[lightIndex].Attach(CreateConstantBuffer(sizeof(Vector4D)));

		m_lightingData.lBufferCBVSDescriptors[lightIndex] = m_GPUDescriptor;
		m_GPUDescriptor.ptr += descriptorSize;

		m_lightingData.lBufferCBPSDescriptors[lightIndex] = m_GPUDescriptor;
		m_GPUDescriptor.ptr += descriptorSize;

		m_lightingData.lSourceBufferCBVS[lightIndex].Attach(CreateConstantBuffer(sizeof(Matrix4x4) + sizeof(Vector4D)));
		m_lightingData.lSourceCBVSDescriptors[lightIndex] = m_GPUDescriptor;
		m_GPUDescriptor.ptr += descriptorSize;
	}

	CreateSphere(m_device.Get(), m_lightingData.lightGeometryData, 250, 20, 1.0f);

	InitGPULightCullng();
}

// Load the rendering pipeline dependencies.
void LightIndexedDeferredRendering::LoadPipeline()
{
    UINT dxgiFactoryFlags = 0;

	InitDebug();

    ComPtr<IDXGIFactory4> factory;
    ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&factory)));

    if (m_useWarpDevice)
    {
        ComPtr<IDXGIAdapter> warpAdapter;
        ThrowIfFailed(factory->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter)));

        ThrowIfFailed(D3D12CreateDevice(
            warpAdapter.Get(),
            D3D_FEATURE_LEVEL_11_0,
            IID_PPV_ARGS(&m_device)
            ));
    }
    else
    {
        ComPtr<IDXGIAdapter1> hardwareAdapter;
        GetHardwareAdapter(factory.Get(), &hardwareAdapter);

        ThrowIfFailed(D3D12CreateDevice(
            hardwareAdapter.Get(),
            D3D_FEATURE_LEVEL_11_0,
            IID_PPV_ARGS(&m_device)
            ));
    }

    // Describe and create the command queue.
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

    ThrowIfFailed(m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue)));

    // Describe and create the swap chain.
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.BufferCount = FrameCount;
    swapChainDesc.Width = m_width;
    swapChainDesc.Height = m_height;
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.SampleDesc.Count = 1;

    ComPtr<IDXGISwapChain1> swapChain;
    ThrowIfFailed(factory->CreateSwapChainForHwnd(
        m_commandQueue.Get(),        // Swap chain needs the queue so that it can force a flush on it.
        Win32Application::GetHwnd(),
        &swapChainDesc,
        nullptr,
        nullptr,
        &swapChain
        ));

    // This sample does not support fullscreen transitions.
    ThrowIfFailed(factory->MakeWindowAssociation(Win32Application::GetHwnd(), DXGI_MWA_NO_ALT_ENTER));

    ThrowIfFailed(swapChain.As(&m_swapChain));
    m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

    // Create descriptor heaps.
    {
        // Describe and create a render target view (RTV) descriptor heap.
        D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
        rtvHeapDesc.NumDescriptors = USHRT_MAX;
        rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        ThrowIfFailed(m_device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap)));

        // Describe and create a shader resource view (SRV) heap for the texture.
        D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
        srvHeapDesc.NumDescriptors = USHRT_MAX;
        srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        ThrowIfFailed(m_device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&m_srvHeap)));

        m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

		D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
		dsvHeapDesc.NumDescriptors = CHAR_MAX;
		dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
		ThrowIfFailed(m_device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_dsvHeap)));
    }

    // Create frame resources.
    {
		m_rtvHandle = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();

        // Create a RTV for each frame.
        for (UINT n = 0; n < FrameCount; n++)
        {
            ThrowIfFailed(m_swapChain->GetBuffer(n, IID_PPV_ARGS(&m_renderTargets[n])));
            m_device->CreateRenderTargetView(m_renderTargets[n].Get(), nullptr, m_rtvHandle);
			m_rtvHandle.Offset(1, m_rtvDescriptorSize);
        }
		if (depthStencilFormat != DXGI_FORMAT_UNKNOWN) {
			m_dsvHandle = m_dsvHeap->GetCPUDescriptorHandleForHeapStart();
			// Describe and create a Texture2D.
			D3D12_RESOURCE_DESC textureDesc = {};
			textureDesc.MipLevels = 1;
			textureDesc.Format = depthStencilFormat;
			textureDesc.Width = m_width;
			textureDesc.Height = m_height;
			textureDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
			textureDesc.DepthOrArraySize = 1;
			textureDesc.SampleDesc.Count = 1;
			textureDesc.SampleDesc.Quality = 0;
			textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

			D3D12_CLEAR_VALUE depthOptimizedClearValue = {};
			depthOptimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT;
			depthOptimizedClearValue.DepthStencil.Depth = 1.0f;
			depthOptimizedClearValue.DepthStencil.Stencil = 0;

			ThrowIfFailed(m_device->CreateCommittedResource(
				&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
				D3D12_HEAP_FLAG_NONE,
				&textureDesc,
				D3D12_RESOURCE_STATE_DEPTH_WRITE,
				&depthOptimizedClearValue,
				IID_PPV_ARGS(&m_depthStencil)));

			const D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilDesc = {
				depthStencilFormat,
				{ D3D12_DSV_DIMENSION_TEXTURE2D },
					D3D12_DSV_FLAG_NONE
			};

			m_device->CreateDepthStencilView(m_depthStencil.Get(), &depthStencilDesc, m_dsvHandle);
		}
    }

    ThrowIfFailed(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocator)));
}

// Load the sample assets.
void LightIndexedDeferredRendering::LoadAssets()
{
	const float minR = 20.0f;
	const float maxR = 30.0f;

	m_lightingData.numLights = numLights;
	m_lightingData.radiuseRange.x = minR;
	m_lightingData.radiuseRange.y = maxR;
	FILE* f = fopen("setup.cfg", "r");
	if (f) {
		int res = fscanf(f, "LightSourceRadiusRange %f %f\r\n", &m_lightingData.radiuseRange.x, &m_lightingData.radiuseRange.y);
		assert(res > 0 && "fscanf failed");
		//res = fscanf(f, "NumLightSources %u", &m_lightingData.numLights);
		//assert(res > 0 && "fscanf failed");
		fclose(f);
	}
	m_srv_cbv_uav_descriptor = m_srvHeap->GetCPUDescriptorHandleForHeapStart();
	m_GPUDescriptor = m_srvHeap->GetGPUDescriptorHandleForHeapStart();

	UINT descriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    // Create the root signature.
    {
        D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};

        // This is the highest version the sample supports. If CheckFeatureSupport succeeds, the HighestVersion returned will not be greater than this.
        featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;

        if (FAILED(m_device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
        {
            featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
        }

		m_constantBuffer.Attach(CreateConstantBuffer(256));
		m_cBDescriptor = m_GPUDescriptor;
		m_GPUDescriptor.ptr += descriptorSize;

        CD3DX12_ROOT_PARAMETER1 rootParameters[4];
		CD3DX12_DESCRIPTOR_RANGE1 ranges[4];
		ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);
		rootParameters[0].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_VERTEX);
		
		m_textureDescriptor = m_GPUDescriptor;
		m_GPUDescriptor.ptr += descriptorSize;

		ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);
		ranges[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);
		ranges[3].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);
		rootParameters[1].InitAsDescriptorTable(1, &ranges[1], D3D12_SHADER_VISIBILITY_PIXEL);
		rootParameters[2].InitAsDescriptorTable(1, &ranges[2], D3D12_SHADER_VISIBILITY_PIXEL);
		rootParameters[3].InitAsDescriptorTable(1, &ranges[3], D3D12_SHADER_VISIBILITY_PIXEL);

        D3D12_STATIC_SAMPLER_DESC sampler = {};
        sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
        sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
        sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
        sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
        sampler.MipLODBias = 0;
        sampler.MaxAnisotropy = 0;
        sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
        sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
        sampler.MinLOD = 0.0f;
        sampler.MaxLOD = D3D12_FLOAT32_MAX;
        sampler.ShaderRegister = 0;
        sampler.RegisterSpace = 0;
        sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

		D3D12_STATIC_SAMPLER_DESC sampler2 = sampler;
		sampler2.ShaderRegister = 1;
		sampler2.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;

		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
		const D3D12_STATIC_SAMPLER_DESC samplers[] = {
			sampler, sampler2
		};
		rootSignatureDesc.Init_1_1(_countof(rootParameters), rootParameters, _countof(samplers), samplers, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

        ComPtr<ID3DBlob> signature;
        ComPtr<ID3DBlob> error;
        ThrowIfFailed(D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, featureData.HighestVersion, &signature, &error));
        ThrowIfFailed(m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)));

		rootSignatureDesc.Init_1_1(1, rootParameters, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
		
		ThrowIfFailed(D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, featureData.HighestVersion, &signature, &error));
		ThrowIfFailed(m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_depthRootSignature)));
    }

    // Create the pipeline state, which includes compiling and loading shaders.
    {
        ComPtr<ID3DBlob> vertexShader;
        ComPtr<ID3DBlob> pixelShader;

		ID3DBlob* ppErrorMsgs = nullptr;
		const wchar_t* psShaderFileName = L"PixelShader.hlsl";
		const wchar_t* vsShaderFileName = L"VertexShader.hlsl";

		HRESULT hr = D3DCompileFromFile(GetAssetFullPath(vsShaderFileName).c_str(), nullptr, nullptr, "VSmain", "vs_5_0", compileFlags, 0, &vertexShader, &ppErrorMsgs);
		outError(ppErrorMsgs);
		hr = D3DCompileFromFile(GetAssetFullPath(psShaderFileName).c_str(), nullptr, nullptr, "psMain", "ps_5_0", compileFlags, 0, &pixelShader, &ppErrorMsgs);
		outError(ppErrorMsgs);
        // Define the vertex input layout.
        D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
        };

        // Describe and create the graphics pipeline state object (PSO).
        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
        psoDesc.pRootSignature = m_rootSignature.Get();
        psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader.Get());
        psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader.Get());
        psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
#ifdef USE_PLANE
		psoDesc.RasterizerState.FrontCounterClockwise = TRUE;
#endif
        psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
        psoDesc.DepthStencilState.DepthEnable = TRUE;
		psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
        psoDesc.DepthStencilState.StencilEnable = FALSE;
        psoDesc.SampleMask = UINT_MAX;
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		psoDesc.DSVFormat = depthStencilFormat;
        psoDesc.SampleDesc.Count = 1;
        ThrowIfFailed(m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)));

		psoDesc.pRootSignature = m_depthRootSignature.Get();
		psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
		psoDesc.PS = CD3DX12_SHADER_BYTECODE();
		psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = 0;
		ThrowIfFailed(m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_depthPipelineState)));
    }

    // Create the command list.
    ThrowIfFailed(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocator.Get(), m_pipelineState.Get(), IID_PPV_ARGS(&m_commandList)));

    // Create the vertex buffer.
    {
        // Define the geometry for a triangle.
        Vertex triangleVertices[] =
        {
			//{ { 0.0f, coord, 10.0f }, { 0.0f, 1.0f, 0.0f } , { 0.5f, 0.0f } },
			//{ { coord, -coord, 10.0f }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 1.0f } },
			//{ { -coord, -coord, 10.0f }, { 0.0f, 1.0f, 0.0f },  { 0.0f, 1.0f } }
			{ { 0.0f, 0.0f, coord }, { 0.0f, 1.0f, 0.0f } , { 0.5f, 0.0f } },
			{ { coord, 0.0f, -coord }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 1.0f } },
			{ { -coord, 0.0f, -coord }, { 0.0f, 1.0f, 0.0f },  { 0.0f, 1.0f } }

        };
        // Note: using upload heaps to transfer static data like vert buffers is not 
        // recommended. Every time the GPU needs it, the upload heap will be marshalled 
        // over. Please read up on Default Heap usage. An upload heap is used here for 
        // code simplicity and because there are very few verts to actually transfer.

		MeshData meshData;
		CreatePlane(m_device.Get(), meshData, 500, 500, 20, 20, Vector3D::Y(), 0);

		m_vertexBuffer.Attach(meshData.vb.Detach());
		m_indexBuffer.Attach(meshData.ib.Detach());
		ibView = meshData.ibView;
		m_vertexBufferView = meshData.vbView;
		nFaces = meshData.numFaces;
    }
    // Note: ComPtr's are CPU objects but this resource needs to stay in scope until
    // the command list that references it has finished executing on the GPU.
    // We will flush the GPU at the end of this method to ensure the resource is not
    // prematurely destroyed.
    ComPtr<ID3D12Resource> textureUploadHeap;

    // Create the texture.
    {
        // Describe and create a Texture2D.
        D3D12_RESOURCE_DESC textureDesc = {};
        textureDesc.MipLevels = 1;
        textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        textureDesc.Width = TextureWidth;
        textureDesc.Height = TextureHeight;
        textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
        textureDesc.DepthOrArraySize = 1;
        textureDesc.SampleDesc.Count = 1;
        textureDesc.SampleDesc.Quality = 0;
        textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

        ThrowIfFailed(m_device->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
            D3D12_HEAP_FLAG_NONE,
            &textureDesc,
            D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr,
            IID_PPV_ARGS(&m_texture)));

        const UINT64 uploadBufferSize = GetRequiredIntermediateSize(m_texture.Get(), 0, 1);

        // Create the GPU upload buffer.
        ThrowIfFailed(m_device->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
            D3D12_HEAP_FLAG_NONE,
            &CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize),
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&textureUploadHeap)));

        // Copy data to the intermediate upload heap and then schedule a copy 
        // from the upload heap to the Texture2D.
        std::vector<UINT8> texture = GenerateTextureData();

        D3D12_SUBRESOURCE_DATA textureData = {};
        textureData.pData = &texture[0];
        textureData.RowPitch = TextureWidth * TexturePixelSize;
        textureData.SlicePitch = textureData.RowPitch * TextureHeight;

        UpdateSubresources(m_commandList.Get(), m_texture.Get(), textureUploadHeap.Get(), 0, 0, 1, &textureData);
        m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_texture.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));

        // Describe and create a SRV for the texture.
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Format = textureDesc.Format;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = 1;
        m_device->CreateShaderResourceView(m_texture.Get(), &srvDesc, m_srv_cbv_uav_descriptor);
		m_srv_cbv_uav_descriptor.ptr += descriptorSize;
    }
	
    // Close the command list and execute it to begin the initial GPU setup.
    ThrowIfFailed(m_commandList->Close());
    ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
    m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

    // Create synchronization objects and wait until assets have been uploaded to the GPU.
    {
        ThrowIfFailed(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
        m_fenceValue = 1;

        // Create an event handle to use for frame synchronization.
        m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        if (m_fenceEvent == nullptr)
        {
            ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
        }

        // Wait for the command list to execute; we are reusing the same command 
        // list in our main loop but for now, we just want to wait for setup to 
        // complete before continuing.
        WaitForPreviousFrame();
    }
	InitLightingSystem();
}

// Generate a simple black and white checkerboard texture.
std::vector<UINT8> LightIndexedDeferredRendering::GenerateTextureData()
{
    const UINT rowPitch = TextureWidth * TexturePixelSize;
    const UINT cellPitch = rowPitch >> 3;        // The width of a cell in the checkboard texture.
    const UINT cellHeight = TextureWidth >> 3;    // The height of a cell in the checkerboard texture.
    const UINT textureSize = rowPitch * TextureHeight;

    std::vector<UINT8> data(textureSize);
    UINT8* pData = &data[0];

    for (UINT n = 0; n < textureSize; n += TexturePixelSize)
    {
        UINT x = n % rowPitch;
        UINT y = n / rowPitch;
        UINT i = x / cellPitch;
        UINT j = y / cellHeight;

        if (i % 2 == j % 2)
        {
            pData[n] = 0xff;        // R
            pData[n + 1] = 0xff;    // G
            pData[n + 2] = 0xff;    // B
            pData[n + 3] = 0xff;    // A
        }
        else
        {
			pData[n] = 0xff;        // R
            pData[n + 1] = 0xff;    // G
            pData[n + 2] = 0xff;    // B
            pData[n + 3] = 0xff;    // A
        }
    }

    return data;
}

void LightIndexedDeferredRendering::CullLights(LightCullingData& lightCullingData, float R)
{
	CullingLightInfo instanceGPUCullData[numLights];

	for (int lightIndex = m_lightingData.numLights - 1; lightIndex >= 0; --lightIndex) {
		const Light& light = m_lightingData.lights[lightIndex];
		CullingLightInfo& cullingLightInfo = instanceGPUCullData[lightIndex];
		LightInstanceData& lightData = cullingLightInfo.instanceData;
		memcpy(&lightData.posRange, light.Position, sizeof(Vector4D));
		lightData.posRange.w = R ? R : m_lightingData.lightsRanges[lightIndex];
		//lightData.lightIndex = Vector4D(light.color.x, light.color.y, light.color.z, 1.0f);
		lightData.lightIndex = m_lightingData.lightVector4Indices[lightIndex];
	}
	UpdateBuffer(lightCullingData.lightCullingDataBuffer.Get(), instanceGPUCullData, m_lightingData.numLights * sizeof(CullingLightInfo));

	const D3D12_DRAW_INDEXED_ARGUMENTS drawIndexedIndirectCommand = {
		3 * m_lightingData.lightGeometryData.numFaces,
		0,
		0,
		0,
		0
	};
	UpdateBuffer(lightCullingData.lightCullingUpdateIndirectBuffer.Get(), &drawIndexedIndirectCommand, sizeof(drawIndexedIndirectCommand));

	D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(lightCullingData.lightCullingIndirectBuffer.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_DEST);
	{
		ResetCommandList(m_commandList.Get(), m_commandAllocator.Get());
		m_commandList->ResourceBarrier(1, &barrier);
		ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
		ThrowIfFailed(m_commandList->Close());
		m_commandQueue->ExecuteCommandLists(1, ppCommandLists);
		WaitForPreviousFrame();
	}

	ResetCommandList(m_lightingData.copyCommandList.Get(), m_copyCommandAllocator.Get());

	m_lightingData.copyCommandList->CopyResource(lightCullingData.lightCullingIndirectBuffer.Get(), lightCullingData.lightCullingUpdateIndirectBuffer.Get());
	ID3D12CommandList* ppCopyCommandLists[] = { m_lightingData.copyCommandList.Get() };
	ThrowIfFailed(m_lightingData.copyCommandList->Close());
	m_lightingData.copyCommandQueue ->ExecuteCommandLists(1, ppCopyCommandLists);

	UINT64 fence = m_lightingData.fence;
	ThrowIfFailed(m_lightingData.copyCommandQueue->Signal(m_lightingData.copyFence.Get(), m_lightingData.fence++));

	// Wait
	if (m_lightingData.copyFence->GetCompletedValue() < fence) {
		ThrowIfFailed(m_lightingData.copyFence->SetEventOnCompletion(fence, m_lightingData.copyEvent));
		WaitForSingleObject(m_lightingData.copyEvent, INFINITE);
	}

	std::swap(barrier.Transition.StateAfter, barrier.Transition.StateBefore);

	{
		ResetCommandList(m_commandList.Get(), m_commandAllocator.Get());
		ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
		ThrowIfFailed(m_commandList->Close());
		m_commandQueue->ExecuteCommandLists(1, ppCommandLists);
		WaitForPreviousFrame();
	}

	UpdateBuffer(lightCullingData.lightCullingConstantBuffer.Get(), &camera.GetFrustum(), sizeof(Frustum));

	ResetCommandList(m_lightingData.computeCommandList.Get(), m_computeCommandAllocator.Get());

	ID3D12DescriptorHeap* ppHeaps[] = { m_srvHeap.Get() };
	m_lightingData.computeCommandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

	m_lightingData.computeCommandList->SetPipelineState(m_lightingData.lightCullingPipeline.Get());
	m_lightingData.computeCommandList->SetComputeRootSignature(m_lightingData.lightCullingRootSignature.Get());

	m_lightingData.computeCommandList->SetComputeRootDescriptorTable(0, lightCullingData.lightCullingDescriptors[0]);

	m_lightingData.computeCommandList->SetComputeRootDescriptorTable(1, lightCullingData.lightCullingDescriptors[2]);
	m_lightingData.computeCommandList->SetComputeRootDescriptorTable(2, lightCullingData.lightCullingDescriptors[3]);

	m_lightingData.computeCommandList->Dispatch(m_lightingData.numLights, 1, 1);

	ThrowIfFailed(m_lightingData.computeCommandList->Close());

	// Execute the command list.
	ID3D12CommandList* ppCommandLists[] = { m_lightingData.computeCommandList.Get() };
	m_lightingData.computeCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	fence = m_lightingData.fence;
	ThrowIfFailed(m_lightingData.computeCommandQueue->Signal(m_lightingData.computeFence.Get(), m_lightingData.fence++));

	// Wait
	if (m_lightingData.computeFence->GetCompletedValue() < fence) {
		ThrowIfFailed(m_lightingData.computeFence->SetEventOnCompletion(fence, m_lightingData.computeEvent));
		WaitForSingleObject(m_lightingData.computeEvent, INFINITE);
	}
}

// Update frame-based values.
void LightIndexedDeferredRendering::OnUpdate()
{
	POINT pt;
	GetCursorPos(&pt);
	float x = static_cast<float>(pt.x);
	float y = static_cast<float>(pt.y);
	DMousePosition.x = MousePosition.x - x;
	DMousePosition.y = MousePosition.y - y;
	MousePosition.x = x;
	MousePosition.y = y;
	camera.Rotate(-DMousePosition * 0.1f);
#ifndef USE_PERSPECTIVE_RIGHT_HANDLED
	camera.UpdateCamera();
#else
	camera.UpdateCamera(-1);
#endif
	camera.ExtractFrustum();

	const Matrix4x4& viewMatrix = camera.GetViewMatrix();

	cbData.m[0] = camera.GetProjectionMatrix() * viewMatrix;
	cbData.m[1] = viewMatrix;
	memcpy(cbData.camPos, camera.GetPosition(), sizeof(Vector3D));
	UpdateBuffer(m_constantBuffer.Get(), &cbData, sizeof(cbData));

	const Vector3D off(1.0f, 0.0f, 1.0f);
	const Vector3D Xdir = off;
	const Vector3D Zdir = -off;
	const float moveTime = 0.01f;
	float dt = timer.DiffTime();
	emulationTime += dt;
	if (emulationTime < moveTime) {
		return;
	}
	emulationTime = 0.0f;

	auto RotatePos = [](Vector3D& pos, size_t lightIndex, Vector3D& dirOffset)
	{
		float angle = Pi / 360.0f + lightIndex / 512.0f;//Math::Rand(Pi / 120, Pi / 90.0f);

		if (lightIndex % 2) {
			angle = -angle;
		}
#if 0
		pos.RotateXZ(angle);
		//pos.RotateXY(angle / 10.0f);
#else
		const float r = 10.0f;
		Vector3D v = dirOffset * r;
		pos -= v;
		dirOffset.RotateXZ(angle);
		v = dirOffset * r;
		pos += v;
#endif
	};

	for (uint lightIndex = 0; lightIndex < m_lightingData.numLights; ++lightIndex) {
		Light& light = m_lightingData.lights[lightIndex];
		Vector4D& LightPos = m_lightingData.lightsPositions[lightIndex];
		Vector3D& dirOffset = m_lightingData.dirPosOffsets[lightIndex];
		Vector3D& pos = reinterpret_cast<Vector3D &>(LightPos);
		RotatePos(pos, lightIndex, dirOffset);
		memcpy(light.Position, LightPos, sizeof(light.Position));
		// move light to View space, to do GPU Side
		TransformCoord(light.Position, viewMatrix);
		//light.Range = 1.0f / LightPos.w;
	}
	UpdateBuffer(m_lightingData.lightBufferConstantBuffer.Get(), m_lightingData.lights.data(), sizeof(m_lightingData.lights));
#ifdef GPU_CULLING
	// TO DO
	CullLights(m_lightingData.lightCullingData);
	CullLights(m_lightingData.lightCullingDataDebug, 1.0f);
#endif
}

// Render the scene.
void LightIndexedDeferredRendering::OnRender()
{
    // Record all the commands we need to render the scene into the command list.
    PopulateCommandList();

    // Execute the command list.
    ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
    m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

    // Present the frame.
    ThrowIfFailed(m_swapChain->Present(0, 0));

    WaitForPreviousFrame();
}

void LightIndexedDeferredRendering::OnDestroy()
{
    // Ensure that the GPU is no longer referencing resources that are about to be
    // cleaned up by the destructor.
    WaitForPreviousFrame();

    CloseHandle(m_fenceEvent);
}

void LightIndexedDeferredRendering::drawToLightBuffer()
{
	// However, when ExecuteCommandList() is called on a particular command 
	// list, that command list can then be reset at any time and must be before 
	// re-recording.
	ID3D12GraphicsCommandList1* cmdList = m_lightingData.rtCommandList.Get();

	ThrowIfFailed(cmdList->Reset(m_commandAllocator.Get(), nullptr));
#ifdef GPU_CULLING

	std::array<CD3DX12_RESOURCE_BARRIER, 2> barriers;

	barriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(m_lightingData.lightCullingData.lightCullingInstanceBuffer.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
	barriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(m_lightingData.lightCullingData.lightCullingIndirectBuffer.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);

	cmdList->ResourceBarrier(2, barriers.data());
#endif

	ID3D12DescriptorHeap* ppHeaps[] = { m_srvHeap.Get() };
	cmdList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
	cmdList->RSSetViewports(1, &m_viewport);
	cmdList->RSSetScissorRects(1, &m_scissorRect);

	// Indicate that the back buffer will be used as a render target.
	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_lightingData.lightBufferRT.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET));

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart(), 2, m_rtvDescriptorSize);
	cmdList->OMSetRenderTargets(1, &rtvHandle, FALSE, m_dsvHandle.ptr ? &m_dsvHandle : nullptr);

	// Record commands.
	const float clearColor[] = { 0.0f, 0.0f, 0.0f, 0.0f };
	cmdList->ClearRenderTargetView(m_lightingData.lBRTVHandle, clearColor, 0, nullptr);
	if (m_dsvHandle.ptr) {
		cmdList->ClearDepthStencilView(m_dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0x0, 0, nullptr);
	}

	cmdList->SetPipelineState(m_depthPipelineState.Get());
	cmdList->SetGraphicsRootSignature(m_depthRootSignature.Get());
	cmdList->SetGraphicsRootDescriptorTable(0, m_cBDescriptor);
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	cmdList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
	cmdList->IASetIndexBuffer(&ibView);

	// draw scene

	cmdList->DrawIndexedInstanced(nFaces * 3, 1, 0, 0, 0);

	// Set necessary state.
	cmdList->SetPipelineState(m_lightingData.lightBufferPipeline.Get());
	cmdList->SetGraphicsRootSignature(m_lightingData.lightBufferRootSignature.Get());
	cmdList->IASetIndexBuffer(&m_lightingData.lightGeometryData.ibView);
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	const float blendconstants[] = {
		0.251f, 0.251f, 0.251f, 0.251f
	};
	cmdList->OMSetBlendFactor(blendconstants);
	
#ifdef GPU_CULLING

	UpdateBuffer(m_lightingData.lBufferCBVS[0].Get(), cbData.m[0], sizeof(cbData.m[0]));

	cmdList->SetGraphicsRootDescriptorTable(0, m_lightingData.lBufferCBVSDescriptors[0]);
	//cmdList->SetGraphicsRootDescriptorTable(1, m_lightingData.lBufferCBPSDescriptors[0]);
	const D3D12_VERTEX_BUFFER_VIEW pViews[] = {
		m_lightingData.lightGeometryData.vbView,
		m_lightingData.lightCullingData.instanceVBView
	};
	cmdList->IASetVertexBuffers(0, static_cast<UINT>(std::size(pViews)), pViews);
	cmdList->IASetVertexBuffers(0, static_cast<UINT>(std::size(pViews)), pViews);

	//cmdList->IASetVertexBuffers(0, 1, &m_lightingData.lightGeometryData.vbView);
	//for (int lightIndex = m_lightingData.numLights - 1; lightIndex >= 0; --lightIndex) {
	//	const Light& light = m_lightingData.lights[lightIndex];
	//	const Vector4D& lightPosRange = m_lightingData.lightsPositions[lightIndex];
	//	if (!camera.IsVisible(reinterpret_cast<const BoundingSphere &>(lightPosRange))) {
	//		continue;
	//	}
	//	UpdateBuffer(m_lightingData.lBufferCBPS[lightIndex].Get(), m_lightingData.lightVector4Indices[lightIndex], sizeof(Vector4D));
	//}
	cmdList->ExecuteIndirect(m_lightingData.commandSignature.Get(), 1, m_lightingData.lightCullingData.lightCullingIndirectBuffer.Get(), 0, nullptr, 0);

#else
	auto CalculateDepthBounds = [](float& nearVal, float& farVal, float lightSize, const Matrix4x4& modelViewMatrix, const Matrix4x4& projectionMatrix, const Vector3D& lightPosition)
	{
		auto clamp = [](float v, float c0, float c1)
		{
			return min(max(v, c0), c1);
		};

		Vector4D diffVector = Vector4D(0.0f, 0.0f, lightSize, 0.0f);

		Vector4D viewSpaceLightPos = modelViewMatrix * Vector4D(lightPosition.x, lightPosition.y, lightPosition.z, 1.0f);
		Vector4D nearVec = projectionMatrix * (viewSpaceLightPos - diffVector);
		Vector4D farVec = projectionMatrix * (viewSpaceLightPos + diffVector);

		nearVal = clamp(nearVec.z / nearVec.w, -1.0f, 1.0f) * 0.5f + 0.5f;
		if (nearVec.w <= 0.0f) {
			nearVal = 0.0f;
		}
		farVal = clamp(farVec.z / farVec.w, -1.0f, 1.0f) * 0.5f + 0.5f;
		if (farVec.w <= 0.0f) {
			farVal = 0.0f;
		}
		// Sanity check
		if (nearVal > farVal) {
			nearVal = farVal;
		}
	};
	cmdList->IASetVertexBuffers(0, 1, &m_lightingData.lightGeometryData.vbView);
	for (int lightIndex = m_lightingData.numLights - 1; lightIndex >= 0; --lightIndex) {
		const Light& light = m_lightingData.lights[lightIndex];
		const Vector4D& lightPosRange = m_lightingData.lightsPositions[lightIndex];
		if (!camera.IsVisible(reinterpret_cast<const BoundingSphere &>(lightPosRange))) {
			continue;
		}
		UpdateBuffer(m_lightingData.lBufferCBPS[lightIndex].Get(), m_lightingData.lightVector4Indices[lightIndex], sizeof(Vector4D));

		const LightData lightData = { cbData.m[0],  m_lightingData.lightsPositions[lightIndex] };
		UpdateBuffer(m_lightingData.lBufferCBVS[lightIndex].Get(), &lightData, sizeof(lightData));

		cmdList->SetGraphicsRootDescriptorTable(0, m_lightingData.lBufferCBVSDescriptors[lightIndex]);
		cmdList->SetGraphicsRootDescriptorTable(1, m_lightingData.lBufferCBPSDescriptors[lightIndex]);

		float nearVal;
		float farVal;
		CalculateDepthBounds(nearVal, farVal, lightPosRange.w, cbData.m[1], cbData.m[0], lightPosRange.xyz());
		cmdList->OMSetDepthBounds(nearVal, farVal);
		cmdList->DrawIndexedInstanced(m_lightingData.lightGeometryData.numFaces * 3, 1, 0, 0, 0);
	}
#endif
	// Indicate that the back buffer will now be used to present.
	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_lightingData.lightBufferRT.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));

	ThrowIfFailed(cmdList->Close());

	// Execute the command list.
	ID3D12CommandList* ppCommandLists[] = { cmdList };
	m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
	WaitForPreviousFrame();
}

void LightIndexedDeferredRendering::drawLightsSources()
{
	ID3D12GraphicsCommandList* cmdList = m_commandList.Get();
	cmdList->SetGraphicsRootSignature(m_lightingData.lightBufferRootSignature.Get());
	cmdList->SetPipelineState(m_lightingData.lightSourcePipeline.Get());
	cmdList->IASetIndexBuffer(&m_lightingData.lightGeometryData.ibView);
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
#ifdef GPU_CULLING

	const Matrix4x4 m = camera.GetProjectionMatrix() * camera.GetViewMatrix();
	UpdateBuffer(m_lightingData.lBufferCBVS[0].Get(), m, sizeof(m));

	cmdList->SetGraphicsRootDescriptorTable(0, m_lightingData.lBufferCBVSDescriptors[0]);
	const D3D12_VERTEX_BUFFER_VIEW pViews[] = {
		m_lightingData.lightGeometryData.vbView,
		m_lightingData.lightCullingDataDebug.instanceVBView
	};
	cmdList->IASetVertexBuffers(0, static_cast<UINT>(std::size(pViews)), pViews);
	cmdList->ExecuteIndirect(m_lightingData.commandSignature.Get(), 1, m_lightingData.lightCullingDataDebug.lightCullingIndirectBuffer.Get(), 0, nullptr, 0);
#else
	cmdList->IASetVertexBuffers(0, 1, &m_lightingData.lightGeometryData.vbView);
	for (int lightIndex = m_lightingData.numLights - 1; lightIndex >= 0; --lightIndex) {
		const Light& light = m_lightingData.lights[lightIndex];
		const Vector4D& lightPosRange = m_lightingData.lightsPositions[lightIndex];
		if (!camera.IsVisible(reinterpret_cast<const BoundingSphere &>(lightPosRange))) {
			continue;
		}

		UpdateBuffer(m_lightingData.lBufferCBPS[lightIndex].Get(), m_lightingData.lights[lightIndex].color, sizeof(Vector4D));

		struct LightData {
			Matrix4x4 m;
			Vector4D lightData;
		};
		const Vector3D& lpos = m_lightingData.lightsPositions[lightIndex].xyz();
		const LightData lightData = { camera.GetProjectionMatrix() * camera.GetViewMatrix(),  Vector4D(lpos.x, lpos.y, lpos.z, 1.0f) };
		//UpdateBuffer(m_lightingData.lBufferCBVS[lightIndex].Get(), &lightData, sizeof(lightData));
		UpdateBuffer(m_lightingData.lSourceBufferCBVS[lightIndex].Get(), &lightData, sizeof(lightData));

		//cmdList->SetGraphicsRootDescriptorTable(0, m_lightingData.lBufferCBVSDescriptors[lightIndex]);
		cmdList->SetGraphicsRootDescriptorTable(0, m_lightingData.lSourceCBVSDescriptors[lightIndex]);
		cmdList->SetGraphicsRootDescriptorTable(1, m_lightingData.lBufferCBPSDescriptors[lightIndex]);

		cmdList->DrawIndexedInstanced(m_lightingData.lightGeometryData.numFaces * 3, 1, 0, 0, 0);
	}
#endif
}

void LightIndexedDeferredRendering::PopulateCommandList()
{
    // Command list allocators can only be reset when the associated 
    // command lists have finished execution on the GPU; apps should use 
    // fences to determine GPU execution progress.
    ThrowIfFailed(m_commandAllocator->Reset());

	drawToLightBuffer();

    // However, when ExecuteCommandList() is called on a particular command 
    // list, that command list can then be reset at any time and must be before 
    // re-recording.
    ThrowIfFailed(m_commandList->Reset(m_commandAllocator.Get(), m_pipelineState.Get()));


    // Set necessary state.
	ID3D12DescriptorHeap* ppHeaps[] = { m_srvHeap.Get() };
	m_commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

	m_commandList->RSSetViewports(1, &m_viewport);
	m_commandList->RSSetScissorRects(1, &m_scissorRect);

    m_commandList->SetGraphicsRootSignature(m_rootSignature.Get());

	m_commandList->SetGraphicsRootDescriptorTable(0, m_cBDescriptor);
	m_commandList->SetGraphicsRootDescriptorTable(1, m_textureDescriptor);
	m_commandList->SetGraphicsRootDescriptorTable(2, m_lightingData.lBufferRTDescriptor);
	m_commandList->SetGraphicsRootDescriptorTable(3, m_lightingData.lightBufferDescriptor);

    // Indicate that the back buffer will be used as a render target.
    m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart(), m_frameIndex, m_rtvDescriptorSize);
	m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, m_dsvHandle.ptr ? &m_dsvHandle : nullptr);

   // Record commands.
    const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
    m_commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
    m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);

#ifdef USE_PLANE
	m_commandList->IASetIndexBuffer(&ibView);
	m_commandList->DrawIndexedInstanced(nFaces * 3, 1, 0, 0, 0);
#else
    m_commandList->DrawInstanced(3, 1, 0, 0);
#endif

	drawLightsSources();
#ifdef GPU_CULLING
	std::array<CD3DX12_RESOURCE_BARRIER, 3> barriers;
#else
	std::array<CD3DX12_RESOURCE_BARRIER, 1> barriers;
#endif
    // Indicate that the back buffer will now be used to present.
    //m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));
	barriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
#ifdef GPU_CULLING
	barriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(m_lightingData.lightCullingData.lightCullingInstanceBuffer.Get(), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	barriers[2] = CD3DX12_RESOURCE_BARRIER::Transition(m_lightingData.lightCullingData.lightCullingIndirectBuffer.Get(), D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
#endif
	m_commandList->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());
    ThrowIfFailed(m_commandList->Close());
}

void LightIndexedDeferredRendering::WaitForPreviousFrame()
{
    // WAITING FOR THE FRAME TO COMPLETE BEFORE CONTINUING IS NOT BEST PRACTICE.
    // This is code implemented as such for simplicity. The D3D12HelloFrameBuffering
    // sample illustrates how to use fences for efficient resource usage and to
    // maximize GPU utilization.

    // Signal and increment the fence value.
    const UINT64 fence = m_fenceValue;
    ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), fence));
    m_fenceValue++;

    // Wait until the previous frame is finished.
    if (m_fence->GetCompletedValue() < fence)
    {
        ThrowIfFailed(m_fence->SetEventOnCompletion(fence, m_fenceEvent));
        WaitForSingleObject(m_fenceEvent, INFINITE);
    }

    m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
}

void LightIndexedDeferredRendering::OnKeyDown(UINT8 key)
{
#define DIK_W               0x11
#define DIK_A               0x1E
#define DIK_S               0x1F
#define DIK_D               0x20
	const float speed = 1.0f;
	if (DIK_W == key || 'W' == key || 'w' == key) {
		camera.Move(Camera::MOVE_FORWARD, speed);
	}
	else if (DIK_S == key || 'S' == key || 's' == key) {
		camera.Move(Camera::MOVE_BACK, speed);
	}
	else if (DIK_A == key || 'A' == key || 'a' == key) {
		camera.Move(Camera::MOVE_LEFT, speed);
	}
	else if (DIK_D == key || 'D' == key || 'd' == key) {
		camera.Move(Camera::MOVE_RIGHT, speed);
	}
}
