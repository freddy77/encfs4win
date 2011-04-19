#include <stdio.h>
#include <stdlib.h>

extern "C" int main_encfs(int argc, char **argv);

int main(int argc, char **argv)
{
	return main_encfs(argc, argv);
}
