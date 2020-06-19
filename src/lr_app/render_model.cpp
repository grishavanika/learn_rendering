#include "render_model.h"

#include "shaders/vs_basic_phong_lighting.h"
#include "shaders/ps_basic_phong_lighting.h"

const D3D11_INPUT_ELEMENT_DESC layout[] =
{
    {
        .SemanticName = "position",
        .SemanticIndex = 0,
        .Format = DXGI_FORMAT_R32G32B32_FLOAT,
        .InputSlot = 0,
        .AlignedByteOffset = offsetof(Vertex, position),
        .InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA,
        .InstanceDataStepRate = 0
    },
    {
        .SemanticName = "normal",
        .SemanticIndex = 0,
        .Format = DXGI_FORMAT_R32G32B32_FLOAT,
        .InputSlot = 0,
        .AlignedByteOffset = offsetof(Vertex, normal),
        .InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA,
        .InstanceDataStepRate = 0
    },
    {
        .SemanticName = "texcoord",
        .SemanticIndex = 0,
        .Format = DXGI_FORMAT_R32G32_FLOAT,
        .InputSlot = 0,
        .AlignedByteOffset = offsetof(Vertex, texture_coord),
        .InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA,
        .InstanceDataStepRate = 0
    },
};

static constexpr DXGI_FORMAT GetIndexBufferFormat()
{
    if constexpr ((sizeof(Index) == 2) && std::is_unsigned_v<Index>)
    {
        return DXGI_FORMAT_R16_UINT;
    }
    else if constexpr ((sizeof(Index) == 4) && std::is_unsigned_v<Index>)
    {
        return DXGI_FORMAT_R32_UINT;
    }
    // Intentionally no return to fail compilation.
    // (Control reaching the end of a constexpr function).
}

static ID3D11ShaderResourceView* GetTexture(const RenderModel& model, std::uint32_t id)
{
    for (const RenderTexture& texture : model.textures)
    {
        if (texture.texture_id == id)
        {
            Panic(texture.texture_view);
            return texture.texture_view.Get();
        }
    }
    return nullptr;
}

/*static*/ RenderMesh RenderMesh::make(ID3D11Device& device, const Mesh& mesh)
{
    RenderMesh render{};
    render.indices_count = UINT(mesh.indices.size());
    render.ps_texture0_id = mesh.texture_diffuse_id;

    D3D11_BUFFER_DESC bd{};

    // VB.
    Panic(!mesh.vertices.empty());
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = UINT(mesh.vertices.size() * sizeof(Vertex));
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bd.CPUAccessFlags = 0;
    D3D11_SUBRESOURCE_DATA InitData{};
    InitData.pSysMem = mesh.vertices.data();
    HRESULT hr = device.CreateBuffer(&bd, &InitData, &render.vertex_buffer);
    Panic(SUCCEEDED(hr));

    // IB.
    Panic(!mesh.indices.empty());
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = UINT(mesh.indices.size() * sizeof(Index));
    bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    bd.CPUAccessFlags = 0;
    InitData.pSysMem = mesh.indices.data();
    hr = device.CreateBuffer(&bd, &InitData, &render.index_buffer);
    Panic(SUCCEEDED(hr));

    return render;
}

/*static*/ RenderTexture RenderTexture::make(ID3D11Device& device, const Texture& texture)
{
    RenderTexture render{};
    render.texture_id = texture.id;

    D3D11_TEXTURE2D_DESC t2d_desc{};
    t2d_desc.Width = texture.width;
    t2d_desc.Height = texture.height;
    t2d_desc.MipLevels = 1;
    t2d_desc.ArraySize = 1;
    t2d_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    t2d_desc.SampleDesc.Count = 1;
    t2d_desc.SampleDesc.Quality = 0;
    t2d_desc.Usage = D3D11_USAGE_DEFAULT;
    t2d_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    t2d_desc.CPUAccessFlags = 0;
    t2d_desc.MiscFlags = 0;

    D3D11_SUBRESOURCE_DATA subresource{};
    subresource.pSysMem = texture.data.data();
    subresource.SysMemPitch = (texture.width * c_texture_channels);
    subresource.SysMemSlicePitch = 0; // not used for 2d textures.

    ComPtr<ID3D11Texture2D> texture2d;
    HRESULT hr = device.CreateTexture2D(&t2d_desc, &subresource, &texture2d);
    Panic(SUCCEEDED(hr));

    D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc{};
    SRVDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    SRVDesc.Texture2D.MipLevels = 1;

    hr = device.CreateShaderResourceView(texture2d.Get(), &SRVDesc, &render.texture_view);
    Panic(SUCCEEDED(hr));

    return render;
}

/*static*/ RenderModel RenderModel::make(ID3D11Device& device, const Model& model)
{
    RenderModel render{};
    render.model = &model;

    // VS & IA.
    HRESULT hr = device.CreateInputLayout(
        layout
        , _countof(layout)
        , k_vs_basic_phong_lighting
        , sizeof(k_vs_basic_phong_lighting)
        , &render.vertex_layout_);
    Panic(SUCCEEDED(hr));

    hr = device.CreateVertexShader(
        k_vs_basic_phong_lighting
        , sizeof(k_vs_basic_phong_lighting)
        , nullptr
        , &render.vertex_shader_);
    Panic(SUCCEEDED(hr));

    // Create the constant buffer for VS.
    D3D11_BUFFER_DESC vs_bd{};
    vs_bd.Usage = D3D11_USAGE_DEFAULT;
    vs_bd.ByteWidth = sizeof(VSConstantBuffer0);
    vs_bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    vs_bd.CPUAccessFlags = 0;
    hr = device.CreateBuffer(&vs_bd, nullptr, &render.vs_constant_buffer0_);
    Panic(SUCCEEDED(hr));

    // PS.
    hr = device.CreatePixelShader(
        k_ps_basic_phong_lighting
        , sizeof(k_ps_basic_phong_lighting)
        , nullptr
        , &render.pixel_shader_);
    Panic(SUCCEEDED(hr));

    D3D11_BUFFER_DESC ps_bd{};
    ps_bd.Usage = D3D11_USAGE_DEFAULT;
    ps_bd.ByteWidth = sizeof(PSConstantBuffer0);
    ps_bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    ps_bd.CPUAccessFlags = 0;
    hr = device.CreateBuffer(&ps_bd, nullptr, &render.ps_constant_buffer0_);
    Panic(SUCCEEDED(hr));

    // Create the sample state.
    // Texture sampling for PS.
    D3D11_SAMPLER_DESC sampler_desc{};
    sampler_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sampler_desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    sampler_desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    sampler_desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    sampler_desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    sampler_desc.MinLOD = 0;
    sampler_desc.MaxLOD = D3D11_FLOAT32_MAX;
    hr = device.CreateSamplerState(&sampler_desc, &render.sampler_linear_);
    Panic(SUCCEEDED(hr));

    for (std::uint32_t i = 0; i < model.meshes_count_; ++i)
    {
        render.meshes.push_back(RenderMesh::make(
            device, model.get_mesh(i)));
    }
    for (std::uint32_t i = 0; i < model.textures_count_; ++i)
    {
        render.textures.push_back(RenderTexture::make(
            device, model.get_texture(i)));
    }
    return render;
}

void RenderModel::render(ID3D11DeviceContext& device_context
    , const DirectX::XMMATRIX& view_transposed
    , const DirectX::XMMATRIX& projection_transposed) const
{
    // Parameters for VS.
    VSConstantBuffer0 vs_cb0;
    vs_cb0.world = DirectX::XMMatrixTranspose(world);
    vs_cb0.view = view_transposed;
    vs_cb0.projection = projection_transposed;

    // Parameters for PS.
    PSConstantBuffer0 ps_cb0;
    ps_cb0.light_color = light_color;
    ps_cb0.viewer_position = viewer_position;
    ps_cb0.light_position = light_position;

    for (const RenderMesh& render_mesh : meshes)
    {
        UINT stride = sizeof(Vertex);
        UINT offset = 0;
        // Input Assembler.
        device_context.IASetVertexBuffers(0, 1, render_mesh.vertex_buffer.GetAddressOf(), &stride, &offset);
        device_context.IASetIndexBuffer(render_mesh.index_buffer.Get(), GetIndexBufferFormat(), 0);
        device_context.IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        device_context.IASetInputLayout(vertex_layout_.Get());
        // Vertex Shader.
        device_context.VSSetShader(vertex_shader_.Get(), nullptr, 0);
        device_context.UpdateSubresource(vs_constant_buffer0_.Get(), 0, nullptr, &vs_cb0, 0, 0);
        device_context.VSSetConstantBuffers(0, 1, vs_constant_buffer0_.GetAddressOf());
        // Pixel Shader.
        device_context.PSSetShader(pixel_shader_.Get(), nullptr, 0);
        device_context.UpdateSubresource(ps_constant_buffer0_.Get(), 0, nullptr, &ps_cb0, 0, 0);
        device_context.PSSetConstantBuffers(0, 1, ps_constant_buffer0_.GetAddressOf());
        if (ID3D11ShaderResourceView* ps_texture0 = GetTexture(*this, render_mesh.ps_texture0_id))
        {
            device_context.PSSetShaderResources(0, 1, &ps_texture0);
            device_context.PSSetSamplers(0, 1, sampler_linear_.GetAddressOf());
        }

        // Actual draw call.
        device_context.DrawIndexed(render_mesh.indices_count, 0, 0);
    }
}
