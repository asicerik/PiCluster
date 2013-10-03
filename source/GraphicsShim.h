#pragma once

static const uint32_t kMaxFramebufferWidth	= 4096;
static const uint32_t kMaxFramebufferHeight	= 4096;
static const uint32_t kMaxFramebufferDepth	= 32;

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

bool CreatePrimaryFramebuffer(VideoCoreFramebufferDescriptor& fbDesc);

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
