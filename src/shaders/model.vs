#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

out vec2 TexCoords;
out vec3 Normal; // 必须有这个 out 变量

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    TexCoords = aTexCoords;
    
    // 将法线转换到世界空间（解决零件旋转后光照不对的问题）
    // 如果嫌性能开销大，可以暂时用 Normal = aNormal; 但建议用下面这行正式写法：
    Normal = mat3(transpose(inverse(model))) * aNormal;

    gl_Position = projection * view * model * vec4(aPos, 1.0);
}