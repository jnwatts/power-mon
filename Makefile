CXXFLAGS = -std=gnu++17

power-mon: eic.o

clean:
	rm -f *.o power-mon
