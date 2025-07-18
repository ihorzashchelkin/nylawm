.PHONY: build
build: format configure
	@cmake --build build

.PHONY: configure
configure:
	@cmake -B build .
	@mv build/compile_commands.json .

.PHONY: format
format:
	@find . -regex '.*\.\(cpp\|hpp\|cc\|cxx\)' -exec clang-format -style=file -i {} \;

.PHONY: clean
clean:
	@rm -rf build

.PHONY: run
run:
	@build/wm

.PHONY: xephyr
xephyr:
	@Xephyr -br -ac -noreset +extension RANDR +extension GLX -screen 1280x720 :1

