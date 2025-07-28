#version 330 core
out vec4 FragColor;
  
in vec3 ourColor;
in vec2 TexCoord;

uniform sampler2D ourTexture;

void main()
{
    vec2 flipped = vec2(TexCoord.x, 1.0 - TexCoord.y);
    FragColor = texture(ourTexture, flipped) * vec4(ourColor, 1.0);
}
