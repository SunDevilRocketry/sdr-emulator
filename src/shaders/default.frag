#version 430 core
in vec3 fragPos;
in vec3 normal;
in vec3 color;

out vec4 FragColor;

struct directionalLight {
    vec3 direction;

    vec3 ambient;
    vec3 diffuse;
    // no specular
};

struct pointLight {
    vec3 position;

    vec3 ambient;
    vec3 diffuse;
    // no specular

    float constant;
    float linear;
    float quadratic;
};

// TODO: Add more detailed material struct uniform or smthn

uniform directionalLight globalLight;

void main() {

    // ambient
    vec3 ambient = globalLight.ambient * color;
    //diffuse
    vec3 norm = normalize(normal);
    vec3 lightDir = -normalize(globalLight.direction); 
    float diff = max(dot(norm, lightDir), 0);
    vec3 diffuse = globalLight.diffuse * diff * color;

    // combineee
    vec3 combined = (diffuse + ambient); 

    FragColor = vec4(combined, 1.0); //vec4(color, 1); 
}
