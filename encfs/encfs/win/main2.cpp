#include <stdio.h>
#include <stdlib.h>

extern "C" int main_encfsctl(int argc, char **argv);

int main(int argc, char **argv)
{
	return main_encfsctl(argc, argv);
}
