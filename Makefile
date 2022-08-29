SRC=x-screenshot.c
EXE=x-screenshot

CFLAGS=-O3 -Wall -Wextra -fopenmp # threading is marginally faster
LDFLAGS=-lX11 -lpng

all: $(EXE)

$(EXE): $(SRC)
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@

clean:
	rm -f $(EXE)

install: $(EXE)
	install -d /usr/local/bin
	install $< /usr/local/bin

uninstall:
	rm -f /usr/local/bin/$(EXE)
