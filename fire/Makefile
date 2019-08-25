CPPFLAGS = -O3 -framework OpenGL -framework SDL -I/Library/Frameworks/SDL.framework/Headers -F/Library/Frameworks

all: fluid.o main.o genfunc.o viewer.o trackball.o FragmentUtils.o spectrum.o
	g++ -o fluid main.o fluid.o genfunc.o viewer.o trackball.o FragmentUtils.o spectrum.o $(CPPFLAGS)

genfunctest: genfunctest.o genfunc.o
	g++ -o genfunctest genfunctest.o genfunc.o $(CPPFLAGS)

clean:
	rm *.o fluid
