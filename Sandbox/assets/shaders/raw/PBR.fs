#version 410 core

out vec4 FragColor;
in vec2 TexCoord;
in vec3 WorldPos;
in vec3 Normal;

uniform vec3 camPos;

uniform vec3 albedo;
uniform float metallic;
uniform float roughness;
uniform float ao;
uniform samplerCube irradianceMap;

uniform vec3 lightPositions[4];
uniform vec3 lightColors[4];

vec3 FresnelSchlick(float cosTheta, vec3 F0);
vec3 FresnelSchlickRoughness(float cosTheta, vec3 Fo, float roughness);
float DistributionGGX(vec3 N, vec3 H, float roughness);
float GeometrySchlickGGX(float NdotV, float roughness);
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness);
vec3 GetNormalFromMap();

const float PI = 3.14159265359;

void main()
{
	// vec3 albedo = pow(texture(albedoMap, TexCoord).rgb, vec3(2.2));
	// float metallic = texture(metallicMap, TexCoord).r;
	// float roughness = texture(roughnessMap, TexCoord).r;
	// float ao = texture(aoMap, TexCoord).r;

	vec3 N = normalize(Normal);
	vec3 V = normalize(camPos - WorldPos);
	vec3 R = reflect(-V, N);

	vec3 Fo = vec3(0.04);
	Fo = mix(Fo, albedo, metallic);

	vec3 Lo = vec3(0.0);

	for(int i = 0; i < 4; ++i)
	{
		vec3 L = normalize(lightPositions[i] - WorldPos);
		vec3 H = normalize(V + L);

		float distance = length(lightPositions[i] - WorldPos);
		float attenuation = 1.0 / (distance * distance);
		vec3 radiance = lightColors[i] * attenuation;

		vec3 F = FresnelSchlick(clamp(dot(H, V), 0.0, 1.0), Fo);
		float NDF = DistributionGGX(N, H, roughness);
		float G = GeometrySmith(N, V, L, roughness);

		vec3 numerator = NDF * G * F;
		float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0);
		vec3 specular = numerator / max(denominator, 0.001);

		vec3 kS = F;
		vec3 kD = vec3(1.0) - kS;

		kD *= (1.0 - metallic);

		float NdotL = max(dot(N, L), 0.0);
		Lo += (kD * albedo / PI + specular) * radiance * NdotL;
	}

	vec3 kS = FresnelSchlickRoughness(max(dot(N, V), 0.0), Fo, roughness);
	vec3 kD = 1.0 - kS;
	vec3 irradiance = texture(irradianceMap, N).rgb;
	vec3 diffuse = irradiance * albedo;
	vec3 ambient = (kD * diffuse) * ao;

	vec3 color = ambient + Lo;

	color = color / (color + vec3(1.0));
	color = pow(color, vec3(1.0 / 2.2));

	FragColor = vec4(color, 1.0);
}


vec3 FresnelSchlick(float cosTheta, vec3 Fo)
{
	return Fo + (1.0 - Fo) * pow(1.0 - cosTheta, 5.0);
}

vec3 FresnelSchlickRoughness(float cosTheta, vec3 Fo, float roughness)
{
	return Fo + (max(vec3(1.0 - roughness), Fo) - Fo) * pow(1.0 - cosTheta, 5.0);
}

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
	float a = roughness * roughness;
	float a2 = a * a;
	float NdotH = max(dot(N, H), 0.0);
	float NdotH2 = NdotH * NdotH;

	float num = a2;
	float denom = (NdotH2 * (a2 - 1.0) + 1.0);
	denom = PI * denom * denom;

	return num / max(denom, 0.001);
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
	float r = roughness + 1.0;
	float k = (r*r) / 8.0;

	return NdotV / (NdotV * (1.0 - k) + k);
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
	float NdotV = max(dot(N, V), 0.0);
	float NdotL = max(dot(N, L), 0.0);
	float ggx2 = GeometrySchlickGGX(NdotV, roughness);
	float ggx1 = GeometrySchlickGGX(NdotL, roughness);

	return ggx1 * ggx2;
}

vec3 GetNormalFromMap()
{
	return vec3(0);
	// vec3 tangentNormal = texture(normalMap, TexCoord).xyz * 2.0 - 1.0;

	// vec3 Q1 = dFdx(WorldPos);
	// vec3 Q2 = dFdy(WorldPos);
	// vec2 st1 = dFdx(TexCoord);
	// vec2 st2 = dFdy(TexCoord);

	// vec3 N = normalize(Normal);
	// vec3 T = normalize(Q1*st2.t - Q2*st1.t);
	// vec3 B = -normalize(cross(N, T));
	// mat3 TBN = mat3(T, B, N);

	// return normalize(TBN * tangentNormal);

}