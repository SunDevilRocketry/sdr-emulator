#version 430 core
in vec3 fragPos;
in vec3 normal;
in vec3 color;

out vec4 FragColor;


uniform vec3 emissionColor;

void main() {
    if (emissionColor == vec3(0, 0, 0)) 
    {
        vec3 norm = normalize(normal);

        vec3 lightDir = normalize(vec3(0, 0, 35) - fragPos);

        float diff = max(dot(norm, lightDir), 0);

        vec3 diffuse = diff * vec3(1, 1, 1);
        vec3 ambient = vec3(0.1, 0.1, 0.1);

        vec3 combined = (diffuse + ambient) * color;

        FragColor = vec4(combined, 1.0); //vec4(color, 1); 
    }
    else
    {
        FragColor = vec4(emissionColor, 1);
    }
}
