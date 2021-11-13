#version 330 core
layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;    // points have to be set sequentially, so always a strip type

// VertexShader outs in an interface block for all points passed 
in VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoord;
} gs_in[];

// outs passed along to FragShader
out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoord;

vec3 CalcNormal();
vec4 extrude(vec4 position, vec3 normal);

void main()
{
    vec3 norm = CalcNormal(); // use our calculated normal

    gl_Position = extrude(gl_in[0].gl_Position, norm);
    FragPos = extrude(vec4(gs_in[0].FragPos, 1.0), norm).xyz;
    Normal = norm;
    TexCoord = gs_in[0].TexCoord;
    EmitVertex();
    
    gl_Position = extrude(gl_in[1].gl_Position, norm);
    FragPos = extrude(vec4(gs_in[1].FragPos, 1.0), norm).xyz;
    Normal = norm;
    TexCoord = gs_in[1].TexCoord;
    EmitVertex();
    
    gl_Position = extrude(gl_in[2].gl_Position, norm);
    FragPos = extrude(vec4(gs_in[2].FragPos, 1.0), norm).xyz;
    Normal = norm;
    TexCoord = gs_in[2].TexCoord;
    EmitVertex();

    EndPrimitive();
}

vec3 CalcNormal()
{
   vec3 a = vec3(gl_in[0].gl_Position) - vec3(gl_in[1].gl_Position);
   vec3 b = vec3(gl_in[2].gl_Position) - vec3(gl_in[1].gl_Position);
   return normalize(cross(a, b));
}

vec4 extrude(vec4 position, vec3 normal)
{
    float amnt = 0.1;
    vec3 dir = normal * amnt;
    return position + vec4(dir, 0.0);
}
