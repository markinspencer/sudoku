#include <windows.h>
#include <stdbool.h>
#include <stdint.h>

#define internal static
#define local_persist static
#define global_variable static

typedef struct 
{
	BITMAPINFO info;
	void *memory;
	int width;
	int height;
	int pitch;
	int bytesPerPixel;
} Win32OffscreenBuffer;

typedef struct  
{
	int width;
	int height;
} Win32WindowDimension;

//TODO(spencer): Remove this from global scope
global_variable bool RUNNING;
global_variable Win32OffscreenBuffer GLOBAL_BACK_BUFFER;

internal Win32WindowDimension
GetWindowDimension(HWND window) 
{
	Win32WindowDimension dimension;
	RECT client_rect;
	GetClientRect(window, &client_rect);

	dimension.width = client_rect.right - client_rect.left;
	dimension.height = client_rect.bottom - client_rect.top;

	return dimension;
}

internal void
RenderGradient(Win32OffscreenBuffer buffer, int blue_offset, int green_offset) 
{
	int width = buffer.width;
	int height = buffer.height;

	uint8_t *row = (uint8_t *)buffer.memory;
	for (int y = 0; y < height; ++y) {
		uint32_t *pixel = (uint32_t *)row;
		
		for (int x = 0; x < width; ++x) {
			uint8_t blue = (x + blue_offset);
			uint8_t green = (y + green_offset);
			
			*pixel++ = ((green << 8) | blue);
		}

		row += buffer.pitch;
	}
}

internal void
Win32ResizeDIBSection(Win32OffscreenBuffer *buffer, int width, int height)
{
	if(buffer->memory) {
		VirtualFree(buffer->memory, 0, MEM_RELEASE);
	}
	
	buffer->width = width;
	buffer->height = height;
	buffer->bytesPerPixel = 4;
 
	buffer->info.bmiHeader.biSize = sizeof(buffer->info.bmiHeader);
	buffer->info.bmiHeader.biWidth = width;
	buffer->info.bmiHeader.biHeight = -height;
	buffer->info.bmiHeader.biPlanes = 1;
	buffer->info.bmiHeader.biBitCount = 32;
	buffer->info.bmiHeader.biCompression = BI_RGB;
	buffer->info.bmiHeader.biSizeImage = 8;

	int bitmap_memory_size = (buffer->width * buffer->height) * buffer->bytesPerPixel;
	
	buffer->memory = VirtualAlloc(0, bitmap_memory_size, MEM_COMMIT, PAGE_READWRITE);
	buffer->pitch = buffer->width * buffer->bytesPerPixel;

	// TODO(spencer): clear to black
}

internal void
Win32CopyBufferToWindow(
	HDC device_context, 
	Win32OffscreenBuffer buffer, 
	int x, 
	int y,
	int width,
	int height)
{
	StretchDIBits(
		device_context,
		0, 0, buffer.width, buffer.height,
		0, 0, width, height,
		buffer.memory,
		&buffer.info,
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

	switch (message) {
		case WM_SIZE: {
			Win32WindowDimension dimension = GetWindowDimension(window);
			Win32ResizeDIBSection(&GLOBAL_BACK_BUFFER, dimension.width, dimension.height);
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
			
			Win32WindowDimension dimension = GetWindowDimension(window);
			
			Win32CopyBufferToWindow(
				device_context, 
				GLOBAL_BACK_BUFFER, 
				x, 
				y, 
				dimension.width, 
				dimension.height);

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

	window_class.style = CS_HREDRAW|CS_VREDRAW;
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
				
				RenderGradient(GLOBAL_BACK_BUFFER, x_offset, y_offset);
				HDC device_context = GetDC(window);
				Win32WindowDimension dimension = GetWindowDimension(window);

				Win32CopyBufferToWindow(
					device_context, 
					GLOBAL_BACK_BUFFER, 
					0, 
					0, 
					dimension.width, 
					dimension.height);

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
