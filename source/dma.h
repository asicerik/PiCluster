#pragma once

#ifdef WIN32
#define PACK
#define ALIGN
#define ALIGN_32
#else
#define PACK __attribute__((__packed__))
#define ALIGN __attribute__ ((aligned (4)))		// 32 bit alignment on ARM
#define ALIGN_32 __attribute__ ((aligned (32)))	// 32 byte (256 bit) alignment on ARM
#endif

// NOTE : this structure needs to be 256-bit aligned
struct DmaControlBlockData
{
	uint32_t	TransferInformation;
	uint32_t	SourceAddress;
	uint32_t	DestinationAddress;
	uint32_t	TransferLength;
	uint32_t	Stride2D;
	uint32_t	NextControlBlock;
	uint32_t	reserved0;
	uint32_t	reserved1;
};

// Control/Status register
#define DMA_CS_ACTIVE			0x00000001
#define DMA_CS_END				0x00000002
#define DMA_CS_INT				0x00000004
#define DMA_CS_DREQ				0x00000008
#define DMA_CS_PAUSED			0x00000010
#define DMA_CS_DREQ_STOPS_DMA	0x00000020
#define DMA_CS_WAITING_FOR_WR	0x00000040
#define DMA_CS_ERROR			0x00000100
#define DMA_CS_PRI_MASK			0x000f0000
#define DMA_CS_PRI_SHIFT				16
#define DMA_CS_PANIC_PRI_MASK	0x00f00000
#define DMA_CS_PANIC_PRI_SHIFT			20
#define DMA_CS_WAIT_FOR_WR		0x10000000
#define DMA_CS_DISABLE_DEBUG	0x20000000
#define DMA_CS_ABORT			0x40000000
#define DMA_CS_RESET			0x80000000

// Transfer information register
#define DMA_TI_INTEN			0x00000001
#define DMA_TI_TD_MODE			0x00000002
#define DMA_TI_WAIT_RESP		0x00000008
#define DMA_TI_DEST_INC			0x00000010
#define DMA_TI_DEST_WIDTH		0x00000020
#define DMA_TI_DEST_DREQ		0x00000040
#define DMA_TI_DEST_IGNORE		0x00000080
#define DMA_TI_SRC_INC			0x00000100
#define DMA_TI_SRC_WIDTH		0x00000200
#define DMA_TI_SRC_DREQ			0x00000400
#define DMA_TI_SRC_IGNORE		0x00000800
#define DMA_TI_BURST_LEN_MASK	0x0000f000
#define DMA_TI_BURST_LEN_SHIFT			12
#define DMA_TI_PERMAP_MASK		0x001f0000
#define DMA_TI_PERMAP_SHIFT				16
#define DMA_TI_ADD_WAITS_MASK	0x03e00000
#define DMA_TI_ADD_WAITS_SHIFT			21
#define DMA_TI_NO_WIDE_BURSTS	0x04000000

// Forward declarations
struct Color32;
struct Rect;

bool dmaInit();
bool dmaBitBlt(Color32* dst, Rect& dstRect, uint32_t dstStride, Color32* src, Rect& srcRect, uint32_t srcStride);
void dmaDumpAll();
void dmaDumpChannel(uint8_t* base);

