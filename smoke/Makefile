#CPPFLAGS = -O3 -framework OpenGL -framework SDL -I/Library/Frameworks/SDL.framework/Headers -F/Library/Frameworks
CPPFLAGS = -O3 -I/Library/Frameworks/SDL2.framework/Headers
LINKFLAGS = -O3 -framework OpenGL -framework SDL2 -F/Library/Frameworks

OBJS = main.o fluid.o viewer.o trackball.o genfunc.o

%.o: %.cpp
	g++ -g -o $@ -c $< $(CPPFLAGS)

all: $(OBJS)
	g++ -o fluid $(OBJS) $(LINKFLAGS) 

clean:
	rm -f $(OBJS) fluid
