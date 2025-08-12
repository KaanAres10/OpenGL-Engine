#version 450 core
layout(location=0) out float FragAO;

in vec2 TexCoords;

uniform sampler2DMS gPositionMS;
uniform sampler2DMS gNormalMS;
uniform sampler2D   texNoise;

uniform vec3  samples[64];
uniform mat4  projection;

uniform float bias;

uniform int   kernelSize;
uniform float radius;
uniform vec2  noiseScale;
uniform vec2 screenSize; 
uniform int   uSamples; 

vec3 fetchPositionMS(vec2 uv){
    vec2 st = clamp(uv, vec2(0.0), vec2(1.0)); 
    ivec2 ip = ivec2(st * vec2(screenSize));
    vec3 acc = vec3(0);
    for(int s=0; s<uSamples; ++s) acc += texelFetch(gPositionMS, ip, s).xyz;
    return acc / float(uSamples);
}
vec3 fetchNormalMS(vec2 uv){
    ivec2 ip = ivec2(uv * vec2(screenSize));
    vec3 acc = vec3(0);
    for(int s=0; s<uSamples; ++s) acc += texelFetch(gNormalMS, ip, s).xyz;
    return normalize(acc / float(uSamples));
}

void main(){
    vec3 fragPos   = fetchPositionMS(TexCoords);
    vec3 normal    = normalize(fetchNormalMS(TexCoords));
    vec3 randomVec = normalize(texture(texNoise, TexCoords * noiseScale).xyz);

    vec3 tangent   = normalize(randomVec - normal * dot(randomVec, normal));
    vec3 bitangent = cross(normal, tangent);
    mat3 TBN       = mat3(tangent, bitangent, normal);

    float occlusion = 0.0;


    for(int i = 0; i<kernelSize; ++i){
        vec3 samp = TBN * samples[i];
        float depth = -fragPos.z;                      
        float r = radius * clamp(depth * 0.2, 0.25, 2.0);
        vec3 sp   = fragPos + (TBN * samples[i]) * r;  
        vec4 offset = projection * vec4(sp, 1.0);
        if (offset.w <= 0.0) continue;
        offset.xyz /= offset.w;
        vec2 uv = offset.xy * 0.5 + 0.5;

        if(uv.x<0.0 || uv.x>1.0 || uv.y<0.0 || uv.y>1.0) continue;

        float sampleDepth = fetchPositionMS(uv).z;

        float rangeCheck = smoothstep(0.0, 1.0, radius / abs(fragPos.z - sampleDepth));
        occlusion += (sampleDepth >= sp.z + bias ? 1.0 : 0.0) * rangeCheck;
    }

    occlusion = 1.0 - (occlusion / float(kernelSize));
    FragAO = occlusion;
}
