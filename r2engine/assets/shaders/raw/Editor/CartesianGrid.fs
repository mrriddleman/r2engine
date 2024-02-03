#version 450 core
#extension GL_NV_gpu_shader5 : enable

#include "Input/UniformBuffers/Matrices.glsl"
#include "Depth/DepthUtils.glsl"

layout (location = 0) out vec4 Accum;
layout (location = 1) out float Reveal;

in VS_OUT
{
    vec3 voutNearPoint;
    vec3 voutFarPoint;

} fs_in;


vec4 grid(vec3 fragPos3D, float scale) {
    vec2 coord = fragPos3D.xy * scale; // use the scale variable to set the distance between the lines
    vec2 derivative = fwidth(coord);
    vec2 grid = abs(fract(coord - 0.5) - 0.5) / derivative;
    float line = min(grid.x, grid.y);
    float minimumz = min(derivative.y, 1);
    float minimumx = min(derivative.x, 1);
    vec4 color = vec4(0.2, 0.2, 0.2, 1.0 - min(line, 1.0));

    if( fract(mod(fragPos3D.y, scale)) > -0.1 * minimumz && fract(mod(fragPos3D.y, scale)) < 0.1 * minimumz)
        color = vec4(0.3, 0.3, 0.3, 1.0);

    if( fract(mod(fragPos3D.x, scale)) > -0.1 * minimumx && fract(mod(fragPos3D.x, scale)) < 0.1 * minimumx)
        color = vec4(0.3, 0.3, 0.3, 1.0);

    // z axis
    if(fragPos3D.x > -0.1 * minimumx && fragPos3D.x < 0.1 * minimumx)
    {
        color = vec4(0.2, 0.9, 0.2, 1.0);
        if(fragPos3D.y < 0)
        {
            color = vec4(0.6, 0.8, 0.6, 1.0);
        }
    }
       
    // x axis
    if(fragPos3D.y > -0.1 * minimumz && fragPos3D.y < 0.1 * minimumz)
    {
        color = vec4(0.9, 0.2, 0.2, 1.0);
        if(fragPos3D.x < 0)
        {
            color = vec4(0.8, 0.6, 0.6, 1.0);
        }
    }
        
    
    return color;
}

float ComputeDepth(vec3 pos) {
    vec4 clip_space_pos = projection * view * vec4(pos.xyz, 1.0);
    return (clip_space_pos.z / clip_space_pos.w);
}

void main()
{
     float t = -fs_in.voutNearPoint.z / (fs_in.voutFarPoint.z - fs_in.voutNearPoint.z);
    
     vec3 fragPos3D = fs_in.voutNearPoint + t * (fs_in.voutFarPoint - fs_in.voutNearPoint);

     float fragDepth = ComputeDepth(fragPos3D);

     gl_FragDepth = (gl_DepthRange.diff * fragDepth + gl_DepthRange.near + gl_DepthRange.far) / 2.0;

     float linearDepth = LinearizeDepth(fragDepth, 0.01, 1);

     float fading = max(0, (0.75 - linearDepth));

    vec4 color = grid(fragPos3D, 20) * float(t > 0);

    // blend func: GL_ONE, GL_ONE
    // switch to pre-multiplied alpha and weight
    Accum = vec4(color.rgb * color.a, color.a) * fading;

    // blend func: GL_ZERO, GL_ONE_MINUS_SRC_COLOR
    Reveal = color.a;
}
