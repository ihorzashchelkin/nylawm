.PHONY: build
build: configure
	@cmake --build build

.PHONY: configure
configure:
	@cmake -B build .
	@mv build/compile_commands.json .

.PHONY: clean
clean:
	@rm -rf build

.PHONY: run
run:
	@build/wm

.PHONY: xephyr
xephyr:
	@Xephyr -br -ac -noreset +extension RANDR +extension GLX -screen 1280x720 :1

