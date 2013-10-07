#pragma once

#include <vector>

static const uint32_t kMaxFramebufferWidth	= 4096;
static const uint32_t kMaxFramebufferHeight	= 4096;
static const uint32_t kMaxFramebufferDepth	= 32;

#define SATURATE_II(x) (((x) > 255) ? 255 : ((x) < 0) ? 0 : (x))
#define SATURATE_FF(x) (((x) > 255) ? 255 : ((x) < 0) ? 0 : (x))
#define SATURATE_FI(x) (((x) > 255.0) ? 255 : ((x) < 0.0) ? 0 : (uint8_t)(x))

struct Point
{
	int16_t		x;
	int16_t		y;
};

struct Rect
{
	int16_t		x;
	int16_t		y;
	int16_t		w;
	int16_t		h;
};

struct Color32
{
	uint8_t		a;
	uint8_t		b;
	uint8_t		g;
	uint8_t		r;
};

struct Color32f
{
	float		a;
	float		b;
	float		g;
	float		r;
	Color32 ToColor32()
	{
		Color32 out;
		out.a = SATURATE_FI(a);
		out.r = SATURATE_FI(r);
		out.g = SATURATE_FI(g);
		out.b = SATURATE_FI(b);
		return out;
	}
};

// A color point in x/y space
struct GradientStop
{
	float		mPosition;
	Color32		mColor;
};

enum Opacity
{
	eTransparent	= 0,
	e25Percent		= 64,
	e50Percent		= 128,
	e75Percent		= 192,
	eOpaque			= 255
};

struct FramebufferProperties
{
	FramebufferProperties()
	{
		mBitsPerPixel 	= 32;
		mStride			= 0;
	}
	Rect		mGeometry;
	int8_t		mBitsPerPixel;
	int32_t		mStride;
};

class GraphicsContextBase
{
public:
	GraphicsContextBase() {};
	virtual ~GraphicsContextBase() {};

	virtual bool	AllocatePrimaryFramebuffer(FramebufferProperties& properties) = 0;
	virtual bool	AllocateFramebuffer(FramebufferProperties& properties) = 0;
	virtual void	FreeFramebuffer() = 0;
	virtual void	FillRectangle(Rect rect, Color32 argb) = 0;
	virtual void	GradientRectangle(int16_t angle, std::vector<GradientStop>& stops) = 0;
	void SetClippingRect(Rect& rect)	{ mClippingRect = rect; };
	Rect GetClippingRect()				{ return mClippingRect; };
	Color32* GetBackBuffer()	{ return mBackBufferPtr; };
	Color32* GetFrontBuffer()	{ return mFrontBufferPtr; };
	
protected:
	Color32*				mFrontBufferPtr;		//!< Pointer to the active framebuffer data
	Color32*				mBackBufferPtr;			//!< Pointer to the inactive framebuffer data (if present)
	FramebufferProperties	mPrimaryFBProperties;	//!< The main framebuffer(s) for screen drawing
	FramebufferProperties	mFBProperties;			//!< Properties for this framebuffer
	Rect					mClippingRect;			//!< Clipping rectangle from drawing functions
};

#ifdef WIN32
class GraphicsContextWin : public GraphicsContextBase
{
public:
	virtual bool	AllocatePrimaryFramebuffer(FramebufferProperties& properties);
	virtual bool	AllocateFramebuffer(FramebufferProperties& properties);
	virtual void	FreeFramebuffer();
	virtual void	FillRectangle(Rect rect, Color32 argb);
	virtual void	GradientRectangle(int16_t angle, std::vector<GradientStop>& stops);
	Color32* GetBackBuffer();
	Color32* GetFrontBuffer();
	
protected:
	bool AllocateWindowsBitmap(FramebufferProperties& properties, HDC& dc, BITMAPINFO& info, HBITMAP& bitmap);
	HDC						mDC;
	BITMAPINFO				mBitmapInfo;
	HBITMAP					mBitmap;
	HDC						mPrimaryDC;
	BITMAPINFO				mPrimaryBitmapInfo;
	HBITMAP					mPrimaryBitmap;
};
typedef GraphicsContextWin GraphicsContext;
#else
class GraphicsContextPi : public GraphicsContextBase
{
public:
	GraphicsContextPi();
	virtual ~GraphicsContextPi();

	virtual bool	AllocatePrimaryFramebuffer(FramebufferProperties& properties);
	virtual bool	AllocateFramebuffer(FramebufferProperties& properties);
	virtual void	FreeFramebuffer();
	virtual void	FillRectangle(Rect rect, Color32 argb);
	virtual void	GradientRectangle(int16_t angle, std::vector<GradientStop>& stops);
	
private:
};
typedef GraphicsContextPi GraphicsContext;

// VideoCore mailbox registers
static uint32_t* VideoCoreMailboxRead		= (uint32_t*)0x2000B880;
static uint32_t* VideoCoreMailboxPeek		= (uint32_t*)0x2000B890;
static uint32_t* VideoCoreMailboxSender		= (uint32_t*)0x2000B894;
static uint32_t* VideoCoreMailboxStatus		= (uint32_t*)0x2000B898;
static uint32_t* VideoCoreMailboxConfig		= (uint32_t*)0x2000B89C;
static uint32_t* VideoCoreMailboxWrite		= (uint32_t*)0x2000B8A0;

// Add this value to mailbox addresses to prevent the VPU from writing the
// result in a cache, which means we won't see it until the cache flushes
static uint32_t  kVideoCoreMailboxDisableCache = 0x40000000;

// VideoCore mailbox status register bits
// The actual bit is active low (0 = ready)
static const uint32_t VideoCoreMailboxStatusWriteReady	= 0x80000000;
static const uint32_t VideoCoreMailboxStatusReadReady	= 0x40000000;

// VideoCore mailbox channels
enum VideoCoreChannel
{
	eVCPowerManagementChannel,
	eVCFramebufferChannel,
	eVCVirtualUARTChannel,
	eVCVHCIQChannel,
	eVCLEDChannel,
	eVCButtonChannel,
	eVCTouchscreenChannel
};

// VideoCore mailbox functions
bool MailboxWrite(uintptr_t message, enum VideoCoreChannel channel);
bool MailboxRead(uintptr_t& message, enum VideoCoreChannel channel);

// VideoCore framebuffer structure
// NOTE : make sure you align your memory to a 16B boundary
struct VideoCoreFramebufferDescriptor
{
	VideoCoreFramebufferDescriptor()
	{
		width			= 0;
		height			= 0;
		virtualWidth	= 0;
		virtualHeight	= 0;
		pitch			= 0;
		depth			= 0;
		xOffset			= 0;
		yOffset			= 0;
		buffer			= 0;
		bufferSize		= 0;
	}
	uint32_t	width;
	uint32_t	height;
	uint32_t	virtualWidth;
	uint32_t	virtualHeight;
	uint32_t	pitch;
	uint32_t	depth;
	uint32_t	xOffset;
	uint32_t	yOffset;
	uint32_t*	buffer;
	uint32_t	bufferSize;
}  __attribute__ ((aligned(16)));


#endif
