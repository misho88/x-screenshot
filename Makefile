SRC=x-screenshot.c
EXE=x-screenshot

all: $(EXE)

$(EXE): $(SRC)
	$(CC) -O3 -Wall -lX11 -lpng $< -o $@

clean:
	rm -f $(EXE)

install: $(EXE)
	install -d /usr/local/bin
	install $< /usr/local/bin

uninstall:
	rm -f /usr/local/bin/$(EXE)
