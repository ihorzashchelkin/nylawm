#include "shader_sources.hpp"

const char* gVertexShaderSrc = "#version 330 core \n\
\n\
layout (location = 0) in vec3 aPos;\n\
layout (location = 1) in vec2 aTexCoord;\n\
\n\
out vec2 TexCoord;\n\
\n\
void\n\
main()\n\
{\n\
    gl_Position = vec4(aPos, 1.0);\n\
    TexCoord = aTexCoord;\n\
}\n\
\n\
";

const char* gFragmentShaderSrc = "#version 330 core\n\
out vec4 FragColor;\n\
  \n\
in vec2 TexCoord;\n\
\n\
uniform sampler2D ourTexture;\n\
\n\
void main()\n\
{\n\
    vec2 flipped = vec2(TexCoord.x, 1.0 - TexCoord.y);\n\
    FragColor = texture(ourTexture, flipped);\n\
}\n\
";

