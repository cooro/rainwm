rainwm: rainwm.c
	gcc -Wall rainwm.c -o rainwm `pkg-config --cflags --libs xcb`
clean:
	rm rainwm
