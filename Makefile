build: configure
	ninja -C build

shaders:
	python3 shaders_postprocess.py

configure:
	meson setup build --reconfigure

clean:
	rm -rf build
	rm compile_commands.json

run:
	build/nyla

xev:
	DISPLAY=:1 xev -root \
		-event structure \
		-event substructure \
		-event property

xephyr:
	Xephyr -screen 1366x768 -ac -br -resizeable -glamor :1

