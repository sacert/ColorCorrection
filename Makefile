CC=clang++ -std=c++11

ColorCor.o:
	$(CC) ColorCor.cpp -o ColorCor -O2 -larmadillo `pkg-config --cflags --libs opencv` -lcurl
