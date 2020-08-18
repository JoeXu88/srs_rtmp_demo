#pragma once
#include "sdlvideorender.h"

class YuvRender :public SDLVideoRender
{
public:
	YuvRender(void);
	~YuvRender(void);

	// Init use parent impl
	//bool Init(HWND show_wnd, RECT show_rect);
	void Deinit();

	// width x height resolution
	// data[] for Y\U\V, stride is linesize of each raw
	void Update(int width, int height, unsigned char *data[3], int stride[3]);
	bool Render();

private:
	bool CreateTexture(int width, int height);
	void FillTexture(unsigned char *data[3], int stride[3]);

private:
	// texture size
	int m_in_width, m_in_height;
	SDL_Texture * m_show_texture;
};

