rainwm: rainwm.c
	gcc -Wall rainwm.c -o rainwm `pkg-config --cflags --libs xcb`
test: test.c
	gcc -Wall test.c -o test `pkg-config --cflags --libs xcb`
clean:
	rm test rainwm
