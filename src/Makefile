CFLAGS = -MMD -Wall -pedantic
CXXFLAGS = -MMD -Wall -pedantic
LIBS = -lm
CC = gcc
CXX = g++
OFILES = $(patsubst %.c,%.o,$(wildcard *.c)) $(patsubst %.cpp,%.o,$(wildcard *.cpp))
DFILES = $(patsubst %.c,%.d,$(wildcard *.c)) $(patsubst %.cpp,%.d,$(wildcard *.cpp))
HFILES = $(wildcard *.h *.hpp)
PROG = sim3

ifdef SANITIZE
CFLAGS += -fsanitize=address
CXXFLAGS += -fsanitize=address
LIBS += -fsanitize=address
endif

ifdef DEBUG
CFLAGS += -DDEBUG
CXXFLAGS += -DDEBUG
endif

ifdef FAST
CFLAGS += -O2
CXXFLAGS += -O2
else
CFLAGS += -g
CXXFLAGS += -g
endif

.PHONY: all clean run gen_ref

all: $(PROG)

$(PROG): $(OFILES)
# 	$(CXX) -o $@ $^ $(LIBS)
	$(CC) -o $@ $^ $(LIBS)

%.o: %.c $(HFILES)
	$(CC) $(CFLAGS) -c -o $@ $<

# %.o: %.cpp $(HFILES)
# 	$(CXX) $(CXXFLAGS) -c -o $@ $<

clean:
	rm -f $(PROG) $(OFILES) $(DFILES)
	rm -f ../files/*.log

-include $(DFILES)

# valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes

run:
	make clean
	make all FAST=1
	./run.sh

gen_ref:
	rm -f ref.log
	./gen_ref.sh
