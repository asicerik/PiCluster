#pragma once

#ifdef WIN32
typedef struct 
{
	HDC			mDC;
	BITMAPINFO	mBitmapInfo;
	HBITMAP		mBitmap;
	uint32_t*	mABGR;
	int32_t		mWidth;
	int32_t		mHeight;
	int8_t		mBitsPerPixel;
	int32_t		mStride;
} GraphicsContext;
#else
typedef struct 
{
	uint32_t*	mABGR;
	int32_t		mWidth;
	int32_t		mHeight;
	int8_t		mBitsPerPixel;
	int32_t		mStride;
} GraphicsContext;

#endif

typedef struct
{
	int32_t		x;
	int32_t		y;
	int32_t		w;
	int32_t		h;
} Rect;
typedef struct
{
	uint8_t		a;
	uint8_t		r;
	uint8_t		g;
	uint8_t		b;
} Color32;

GraphicsContext*	CreateGraphicsContext();
void	FreeGraphicsContext(GraphicsContext* ctx);
int32_t	AllocateFrameBuffer(GraphicsContext* ctx);
void	FreeFrameBuffer(GraphicsContext* ctx);
void	FillRectangle(GraphicsContext* ctx, Rect rect, Color32 argb); 