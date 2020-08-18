
#pragma  once
// 程序退出时建议调用SDL_Quit()函数，用于清理SDL库

#include <SDL.h>
//#pragma comment(lib, "SDL2.lib")

class SDLVideoRender
{
public:
	SDLVideoRender();
	virtual ~SDLVideoRender();

	//virtual bool Init(HWND show_wnd, RECT show_rect);
	virtual bool Init();
	virtual void Deinit();

	// YUV分辨率为width*height
	// data[]分别存储Y\U\V数据, stride[]为每个分量的行宽度
	virtual void Update(int width, int height, unsigned char *data[3], int stride[3]) = 0;
	virtual bool Render() = 0;

protected:
	void updateWindowSize(int w, int h);
protected:
	SDL_Window * m_sdl_window;
	SDL_Renderer * m_sdl_renderer;
	SDL_Rect m_show_rect;
};