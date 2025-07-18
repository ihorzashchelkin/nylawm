.PHONY: build
build: configure
	@cmake --build build

.PHONY: configure
configure:
	@cmake -DCMAKE_BUILD_TYPE=Debug -B build .
	@mv build/compile_commands.json .

.PHONY: clean
clean:
	@rm -rf build

.PHONY: run
run:
	build/wm

.PHONY: xev
xev:
	DISPLAY=:1 xev -root \
		-event structure \
		-event substructure \
		-event property

.PHONY: xephyr
xephyr:
	@Xephyr -br -ac -noreset +extension RANDR +extension GLX -screen 1280x720 :1

