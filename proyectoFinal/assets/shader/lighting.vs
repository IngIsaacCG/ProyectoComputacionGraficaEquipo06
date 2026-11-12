#version 330 core
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 texCoords;

out vec3 Normal;
out vec3 FragPos;
out vec2 TexCoords;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

uniform bool windEnabled;
uniform float windTime;

void main()
{
    vec3 animatedPosition = position;

    if (windEnabled && abs(normal.x) > 0.98f)
    {
        float wind =
            sin(windTime * 2.5f + position.z * 3.0f) * 0.018f
            + sin(windTime * 4.0f + position.z * 1.5f) * 0.008f;

        animatedPosition.x += wind;
    }

    gl_Position = projection * view * model * vec4(animatedPosition, 1.0f);
    FragPos = vec3(model * vec4(animatedPosition, 1.0f));
    Normal = mat3(transpose(inverse(model))) * normal;
    TexCoords = texCoords;
}