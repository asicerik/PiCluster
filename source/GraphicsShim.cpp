#include "stdint.h"
#ifdef WIN32
#include "windows.h"
#endif
#include "GraphicsShim.h"

#ifdef WIN32
GraphicsContext* CreateGraphicsContext()
{
	GraphicsContext* ctx = new GraphicsContext();

	if (ctx != NULL)
	{
		ctx->mDC = CreateCompatibleDC(NULL);
	}
	return ctx;
}

void FreeGraphicsContext(GraphicsContext* ctx)
{
	if (ctx != NULL)
	{
		DeleteDC(ctx->mDC);
		free(ctx);
	}
}

int32_t AllocateFrameBuffer(GraphicsContext* ctx)
{
	if (ctx == NULL || ctx->mDC == NULL || (ctx->mWidth < 1) || (ctx->mHeight < 1) || (ctx->mBitsPerPixel != 32))
		return -1;

	ctx->mBitmapInfo.bmiHeader.biSize				= sizeof( ctx->mBitmapInfo.bmiHeader );
	ctx->mBitmapInfo.bmiHeader.biWidth				= ctx->mWidth;
	ctx->mBitmapInfo.bmiHeader.biHeight				= -ctx->mHeight;
	ctx->mBitmapInfo.bmiHeader.biPlanes				= 1;
	ctx->mBitmapInfo.bmiHeader.biCompression		= BI_RGB;
	ctx->mBitmapInfo.bmiHeader.biBitCount			= ctx->mBitsPerPixel;
	ctx->mBitmapInfo.bmiHeader.biSizeImage			= (ctx->mBitsPerPixel/8)*ctx->mWidth*ctx->mHeight; 

	// Create the source bitmap.
	ctx->mBitmap = CreateDIBSection( ctx->mDC, &ctx->mBitmapInfo, DIB_RGB_COLORS, (void **)&ctx->mABGR, NULL, 0 );

	// Now select the back buffer.
	SelectObject( ctx->mDC, ctx->mBitmap );

	return 0;
}

void FreeFrameBuffer(GraphicsContext* ctx)
{
	DeleteObject(ctx->mBitmap);
	ctx->mBitmap = NULL;
	ctx->mBitmap = NULL;
}

void FillRectangle(GraphicsContext* ctx, Rect rect, Color32 argb)
{
	RECT winRect;
	HBRUSH brush;
	winRect.left	= rect.x;
	winRect.right	= rect.x + rect.w;
	winRect.top		= rect.y;
	winRect.bottom	= rect.y + rect.h;
	brush = CreateSolidBrush(RGB(argb.r, argb.g, argb.b));
	FillRect(ctx->mDC, &winRect, brush);
	DeleteObject(brush);
}

#else
GraphicsContext* CreateGraphicsContext()
{
	return 0;
}

void FreeGraphicsContext(GraphicsContext* ctx)
{
}

int32_t AllocateFrameBuffer(GraphicsContext* ctx)
{
	return -1;
}

void FreeFrameBuffer(GraphicsContext* ctx)
{
}

#endif
