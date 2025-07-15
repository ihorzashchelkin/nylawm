.PHONY: build
build:
	@cmake -B build .
	@mv build/compile_commands.json .
	@cmake --build build

.PHONY: xephyr
xephyr:
	Xephyr -br -ac -noreset +extension RANDR +extension GLX -screen 1280x720 :1

