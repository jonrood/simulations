FLAGS = -O3 -I/Library/Frameworks/SDL2.framework/Headers
LINKFLAGS = -O3 -framework OpenGL -framework SDL2 -F/Library/Frameworks

OBJS = fluid.o main.o genfunc.o viewer.o trackball.o FragmentUtils.o spectrum.o 

%.o: %.cpp
	g++ -o $@ -c $< $(FLAGS)

%.o: %.c
	gcc -o $@ -c $< $(FLAGS)

all: $(OBJS)
	g++ -o fire $(OBJS) $(LINKFLAGS)

clean:
	rm -f $(OBJS) fire
