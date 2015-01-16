SRC = bf2llvm.cpp
CFLAGS = `llvm-config --cxxflags --ldflags --libs` -g
 
all: comp
 
comp: $(SRC)
	clang++ $(CFLAGS) -o bf2llvm $(SRC) > compile.log 2>&1
 
.PHONY: clean
clean:
	rm -f *.log bf2llvm
