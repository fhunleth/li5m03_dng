
TARGET = li5m03_dng
OBJECTS = li5m03_dng.o
CC = gcc

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) -o $@ $< -O2 -Wall -lm -ltiff

clean:
	-rm $(TARGET) $(OBJECTS)
