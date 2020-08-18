
//#include "stdafx.h"
#include "SDLVideoRender.h"
#include <stdio.h>

static const int DEFAULT_WIDTH = 640;
static const int DEFAULT_HEIGHT = 480;
static const int DEFAULT_SCREEN_LEFT = 20;
static const int DEFAULT_SCREEN_TOP = 30;

SDLVideoRender::SDLVideoRender()
	: m_sdl_window(nullptr)
	, m_sdl_renderer(nullptr)
{
	m_show_rect.x = m_show_rect.y = 0;
	m_show_rect.w = DEFAULT_WIDTH;
	m_show_rect.h = DEFAULT_HEIGHT;
}

SDLVideoRender::~SDLVideoRender()
{
	Deinit();
}


bool SDLVideoRender::Init()
{
	if (nullptr != m_sdl_window)
	{
		return true;
	}

	/*int flags = SDL_INIT_VIDEO | SDL_INIT_TIMER;
	if (SDL_Init(flags)) {
		printf("SDL init err %d\n", SDL_GetError());
		return false;
	}*/

	// 查询VIDEO子系统是否初始化，如果没有的话，初始化
	if (0 == SDL_WasInit(SDL_INIT_VIDEO))
	{
		SDL_InitSubSystem(SDL_INIT_VIDEO);
	}

	int flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN | SDL_WINDOW_MOUSE_CAPTURE;
	//m_sdl_window = SDL_CreateWindowFrom(show_wnd);
	m_sdl_window = SDL_CreateWindow("sdl_render", DEFAULT_SCREEN_LEFT, DEFAULT_SCREEN_TOP, DEFAULT_WIDTH, DEFAULT_HEIGHT, flags);
	if (nullptr == m_sdl_window)
	{
		printf("SDL CreateWindow err %d\n", SDL_GetError());
		return false;
	}

	m_sdl_renderer = SDL_CreateRenderer(m_sdl_window, -1, SDL_RENDERER_ACCELERATED);
	if (nullptr == m_sdl_renderer)
	{
		Deinit();
		return false;
	}

	return true;
}

void SDLVideoRender::updateWindowSize(int w, int h)
{
	SDL_SetWindowSize(m_sdl_window, w, h);
	SDL_ShowWindow(m_sdl_window);
}

void SDLVideoRender::Deinit()
{
	if (nullptr != m_sdl_renderer)
	{
		SDL_DestroyRenderer(m_sdl_renderer);
		m_sdl_renderer = nullptr;
	}

	if (nullptr != m_sdl_window)
	{
		SDL_DestroyWindow(m_sdl_window);
		m_sdl_window = nullptr;
	}

	//SDL_Quit();
}