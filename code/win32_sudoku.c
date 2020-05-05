#include <windows.h>
#include <stdbool.h>

#define internal static
#define local_persist static
#define global_variable static

global_variable void *BitmapMemory;
global_variable BITMAPINFO bitmap_info;
global_variable HBITMAP bitmap_handle;
global_variable HDC bitmap_device_context;

//TODO(spencer): Remove this from global scope
global_variable bool running;

internal void
Win32ResizeDIBSection(int width, int height)
{
	// TODO(spencer): Bulletproof this.
	if (bitmap_handle) {
		DeleteObject(bitmap_handle);
	}

	if (!bitmap_device_context) {
		bitmap_device_context = CreateCompatibleDC(0);
	}

	bitmap_info.bmiHeader.biSize = sizeof(bitmap_info.bmiHeader);
	bitmap_info.bmiHeader.biWidth = width;
	bitmap_info.bmiHeader.biHeight = height;
	bitmap_info.bmiHeader.biPlanes = 1;
	bitmap_info.bmiHeader.biBitCount = 32;
	bitmap_info.bmiHeader.biCompression = BI_RGB;
	bitmap_info.bmiHeader.biSizeImage = 8;

	bitmap_handle = CreateDIBSection(
		bitmap_device_context,
		&bitmap_info,
		DIB_RGB_COLORS,
		&BitmapMemory,
		0,
		0);
}

internal void
Win32UpdateWindow(HDC device_context, int x, int y, int width, int height)
{
	StretchDIBits(
		device_context,
		x, y, width, height,
   	x, y,	width, height,
		BitmapMemory,
		&bitmap_info,
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
			running = false;
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
			Win32UpdateWindow(device_context, x, y, width, height);

			EndPaint(window, &paint);
			break;
		}
		case WM_DESTROY: {
			//TODO(spencer): Handle this as an error - recreate window?
			running = false;
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
		HWND window_handle = Win32CreateWindow(instance, window_class.lpszClassName);

		if (window_handle) {
			running = true;
			while (running)	{
				MSG message;
				BOOL message_result = GetMessage(&message, 0, 0, 0);

				if (message_result > 0) {
					TranslateMessage(&message);
					DispatchMessage(&message);
				}
				else {
					break;
				}
			}
		}
		else {
			//TODO(spencer): Logging
		}
	}

	return 0;
}
