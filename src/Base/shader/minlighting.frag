#version 330

in vec3 normal; // interpolated normal

uniform int numLights;

struct LightInfo {
    vec3 direction;
    vec3 intensity;
    vec3 ambientIntensity;
};

uniform LightInfo lights[2];

uniform vec3 diffuseColor;
uniform vec3 ambientColor;

layout(location = 0) out vec3 color;

void main()
{
    color = vec3(0.0, 0.0, 0.0);
    for(int i=0; i < numLights; ++i){
        LightInfo light = lights[i];
        color += light.intensity * diffuseColor * abs(dot(light.direction, normal));
        color += light.ambientIntensity * ambientColor;
    }
}