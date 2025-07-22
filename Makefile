.PHONY: build configure clean run xev xephyr

build: configure
	cmake --build build

configure:
	cmake -B build .
	#cmake -DCMAKE_BUILD_TYPE=Debug -B build .
	mv build/compile_commands.json .

clean:
	rm -rf build
	rm compile_commands.json

run:
	build/wm

xev:
	DISPLAY=:1 xev -root \
		-event structure \
		-event substructure \
		-event property

xephyr:
	Xephyr -br -ac -noreset +extension RANDR +extension GLX -screen 1920x1080 :1

