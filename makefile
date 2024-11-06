# Compiler
CC = gcc

# Compiler flags
CFLAGS = -Wall -g -DDEBUG

# Libraries to link
LIBS = -lcurl

# Output file name
TARGET = server

# Source files
SRCS = server.c

# Object files (automatically derived from source files)
OBJS = $(SRCS:.c=.o)

# Default target
all: $(TARGET)

# Rule for building the final executable
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(LIBS)

# Rule for compiling source files into object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up build files
clean:
	rm -f $(OBJS) $(TARGET)

# Optional: Rebuild everything from scratch
rebuild: clean all
