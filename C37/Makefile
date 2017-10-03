CC = gcc
DEPS = C37_tools.h
OBJ = config.o tools.o
LNK = -lhammer `pkg-config --libs --cflags glib-2.0`

dt: $(OBJ)
	$(CC) -o $@ $^ $(LNK) -ggdb

config.o: C37_config1.c $(DEPS)
	$(CC) -c -o $@ $< -ggdb

tools.o: C37_tools.c $(DEPS)
	$(CC) -c -o $@ $< -ggdb

clean:
	rm tools.o dt config.o
