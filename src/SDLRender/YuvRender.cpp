//#include "StdAfx.h"
#include "YuvRender.h"
#include <cstdio>
#include <stdlib.h>
#include <string.h>

YuvRender::YuvRender(void)
	: SDLVideoRender()
	, m_in_width(0), m_in_height(0)
	, m_show_texture(nullptr)
{

}

YuvRender::~YuvRender(void)
{
	Deinit();
}

void YuvRender::Deinit()
{
	if (nullptr != m_show_texture)
	{
		SDL_DestroyTexture(m_show_texture);
		m_show_texture = NULL;
	}

	SDLVideoRender::Deinit();
}

bool YuvRender::CreateTexture(int width, int height)
{
	if (nullptr != m_show_texture)
	{
		SDL_DestroyTexture(m_show_texture);
		m_show_texture = NULL;
	}

	printf("CreateTexture w: %d, h: %d\n", width, height);

	SDL_assert(width > 0 && width < 10000);
	SDL_assert(height > 0 && height < 10000);

	m_show_texture = SDL_CreateTexture(m_sdl_renderer, SDL_PIXELFORMAT_IYUV, 
		SDL_TEXTUREACCESS_STREAMING, width, height);
	if (nullptr != m_show_texture)
	{
		m_in_width = width;
		m_in_height = height;
	}

	m_show_rect.x = 0;
	m_show_rect.y = 0;
	m_show_rect.w = m_in_width;
	m_show_rect.h = m_in_height;

	return NULL != m_show_texture;
}
void YuvRender::FillTexture(unsigned char *data[3], int stride[3])
{
	void * pixel = NULL;
	int pitch = 0;
	if(0 == SDL_LockTexture(m_show_texture, NULL, &pixel, &pitch))
	{
		// for Y
		int h = m_in_height;
		int w = m_in_width;
		unsigned char * dst = reinterpret_cast<unsigned char *>(pixel);
		unsigned char * src = data[0];
		for (int i = 0; i < h; ++i)
		{
			memcpy(dst, src, w);
			dst += pitch;
			src += stride[0];
		}

		h >>= 1;
		w >>= 1;
		pitch >>= 1;
		// for U
		src = data[1];
		for (int i = 0; i < h; ++i)
		{
			memcpy(dst, src, w);
			dst += pitch;
			src += stride[1];
		}

		// for V
		src = data[2];
		for (int i = 0; i < h; ++i)
		{
			memcpy(dst, src, w);
			dst += pitch;
			src += stride[2];
		}
		SDL_UnlockTexture(m_show_texture);
	}
}

// width x height resolution
// data[] for Y\U\V, stride is linesize of each raw
void YuvRender::Update(int width, int height, unsigned char *data[3], int stride[3])
{
	if (nullptr == m_show_texture || m_in_height != height || m_in_width != width)
	{
		CreateTexture(width, height);
		updateWindowSize(m_in_width, m_in_height);
	}

	if (nullptr != m_show_texture)
	{
		FillTexture(data, stride);
	}
}

bool YuvRender::Render()
{
	if (NULL != m_show_texture)
	{
		SDL_RenderCopy(m_sdl_renderer, m_show_texture, NULL, &m_show_rect);    
		SDL_RenderPresent(m_sdl_renderer); 
	}

	return true;
}
