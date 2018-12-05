#version 150 core

in vec3 Color;
in vec3 vertNormal;
in vec3 pos;
in vec3 lightDir;
in vec2 texcoord;

out vec4 outColor;

uniform sampler2D brick;

const float ambient = .3;

void main() {
    vec3 color;
    vec3 normal = normalize(vertNormal);

    color = texture(brick, texcoord).rgb;
//    color = Color;

    vec3 diffuseC = color*max(dot(-lightDir, normal),0.0);
    vec3 ambC = color*ambient;
    vec3 viewDir = normalize(-pos);
    vec3 reflectDir = reflect(viewDir, normal);
    float spec = max(dot(reflectDir, lightDir),0.0);
    if(dot(-lightDir,normal) <= 0.0) spec = 0;
    vec3 specC = vec3(.8,.8,.8)*pow(spec,4);
    vec3 oColor = ambC + diffuseC + specC;
    outColor = vec4(oColor, 1);

}