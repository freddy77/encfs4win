#include <windows.h>

extern "C" int main_gui(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine,int nCmdShow);

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine,int nCmdShow)
{
	return main_gui(hInstance, hPrevInstance, lpCmdLine, nCmdShow);
}

