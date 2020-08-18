
#pragma  once
// �����˳�ʱ�������SDL_Quit()��������������SDL��

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

	// YUV�ֱ���Ϊwidth*height
	// data[]�ֱ�洢Y\U\V����, stride[]Ϊÿ���������п��
	virtual void Update(int width, int height, unsigned char *data[3], int stride[3]) = 0;
	virtual bool Render() = 0;

protected:
	void updateWindowSize(int w, int h);
protected:
	SDL_Window * m_sdl_window;
	SDL_Renderer * m_sdl_renderer;
	SDL_Rect m_show_rect;
};