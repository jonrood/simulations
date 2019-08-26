FLAGS = -O3 -I/Library/Frameworks/SDL2.framework/Headers
LINKFLAGS = -O3 -framework OpenGL -framework SDL2 -F/Library/Frameworks

OBJS = main.o fluid.o viewer.o trackball.o genfunc.o

%.o: %.cpp
	g++ -o $@ -c $< $(FLAGS)

%.o: %.c
	gcc -o $@ -c $< $(FLAGS)

all: $(OBJS)
	g++ -o smoke $(OBJS) $(LINKFLAGS) 

clean:
	rm -f $(OBJS) smoke
