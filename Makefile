all: x-screenshot.c
	$(CC) -O3 -Wall -lX11 -lpng $< -o x-screenshot

clean:
	rm -f x-screenshot
