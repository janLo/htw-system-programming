all: auf1 auf2a auf2b

auf1: auf1.c
	gcc -o auf1 auf1.c

auf2a: auf2.c
	gcc -o auf2a -DUSE_PS auf2.c

auf2b: auf2.c
	gcc -o auf2b auf2.c

clean:
	rm -f auf1 auf2a auf2b
