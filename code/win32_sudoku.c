#include <windows.h>
#include <stdbool.h>

#define internal static
#define local_persist static
#define global_variable static

//TODO(spencer): Remove this from global scope
global_variable bool running;
 
internal HWND 
NewWindowHandle(HINSTANCE instance, LPCSTR className) 
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

internal void 
DrawToWindow(HWND window) 
{
	PAINTSTRUCT paint;
	HDC device_context = BeginPaint(window, &paint);

	int x = paint.rcPaint.left;
	int y = paint.rcPaint.top;
	LONG w = paint.rcPaint.right - paint.rcPaint.left;
	LONG h = paint.rcPaint.bottom - paint.rcPaint.top;

	PatBlt(device_context, x, y, w, h, WHITENESS);

	EndPaint(window, &paint);
}

LRESULT CALLBACK
MainWindowCallback(
	HWND window,
	UINT message,
	WPARAM w_param,
	LPARAM l_param
) 
{
	LRESULT result;

	switch(message) {
		case WM_SIZE: 
			OutputDebugString("WM_SIZE\n");
			break;
		case WM_CLOSE: 
			//TODO(spencer): Handle this with a message to the user?
			running = false;
			break;
		case WM_ACTIVATEAPP:
			OutputDebugString("WM_ACTIVATEAPP\n");
			break;
		case WM_PAINT:
			DrawToWindow(window);
			break;
		case WM_DESTROY:
			//TODO(spencer): Handle this as an error - recreate window?
			running = false;
			break;
		default:
			result = DefWindowProc(window, message, w_param, l_param);
			break;
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
	window_class.lpfnWndProc = MainWindowCallback;
	window_class.hInstance = instance;
	window_class.lpszClassName = "SudokuClass";

	if(RegisterClass(&window_class)) {
		HWND window_handle = NewWindowHandle(instance, window_class.lpszClassName);

		if(window_handle) {
			running = true;
			while(running) {
				MSG message;
				BOOL message_result = GetMessage(&message, 0 ,0 ,0);

				if(message_result > 0) {
					TranslateMessage(&message);
					DispatchMessage(&message);
				}
				else {
					break;
				}
			}
		} else {
			//TODO(spencer): Logging
		}
	}

	return 0;
}

