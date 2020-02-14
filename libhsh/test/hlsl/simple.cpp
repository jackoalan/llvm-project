// RUN: %hshgen --hlsl -I%hsh_include_dir -source-dump %s - | FileCheck %s
#include "example.cpp.hshhead"

using namespace hsh::pipeline;

struct UniformData {
// CHECK: cbuffer UniformData : register(b0) {
  hsh::float4x4 xf;
  // CHECK-NEXT: float4x4 u_xf : packoffset(c0.x);
  hsh::float3 lightDir;
  // CHECK-NEXT: float3 u_lightDir : packoffset(c4.x);
  float afloat;
  // CHECK-NEXT: float u_afloat : packoffset(c4.w);
  hsh::aligned_float3x3 dynColor;
  // CHECK-NEXT: float3x3 u_dynColor : packoffset(c5.x);
  float bfloat;
  // CHECK-NEXT: float u_bfloat : packoffset(c8.x);
};
// CHECK-NEXT: };

struct VertexFormat {
// CHECK: struct host_vert_data {
  hsh::float3 position;
  // CHECK-NEXT: float3 v_position : ATTR0;
  hsh::float3 normal;
  // CHECK-NEXT: float3 v_normal : ATTR1;
};
// CHECK-NEXT: };

struct DrawSomething : pipeline<color_attachment<>> {
  DrawSomething(hsh::dynamic_uniform_buffer<UniformData> u,
                hsh::vertex_buffer<VertexFormat> v,
                hsh::texture2d<float> tex0) {
    // CHECK: vertex_to_fragment main(in host_vert_data _vert_data) {
    // CHECK-NEXT: vertex_to_fragment _to_fragment;
    position = u->xf * hsh::float4{v->position, 1.f};
    // CHECK-NEXT: _to_fragment._position = mul(u_xf, float4(_vert_data.v_position, 1.F));
    // CHECK-NEXT: return _to_fragment;
    // CHECK: color_targets_out main(in vertex_to_fragment _from_vertex) {
    // CHECK-NEXT: color_targets_out _targets_out;
    color_out[0] = hsh::float4{1.f, 1.f, 1.f, 1.f};
    // CHECK-NEXT: _targets_out._color_out[0] = float4(1.F, 1.F, 1.F, 1.F);
    // CHECK-NEXT: return _targets_out;
  }
};
