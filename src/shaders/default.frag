#version 460 core
in vec3 fragPos;
in vec3 normal;

out vec4 FragColor;



void main() {

    vec3 norm = normalize(normal);

    vec3 lightDir = normalize(vec3(0, 0, 5) - fragPos);

    float diff = max(dot(norm, lightDir), 0);

    vec3 color = vec3(1.0f, 0.6f, 0.8f);
    vec3 diffuse = diff * vec3(1, 1, 1);
    vec3 ambient = vec3(0.1, 0.1, 0.1);

    vec3 combined = (diffuse + ambient) * color;

    FragColor = vec4(combined, 1.0); //vec4(color, 1); 
}
