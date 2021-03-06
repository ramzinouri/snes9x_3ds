#include "copyright.h"

#ifndef _TILE_H_
#define _TILE_H_

#define SUBSCREEN_DELTA  524288
//#define TILE_AssignPixel(N, value) Screen[N]=(value);Depth[N]=GFX.Z2;
#define TILE_AssignPixel(N, value) Screen[N]=(value);
#define TILE_AssignTwoPixels(N, value) Screen32[N]=(value);

#define TILE_SetPixel(N, Pixel)   TILE_AssignPixel(N, (uint8) GFX.ScreenColors [Pixel]);
#define TILE_SetPixel16(N, Pixel) TILE_AssignPixel(N, GFX.ScreenColors [Pixel]);

#define TILE_AddPixel16(N, Pixel)      if (Screen[SUBSCREEN_DELTA+N]) { TILE_AssignPixel(N, COLOR_ADD(GFX.ScreenColors[Pixel], Screen[SUBSCREEN_DELTA+N])); } else { TILE_SetPixel16(N, Pixel) }
#define TILE_AddFPixel16(N, Pixel)     TILE_AssignPixel(N, COLOR_ADD(GFX.ScreenColors[Pixel], GFX.FixedColour));
#define TILE_AddPixel16Half(N, Pixel)  if (Screen[SUBSCREEN_DELTA+N]) { TILE_AssignPixel(N, (uint16)COLOR_ADD1_2(GFX.ScreenColors[Pixel], Screen[SUBSCREEN_DELTA+N])); } else { TILE_SetPixel16(N, Pixel) }
#define TILE_AddFPixel16Half(N, Pixel) TILE_AssignPixel(N, (uint16)COLOR_ADD1_2(GFX.ScreenColors[Pixel], GFX.FixedColour));

#define TILE_SubPixel16(N, Pixel)      if (Screen[SUBSCREEN_DELTA+N]) { TILE_AssignPixel(N, COLOR_SUB(GFX.ScreenColors[Pixel], Screen[SUBSCREEN_DELTA+N])); } else { TILE_SetPixel16(N, Pixel) }
#define TILE_SubFPixel16(N, Pixel)     TILE_AssignPixel(N, COLOR_SUB(GFX.ScreenColors[Pixel], GFX.FixedColour));
#define TILE_SubPixel16Half(N, Pixel)  if (Screen[SUBSCREEN_DELTA+N]) { TILE_AssignPixel(N, (uint16)COLOR_SUB1_2(GFX.ScreenColors[Pixel], Screen[SUBSCREEN_DELTA+N])); } else { TILE_SetPixel16(N, Pixel) }
#define TILE_SubFPixel16Half(N, Pixel) TILE_AssignPixel(N, (uint16)COLOR_SUB1_2(GFX.ScreenColors[Pixel], GFX.FixedColour));

/*
#define TILE_Select3(N,Pixel,FAddType, AddType) \
    switch(SubDepth[N]) \
    { \
        case 0: TILE_SetPixel16(N, Pixel); break; \
        case 1: TILE_##FAddType(N, Pixel); break; \
        default: TILE_##AddType(N, Pixel); break; \
    }
    */
/*
#define TILE_Select2(N,Pixel,FAddType) \
    switch(SubDepth[N]) \
    { \
        case 1: TILE_##FAddType(N, Pixel); break; \
        default: TILE_SetPixel16(N, Pixel); break; \
    }
*/

#define TILE_Select3(N,Pixel,FAddType, AddType) \
    TILE_##AddType(N, Pixel); 
    
#define TILE_Select2(N,Pixel,FAddType) \
    TILE_##FAddType(N, Pixel); 
    

#define TILE_SelectAddPixel16(N, Pixel)     TILE_Select3(N,Pixel, AddFPixel16, AddPixel16)
#define TILE_SelectAddPixel16Half(N, Pixel) TILE_Select3(N,Pixel, AddFPixel16, AddPixel16Half)
#define TILE_SelectSubPixel16(N, Pixel)     TILE_Select3(N,Pixel, SubFPixel16, SubPixel16)
#define TILE_SelectSubPixel16Half(N, Pixel) TILE_Select3(N,Pixel, SubFPixel16, SubPixel16Half)

#define TILE_SelectFAddPixel16Half(N, Pixel) TILE_Select2(N,Pixel, AddFPixel16Half)
#define TILE_SelectFSubPixel16Half(N, Pixel) TILE_Select2(N,Pixel, SubFPixel16Half)

/*******************/

#define TILE_PREAMBLE \
    uint8 *pCache; \
    uint16 *pCache16; \
\
    uint32 TileAddr = BG.TileAddress + ((Tile & 0x3ff) << BG.TileShift); \
    if ((Tile & 0x1ff) >= 256) \
	TileAddr += BG.NameSelect; \
\
    TileAddr &= 0xffff; \
\
    uint32 TileNumber; \
    pCache = &BG.Buffer[(TileNumber = (TileAddr >> BG.TileShift)) << 6]; \
\
    if (!BG.Buffered [TileNumber]) \
    { \
	    BG.Buffered[TileNumber] = ConvertTile (pCache, TileAddr); \ 
        if (BG.Buffered [TileNumber] == BLANK_TILE) \
            return; \  
        GFX.VRAMPaletteFrame[TileAddr][0] = 0; \
        GFX.VRAMPaletteFrame[TileAddr][1] = 0; \
        GFX.VRAMPaletteFrame[TileAddr][2] = 0; \
        GFX.VRAMPaletteFrame[TileAddr][3] = 0; \
        GFX.VRAMPaletteFrame[TileAddr][4] = 0; \
        GFX.VRAMPaletteFrame[TileAddr][5] = 0; \
        GFX.VRAMPaletteFrame[TileAddr][6] = 0; \
        GFX.VRAMPaletteFrame[TileAddr][7] = 0; \
        GFX.VRAMPaletteFrame[TileAddr][8] = 0; \
        GFX.VRAMPaletteFrame[TileAddr][9] = 0; \
        GFX.VRAMPaletteFrame[TileAddr][10] = 0; \
        GFX.VRAMPaletteFrame[TileAddr][11] = 0; \
        GFX.VRAMPaletteFrame[TileAddr][12] = 0; \
        GFX.VRAMPaletteFrame[TileAddr][13] = 0; \
        GFX.VRAMPaletteFrame[TileAddr][14] = 0; \
        GFX.VRAMPaletteFrame[TileAddr][15] = 0; \
    } \
\
    if (BG.Buffered [TileNumber] == BLANK_TILE) \
	    return; \
    \
\
    register uint32 l; \
    uint8 pal; \
    if (BG.DirectColourMode) \
    { \
        if (IPPU.DirectColourMapsNeedRebuild) \
            S9xBuildDirectColourMaps (); \
        pal = (Tile >> 10) & BG.PaletteMask; \
        pCache16 = 0; \
        GFX.ScreenColors = DirectColourMaps [pal]; \
        if (GFX.VRAMPaletteFrame[TileAddr][pal] != GFX.PaletteFrame[pal + BG.StartPalette / 16]) \
        { \
            GFX.VRAMPaletteFrame[TileAddr][pal] = GFX.PaletteFrame[pal + BG.StartPalette / 16]; \
            ConvertTileTo16Bit(GFX.ScreenColors, pCache, pCache16); \
        } \
    } \
    else \
    { \
        pal = (Tile >> 10) & BG.PaletteMask; \
        pCache16 = 0; \
        GFX.ScreenColors = &IPPU.ScreenColors [(pal << BG.PaletteShift) + BG.StartPalette]; \
        if (GFX.VRAMPaletteFrame[TileAddr][pal] != GFX.PaletteFrame[pal + BG.StartPalette / 16]) \
        { \
            GFX.VRAMPaletteFrame[TileAddr][pal] = GFX.PaletteFrame[pal + BG.StartPalette / 16]; \
            ConvertTileTo16Bit(GFX.ScreenColors, pCache, pCache16); \
        } \
    }
    

   /* 
#define RENDER_TILE(NORMAL, FLIPPED, N) \
    if (!(Tile & (V_FLIP | H_FLIP))) \
    { \
    bp = (uint8 *)(pCache + StartLine); \
	for (l = LineCount; l != 0; l--, bp += 8, Offset += GFX.PPL) \
	{ \
		NORMAL (Offset, bp); \
		NORMAL (Offset + N, bp + 4); \
	} \
    } \
    else \
    if (!(Tile & V_FLIP)) \
    { \
    bp = (uint8 *)(pCache + StartLine); \
	for (l = LineCount; l != 0; l--, bp += 8, Offset += GFX.PPL) \
	{ \
		FLIPPED (Offset, bp + 4); \
		FLIPPED (Offset + N, bp); \
	} \
    } \
    else \
    if (Tile & H_FLIP) \
    { \
	bp = (uint8 *)(pCache + 56 - StartLine); \
	for (l = LineCount; l != 0; l--, bp -= 8, Offset += GFX.PPL) \
	{ \
		FLIPPED (Offset, bp + 4); \
		FLIPPED (Offset + N, bp); \
	} \
    } \
    else \
    { \
	bp = (uint8 *)(pCache + 56 - StartLine); \
	for (l = LineCount; l != 0; l--, bp -= 8, Offset += GFX.PPL) \
	{ \
		NORMAL (Offset, bp); \
		NORMAL (Offset + N, bp + 4); \
	} \
    }
    */

#define RENDER_TILE(NORMAL, FLIPPED, N) \
    if (!(Tile & (V_FLIP | H_FLIP))) \
    { \
    bp = (uint8 *)(pCache16 + StartLine); \
	for (l = LineCount; l != 0; l--, bp += 16, Offset += GFX.PPL) \
	{ \
		NORMAL (Offset, bp); \
		NORMAL (Offset + N, bp + 8); \
	} \
    } \
    else \
    if (!(Tile & V_FLIP)) \
    { \
    bp = (uint8 *)(pCache16 + StartLine); \
	for (l = LineCount; l != 0; l--, bp += 16, Offset += GFX.PPL) \
	{ \
		FLIPPED (Offset, bp + 8); \
		FLIPPED (Offset + N, bp); \
	} \
    } \
    else \
    if (Tile & H_FLIP) \
    { \
	bp = (uint8 *)(pCache16 + 56 - StartLine); \
	for (l = LineCount; l != 0; l--, bp -= 16, Offset += GFX.PPL) \
	{ \
		FLIPPED (Offset, bp + 8); \
		FLIPPED (Offset + N, bp); \
	} \
    } \
    else \
    { \
	bp = (uint8 *)(pCache16 + 56 - StartLine); \
	for (l = LineCount; l != 0; l--, bp -= 16, Offset += GFX.PPL) \
	{ \
		NORMAL (Offset, bp); \
		NORMAL (Offset + N, bp + 8); \
	} \
    }

#define TILE_CLIP_PREAMBLE \
    uint32 dd; \
    uint32 d1; \
    uint32 d2; \
\
    if (StartPixel < 4) \
    { \
	d1 = HeadMask [StartPixel]; \
	if (StartPixel + Width < 4) \
	    d1 &= TailMask [StartPixel + Width]; \
    } \
    else \
	d1 = 0; \
\
    if (StartPixel + Width > 4) \
    { \
	if (StartPixel > 4) \
	    d2 = HeadMask [StartPixel - 4]; \
	else \
	    d2 = 0xffffffff; \
\
	d2 &= TailMask [(StartPixel + Width - 4)]; \
    } \
    else \
	d2 = 0;


#define RENDER_CLIPPED_TILE(NORMAL, FLIPPED, N) \
    if (!(Tile & (V_FLIP | H_FLIP))) \
    { \
	bp = pCache + StartLine; \
	for (l = LineCount; l != 0; l--, bp += 8, Offset += GFX.PPL) \
	{ \
	    if ((dd = (*(uint32 *) bp) & d1)) \
		NORMAL (Offset, (uint8 *) &dd); \
	    if ((dd = (*(uint32 *) (bp + 4)) & d2)) \
		NORMAL (Offset + N, (uint8 *) &dd); \
	} \
    } \
    else \
    if (!(Tile & V_FLIP)) \
    { \
	bp = pCache + StartLine; \
	SWAP_DWORD (d1); \
	SWAP_DWORD (d2); \
	for (l = LineCount; l != 0; l--, bp += 8, Offset += GFX.PPL) \
	{ \
	    if ((dd = *(uint32 *) (bp + 4) & d1)) \
		FLIPPED (Offset, (uint8 *) &dd); \
	    if ((dd = *(uint32 *) bp & d2)) \
		FLIPPED (Offset + N, (uint8 *) &dd); \
	} \
    } \
    else \
    if (Tile & H_FLIP) \
    { \
	bp = pCache + 56 - StartLine; \
	SWAP_DWORD (d1); \
	SWAP_DWORD (d2); \
	for (l = LineCount; l != 0; l--, bp -= 8, Offset += GFX.PPL) \
	{ \
	    if ((dd = *(uint32 *) (bp + 4) & d1)) \
		FLIPPED (Offset, (uint8 *) &dd); \
	    if ((dd = *(uint32 *) bp & d2)) \
		FLIPPED (Offset + N, (uint8 *) &dd); \
	} \
    } \
    else \
    { \
	bp = pCache + 56 - StartLine; \
	for (l = LineCount; l != 0; l--, bp -= 8, Offset += GFX.PPL) \
	{ \
	    if ((dd = (*(uint32 *) bp) & d1)) \
		NORMAL (Offset, (uint8 *) &dd); \
	    if ((dd = (*(uint32 *) (bp + 4)) & d2)) \
		NORMAL (Offset + N, (uint8 *) &dd); \
	} \
    }

#define RENDER_TILE_LARGE(PIXEL, FUNCTION) \
    if (!(Tile & (V_FLIP | H_FLIP))) \
    { \
	if ((pixel = *(pCache + StartLine + StartPixel))) \
	{ \
	    pixel = PIXEL; \
	    for (l = LineCount; l != 0; l--, sp += GFX.PPL, Depth += GFX.PPL) \
	    { \
		for (int z = Pixels - 1; z >= 0; z--) \
		    if (GFX.Z1 > Depth [z]) \
		    { \
			sp [z] = FUNCTION(sp + z, pixel); \
			Depth [z] = GFX.Z2; \
		    }\
	    } \
	} \
    } \
    else \
    if (!(Tile & V_FLIP)) \
    { \
	StartPixel = 7 - StartPixel; \
	if ((pixel = *(pCache + StartLine + StartPixel))) \
	{ \
	    pixel = PIXEL; \
	    for (l = LineCount; l != 0; l--, sp += GFX.PPL, Depth += GFX.PPL) \
	    { \
		for (int z = Pixels - 1; z >= 0; z--) \
		    if (GFX.Z1 > Depth [z]) \
		    { \
			sp [z] = FUNCTION(sp + z, pixel); \
			Depth [z] = GFX.Z2; \
		    }\
	    } \
	} \
    } \
    else \
    if (Tile & H_FLIP) \
    { \
	StartPixel = 7 - StartPixel; \
	if ((pixel = *(pCache + 56 - StartLine + StartPixel))) \
	{ \
	    pixel = PIXEL; \
	    for (l = LineCount; l != 0; l--, sp += GFX.PPL, Depth += GFX.PPL) \
	    { \
		for (int z = Pixels - 1; z >= 0; z--) \
		    if (GFX.Z1 > Depth [z]) \
		    { \
			sp [z] = FUNCTION(sp + z, pixel); \
			Depth [z] = GFX.Z2; \
		    }\
	    } \
	} \
    } \
    else \
    { \
	if ((pixel = *(pCache + 56 - StartLine + StartPixel))) \
	{ \
	    pixel = PIXEL; \
	    for (l = LineCount; l != 0; l--, sp += GFX.PPL, Depth += GFX.PPL) \
	    { \
		for (int z = Pixels - 1; z >= 0; z--) \
		    if (GFX.Z1 > Depth [z]) \
		    { \
			sp [z] = FUNCTION(sp + z, pixel); \
			Depth [z] = GFX.Z2; \
		    }\
	    } \
	} \
    }
#endif



// ------------------------------------------------------------
// Optimizations
// ------------------------------------------------------------
#define TILE_PREAMBLE2(isBG) \
    uint8 *pCache; \
    uint16 *pCache16; \
\
    uint32 TileAddr = BG.TileAddress + ((Tile & 0x3ff) << BG.TileShift); \
    \
    if (!isBG) \
    { \
        if ((Tile & 0x1ff) >= 256) \
            TileAddr += BG.NameSelect; \
\
        TileAddr &= 0xffff; \
    }\
\
    uint32 TileNumber; \
    pCache = &BG.Buffer[(TileNumber = (TileAddr >> BG.TileShift)) << 6]; \
\
    if (!BG.Buffered [TileNumber]) \
    { \
	    BG.Buffered[TileNumber] = ConvertTile (pCache, TileAddr); \ 
        if (BG.Buffered [TileNumber] == BLANK_TILE) \
            return; \  
        GenerateTileMask(&BG, pCache, TileNumber); \
        GFX.VRAMPaletteFrame[TileAddr][0] = 0; \
        GFX.VRAMPaletteFrame[TileAddr][1] = 0; \
        GFX.VRAMPaletteFrame[TileAddr][2] = 0; \
        GFX.VRAMPaletteFrame[TileAddr][3] = 0; \
        GFX.VRAMPaletteFrame[TileAddr][4] = 0; \
        GFX.VRAMPaletteFrame[TileAddr][5] = 0; \
        GFX.VRAMPaletteFrame[TileAddr][6] = 0; \
        GFX.VRAMPaletteFrame[TileAddr][7] = 0; \
        GFX.VRAMPaletteFrame[TileAddr][8] = 0; \
        GFX.VRAMPaletteFrame[TileAddr][9] = 0; \
        GFX.VRAMPaletteFrame[TileAddr][10] = 0; \
        GFX.VRAMPaletteFrame[TileAddr][11] = 0; \
        GFX.VRAMPaletteFrame[TileAddr][12] = 0; \
        GFX.VRAMPaletteFrame[TileAddr][13] = 0; \
        GFX.VRAMPaletteFrame[TileAddr][14] = 0; \
        GFX.VRAMPaletteFrame[TileAddr][15] = 0; \
    } \
\
    if (BG.Buffered [TileNumber] == BLANK_TILE) \
	    return; \
    \
\
    register uint32 l; \
    uint8 pal; \
    if (BG.DirectColourMode && isBG) \
    { \
        if (IPPU.DirectColourMapsNeedRebuild) \
            S9xBuildDirectColourMaps (); \
        pal = (Tile >> 10) & BG.PaletteMask; \
        pCache16 = 0; \
        GFX.ScreenColors = DirectColourMaps [pal]; \
        if (GFX.VRAMPaletteFrame[TileAddr][pal] != GFX.PaletteFrame[pal + BG.StartPalette / 16]) \
        { \
            GFX.VRAMPaletteFrame[TileAddr][pal] = GFX.PaletteFrame[pal + BG.StartPalette / 16]; \
            ConvertTileTo16Bit(GFX.ScreenColors, pCache, pCache16); \
        } \
    } \
    else \
    { \
        pal = (Tile >> 10) & BG.PaletteMask; \
        pCache16 = 0; \
        GFX.ScreenColors = &IPPU.ScreenColors [(pal << BG.PaletteShift) + BG.StartPalette]; \
        if (GFX.VRAMPaletteFrame[TileAddr][pal] != GFX.PaletteFrame[pal + BG.StartPalette / 16]) \
        { \
            GFX.VRAMPaletteFrame[TileAddr][pal] = GFX.PaletteFrame[pal + BG.StartPalette / 16]; \
            ConvertTileTo16Bit(GFX.ScreenColors, pCache, pCache16); \
        } \
    }    

#define RENDER_TILE_FAST(NORMAL, FLIPPED, FULL_NORMAL, FULL_FLIPPED) \
    if (!(Tile & (V_FLIP | H_FLIP))) \
    { \
        bp = (uint8 *)(pCache16 + StartLine); \
        if (Offset & 1) \
            for (l = LineCount; l != 0; l--, bp += 16, Offset += GFX.PPL) NORMAL (Offset, bp); \
        else \
            for (l = LineCount; l != 0; l--, bp += 16, Offset += GFX.PPL) FULL_NORMAL (Offset, bp); \
    } \
    else \
    if (!(Tile & V_FLIP)) \
    { \
        bp = (uint8 *)(pCache16 + StartLine); \
        if (Offset & 1) \
            for (l = LineCount; l != 0; l--, bp += 16, Offset += GFX.PPL) FLIPPED (Offset, bp); \
        else \
            for (l = LineCount; l != 0; l--, bp += 16, Offset += GFX.PPL) FULL_FLIPPED (Offset, bp); \
    } \
    else \
    if (Tile & H_FLIP) \
    { \
        bp = (uint8 *)(pCache16 + 56 - StartLine); \
        if (Offset & 1) \
            for (l = LineCount; l != 0; l--, bp -= 16, Offset += GFX.PPL) FLIPPED (Offset, bp); \
        else \
            for (l = LineCount; l != 0; l--, bp -= 16, Offset += GFX.PPL) FULL_FLIPPED (Offset, bp); \
    } \
    else \
    { \
        bp = (uint8 *)(pCache16 + 56 - StartLine); \
        if (Offset & 1) \
            for (l = LineCount; l != 0; l--, bp -= 16, Offset += GFX.PPL) NORMAL (Offset, bp); \
        else \
            for (l = LineCount; l != 0; l--, bp -= 16, Offset += GFX.PPL) FULL_NORMAL (Offset, bp); \
    }

#define RENDER_CLIPPED_TILE_FAST(NORMAL, FLIPPED) \
    if (!(Tile & (V_FLIP | H_FLIP))) \
    { \
        bp = (uint8 *)(pCache16 + StartLine); \
        for (l = LineCount; l != 0; l--, bp += 16, Offset += GFX.PPL) \
        { \
            NORMAL (Offset, bp, StartPixel, Width); \
        } \
    } \
    else \
    if (!(Tile & V_FLIP)) \
    { \
        bp = (uint8 *)(pCache16 + StartLine); \
        for (l = LineCount; l != 0; l--, bp += 16, Offset += GFX.PPL) \
        { \
            FLIPPED (Offset, bp, StartPixel, Width); \
        } \
    } \
    else \
    if (Tile & H_FLIP) \
    { \
        bp = (uint8 *)(pCache16 + 56 - StartLine); \
        for (l = LineCount; l != 0; l--, bp -= 16, Offset += GFX.PPL) \
        { \
            FLIPPED (Offset, bp, StartPixel, Width); \
        } \
    } \
    else \
    { \
        bp = (uint8 *)(pCache16 + 56 - StartLine); \
        for (l = LineCount; l != 0; l--, bp -= 16, Offset += GFX.PPL) \
        { \
            NORMAL (Offset, bp, StartPixel, Width); \
        } \
    }

uint8 ConvertTile (uint8 *pCache, uint32 TileAddr);

