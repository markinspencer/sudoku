#include <windows.h>
#include <stdbool.h>
#include <stdint.h>

#define internal static
#define local_persist static
#define global_variable static

//TODO(spencer): Remove this from global scope
global_variable bool RUNNING;
global_variable void *BITMAP_MEMORY;
global_variable BITMAPINFO BITMAP_INFO;
global_variable int BITMAP_WIDTH;
global_variable int BITMAP_HEIGHT;
global_variable int BYTES_PER_PIXEL = 4;

internal void
RenderGradient(int x_offset, int y_offset) 
{
	int width = BITMAP_WIDTH;
	int height = BITMAP_HEIGHT;

	int pitch = width * BYTES_PER_PIXEL;
	uint8_t *row = (uint8_t *)BITMAP_MEMORY;
	for (int y = 0; y < height; ++y) {
		uint32_t *pixel = (uint32_t *)row;
		
		for (int x = 0; x < width; ++x) {
			uint8_t blue = (x + x_offset);
			uint8_t green = (y + y_offset);
			
			*pixel++ = ((green << 8) | blue);
		}

		row += pitch;
	}
}

internal void
Win32ResizeDIBSection(int width, int height)
{

	if(BITMAP_MEMORY) {
		VirtualFree(BITMAP_MEMORY, 0, MEM_RELEASE);
	}
	
	BITMAP_WIDTH = width;
	BITMAP_HEIGHT = height;
 
	BITMAP_INFO.bmiHeader.biSize = sizeof(BITMAP_INFO.bmiHeader);
	BITMAP_INFO.bmiHeader.biWidth = BITMAP_WIDTH;
	BITMAP_INFO.bmiHeader.biHeight = -BITMAP_HEIGHT;
	BITMAP_INFO.bmiHeader.biPlanes = 1;
	BITMAP_INFO.bmiHeader.biBitCount = 32;
	BITMAP_INFO.bmiHeader.biCompression = BI_RGB;
	BITMAP_INFO.bmiHeader.biSizeImage = 8;

	int bitmap_memory_size = (BITMAP_WIDTH * BITMAP_HEIGHT) * BYTES_PER_PIXEL;
	
	BITMAP_MEMORY = VirtualAlloc(0, bitmap_memory_size, MEM_COMMIT, PAGE_READWRITE);

	// TODO(spencer): clear to black
}

internal void
Win32UpdateWindow(HDC device_context, RECT *client_rect, int x, int y, int width, int height)
{
	int window_width = client_rect -> right - client_rect -> left;
	int window_height = client_rect -> bottom - client_rect -> top;

	StretchDIBits(
		device_context,
		0, 0, BITMAP_WIDTH, BITMAP_HEIGHT,
		0, 0, window_width, window_height,
		BITMAP_MEMORY,
		&BITMAP_INFO,
		DIB_RGB_COLORS, 
		SRCCOPY);
}

internal HWND
Win32CreateWindow(HINSTANCE instance, LPCSTR className)
{
	return CreateWindowEx(
		0,
		className,
		"Sudoku",
		WS_OVERLAPPEDWINDOW|WS_VISIBLE,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		0,
		0,
		instance,
		0);
}

LRESULT CALLBACK
Win32MainWindowCallback(
	HWND window,
	UINT message,
	WPARAM w_param,
	LPARAM l_param)
{
	LRESULT result;

	switch (message)
	{
		case WM_SIZE: {
			RECT client_rect;
			GetClientRect(window, &client_rect);
			int width = client_rect.right - client_rect.left;
			int height = client_rect.bottom - client_rect.top;
			Win32ResizeDIBSection(width, height);
			break;
		}
		case WM_CLOSE: {
			//TODO(spencer): Handle this with a message to the user?
			RUNNING = false;
			break;
		}
		case WM_ACTIVATEAPP: {
			OutputDebugString("WM_ACTIVATEAPP\n");
			break;
		}
		case WM_PAINT: {
			PAINTSTRUCT paint;
			HDC device_context = BeginPaint(window, &paint);
			int x = paint.rcPaint.left;
			int y = paint.rcPaint.top;
			LONG width = paint.rcPaint.right - paint.rcPaint.left;
			LONG height = paint.rcPaint.bottom - paint.rcPaint.top;

			RECT client_rect;
			GetClientRect(window, &client_rect);

			Win32UpdateWindow(device_context, &client_rect, x, y, width, height);

			EndPaint(window, &paint);
			break;
		}
		case WM_DESTROY: {
			//TODO(spencer): Handle this as an error - recreate window?
			RUNNING = false;
			break;
		}
		default: {
			result = DefWindowProc(window, message, w_param, l_param);
			break;
		}
	}

	return result;
}

int CALLBACK
WinMain(
	HINSTANCE instance,
	HINSTANCE previous_instance,
	LPSTR command_line,
	int show_code)
{

	WNDCLASS window_class = {0};

	window_class.style = CS_OWNDC|CS_HREDRAW|CS_VREDRAW;
	window_class.lpfnWndProc = Win32MainWindowCallback;
	window_class.hInstance = instance;
	window_class.lpszClassName = "SudokuClass";

	if (RegisterClass(&window_class))	{
		HWND window = Win32CreateWindow(instance, window_class.lpszClassName);

		if (window) {
			int x_offset = 0;
			int y_offset = 0;
			RUNNING = true;
			while (RUNNING)	{
				
				MSG message;
				
				while (PeekMessage(&message, 0, 0, 0, PM_REMOVE)) {
					if(message.message == WM_QUIT) {
						RUNNING = false;
					}
					
					TranslateMessage(&message);
					DispatchMessage(&message);
				}
				
				RenderGradient(x_offset, y_offset);
				HDC device_context = GetDC(window);
				RECT client_rect;
				GetClientRect(window, &client_rect);
				int window_width = client_rect.right - client_rect.left;
				int window_height = client_rect.bottom - client_rect.top;

				Win32UpdateWindow(device_context, &client_rect, 0, 0, window_width, window_height);

				ReleaseDC(window, device_context);
				++x_offset;
			}
		}
		else {
			//TODO(spencer): Logging
		}
	}

	return 0;
}
