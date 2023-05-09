//tcc -Wall -luser32 hello.c

#ifdef _WIN
#include <windows.h> //*or on unix*/ #include <stdlib.h>
#endif
#ifndef _WO
//This declaration is required to make printf external.
int printf(char *format, ...);
#endif
//See tinycc.c on how to do argc and argv
int main()
{
#ifdef _WIN
	MessageBox(NULL, NULL, "Hello World", 0); //windows only
#endif
#ifndef _WO
	printf("Hello World\n"); //crt is usable
#endif
	return 1;
}