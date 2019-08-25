CPPFLAGS = -O3 -framework OpenGL -framework SDL -I/Library/Frameworks/SDL.framework/Headers -F/Library/Frameworks

all: fluid.o main.o viewer.o trackball.o genfunc.o
	g++ -o fluid main.o fluid.o viewer.o trackball.o genfunc.o $(CPPFLAGS) 

clean:
	rm *.o fluid
