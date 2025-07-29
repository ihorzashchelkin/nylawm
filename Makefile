build: configure shaders
	ninja -C build

shaders:
	python3 shaders_postprocess.py

configure:
	meson setup build --reconfigure

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
	Xephyr -br -ac -noreset +extension RANDR +extension GLX -screen 1200x800 :1

