from os import scandir

def main():

    header = "#pragma once\n\n"
    impl = "#include \"shader_sources.hpp\"\n\n"

    for entry in scandir("data/gl/"):
        if (not entry.is_file()) or (not entry.path.endswith(".glsl")):
            continue

        header += 'extern const char* g' + entry.name.removesuffix(".glsl") + 'Src';
        impl += 'const char* g' + entry.name.removesuffix(".glsl") + 'Src = "';

        with open(entry.path) as infile:
            for ch in infile.read():
                if ch == '"':
                    impl += '\\"';
                elif ch == '\n':
                    impl += '\\n\\\n';
                else:
                    impl += ch

        impl += '";\n\n'
        header += ';\n'

    with open("src/shader_sources.h", "w") as outfile:
        outfile.write(header)
    with open("src/shader_sources.c", "w") as outfile:
        outfile.write(impl)


if __name__ == "__main__":
    main()

