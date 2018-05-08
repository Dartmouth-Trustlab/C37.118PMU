CC = gcc
DEPS = C37_tools.h
OBJ = C37_config1.o C37_tools.o
LNK = -lhammer `pkg-config --libs --cflags glib-2.0`

dt: $(OBJ)
	$(CC) -o $@ $^ $(LNK) -ggdb

coverage: $(OBJ)
	$(CC) -o coverage.out $^ $(LNK) -ggdb

C37_config1.o: C37_config1.c $(DEPS)
	$(CC) -c -o $@ $< -ggdb 

C37_tools.o: C37_tools.c $(DEPS)
	$(CC) -c -o $@ $< -ggdb

tests: tools.o config.o
	$(CC) -o $@_config $(LNK) tools.o config.o config_test.c
	$(CC) -o $@_data $(LNK) tools.o config.o data_test.c
