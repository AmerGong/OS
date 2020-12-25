linker: linker.cpp
	g++ -w -std=c++11 -o linker linker.cpp
clean:
	rm -f linker *~