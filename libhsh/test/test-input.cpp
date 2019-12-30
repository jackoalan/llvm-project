namespace MyNS {
enum class PostMode {
  Nothing,
  AddDynamicColor,
  AddDynamicColor2,
  MultiplyDynamicColor
};
}

#include "test-input.cpp.hshhead"

namespace MyNS {

struct MyFormat : hsh::vertex_format {
  hsh::float3 position;
  hsh::float3 normal;
  constexpr MyFormat(hsh::float3 position, hsh::float3 normal)
  : position(std::move(position)), normal(std::move(normal)) {}
};
float SomeAlpha = 0.5f;
void DrawSomething(const hsh::float4x4& xf, const hsh::float3& lightDir,
                   const hsh::float4& dynColor, PostMode postMode [[hsh::host_condition]]) {
  constexpr MyFormat MyBuffer[] = {
    {{-1.f, -1.f, 0.f}, {0.f, 0.f, 1.f}},
    {{ 1.f, -1.f, 0.f}, {0.f, 0.f, 1.f}},
    {{-1.f,  1.f, 0.f}, {0.f, 0.f, 1.f}},
    {{ 1.f,  1.f, 0.f}, {0.f, 0.f, 1.f}}
  };

  constexpr hsh::sampler TestSampler(hsh::wrap::repeat, hsh::filter::nearest);

  // Generated include defines anonymous struct and opens initialization bracket.
  // Captured values the shader is interested in are assigned to the first
  // constructor parameters bound at the end of the include.
  auto MyBinding = hsh_DrawSomething
  [&](const MyFormat vertData [[hsh::vertex_buffer(0)]], // Stands in for current vertex (vertex shader) or
                                                          // interpolated value (fragment shader)
      hsh::texture2d<float> tex0 [[hsh::fragment_texture(0)]], // texture sampler
      hsh::float4 vertPos [[hsh::position]],            // Output of vertex shader
      hsh::float4 fragColor [[hsh::color_target(0)]] = {1.f, 1.f, 1.f, 1.f},   // Output of fragment shader
      auto topology [[hsh::topology]] = hsh::TriangleStrips,
      auto srcBlend [[hsh::source_blend]] = hsh::One,
      auto destBlend [[hsh::destination_blend]] = hsh::Zero) {

    /** Vertex Shader Pass
     * For final [[hsh::position]] assignment:
     * Post-order traverse to [[hsh::vertex_buffer(0)]] and captured host variables;
     * promoting AST nodes from host to vertex as necessary. When an operator is promoted
     * and the other side's expression must be fetched from host, find or create a uniform
     * variable in the HostToVertex RecordDecl for the expression and store its handle in
     * the AST node.
     */

    /** Fragment Shader Pass
     * For each [[hsh::color_target]] assignment:
     * Post-order traverse to [[hsh::vertex_buffer(0)]] and captured host variables;
     * promoting AST nodes from host to vertex to fragment as necessary. When an operator
     * is promoted and the other side's expression must be fetched from host or vertex, find
     * or create a uniform/interpolated variable in the HostToFragment/VertexToFragment
     * RecordDecl for the expression and store its handle in the AST node.
     */

    /** Other Shader Stage Passes
     * Essentially just combining the end of the vertex shader pass with the beginning
     * of the fragment shader pass is how these are implemented. Relevant stage-specific
     * semantic attributes are defined as well.
     */

    /** Assembly Pass
     * 1. Create new host CompoundStmt and insert all root stmts that are not promoted to *all* stages.
     * 2. Create new host VarDecl of the HostToVertex type and initialize with gathered expressions.
     * 3. Create new vertex ParmVarDecl of the HostToVertex type.
     * 4. Create new vertex CompoundStmt and insert all root stmts that are marked as vertex.
     * 5. Insert [[hsh::position]] assignment into vertex CompoundStmt.
     * 6. Create new vertex VarDecl of the VertexToFragment type.
     * 7. Create assignment operators populating the VertexToFragment with gathered expressions.
     * 8. Create new fragment ParmVarDecl of the HostToFragment/VertexToFragment types.
     * 9. Create new fragment CompoundStmt and insert all root stmts that are marked as fragment.
     * 10. Insert all [[hsh::color_target]] assignments into vertex CompoundStmt.
     */

    /** Printing Pass
     * The host lambda generator and each shading language has a printing subclass
     * responsible for printing root declarations and interface directives, the function
     * definition begin, the relevant CompoundStmt traversal (with appropriate PrintingPolicy),
     * and the function definition end.
     *
     * Clang's built-in printing functions are modified to recognise shader printing policies
     * and print with the appropriate syntax variations (types, intrinsic functions, etc...).
     */

    // When vertData is a dependency, expressions are automatically transferred to the shader code.
    // xf will be a field of vertex uniform data.
    vertPos/*v*/ =/*v*/ xf/*h*/ */*v trigger host left-fetch*/ hsh::float4{vertData.position/*v*/, 1.f}/*v*/;

    // normalXf value is not needed within the shader until its multiplication with vertData.normal.
    // The host will compute the value up until the left binary operand; which is another field of vertex uniform data.
    hsh::float3x3 normalXf/*h*/ = xf/*h*/;

    // This multiplication will occur within the vertex shader.
    hsh::float3 finalNormal/*v*/ = normalXf/*h*/ */*v trigger host left-fetch*/ vertData.normal/*v*/;

    // lightDir becomes an rvalue after the negation operator. This rvalue will be a field of fragment uniform data.
    fragColor/*f*/ =/*f*/ hsh::float4{hsh::float3{
      tex0.sample({0.f, 0.f}, TestSampler).xyz() *
      hsh::dot(finalNormal/*v*/, -/*h*/lightDir/*h*/)/*v promoted to f via distribution test,
                                                        trigger vertex left fetch,
                                                        trigger host right fetch*/}/*f*/, 1.f}/*f*/;
#if 0
    bool SomeVar = true;

    const int i = 1, j = 1, k = 0;
    if (i && j && k)
      fragColor = hsh::float4{0,0,0,0};
    else
      fragColor = hsh::float4{1,0,0,0};
    //do { fragColor = hsh::float4{}; if (i) break;  } while (i && j && k);

    for (int i = 0; i < 10; ++i) {
      if (i == 5)
        continue;
      fragColor = hsh::float4{0,0,0,0};
    }
#endif

#if 1
    PostMode postMode2 = PostMode::AddDynamicColor;

    if (postMode2 == PostMode::MultiplyDynamicColor)
      vertPos += dynColor;

    switch (postMode2) {
    case PostMode::AddDynamicColor:
    case PostMode::AddDynamicColor2:
      fragColor += dynColor;
      break;
    case PostMode::MultiplyDynamicColor:
      fragColor *= dynColor;
      break;
    default:
      break;
    }
#endif

    hsh::float4 Blah(0,0,0,0);
    fragColor += Blah;
  };

  // The hsh_binding object has handles of host-processed data buffers ready to draw with.
  MyBinding.bind(hsh::vertex_buffer{0, MyBuffer});
  MyBinding.draw(0, 4);
}
}
