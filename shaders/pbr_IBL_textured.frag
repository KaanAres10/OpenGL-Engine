#version 450 core
out vec4 FragColor;

in VS_OUT {
    vec3 WorldPos;
    vec3 Normal;
    vec2 TexCoords;
} fs_in;

uniform vec3 camPos;

uniform sampler2D albedoMap;
uniform sampler2D normalMap;
uniform sampler2D metallicMap;
uniform sampler2D roughnessMap;
uniform sampler2D aoMap;

uniform samplerCube irradianceMap;
uniform samplerCube prefilterMap;
uniform sampler2D brdfIntegrationMap;

#define NR_POINT_LIGHTS 9
uniform vec3 lightPositions[NR_POINT_LIGHTS];
uniform vec3 lightColors[NR_POINT_LIGHTS];
uniform int  uLightCount;

const float PI = 3.14159265359;


vec3 getNormalFromMap()
{
    vec3 tangentNormal = texture(normalMap, fs_in.TexCoords).xyz * 2.0 - 1.0;

    vec3 Q1  = dFdx(fs_in.WorldPos);
    vec3 Q2  = dFdy(fs_in.WorldPos);
    vec2 st1 = dFdx(fs_in.TexCoords);
    vec2 st2 = dFdy(fs_in.TexCoords);

    vec3 N   = normalize(fs_in.Normal);
    vec3 T  = normalize(Q1*st2.t - Q2*st1.t);
    vec3 B  = -normalize(cross(N, T));
    mat3 TBN = mat3(T, B, N);

    return normalize(TBN * tangentNormal);
}

vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}  

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
	return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a      = roughness*roughness;
    float a2     = a*a;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
	
    return a2 / (PI * denom * denom);
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    return NdotV / (NdotV * (1.0 - k) + k);
}
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2  = GeometrySchlickGGX(NdotV, roughness);
    float ggx1  = GeometrySchlickGGX(NdotL, roughness);
	
    return ggx1 * ggx2;
}

void main() {
	vec3 albedo = texture(albedoMap, fs_in.TexCoords).rgb;
    float metallic = texture(metallicMap, fs_in.TexCoords).r;
    float roughness = texture(roughnessMap, fs_in.TexCoords).r;
    float ao = texture(aoMap, fs_in.TexCoords).r;
	
	vec3 N = getNormalFromMap();
	vec3 V = normalize(camPos - fs_in.WorldPos);
	vec3 R = reflect(-V, N);

	vec3 F0 = vec3(0.04);
	F0 = mix(F0, albedo, metallic);

	vec3 Lo = vec3(0.0);
	for (int i = 0; i < uLightCount; i++)
	{
		vec3 L = normalize(lightPositions[i] - fs_in.WorldPos);
		vec3 H = normalize(V + L);
		float NdotL = max(dot(N, L), 0.0);        

		float distance = length(lightPositions[i] - fs_in.WorldPos);
		float attenuation = 1.0 / (distance * distance);
		vec3 radiance = lightColors[i] * attenuation;
		

		float NDF = DistributionGGX(N, H, roughness);       
		float G = GeometrySmith(N, V, L, roughness);
		vec3 F  = fresnelSchlick(max(dot(H, V), 0.0), F0);

		vec3 numerator = NDF * G * F;
		float denominator = 4.0 * max(dot(N, V), 0.0) * NdotL  + 0.0001;
		vec3 specular = numerator / denominator;  

		vec3 kS = F;
		vec3 kD = (vec3(1.0) - kS) * (1.0 - metallic);

		Lo += (kD * albedo / PI + specular) * radiance * NdotL;
	}

	vec3 F = fresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness);

	vec3 kS = F; 
	vec3 kD = 1.0 - kS;
	kD *= 1.0 - metallic;	  
	vec3 irradiance = texture(irradianceMap, N).rgb;
	vec3 diffuse = irradiance * albedo;

	const float MAX_REFLECTION_LOD = 4.0;
    vec3 prefilteredColor = textureLod(prefilterMap, R,  roughness * MAX_REFLECTION_LOD).rgb;  
	vec2 brdf  = texture(brdfIntegrationMap, vec2(max(dot(N, V), 0.0), roughness)).rg;
    vec3 specular = prefilteredColor * (F * brdf.x + brdf.y);


	vec3 ambient = (kD * diffuse + specular) * ao; 

	vec3 color = ambient + Lo;

	FragColor = vec4(color, 1.0);
}
