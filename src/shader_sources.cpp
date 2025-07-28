#include "shader_sources.hpp"

const char* gVertexShaderSrc = "#version 330 core \n\
\n\
layout (location = 0) in vec3 aPos;\n\
\n\
void\n\
main()\n\
{\n\
    gl_Position = vec4(aPos, 1.0);\n\
}\n\
\n\
";

const char* gFragmentShaderSrc = "#version 330 core \n\
\n\
out vec4 fragColor;\n\
\n\
void\n\
main()\n\
{\n\
    fragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);\n\
}\n\
\n\
";

