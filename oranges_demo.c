// sc LINK oranges_demo.c CPU=68060 MATH=68882 DATA=far CODE=far IDIR=libinclude: sage:lib/sage_debug.lib NOICONS

/*
 TODO

  . swap chess floor with a 3d plane with chess texture movement in X only with slowdown near the end
    in both left/right sides
*/
 
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <dos/dos.h>
#include <clib/dos_protos.h>

#include <sage/sage.h>

#include <proto/Maggie.h>
#include <maggie_vec.h>
#include <maggie_vertex.h>
#include <maggie_flags.h>

// GLOBAL VARIABLES

#define GLOBAL_TRANSPARENCY         0xff00ff
#define GLOBAL_BLACK_TRANSPARENCY   0x000000

// screen definition
#define SCREEN_WIDTH							320
#define SCREEN_HEIGHT   					256
#define SCREEN_DEPTH    					16

// main layer
#define MAIN_LAYER 								0
#define MAIN_LAYER_WIDTH					SCREEN_WIDTH
#define MAIN_LAYER_HEIGHT					SCREEN_HEIGHT

// atlas layer 
#define	ATLAS_LAYER								1
#define ATLAS_FILENAME						"assets/atlas.png"
#define ATLAS_WIDTH								416
#define ATLAS_HEIGHT   						384

// oranges ascii logo layer 
#define	ORANGES_LAYER							2
#define ORANGES_LOGO_FILENAME			"assets/vampire-girl.png" //"assets/oranges_ascii_logo.png"
#define ORANGES_WIDTH							SCREEN_WIDTH
#define ORANGES_HEIGHT   					SCREEN_HEIGHT

// grid layer 

#define GRID_CHESS_FILENAME				"assets/grid_chess_320.png"
#define GRID_CHESS_WIDTH					320
#define GRID_CHESS_HEIGHT   			64
#define	GRID_CHESS_LAYER					3
#define GRID_CHESS_LAYER_WIDTH	  GRID_CHESS_WIDTH
#define GRID_CHESS_LAYER_HEIGHT	  GRID_CHESS_HEIGHT

// logos
// running on
#define ATLAS_RUNNING_ON_X				0
#define ATLAS_RUNNING_ON_Y				64
#define ATLAS_RUNNING_ON_WIDTH		192
#define ATLAS_RUNNING_ON_HEIGHT		64
#define	RUNNING_ON_LAYER					4
#define RUNNING_ON_LAYER_WIDTH		ATLAS_RUNNING_ON_WIDTH
#define RUNNING_ON_LAYER_HEIGHT		ATLAS_RUNNING_ON_HEIGHT

// vampire logo
#define ATLAS_VAMPIRE_LOGO_X			0
#define ATLAS_VAMPIRE_LOGO_Y			128
#define ATLAS_VAMPIRE_LOGO_WIDTH	224
#define ATLAS_VAMPIRE_LOGO_HEIGHT	128
#define	VAMPIRE_LAYER							5
#define VAMPIRE_LAYER_WIDTH 	    ATLAS_VAMPIRE_LOGO_WIDTH
#define VAMPIRE_LAYER_HEIGHT	    ATLAS_VAMPIRE_LOGO_HEIGHT

// powered by
#define ATLAS_POWERED_BY_X				0
#define ATLAS_POWERED_BY_Y				0
#define ATLAS_POWERED_BY_WIDTH		192
#define ATLAS_POWERED_BY_HEIGHT		64
#define	POWERED_BY_LAYER				  6
#define POWERED_BY_LAYER_WIDTH 		ATLAS_POWERED_BY_WIDTH
#define POWERED_BY_LAYER_HEIGHT		ATLAS_POWERED_BY_HEIGHT

// sage
#define ATLAS_SAGE_X							0
#define ATLAS_SAGE_Y							256
#define ATLAS_SAGE_WIDTH					128
#define ATLAS_SAGE_HEIGHT					64
#define	SAGE_LAYER				        7
#define SAGE_LAYER_WIDTH 		      ATLAS_SAGE_WIDTH
#define SAGE_LAYER_HEIGHT		      ATLAS_SAGE_HEIGHT

// maggie
#define ATLAS_MAGGIE_LIBRARY_X			    0
#define ATLAS_MAGGIE_LIBRARY_Y			    320
#define ATLAS_MAGGIE_LIBRARY_WIDTH	    320
#define ATLAS_MAGGIE_LIBRARY_HEIGHT	    64
#define	MAGGIE_LIBRARY_LAYER		        8
#define MAGGIE_LIBRARY_LAYER_WIDTH 			ATLAS_MAGGIE_LIBRARY_WIDTH
#define MAGGIE_LIBRARY_LAYER_HEIGHT			ATLAS_MAGGIE_LIBRARY_HEIGHT

#define	TRANSITION_LAYER		    10

// Sprites

#define SPRITE_BANK             0                           
#define RUNNING_ON_SPRITE       0
#define POWERED_BY_SPRITE       1
#define VAMPIRE_LOGO_SPRITE     2
#define SAGE_SPRITE             3
#define MAGGIE_LIBRARY_SPRITE   4

// FONT

#define FONT_FILENAME					"assets/font_16x20.png"
#define FONT_WIDTH            16
#define FONT_HEIGHT           20
#define FONT_NUM              60
#define FONTPIC_WIDTH         160
#define FONTPIC_HEIGHT        120

#define TEXTFIELD_WIDTH       SCREEN_WIDTH+FONT_WIDTH
#define TEXTFIELD_HEIGHT      FONT_HEIGHT
#define TEXTFIELD_LAYER       9

#define TEXTSCROLL_SPEED     	2
#define TEXTSCROLL_POSX       0

SAGE_Music *music = NULL;
  
// TEXT MESSAGE

#define M_PI                  3.14159265358979323846
#define MESSAGE_FILENAME			"assets/message.txt"
#define RAD(x)                ((x)*PI/180.0)
#define CURVE_SCROLL          112 // smaller numbers create faster movement
FLOAT curve[CURVE_SCROLL];
UWORD curve_idx = 0;

// MUSIC

#define MUSIC_SLOT            10
char *filename_music = "assets/funky_colors_fusion.mod";

// ENUMS

enum direction { towardLeft, towardRight, towardUp, towardDown };
enum ballXBoundary { left = 0.0f , right = 0.0f };
enum ditherTransitionType { worm, fullLine, fullColumn };
enum ditherTransitionFlow { in, out };
enum ditherTransitionSpeed { slow, normal, fast };

// GLOBAL VARIABLES

BOOL finish3DSection1 = FALSE;
BOOL isFinishSection1 = FALSE;
BOOL isFinishSection2 = FALSE;
BOOL isFinishSection3 = FALSE;
BOOL startTransitionToSection2 = FALSE;
BOOL calledVampireLogoOutro = FALSE;
BOOL calledGridChessOutro = FALSE;
BOOL calledBallsOutro = FALSE;

SAGE_Picture *atlas_picture, *oranges_logo_picture, *grid_chess_picture, *font_picture;

STRPTR message = NULL;
UWORD message_pos = 0, font_posx[FONT_NUM], font_posy[FONT_NUM];
UWORD char_posx = SCREEN_WIDTH, char_load = 0;
UWORD layer_posx = SCREEN_WIDTH+FONT_WIDTH, layer_posy = 0, scroll_posy = 0;

// GENERIC HELPERS

float* randomFloatNumbers(int size, float min, float max) {
  int i;
  float* array = (float*)malloc(size * sizeof(float));
  
  srand((unsigned int)time(NULL));
  
  for (i=0; i<size; i++) {
    array[i] = min + ((float)rand()/(float)(RAND_MAX)) * (max - min);
  }
  
  return array;
}

float* randomFloatNumber(float min, float max) {
  srand((unsigned int)time(NULL));
  return min + ((float)rand()/(float)(RAND_MAX)) * (max - min);
}

int rgb888_to_rgb565(int red8, int green8, int blue8) {
  int red5 = red8 >> 3;
  int green6 = green8 >> 2;
  int blue5 = blue8 >> 3;
  
  int red5_shifted = red5 << 11;
  int green6_shifted = green6 << 5;
  
  int rgb565 = red5_shifted | green6_shifted | blue5;
//printf("r8=%d\n", red8);
//printf("g8=%d\n", green8);
//printf("b8=%d\n", blue8); 
  return rgb565;
}

// FILL

void fillArea(int x, int y, int width, int height, int color) {
  SAGE_Bitmap *back_bitmap = SAGE_GetBackBitmap();
  short *bitmap_buffer = back_bitmap->bitmap_buffer;
  int i, j, current_x, current_y;

  for (i=0; i<width; i++) {
    for (j=0; j<height; j++) {
      current_x = x + i;
      current_y = y + j;
      bitmap_buffer[(SCREEN_WIDTH * current_y) + current_x] = (short)color;
    }
  }
}

void fillAreaLayer(int index, int x, int y, int width, int height, int color) {
  SAGE_Bitmap *bitmap = SAGE_GetLayerBitmap(index);
  short *buffer = bitmap->bitmap_buffer;
  int i, j, current_x, current_y;

  for (i=0; i<width; i++) {
    for (j=0; j<height; j++) {
      current_x = x + i;
      current_y = y + j;
      buffer[(SCREEN_WIDTH * current_y) + current_x] = (short)color;
    }
  }
}

void clearLayerBitmap(int index, int color) {
  int x, y;
  SAGE_Bitmap *bitmap = SAGE_GetLayerBitmap(index);
  short *buffer = bitmap->bitmap_buffer;

  for (y=0; y<SCREEN_HEIGHT; y++) {
    for (x=0; x<SCREEN_WIDTH; x++) {
      buffer[(SCREEN_WIDTH * y) + x] = (short)color;
    }
  }
}

void clearBackScreen(int color) {
  int x, y;
  SAGE_Bitmap *bitmap = SAGE_GetBackBitmap();
  short *buffer = bitmap->bitmap_buffer;

  for (y=0; y<SCREEN_HEIGHT; y++) {
    for (x=0; x<SCREEN_WIDTH; x++) {
      buffer[(SCREEN_WIDTH * y) + x] = (short)color;
    }
  }
}

void clearFrontScreen(int color) {
  int x, y;
  SAGE_Bitmap *bitmap = SAGE_GetFrontBitmap();
  short *buffer = bitmap->bitmap_buffer;

  for (y=0; y<SCREEN_HEIGHT; y++) {
    for (x=0; x<SCREEN_WIDTH; x++) {
      buffer[(SCREEN_WIDTH * y) + x] = (short)color;
    }
  }
}

// 

BOOL createAllLayers(void) {
  if (!SAGE_CreateLayer(MAIN_LAYER, MAIN_LAYER_WIDTH, MAIN_LAYER_HEIGHT)) {
    SAGE_SetLayerTransparency(MAIN_LAYER, GLOBAL_BLACK_TRANSPARENCY);
    return FALSE;
  }
  
  if (!SAGE_CreateLayer(ATLAS_LAYER, ATLAS_WIDTH, ATLAS_HEIGHT)) {  
    return FALSE;
  }
  
  if (!SAGE_CreateLayer(ORANGES_LAYER, ORANGES_WIDTH, ORANGES_HEIGHT)) {
    SAGE_SetLayerTransparency(ORANGES_LAYER, GLOBAL_BLACK_TRANSPARENCY);
    return FALSE;
  }
  
  if (!SAGE_CreateLayer(GRID_CHESS_LAYER, GRID_CHESS_WIDTH, GRID_CHESS_HEIGHT)) {  
    return FALSE;
  }

  if (!SAGE_CreateLayer(RUNNING_ON_LAYER, RUNNING_ON_LAYER_WIDTH, RUNNING_ON_LAYER_HEIGHT)) {
    SAGE_SetLayerTransparency(RUNNING_ON_LAYER, GLOBAL_BLACK_TRANSPARENCY);
    return FALSE;
  }
  
  if (!SAGE_CreateLayer(POWERED_BY_LAYER, POWERED_BY_LAYER_WIDTH, POWERED_BY_LAYER_HEIGHT)) {
    SAGE_SetLayerTransparency(POWERED_BY_LAYER, GLOBAL_BLACK_TRANSPARENCY);
    return FALSE;
  }

  if (!SAGE_CreateLayer(VAMPIRE_LAYER, VAMPIRE_LAYER_WIDTH, VAMPIRE_LAYER_HEIGHT)) {  
    SAGE_SetLayerTransparency(VAMPIRE_LAYER, GLOBAL_BLACK_TRANSPARENCY);
    return FALSE;
  }

  if (!SAGE_CreateLayer(SAGE_LAYER, SAGE_LAYER_WIDTH, SAGE_LAYER_HEIGHT)) {
    SAGE_SetLayerTransparency(SAGE_LAYER, GLOBAL_BLACK_TRANSPARENCY);
    return FALSE;
  }
  
  if (!SAGE_CreateLayer(MAGGIE_LIBRARY_LAYER, MAGGIE_LIBRARY_LAYER_WIDTH, MAGGIE_LIBRARY_LAYER_HEIGHT)) {
    SAGE_SetLayerTransparency(MAGGIE_LIBRARY_LAYER, GLOBAL_BLACK_TRANSPARENCY);
    return FALSE;
  }
  
  if (!SAGE_CreateLayer(TRANSITION_LAYER, SCREEN_WIDTH, SCREEN_HEIGHT)) {
    return FALSE;
  }
}

void releaseAllLayers(void) {
  int i;
  
  for (i=0; i<10; i++) {
    SAGE_ReleaseLayer(i);
  }
}

void clearMainLayer(void) {
	if (MAIN_LAYER != NULL) {
		clearBackScreen(rgb888_to_rgb565(0,0,0));
	}
}

// Sprites

BOOL createAllSprites(void) {
  if (atlas_picture != NULL) {
    SAGE_CreateSpriteBank(SPRITE_BANK, 5, atlas_picture);
    SAGE_SetSpriteBankTransparency(SPRITE_BANK, GLOBAL_TRANSPARENCY);
    
    SAGE_AddSpriteToBank(
      SPRITE_BANK,
      RUNNING_ON_SPRITE,
      ATLAS_RUNNING_ON_X,
      ATLAS_RUNNING_ON_Y,
      ATLAS_RUNNING_ON_WIDTH,
      ATLAS_RUNNING_ON_HEIGHT,
      SSPR_HS_MIDDLE);
      
    SAGE_AddSpriteToBank(
      SPRITE_BANK,
      POWERED_BY_SPRITE,
      ATLAS_POWERED_BY_X,
      ATLAS_POWERED_BY_Y,
      ATLAS_POWERED_BY_WIDTH,
      ATLAS_POWERED_BY_HEIGHT,
      SSPR_HS_MIDDLE);
    
    SAGE_AddSpriteToBank(
      SPRITE_BANK,
      VAMPIRE_LOGO_SPRITE,
      ATLAS_VAMPIRE_LOGO_X,
      ATLAS_VAMPIRE_LOGO_Y,
      ATLAS_VAMPIRE_LOGO_WIDTH,
      ATLAS_VAMPIRE_LOGO_HEIGHT,
      SSPR_HS_MIDDLE);
    
    SAGE_AddSpriteToBank(
      SPRITE_BANK,
      SAGE_SPRITE,
      ATLAS_SAGE_X,
      ATLAS_SAGE_Y,
      ATLAS_SAGE_WIDTH,
      ATLAS_SAGE_HEIGHT,
      SSPR_HS_MIDDLE);
    
    SAGE_AddSpriteToBank(
      SPRITE_BANK,
      MAGGIE_LIBRARY_SPRITE,
      ATLAS_MAGGIE_LIBRARY_X,
      ATLAS_MAGGIE_LIBRARY_Y,
      ATLAS_MAGGIE_LIBRARY_WIDTH,
      ATLAS_MAGGIE_LIBRARY_HEIGHT,
      SSPR_HS_MIDDLE);
  }
}

// ORANGES LOGO

BOOL loadOrangesLogo(void) {
	oranges_logo_picture = SAGE_LoadPicture(ORANGES_LOGO_FILENAME);

	if (oranges_logo_picture == NULL) {
  	SAGE_DisplayError();
  	return FALSE;
  }
  
  if (SAGE_CreateLayerFromPicture(ORANGES_LAYER, oranges_logo_picture)) { 
  	return TRUE;
 	}
  
  SAGE_DisplayError();
  return FALSE;
}

void showOrangesLogo(void) {
  SAGE_BlitPictureToLayer(
		oranges_logo_picture,
	 	0,
	 	0,
	 	SCREEN_WIDTH,
	 	SCREEN_HEIGHT,
	 	MAIN_LAYER,
	 	0,
	 	0);

	SAGE_BlitLayerToScreen(MAIN_LAYER, 0, 0);
}

// ATLAS

BOOL loadAtlas(void) {
	atlas_picture = SAGE_LoadPicture(ATLAS_FILENAME);
	
	SAGE_SetPictureTransparency(atlas_picture, GLOBAL_TRANSPARENCY);
  
	if (atlas_picture == NULL) {
  	SAGE_DisplayError();
  	return FALSE;
  }
    
  //if (SAGE_CreateLayerFromPicture(ATLAS_LAYER, atlas_picture)) {  
  //	return TRUE;
 	//}
  
  SAGE_DisplayError();
  return FALSE;
}

// GRID CHESS

BOOL loadGridChess(void) {
	grid_chess_picture = SAGE_LoadPicture(GRID_CHESS_FILENAME);

	if (grid_chess_picture == NULL) {
  	SAGE_DisplayError();
  	return FALSE;
  }
    
  if (SAGE_CreateLayerFromPicture(GRID_CHESS_LAYER, grid_chess_picture)) { 
  	return TRUE;
 	}
  
  SAGE_DisplayError();
  return FALSE;
}

// FONT

BOOL createTextfieldLayer(void) {
  if (SAGE_CreateLayer(TEXTFIELD_LAYER, TEXTFIELD_WIDTH, TEXTFIELD_HEIGHT)) {
    //SAGE_SetLayerTransparency(TEXTFIELD_LAYER, GLOBAL_BLACK_TRANSPARENCY);
    return TRUE;
  }
  SAGE_DisplayError();
  return FALSE;
}

BOOL loadFont(void) {
  UWORD x, y, idx, line, column;

	font_picture = SAGE_LoadPicture(FONT_FILENAME);
	
  if (font_picture != NULL) {
    idx = 0;
    y = 0;

    for (line = 0; line < (FONTPIC_HEIGHT / FONT_HEIGHT); line++) {
      x = 0;
      for (column = 0;column < (FONTPIC_WIDTH / FONT_WIDTH); column++) {
        font_posx[idx] = x;
        font_posy[idx] = y;
        x += FONT_WIDTH;
        idx++;
      }
      y += FONT_HEIGHT;
    }
    return TRUE;
  }
  
  SAGE_DisplayError();
  return FALSE;
}

// TEXT MESSAGE

BOOL loadMessage(void) {
  BPTR fdesc;
  LONG bytes_read;

  fdesc = Open(MESSAGE_FILENAME, MODE_OLDFILE);
  if (fdesc) {
    // Get the file size
    bytes_read = Seek(fdesc, 0, OFFSET_END);
    bytes_read = Seek(fdesc, 0, OFFSET_BEGINNING);
    message = (STRPTR) SAGE_AllocMem(bytes_read + 2);
    if (message != NULL) {
      if (Read(fdesc, message, bytes_read) == bytes_read) {
        Close(fdesc);
        return TRUE;
      }
    }
    Close(fdesc);
  }
  SAGE_DisplayError();
  return FALSE;
}

// Blit

void atlasBlitRunningOn(int x_, int y_) {
	SAGE_BlitPictureToLayer(
		atlas_picture,
	 	ATLAS_RUNNING_ON_X,
	 	ATLAS_RUNNING_ON_Y,
	 	ATLAS_RUNNING_ON_WIDTH,
	 	ATLAS_RUNNING_ON_HEIGHT,
	 	RUNNING_ON_LAYER,
	 	0,
	 	0);
  SAGE_BlitLayerToScreen(RUNNING_ON_LAYER, x_, y_);
}

void atlasBlitPoweredBy(int x_, int y_) {
	SAGE_BlitPictureToLayer(
		atlas_picture,
	 	ATLAS_POWERED_BY_X,
	 	ATLAS_POWERED_BY_Y,
	 	ATLAS_POWERED_BY_WIDTH,
	 	ATLAS_POWERED_BY_HEIGHT,
	 	POWERED_BY_LAYER,
	 	0,
	 	0);
  SAGE_BlitLayerToScreen(POWERED_BY_LAYER, x_, y_);
}

void atlasBlitVampireLogo(int x_, int y_) {
	SAGE_BlitPictureToLayer(
		atlas_picture,
	 	ATLAS_VAMPIRE_LOGO_X,
	 	ATLAS_VAMPIRE_LOGO_Y,
	 	ATLAS_VAMPIRE_LOGO_WIDTH,
	 	ATLAS_VAMPIRE_LOGO_HEIGHT,
	 	VAMPIRE_LAYER,
	 	0,
	 	0);
	 	SAGE_SetLayerTransparency(VAMPIRE_LAYER, GLOBAL_BLACK_TRANSPARENCY);
  SAGE_BlitLayerToScreen(VAMPIRE_LAYER, x_, y_);
}

void atlasBlitSage(int x_, int y_) {
	SAGE_BlitPictureToLayer(
		atlas_picture,
	 	ATLAS_SAGE_X,
	 	ATLAS_SAGE_Y,
	 	ATLAS_SAGE_WIDTH,
	 	ATLAS_SAGE_HEIGHT,
	 	SAGE_LAYER,
	 	0,
	 	0);
  SAGE_BlitLayerToScreen(SAGE_LAYER, x_, y_);
}

void atlasBlitMaggieLibrary(int x_, int y_) {
	SAGE_BlitPictureToLayer(
		atlas_picture,
	 	ATLAS_MAGGIE_LIBRARY_X,
	 	ATLAS_MAGGIE_LIBRARY_Y,
	 	ATLAS_MAGGIE_LIBRARY_WIDTH,
	 	ATLAS_MAGGIE_LIBRARY_HEIGHT,
	 	MAGGIE_LIBRARY_LAYER,
	 	0,
	 	0);
	SAGE_BlitLayerToScreen(MAGGIE_LIBRARY_LAYER, x_, y_);
	  SAGE_DisplayError();
}
                          
void atlasBlitGridChess(void) {
	SAGE_BlitPictureToBitmap(
		grid_chess_picture,
	 	0,
	 	0,
	 	GRID_CHESS_WIDTH,
	 	GRID_CHESS_HEIGHT,
	 	SAGE_GetBackBitmap(),
	 	0,
	 	142);
	//SAGE_BlitLayerToScreen(MAIN_LAYER, 0, 0);
}

// Animate

float Lerp(float start_value, float end_value, float pct) {
  return (start_value + (end_value - start_value) * pct);
}

float Flip(float x) {
  return 1 - x;
}

float QuadraticEaseIn(float t) {
  return t * t;
}

float QuadraticEaseOut(float t) {
  return -(t * (t - 2));
}

float EaseInElastic(float t) {
  float c4 = (2 * M_PI) / 3;
  return t == 0 
  ? 0
  : t == 1
  ? 1
  : -pow(2, 10 * t - 10) * sin((t * 10 - 10.5) * c4);
}

float EaseOutElastic(float t) {
  float c4 = (2 * M_PI) / 3;
  return t == 0 
  ? 0
  : t == 1
  ? 1
  : pow(2, -10 * t) * sin((t * 10 - 0.5) * c4) + 1;
}

float EaseInQuint(float t) {
  return t * t * t * t * t;
}

float EaseOutQuint(float t) {
  return 1 - pow(1 - t, 5);
}

void animateRunningOnIn(void) {
  float time = 0;
  float duration = 100;
  int pointA = 0-ATLAS_RUNNING_ON_HEIGHT;
  int pointB = 48;
  int x_ = SCREEN_WIDTH / 2;
  
  while (time <= duration) {
    SAGE_ClearScreen();
    SAGE_BlitSpriteToScreen(
      SPRITE_BANK,
      RUNNING_ON_SPRITE,
      x_,
      Lerp(pointA, pointB, EaseOutElastic(time/duration)));
      
    SAGE_RefreshScreen();    
    SAGE_Pause(1);
    time += 1;
  }
}

void animateRunningOnOut(void) {
  float time = 0;
  float duration = 100;
  int pointA = 0-ATLAS_RUNNING_ON_HEIGHT;
  int pointB = 48;
  int x_ = SCREEN_WIDTH / 2;
  
  while (time <= duration) {
    SAGE_ClearScreen();
    SAGE_BlitSpriteToScreen(
      SPRITE_BANK,
      RUNNING_ON_SPRITE,
      x_,
      Lerp(pointB, pointA, EaseInElastic(time/duration)));

    SAGE_RefreshScreen();    
    SAGE_Pause(1);
    time += 1;
  }
}

void animatePoweredByIn(void) {
  float time = 0;
  float duration = 100;
  int pointA = 0-ATLAS_POWERED_BY_HEIGHT;
  int pointB = 48;
  int x_ = SCREEN_WIDTH / 2;
        
  while (time <= duration) {
    SAGE_ClearScreen();
    SAGE_BlitSpriteToScreen(
      SPRITE_BANK,
      POWERED_BY_SPRITE,
      x_,
      Lerp(pointA, pointB, EaseOutElastic(time/duration)));
      
    SAGE_RefreshScreen();    
    SAGE_Pause(1);
    time += 1;
  }
}
 
void animatePoweredByOut(void) {
  float time = 0;
  float duration = 100;
  int pointA = 0-ATLAS_POWERED_BY_HEIGHT;
  int pointB = 48;
  int x_ = SCREEN_WIDTH / 2;

  while (time <= duration) {
    SAGE_ClearScreen();
    SAGE_BlitSpriteToScreen(
      SPRITE_BANK,
      POWERED_BY_SPRITE,
      x_,
      Lerp(pointB, pointA, EaseInElastic(time/duration)));

    SAGE_RefreshScreen();    
    SAGE_Pause(1);
    time += 1;
  }
}
 
void animateVampireLogo(int x, int yA, int yB) {
  float time = 0;
  float duration = 80;
  int pointA = yA;
  int pointB = yB;
  int x_ = x;
  
  while (time <= duration) {
    SAGE_BlitSpriteToScreen(
      SPRITE_BANK,
      VAMPIRE_LOGO_SPRITE,
      x_,
      (int)(Lerp(pointA, pointB, EaseOutQuint(time/duration))));
    SAGE_RefreshScreen();    
    SAGE_Pause(1);
    time += 1;
  }
}

void animateSage(void) {
  float time = 0;
  float duration = 80;
  int pointA = SCREEN_HEIGHT+ATLAS_SAGE_HEIGHT;
  int pointB = 160;
  int x_ = SCREEN_WIDTH / 2;
  
  while (time <= duration) {
    SAGE_ClearScreen();
    atlasBlitPoweredBy((SCREEN_WIDTH-ATLAS_RUNNING_ON_WIDTH)/2, 16);
    SAGE_BlitSpriteToScreen(
      SPRITE_BANK,
      SAGE_SPRITE,
      x_,
      Lerp(pointA, pointB, EaseOutQuint(time/duration)));
      
    SAGE_RefreshScreen();
    SAGE_Pause(1);
    time += 1;
  }
}

void animateMaggieLibrary(void) {
  float time = 0;
  float duration = 80;
  int pointA = SCREEN_HEIGHT+ATLAS_MAGGIE_LIBRARY_HEIGHT;
  int pointB = 160;
  int x_ = SCREEN_WIDTH / 2;
  
  while (time <= duration) {
    SAGE_ClearScreen();
    atlasBlitPoweredBy((SCREEN_WIDTH-ATLAS_RUNNING_ON_WIDTH)/2, 16);
    SAGE_BlitSpriteToScreen(
      SPRITE_BANK,
      MAGGIE_LIBRARY_SPRITE,
      x_,
      Lerp(pointA, pointB, EaseOutQuint(time/duration)));
      
    SAGE_RefreshScreen();    
    SAGE_Pause(1);
    time += 1;
  }
}

// FADE effect

float fadeStatus = -0.1f, flashStatus = -0.1f;
BOOL goingUp = TRUE;

void FadeScreen(short *srcBuffer,
                short *destBuffer,
                int startX,
                int startY,
                int width,
                int height,
                int screenWidth,
                float fadeValue) {
  int i;
  // find the start position of the pixels to start the fade 
  int modulus = (screenWidth * startY) + startX;
  // get the fade value
  ULONG iT = fadeValue * 255.0f;
	ULONG nPixels = width * height;
    
  for (i = 0; i < nPixels; i++) {
   	ULONG pixel = srcBuffer[i];
    int r = ((int)((pixel & 0x00ff0000) * iT) >> 16) & 0x00ff0000;
    int g = ((int)((pixel & 0x0000ff00) * iT) >> 8) & 0x0000ff00;
    int b = ((int)((pixel & 0x000000ff) * iT)) & 0x000000ff;
    destBuffer[i] = (short)rgb888_to_rgb565(r,g,b);
  }
}

void FadeInOut(float step, int indexLayer) {
  SAGE_Bitmap *bitmap;
  
  switch (indexLayer) {
    case -3:
      bitmap = SAGE_GetWaitBitmap();
    break;
    
    case -2:
      bitmap = SAGE_GetBackBitmap();
    break;
    
    case -1:
      bitmap = SAGE_GetFrontBitmap();
    break;
    
    default:
      bitmap = SAGE_GetLayer(indexLayer)->bitmap;
    break;
  }
  	
	FadeScreen(
  	bitmap->bitmap_buffer, 
  	bitmap->bitmap_buffer,
  	0,
  	0, 
  	SCREEN_WIDTH,
  	SCREEN_HEIGHT,
  	SCREEN_WIDTH,
  	fadeStatus);
	
	if (goingUp) {
		fadeStatus = fadeStatus + step;
		
		if (fadeStatus >= 1.0f) {
			fadeStatus = 1.0f;
			goingUp = FALSE;
		}
	}
	else {
		fadeStatus = fadeStatus - step;
		
		if (fadeStatus <= 0.0f) {
			fadeStatus = 0.0f;
			goingUp = TRUE;
		}
	}
}

void Flash(short *srcBuffer,
           short *destBuffer,
           int width,
           int height,
           float flashValue) {
  int i;
  float iT = flashValue;
	ULONG nPixels = width * height;
  int r, g, b;
  int nr, ng, nb;
  
  for (i = 0; i < nPixels; i++) {
   	UWORD pixel = (UWORD)srcBuffer[i];
    //printf("pixel %hu\n", pixel);
    r = (pixel >> 11) & 31;
    g = (pixel >> 5) & 63;
    b = pixel & 31;
    
    //printf("r=%d\n",r);    
    //printf("g=%d\n",g);    
    //printf("b=%d\n",b);
    
    nr = (float)r*flashValue;
    ng = (float)g*flashValue;
    nb = (float)b*flashValue;

//    printf("nr=%d\n",nr);    
//    printf("ng=%d\n",ng);    
//    printf("nb=%d\n",nb);    
//    printf("======================\n\n");    
    
    destBuffer[i] = (short)rgb888_to_rgb565((int)nr, (int)ng, (int)nb);
  }
}

void FlashIn(int indexLayer) {
  SAGE_Bitmap *bitmap, *front;
  float step = 1.0f;
  front = SAGE_GetFrontBitmap();
  
  switch (indexLayer) {
    case -3:
      bitmap = SAGE_GetWaitBitmap();
    break;
    
    case -2:
      bitmap = SAGE_GetBackBitmap();
    break;
    
    case -1:
      bitmap = SAGE_GetFrontBitmap();
    break;
    
    default:
      bitmap = SAGE_GetLayer(indexLayer)->bitmap;
    break;
  }

  while (step >= -0.01f) {
    if (SAGE_IsFrontMostScreen) {
      //SAGE_WaitVbl();
      //printf("step=%f\n--------------\n", step);  
      //clearBackScreen(rgb888_to_rgb565(0,0,0));
      //SAGE_RefreshScreen();
      //SAGE_ClearScreen();
      //fillArea(0, 0, 120, 120, rgb888_to_rgb565(255, 0, 0));
      showOrangesLogo();
      //SAGE_BlitPictureToScreen(oranges_logo_picture, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
      
      Flash(
      	bitmap->bitmap_buffer, 
      	front->bitmap_buffer,
      	SCREEN_WIDTH,
      	SCREEN_HEIGHT,
      	step);
  	
      step -= 0.01f;

    	//SAGE_RefreshScreen();
    	
      SAGE_Pause(1);      
    }
  }
}

// choose blit width and height as 16 or 32 to match the dither mask sequence
void ditheringTransition(int startX,
                         int startY,
                         int blitWidth,
                         int blitHeight,
                         int lines,
                         int columns,
                         enum ditherTransitionSpeed speed,
                         int layerIndex,
                         int layerX,
                         int layerY,
                         enum ditherTransitionType type,
                         enum ditherTransitionFlow flow,
                         int extra) {
  int i, x=10, line, column, ditherXPosition, trackXposition = 0;

  // the X position is related to the 4 columns of dither animation mask effect in the atlas picture
  if (blitWidth == 32) {
    if (fast) {
      ditherXPosition = ATLAS_WIDTH - 32;
    }
    else {
      ditherXPosition = ATLAS_WIDTH - 80;
    }
  }
  if (blitWidth == 16) {
    if (fast) {
      ditherXPosition = ATLAS_WIDTH - 48;
    }
    else {
      ditherXPosition = ATLAS_WIDTH - 96;
    }
  }
  
  // things comes
  if (flow == in) {
    if (type == fullLine) {  
      x = 0; 
      
      while (x<10) {
        // recreate the layer without the previous blit
        // for now we set it to static connected to the chess grid
        // TODO: make this dynamic to recreate from a param any layer from any picture
        SAGE_CreateLayerFromPicture(GRID_CHESS_LAYER, grid_chess_picture);
        
        SAGE_ClearScreen();
        
        // start the dithering filter animation
      	for (line = 0; line < lines; line++) {
          for (column = 0; column < columns; column++) {
            trackXposition = startX + (column * blitWidth);
            
          	SAGE_BlitPictureToLayer(
                 atlas_picture, // dither filter animation is in atlas picture
                 ditherXPosition,
                 blitHeight * x,
                 blitWidth,
                 blitHeight,
                 layerIndex,
                 trackXposition,
                 startY + (line * blitHeight));
          }
        }
        
        SAGE_BlitLayerToScreen(layerIndex, layerX, layerY);
            
        SAGE_RefreshScreen(); 
        SAGE_Pause(3);
        
        if (speed == normal) {
          x += 2;   
        }
        else {
          x += 1;
        }
      }
    }
  }
  
  // things goes
  if (flow == out) {
    if (type == worm) { 
      // start the dithering filter animation
    	for (line = 0; line < lines; line++) {
        for (column = 0; column < columns; column++) {
          // make a worm style effect
        	if (line%2 == 0) {
          	trackXposition = startX + (column * blitWidth);
        	}
        	else {
          	trackXposition = (startX + ((columns - 1) * blitWidth)) - (column * blitWidth);
        	}
        	
          while (x>=0) {
            SAGE_ClearScreen();
            
            // blit "Running on" or "Powered by"
            if (extra != 0) {
              if (extra == -999) {
                atlasBlitRunningOn((SCREEN_WIDTH - RUNNING_ON_LAYER_WIDTH) / 2, 16);
              }
              if (extra == -998) {
                atlasBlitPoweredBy((SCREEN_WIDTH - POWERED_BY_LAYER_WIDTH) / 2, 16);
              }    
            }
            
            SAGE_BlitPictureToLayer(
               atlas_picture, // dither filter animation is in atlas picture
               ditherXPosition,
               blitHeight * x,
               blitWidth,
               blitHeight,
               layerIndex,
               trackXposition,
               startY + (line * blitHeight));
               
            SAGE_BlitLayerToScreen(layerIndex, layerX, layerY);
            
            SAGE_RefreshScreen(); 
            SAGE_Pause(1);
            
            x-=2;
          }
          
          if (speed == normal) {
            x = 5;   
          }
          else {
            x = 10;
          }
        }
      }
    }
    
    if (type == fullLine) {   
      // start the dithering filter animation
    	for (line = 0; line < lines; line++) {  
        while (x>=0) {
          SAGE_ClearScreen();
          
          // blit "Running on" or "Powered by"
          if (extra != 0) {
            if (extra == -999) {
              atlasBlitRunningOn((SCREEN_WIDTH - RUNNING_ON_LAYER_WIDTH) / 2, 16);
            }
            if (extra == -998) {
              atlasBlitPoweredBy((SCREEN_WIDTH - POWERED_BY_LAYER_WIDTH) / 2, 16);
            }    
          }
          
          for (column = 0; column < columns; column++) {
            trackXposition = startX + (column * blitWidth);
            
          	SAGE_BlitPictureToLayer(
                 atlas_picture, // dither filter animation is in atlas picture
                 ditherXPosition,
                 blitHeight * x,
                 blitWidth,
                 blitHeight,
                 layerIndex,
                 trackXposition,
                 startY + (line * blitHeight));
          }
          
          SAGE_BlitLayerToScreen(layerIndex, layerX, layerY);
            
          SAGE_RefreshScreen(); 
          SAGE_Pause(1);
          
          x-=1;
        }
        
        if (speed == normal) {
          x = 5;   
        }
        else {
          x = 10;
        }
      }
    }
    
    if (type == fullColumn) {
      for (column = 0; column < columns; column++) {
        while (x>=0) {
          SAGE_ClearScreen();
          
          // blit "Running on" or "Powered by"
          if (extra != 0) {
            if (extra == -999) {
              atlasBlitRunningOn((SCREEN_WIDTH - RUNNING_ON_LAYER_WIDTH) / 2, 16);
            }
            if (extra == -998) {
              atlasBlitPoweredBy((SCREEN_WIDTH - POWERED_BY_LAYER_WIDTH) / 2, 16);
            }    
          }        
         
          for (line = 0; line < lines; line++) {
            trackXposition = startX + (column * blitWidth);
        	
          	SAGE_BlitPictureToLayer(
                 atlas_picture, // dither filter animation is in atlas picture
                 ditherXPosition,
                 blitHeight * x,
                 blitWidth,
                 blitHeight,
                 layerIndex,
                 trackXposition,
                 startY + (line * blitHeight));
          }
          
          SAGE_BlitLayerToScreen(layerIndex, layerX, layerY);
            
          SAGE_RefreshScreen(); 
          SAGE_Pause(1);
          
          x-=1;
        }
  
        if (speed == normal) {
          x = 5;   
        }
        else {
          x = 10;
        }
      }
    }
  }
}

// 3D

struct Library *MaggieBase = NULL;

char *filename_object;
char *filename_texture;

int totalVertexBufferCount = 0;
int totalTrianglesCount = 0;

static struct MaggieVertex BallVertices[10000];
UWORD BallIndices[10000];

UWORD txtr = 0xffff;
mat4 worldMatrix, viewMatrix, perspective;

// note: using 640x512 we need to use 4:3 ratio
// otherwise for wider resolution we use 16:9 ratio as  9.0f / 16.0f;
float targetRatio = 3.0f / 4.0f;

int vBuffer;
int iBuffer;
vec3 cameraPosition = { 0.0f, 0.0f, 9.0f };
float cameraStepMovement = 0.05f;

// set initial balls position XYZ and directions
#define floorY              2.5f
#define leftXBoundary       -12.0f
#define rightXBoundary      12.0f

#define totalBalls                  10
#define ballYPositionsTotal         128
#define ballYFallingPositionsTotal  362

float rotationX = 0.0f, rotationY = 0.0f;

// simulate the bouncing ball via sin table
float ballYPosition[] = {
  -0.03695945, -0.1108554, -0.18468332, -0.25839984, -0.33195877, -0.4053169, -0.4784282, -0.55124915,
  -0.623736, -0.6958436, -0.76752937, -0.8387486, -0.9094586, -0.979617, -1.0491802, -1.118107,
  -1.1863544, -1.2538815, -1.320648, -1.3866122, -1.4517351, -1.5159762, -1.5792968, -1.6416595,
  -1.7030246, -1.7633567, -1.8226173, -1.8807718, -1.9377848, -1.9936211, -2.0482473, -2.1016297,
  -2.153736, -2.2045355, -2.253996, -2.3020883, -2.3487833, -2.3940523, -2.4378672, -2.4802027,
  -2.5210328, -2.560332, -2.5980763, -2.6342442, -2.6688128, -2.7017603, -2.7330682, -2.762717,
  -2.790688, -2.816965, -2.8415325, -2.8643742, -2.885477, -2.9048283, -2.922416, -2.9382293,
  -2.952259, -2.9644966, -2.974934, -2.9835658, -2.990386, -2.995391, -2.998577, -2.9999433,
  -2.9999433, -2.998577, -2.995391, -2.990386, -2.9835658, -2.974934, -2.9644966,
  -2.952259, -2.9382293, -2.922416, -2.9048283, -2.885477, -2.8643742, -2.8415325, -2.816965,
  -2.790688, -2.762717, -2.7330682, -2.7017603, -2.6688128, -2.6342442, -2.5980763, -2.560332,
  -2.5210328, -2.4802027, -2.4378672, -2.3940523, -2.3487833, -2.3020883, -2.253996, -2.2045355,
  -2.153736, -2.1016297, -2.0482473, -1.9936211, -1.9377848, -1.8807718, -1.8226173, -1.7633567,
  -1.7030246, -1.6416595, -1.5792968, -1.5159762, -1.4517351, -1.3866122, -1.320648, -1.2538815,
  -1.1863544, -1.118107, -1.0491802, -0.979617, -0.9094586, -0.8387486, -0.76752937, -0.6958436,
  -0.623736, -0.55124915, -0.4784282, -0.4053169, -0.33195877, -0.25839984, -0.18468332, -0.1108554,
  -0.03695945
};

// simulate the bouncing ball via sin table
// including the "fall" positions
float ballYFallingPosition[] = {
  -0.03695945, -0.1108554, -0.18468332, -0.25839984, -0.33195877, -0.4053169, -0.4784282, -0.55124915,
  -0.623736, -0.6958436, -0.76752937, -0.8387486, -0.9094586, -0.979617, -1.0491802, -1.118107,
  -1.1863544, -1.2538815, -1.320648, -1.3866122, -1.4517351, -1.5159762, -1.5792968, -1.6416595,
  -1.7030246, -1.7633567, -1.8226173, -1.8807718, -1.9377848, -1.9936211, -2.0482473, -2.1016297,
  -2.153736, -2.2045355, -2.253996, -2.3020883, -2.3487833, -2.3940523, -2.4378672, -2.4802027,
  -2.5210328, -2.560332, -2.5980763, -2.6342442, -2.6688128, -2.7017603, -2.7330682, -2.762717,
  -2.790688, -2.816965, -2.8415325, -2.8643742, -2.885477, -2.9048283, -2.922416, -2.9382293,
  -2.952259, -2.9644966, -2.974934, -2.9835658, -2.990386, -2.995391, -2.998577, -2.9999433,
  -2.9999433, -2.998577, -2.995391, -2.990386, -2.9835658, -2.974934, -2.9644966,
  -2.952259, -2.9382293, -2.922416, -2.9048283, -2.885477, -2.8643742, -2.8415325, -2.816965,
  -2.790688, -2.762717, -2.7330682, -2.7017603, -2.6688128, -2.6342442, -2.5980763, -2.560332,
  -2.5210328, -2.4802027, -2.4378672, -2.3940523, -2.3487833, -2.3020883, -2.253996, -2.2045355,
  -2.153736, -2.1016297, -2.0482473, -1.9936211, -1.9377848, -1.8807718, -1.8226173, -1.7633567,
  -1.7030246, -1.6416595, -1.5792968, -1.5159762, -1.4517351, -1.3866122, -1.320648, -1.2538815,
  -1.1863544, -1.118107, -1.0491802, -0.979617, -0.9094586, -0.8387486, -0.76752937, -0.6958436,
  -0.623736, -0.55124915, -0.4784282, -0.4053169, -0.33195877, -0.25839984, -0.18468332, -0.1108554,
  -0.03695945,
  0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0, 1.1, 1.2, 1.3, 1.4, 1.5, 1.6, 1.7, 1.8, 1.9,
  2.1, 2.2, 2.3, 2.4, 2.5, 2.6, 2.7, 2.8, 2.9, 3.0, 3.1, 3.2, 3.3, 3.4, 3.5, 3.6, 3.7, 3.8, 3.9,
  4.1, 4.2, 4.3, 4.4, 4.5, 4.6, 4.7, 4.8, 4.9, 5.0, 5.1, 5.2, 5.3, 5.4, 5.5, 5.6, 5.7, 5.8, 5.9,
  6.1, 6.2, 6.3, 6.4, 6.5, 6.6, 6.7, 6.8, 6.9, 7.0, 7.1, 7.2, 7.3, 7.4, 7.5, 7.6, 7.7, 7.8, 7.9,
  8.1, 8.2, 8.3, 8.4, 8.5, 8.6, 8.7, 8.8, 8.9, 9.0, 9.1, 9.2, 9.3, 9.4, 9.5, 9.6, 9.7, 9.8, 9.9,
  10.1, 10.2, 10.3, 10.4, 10.5, 10.6, 10.7, 10.8, 10.9, 11.0, 11.1, 11.2, 11.3, 11.4, 11.5, 11.6, 11.7, 11.8, 11.9,
  12.1, 12.2, 12.3, 12.4, 12.5, 12.6, 12.7, 12.8, 12.9, 13.0, 13.1, 13.2, 13.3, 13.4, 13.5, 13.6, 13.7, 13.8, 13.9,
  14.1, 14.2, 14.3, 14.4, 14.5, 14.6, 14.7, 14.8, 14.9, 15.0, 15.1, 15.2, 15.3, 15.4, 15.5, 15.6, 15.7, 15.8, 15.9,
  16.1, 16.2, 16.3, 16.4, 16.5, 16.6, 16.7, 16.8, 16.9, 17.0, 17.1, 17.2, 17.3, 17.4, 17.5, 17.6, 17.7, 17.8, 17.9,
  18.1, 18.2, 18.3, 18.4, 18.5, 18.6, 18.7, 18.8, 18.9, 19.0, 19.1, 19.2, 19.3, 19.4, 19.5, 19.6, 19.7, 19.8, 19.9,
  20.1, 20.2, 20.3, 20.4, 20.5, 20.6, 20.7, 20.8, 20.9, 21.0, 21.1, 21.2, 21.3, 21.4, 21.5, 21.6, 21.7, 21.8, 21.9,
  22.1, 22.2, 22.3, 22.4, 22.5, 22.6, 22.7, 22.8, 22.9, 23.0, 23.1, 23.2, 23.3, 23.4, 23.5, 23.6, 23.7, 23.8, 23.9,
  24.1, 24.2, 24.3, 24.4, 24.5, 24.6, 24.7, 24.8, 24.9, 25.0, 25.1, 25.2, 25.3, 25.4, 25.5, 25.6, 25.7, 25.8, 25.9,
};

enum ballXBoundary ballXBoundaries[totalBalls];
int ballYPositionShift[totalBalls];

vec3 ballRotationDefaults[totalBalls] = {
  { 0.04, 0.0323, 0.04 },
  { 0.025, 0.04, 0.0 },
  { 0.03, 0.023, 0.016 },
  { 0.024, 0.06, 0.0 },
  { 0.032, 0.056, 0.08 },
  { 0.04, 0.0323, 0.04 },
  { 0.025, 0.04, 0.0 },
  { 0.03, 0.023, 0.016 },
  { 0.024, 0.06, 0.0 },
  { 0.032, 0.056, 0.08 }
}; 
vec3 ballRotation[totalBalls]; 
mat4 ballRotationMatrix[totalBalls];

float ballXPosition[totalBalls];
enum direction ballDirection[totalBalls];

// 

BOOL initCurve(void) {
  FLOAT angle;
  ULONG i, amplitude;

  angle = 180.0;
  amplitude = 16;
  
  for (i = 0;i < CURVE_SCROLL;i++) {
    curve[i] = (int)(amplitude + (sin(RAD(angle)) * amplitude));
    angle += (360.0 / CURVE_SCROLL);
  }
  return TRUE;
}

// OBJ LOADER

static void StripWhites(char *dest, char *src) {
	int spos = 0;
  int dlen = 0;
  
	while(src[spos] && isspace(src[spos]))
		spos++;

	strcpy(dest, src + spos);

	dlen = strlen(dest) - 1;
	while(dest[dlen] && isspace(dest[dlen])) {
		dest[dlen] = 0;
		dlen--;
	}
}

static void GetBasePath(char *basePath, int maxLen, const char *modelName) {
  int pathLen = 0;

	strncpy(basePath, modelName, maxLen - 1);
	pathLen = strlen(basePath);
	
	if(pathLen > maxLen)
		pathLen = maxLen;
		
	while(pathLen) {
		if(basePath[pathLen] == '/') {
			basePath[pathLen] = 0;
			printf("basePath : %s\n", basePath);
			return;
		}
		pathLen--;
	}
	basePath[0] = 0;
}

vec3 allVertexes[10000];
vec3 allNormals[10000];
vec3 allUV[10000];

BOOL LoadObjModel(const char *modelName) {
	vec3 vertex;
	vec3 normals;
	vec3 uv;
	
	int vertexCount = 0;
	int normalsCount = 0;
	int uvCount = 0;
	int indexInternalCount = 0;

	FILE *objfp;
	char basePath[256];

	objfp = fopen(modelName, "rb");

	if(!objfp) return FALSE;

	memset(basePath, 0, sizeof(basePath));

	GetBasePath(basePath, sizeof(basePath)-1, modelName);

	while(!feof(objfp)) {
		static char line[2048];
		static char paramline[2048];
		memset(line, 0, sizeof(line));
		memset(paramline, 0, sizeof(paramline));

		fgets(line, 2048, objfp);
		
		// extract and add the vertexes x/y/z
		if(!strncmp(line, "v ", 2)) {
			StripWhites(paramline, line + 1);
			sscanf(paramline, "%f %f %f", &vertex.x, &vertex.y, &vertex.z);
			
			vertex.x *= 2.0;
			vertex.y *= 2.0;
			vertex.z *= 2.0;
			
			allVertexes[vertexCount] = vertex;
			
			vertexCount++;
		}
		
		// extract and add the normals
		else if(!strncmp(line, "vn ", 3)) {
			StripWhites(paramline, line + 2);
			sscanf(paramline, "%f %f %f", &normals.x, &normals.y, &normals.z);

			allNormals[normalsCount] = normals;
			
			normalsCount++;
		}
		
		// extract and add texture coords x/y/z
		else if(!strncmp(line, "vt ", 3)) {
			StripWhites(paramline, line + 2);
			sscanf(paramline, "%f %f", &uv.x, &uv.y);
			
			uv.y = uv.y;
			uv.z = 0.0f;
			
			allUV[uvCount] = uv;
			
			uvCount++;
		}

		// 
		else if(!strncmp(line, "f ", 2)) {
			int aV,bV,cV,dV;
			int aVN,bVN,cVN,dVN;
			int aVT,bVT,cVT,dVT;
	    int ret;
			
			StripWhites(paramline, line + 1);
			
	    ret = sscanf(paramline, "%d/%d/%d %d/%d/%d %d/%d/%d %d/%d/%d",
	     &aV, &aVT, &aVN,
	     &bV, &bVT, &bVN,
	     &cV, &cVT, &cVN, 
	     &dV, &dVT, &dVN);
	     
			// get the positions and normals
			BallVertices[totalVertexBufferCount].pos = allVertexes[aV-1];
			BallVertices[totalVertexBufferCount].normal = allNormals[aVN-1];		
			BallVertices[totalVertexBufferCount].colour = 0x00ffffff;
			BallVertices[totalVertexBufferCount].tex[0].u = allUV[aVT-1].x;
			BallVertices[totalVertexBufferCount].tex[0].v = allUV[aVT-1].y;
			
			totalVertexBufferCount++;
			
			BallVertices[totalVertexBufferCount].pos = allVertexes[bV-1];
			BallVertices[totalVertexBufferCount].normal = allNormals[bVN-1];
			BallVertices[totalVertexBufferCount].colour = 0x00ffffff;
			BallVertices[totalVertexBufferCount].tex[0].u = allUV[bVT-1].x;
			BallVertices[totalVertexBufferCount].tex[0].v = allUV[bVT-1].y;
							
			totalVertexBufferCount++;
			
			BallVertices[totalVertexBufferCount].pos = allVertexes[cV-1];
			BallVertices[totalVertexBufferCount].normal = allNormals[cVN-1];
			BallVertices[totalVertexBufferCount].colour = 0x00ffffff;
			BallVertices[totalVertexBufferCount].tex[0].u = allUV[cVT-1].x;
			BallVertices[totalVertexBufferCount].tex[0].v = allUV[cVT-1].y;
			
			totalVertexBufferCount++;
			
			if (ret == 9) {
				BallIndices[totalTrianglesCount] = indexInternalCount;
				BallIndices[totalTrianglesCount+1] = indexInternalCount + 1;
				BallIndices[totalTrianglesCount+2] = indexInternalCount + 2;
				//BallIndices[totalTrianglesCount+3] = 0xffff;
				
				indexInternalCount = indexInternalCount + 3;
				totalTrianglesCount = totalTrianglesCount + 3;
			}
			
			// in case we have a quad, we convert it to a tris: a+b+c and b+c+d
			if (ret == 12) {
				BallVertices[totalVertexBufferCount].pos = allVertexes[bV-1];
				BallVertices[totalVertexBufferCount].normal = allNormals[bVN-1];
				BallVertices[totalVertexBufferCount].colour = 0x00ffffff;
				BallVertices[totalVertexBufferCount].tex[0].u = allUV[bVT-1].x;
				BallVertices[totalVertexBufferCount].tex[0].v = allUV[bVT-1].y;
				
				totalVertexBufferCount++;
			
				BallVertices[totalVertexBufferCount].pos = allVertexes[dV-1];
				BallVertices[totalVertexBufferCount].normal = allNormals[dVN-1];
				BallVertices[totalVertexBufferCount].colour = 0x00ffffff;
				BallVertices[totalVertexBufferCount].tex[0].u = allUV[dVT-1].x;
				BallVertices[totalVertexBufferCount].tex[0].v = allUV[dVT-1].y;
			
				totalVertexBufferCount++;	
							
				BallVertices[totalVertexBufferCount].pos = allVertexes[cV-1];
				BallVertices[totalVertexBufferCount].normal = allNormals[cVN-1];
				BallVertices[totalVertexBufferCount].colour = 0x00ffffff;
				BallVertices[totalVertexBufferCount].tex[0].u = allUV[cVT-1].x;
				BallVertices[totalVertexBufferCount].tex[0].v = allUV[cVT-1].y;
				
			  totalVertexBufferCount++;
				
				BallIndices[totalTrianglesCount] = indexInternalCount;
				BallIndices[totalTrianglesCount+1] = indexInternalCount + 1;
				BallIndices[totalTrianglesCount+2] = indexInternalCount + 2;
				//BallIndices[totalTrianglesCount+3] = 0xffff;
				
				indexInternalCount = indexInternalCount + 3;
				totalTrianglesCount = totalTrianglesCount + 3;
			}
		}
	}

	//printf("total vertexes count= %d\n", totalVertexBufferCount);
	//printf("total triangles count= %d\n", totalTrianglesCount);

	fclose(objfp);

	return TRUE;
}

BOOL LoadTexture(const char *textureName) {
  UBYTE *data = NULL;
  FILE *fp;
  int size;
  int mipMappingSize = 6;
  
  fp = fopen(textureName, "rb");
  
  if (!fp) return FALSE;

  fseek(fp, 0, SEEK_END);
  size = ftell(fp);
  fseek(fp, 128, SEEK_SET);

  data = AllocMem(size - 128, MEMF_ANY);
  fread(data, 1, size - 128, fp);
  fclose(fp);

  /*
  	The mipmappig texture size supported are
		9 - 512x512
		8 - 256x256
		7 - 128x128
		6 - 64x64
	*/
	txtr = magAllocateTexture(mipMappingSize);
  magUploadTexture(txtr, mipMappingSize, data, 0);

  FreeMem(data, size - 128);

  return TRUE;
}

BOOL initMaggieEngine(void) {
  mat4_identity(&worldMatrix);
  mat4_translate(&viewMatrix, cameraPosition.x, cameraPosition.y, cameraPosition.z);
  mat4_perspective(&perspective, 60.0f, targetRatio, 0.01f, 120.0f);
 	
  vBuffer = magAllocateVertexBuffer(totalVertexBufferCount);
  iBuffer = magAllocateIndexBuffer(totalTrianglesCount);
  
  magUploadVertexBuffer(vBuffer, BallVertices, 0, totalVertexBufferCount);
  magUploadIndexBuffer(iBuffer, BallIndices, 0, totalTrianglesCount);

  return TRUE;
}

// CORE FUNCTIONS

void visualDebug(void) {
	// Draw the fps counter
  SAGE_PrintFText(10, 10, "%d fps", SAGE_GetFps());
}


void updateKeyboardKeysListener(void) {
  SAGE_Event *event = NULL;
  
  // read all events raised by the screen
	while ((event = SAGE_GetEvent()) != NULL) {
		// If we click on mouse button, we stop the loop
  	if (event->type == SEVT_MOUSEBT) {
   		finish3DSection1 = TRUE;
  	}
  	// If we press the ESC key, we stop the loop
		else if (event->type == SEVT_RAWKEY && event->code == SKEY_EN_ESC) {
   		finish3DSection1 = TRUE;
   	}
  }
}

void update3DBalls(void) {
  mat4 xRot, yRot, zRot;
  int i;
  
  // rotate all the ball independently
	for (i = 0; i<totalBalls; i++) {
  	mat4_rotateX(&xRot, ballRotation[i].x);
    mat4_rotateY(&yRot, ballRotation[i].y);
    //mat4_rotateZ(&zRot, ballRotation[i].z);
    mat4_mul(&ballRotationMatrix[i], &xRot, &yRot);
    
    switch (ballDirection[i]) {
    	case towardRight:
      	ballRotation[i].x -= ballRotationDefaults[i].x;
        ballRotation[i].y -= ballRotationDefaults[i].y;
        ballRotation[i].z -= ballRotationDefaults[i].z;
    	break;
    	
    	case towardLeft:
      	ballRotation[i].x += ballRotationDefaults[i].x;
        ballRotation[i].y += ballRotationDefaults[i].y;
        ballRotation[i].z -= ballRotationDefaults[i].z;
    	break;
  	}
  }
  
 	// camera position
 	//mat4_translate(&viewMatrix, cameraPosition.x, cameraPosition.y, cameraPosition.z);
}

void updateScrolltext(void) {
  UBYTE new_char;
  UWORD char_index;
  
  // Should we load a new char from the message
  if (char_load == 0 && !isFinishSection1) {
    new_char = message[message_pos];
    
    // have we reach the end of the message?
    // let's call it a close for section1 and move to the next section
    if (new_char == 0) {
      //isFinishSection1 = TRUE;
      startTransitionToSection2 = TRUE;
      calledVampireLogoOutro = TRUE;
      // wrap the message;
      //message_pos = 0;
      //new_char = message[message_pos];
    }
    
    char_load = FONT_WIDTH;
    
    // next char
    message_pos++;          
    
    if (new_char < ' ') {
      char_index = 0;
    }
    else {
    	// "Space" is our first char in the font picture
      char_index = new_char - ' ';
      // If we have some chars that are not in our fonts
      if (char_index >= FONT_NUM) {
        char_index = 0;
      }
    }
    
    // blit the char to the layer
    SAGE_BlitPictureToLayer(
    	font_picture,
     	font_posx[char_index],
     	font_posy[char_index],
     	FONT_WIDTH,
     	FONT_HEIGHT,
     	TEXTFIELD_LAYER,
     	char_posx,
     	0);
    
    
    char_posx += FONT_WIDTH;
    
    if (char_posx > SCREEN_WIDTH) {
      char_posx = 0;
    }
  }
  
  char_load -= TEXTSCROLL_SPEED;
  layer_posx += TEXTSCROLL_SPEED;
  
  if (layer_posx >= (SCREEN_WIDTH + FONT_WIDTH)) {
    layer_posx = 0;
  }
    
  // Y position of the text
  // Move the scroll up and down
  scroll_posy = 204 + curve[curve_idx++];
  curve_idx %= CURVE_SCROLL;
}

void updateSection1(void) {
  updateKeyboardKeysListener();
  update3DBalls();
  
  if (!startTransitionToSection2) {
    updateScrolltext();
  }
}

// vars for the outro sequence from section 1 to section 2
float vampireLogoOutroTime = 0;
float vampireLogoOutroDuration = 80;
int currentLine = 0;
int currentTotalLines = 0;
int currentColumn = 0;
int currentTotalColumns = 0;
int currentX = 10, trackXposition = 0, currentDitherXPosition = 0;

void renderSection1(void) {
  int i = 0, b = 0, totalBallsYCompleted = 0;
  float startZ = 40.0f;
  float *randomFloatArray;
  
  int ballsYCompleted[] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0
  };
  
  // Sage to get the current pixel buffer
  SAGE_Bitmap *back_bitmap = SAGE_GetBackBitmap();
  
	// clear the back screen
	SAGE_ClearScreen();

	if (startTransitionToSection2) {
  	// moving the vampire logo out of the screen
  	if (vampireLogoOutroTime <= vampireLogoOutroDuration) {
      SAGE_BlitSpriteToScreen(
        SPRITE_BANK,
        VAMPIRE_LOGO_SPRITE,
        (int)(SCREEN_WIDTH / 2),
        (int)(Lerp(48, -48, EaseOutQuint(vampireLogoOutroTime/vampireLogoOutroDuration))));
      vampireLogoOutroTime += 1;
    }
    else {
      calledGridChessOutro = TRUE;
      
      currentTotalLines = GRID_CHESS_LAYER_HEIGHT/16;
      currentTotalColumns = GRID_CHESS_LAYER_WIDTH/16;
      currentDitherXPosition = ATLAS_WIDTH - 96;
    }
     
    if (!calledGridChessOutro) { 
      atlasBlitGridChess();
    }
    else {
      // start the dithering filter animation outro for the the grid chess
      if (currentLine < currentTotalLines) {
        if (currentX >= 0) {
           for (currentColumn = 0; currentColumn < currentTotalColumns; currentColumn++) {
            trackXposition = currentColumn * 16;
            
          	SAGE_BlitPictureToLayer(
                 atlas_picture, // dither filter animation is in atlas picture
                 currentDitherXPosition,
                 16 * currentX,
                 16,
                 16,
                 GRID_CHESS_LAYER,
                 trackXposition,
                 currentLine * 16);
          }
          
          SAGE_BlitLayerToScreen(GRID_CHESS_LAYER,
                                 (SCREEN_WIDTH - GRID_CHESS_WIDTH) / 2,
                                 (SCREEN_HEIGHT - GRID_CHESS_HEIGHT) / 2 + 46);
          currentX -= 1;
        }
        
        if (currentX < 0) {
          currentX = 10;
          currentLine += 1;
        }
      }
      else {
        calledBallsOutro = TRUE;
      }
    }
	}
	else {
  	// blit the logo (as a sprite) and the chess grid
    atlasBlitGridChess();
    SAGE_BlitSpriteToScreen(
      SPRITE_BANK,
      VAMPIRE_LOGO_SPRITE,
      (int)(SCREEN_WIDTH / 2),
      (int)46);
	}
	
	// Set the text layer view (using the wrapping feature of layers to simulate infinite scroll)
  SAGE_SetLayerView(TEXTFIELD_LAYER, layer_posx, layer_posy, SCREEN_WIDTH, TEXTFIELD_HEIGHT);
  // Blit the text layer to the screen
  SAGE_BlitLayerToScreen(TEXTFIELD_LAYER, TEXTSCROLL_POSX, scroll_posy);
  
	// Maggie render

	// --> Maggie start block
  magBeginScene();

	// draw options:
	// MAG_DRAWMODE_DEPTHBUFFER | MAG_DRAWMODE_32BIT | MAG_DRAWMODE_NORMAL | MAG_DRAWMODE_BILINEAR | MAG_DRAWMODE_LIGHTING
	magSetDrawMode(MAG_DRAWMODE_NORMAL | MAG_DRAWMODE_LIGHTING);
	
  magSetScreenMemory(back_bitmap->bitmap_buffer, SCREEN_WIDTH, SCREEN_HEIGHT);
  
  // clear options
	// MAG_CLEAR_COLOUR | MAG_CLEAR_DEPTH
	//magClear(MAG_CLEAR_DEPTH);

  magSetWorldMatrix((float *)&worldMatrix);
  magSetViewMatrix((float *)&viewMatrix);
  magSetPerspectiveMatrix((float *)&perspective);

  // add lights
	magSetLightPosition(0, 20.0f, -20.0f, -10.0f);
	magSetLightColour(0, 0x00ffffff);
	magSetLightAttenuation(0, 1400.0f);	
	magSetLightType(0, MAG_LIGHT_POINT);

	magSetLightPosition(1, -20.0f, 20.0f, 20.0f);
	magSetLightColour(1, 0x00ffffff);
	magSetLightAttenuation(1, 300.0f);	
	magSetLightType(1, MAG_LIGHT_POINT);
	
	magSetLightColour(2, 0x000000);	
	magSetLightType(2, MAG_LIGHT_AMBIENT);
		
	// set the main UV texture
	magSetTexture(0, txtr);
  magSetVertexBuffer(vBuffer);
  magSetIndexBuffer(iBuffer);
  
  randomFloatArray = randomFloatNumbers(totalBalls, 0.05, 0.1);
  
  // create N amount of balls
	for (i = 0; i<totalBalls; i++) {
    mat4 transMatrix;
    
    if (calledBallsOutro) {
      // change the position of XYZ
      // increment the position in Y to simulate the balls falling
      mat4_translate(&transMatrix,
                    ballXPosition[i],
                    floorY + (ballYFallingPosition[ballYPositionShift[i]] * 1.4),
                    startZ);
               
      // shift of N position of the Y position array in order to have a wave effect where all the balls are
  		// shifted in Y position to not look boring
      ballYPositionShift[i] += 3;
      
      // reaching the end of the array means we start from index 0
      if (ballYPositionShift[i] >= ballYFallingPositionsTotal) {
        ballYPositionShift[i] = ballYFallingPositionsTotal - 1;
        ballsYCompleted[i] = 1;
      }
      
      for (b = 0; b<totalBalls; b++) {
        totalBallsYCompleted += ballsYCompleted[b];
      }         
      
      if (totalBallsYCompleted == totalBalls) {
        finish3DSection1 = TRUE;
      }
    }
    else {
      // change the position of XYZ
      mat4_translate(&transMatrix,
                    ballXPosition[i],
                    floorY + (ballYPosition[ballYPositionShift[i]] * 1.4),
                    startZ);
                    
      // shift of N position of the Y position array in order to have a wave effect where all the balls are
  		// shifted in Y position to not look boring
      ballYPositionShift[i] += 3;
      
      // reaching the end of the array means we start from index 0
      if (ballYPositionShift[i] >= ballYPositionsTotal) {
        ballYPositionShift[i] = 0;
      }
    }
    
    // apply the position+rotation to the world
		mat4_mul(&worldMatrix, &transMatrix, &ballRotationMatrix[i]);
		
		// update of the world
		magSetWorldMatrix((float *)&worldMatrix);

    // draw the object with the new position and rotation
		magDrawIndexedTriangles(0, totalVertexBufferCount, 0, totalTrianglesCount);
		
    // move in Z toward the camera
		startZ -= 3.5;
		
		// move in X
  	switch (ballDirection[i]) {
    	
    	case towardRight:
      	ballXPosition[i] += 0.08;//randomFloatArray[i];
      	
      	// switch direction if we hit the right boundary
      	if (ballXPosition[i] > rightXBoundary) {
          ballDirection[i] = towardLeft;
      	}
    	break;
    	
    	case towardLeft:
      	ballXPosition[i] -= 0.08;//randomFloatArray[i];
      	
      	// switch direction if we hit the left boundary
      	if (ballXPosition[i] < leftXBoundary) {
          ballDirection[i] = towardRight;
      	}
    	break;
  	}
	}
	
	free(randomFloatArray);
	
	// <-- Maggie end block
  magEndScene();
  
 	//visualDebug();
 	
  // Switch screen buffers
  SAGE_RefreshScreen();
}

void transitionTo3DSection2(int fillWidth,
                            int fillHeight,
                            int lines,
                            int columns,
                            int color) {
  int i, line, column, trackXposition;
  
  // start the dithering filter animation
	for (line = 0; line < lines; line++) {
    for (column = 0; column < columns; column++) {
      // make a worm style effect
    	if (line%2 == 0) {
      	trackXposition = column * fillWidth;
    	}
    	else {
      	trackXposition = (((columns - 1) * fillWidth)) - (column * fillWidth);
    	}
    	
    	fillAreaLayer(TRANSITION_LAYER,
                  	trackXposition,
                  	line * fillHeight,
                  	fillWidth,
                  	fillHeight,
                  	rgb888_to_rgb565(255, 255, 255));
                  	
      SAGE_BlitLayerToScreen(TRANSITION_LAYER, 0, 0);
      SAGE_RefreshScreen(); 
      //SAGE_Pause(1);
    }
  }
}

void restore(void) {
	if (MaggieBase != NULL) {
    CloseLibrary(MaggieBase);
  }
  
  releaseAllLayers();
  
  magFreeTexture(txtr);
  magFreeVertexBuffer(vBuffer);
  magFreeIndexBuffer(iBuffer);
  
 	free(BallVertices);
	free(BallIndices);
	free(allVertexes);
	free(allNormals);
	free(allUV);
}

// MAIN

void main(int argc, char* argv[]) {
  // generate random X position for each ball and also random intial Y position
  int i, x = ballYPositionsTotal, b;
  float rand, startXPosition = 40.0;
  float* randomFloatArray;

  srand((unsigned int)time(NULL));
  
  randomFloatArray = randomFloatNumbers(totalBalls, leftXBoundary, rightXBoundary);

  for (i=0; i<totalBalls; i++) {
    rand = randomFloatArray[i];
    
    //ballXPosition[i] = rand * 5.0;

    if (i % 2 == 0) {
      ballXPosition[i] = startXPosition;
      ballDirection[i] = towardLeft;
    }
    else {
      ballXPosition[i] = startXPosition * -1.0;
      ballDirection[i] = towardRight;
    }
    
    startXPosition -= 2.5;
    
    // random Y position between each ball to not have all of them jumping from the same height
    ballYPositionShift[i] = x;
    
    x -= (int)fabs(rand * 1.9);
  }
  
  free(randomFloatArray);
  
  // create the sinus wave lookout table for the text
  if (!initCurve()) {
    return FALSE;
  }
  
	/*
	// nor model nor texture
	if (argc == 1) {
		printf("no arguments for 3D object and texture, using default 'assets/cube.obj' 'assets/uv_grid_checker_1.dds'\n");
		filename_object = "assets/cube.obj";
		filename_texture = "assets/uv_grid_checker.dds";
	}

	// just the model
	if (argc == 2) {
		filename_object = argv[1];
		filename_texture = "assets/uv_grid_checker.dds";
	}
	
	// model and the texture
	if (argc == 3) {
		filename_object = argv[1];
		filename_texture = argv[2];
	}
	*/
	filename_object = "assets/crystal.obj";
	filename_texture = "assets/ball_uv_map_64x64.dds";
	
	// INITIALIZE THE DEMO
	
	// the demo will only run on an Apollo Vampire 080
	if (SAGE_ApolloCore() == FALSE) {
		SAGE_AppliLog("Apollo Vampire not found! o_O");
		return;
	}
	else {
		SAGE_AppliLog("Apollo Vampire found! ^_^");
	}
	    
	// init the SAGE system
  if (SAGE_Init(SMOD_VIDEO|SMOD_AUDIO|SMOD_INTERRUPTION)) {
  	
  	// maggie library must be present
  	MaggieBase = OpenLibrary((UBYTE *)"maggie.library", 0);
		if (!MaggieBase) {
    	SAGE_ErrorLog("Can't open maggie.library");
    	return;
  	}
  		
		// load the 3d model
  	if (!LoadObjModel(filename_object)) {
  		SAGE_ErrorLog("Cannot load 3D object model with name %s\n", filename_object);
	  	return;
		}

	 	// load the texture
  	if (!LoadTexture(filename_texture)) {
  		SAGE_ErrorLog("Cannot load texture with name %s\n", filename_texture);
   		return;
  	}
  
		if (!initMaggieEngine()) {
  		SAGE_ErrorLog("Cannot initiate Maggie engine\n");
	 	 	return;
		}
		
		music = SAGE_LoadMusic(filename_music);
  	if (music) {
      if (!SAGE_AddMusic(MUSIC_SLOT, music)) {
        SAGE_ErrorLog("Cannot load music\n");
        SAGE_DisplayError();
        return;
      }
    }
    else {
      SAGE_ErrorLog("Cannot initiate music\n");
  		SAGE_DisplayError();
  		return;
  	}
				
		if (SAGE_OpenScreen(SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_DEPTH, SSCR_STRICTRES)) {	
  		SAGE_HideMouse();
      SAGE_SetTextColor(0,255);
  	 	SAGE_SetDrawingMode(SSCR_TXTTRANSP);
  	 	
  		// fps counter
      if (SAGE_EnableFrameCount(TRUE)) {
        SAGE_MaximumFPS(30);
        // setting to TRUE will set the fps to 60 and mutually negate MaximumFPS()
       	SAGE_VerticalSynchro(FALSE);
      }
      else {
        SAGE_ErrorLog("Can't activate frame rate counter !\n");
      }
			
			loadAtlas();
				
  		// create main layers aka pixel buffers
      createAllLayers();
      createAllSprites();
  		createTextfieldLayer();
  		
  		// load all the assets
  		loadGridChess();
  		loadFont();
  		loadMessage();
  		
  		SAGE_Pause(100);
      
  		// start playing the music
  		SAGE_PlayMusic(MUSIC_SLOT);

			// intro section 1
			animateRunningOnIn();
      animateVampireLogo(SCREEN_WIDTH/2, SCREEN_HEIGHT+ATLAS_VAMPIRE_LOGO_HEIGHT, 152);			
      
      SAGE_Pause(100);
      
      atlasBlitVampireLogo(0, 0);
      ditheringTransition(0,
                          0,
                          16,
                          16,
                          ATLAS_VAMPIRE_LOGO_HEIGHT/16,
                          ATLAS_VAMPIRE_LOGO_WIDTH/16,
                          normal,
                          VAMPIRE_LAYER,
                          (SCREEN_WIDTH - ATLAS_VAMPIRE_LOGO_WIDTH) / 2,
                          (SCREEN_HEIGHT - ATLAS_VAMPIRE_LOGO_HEIGHT) / 2 + 24,
                          fullLine,
                          out,
                          -999);

      animateRunningOnOut();
      
      // intro section 2.a                         
      animatePoweredByIn();
      animateSage();

      SAGE_Pause(100);
      
      atlasBlitSage(0, 0);
      ditheringTransition(0,
                          0,
                          16,
                          16,
                          ATLAS_SAGE_HEIGHT/16,
                          ATLAS_SAGE_WIDTH/16,
                          normal,
                          SAGE_LAYER,
                          (SCREEN_WIDTH - ATLAS_SAGE_WIDTH) / 2,
                          (SCREEN_HEIGHT - ATLAS_SAGE_HEIGHT) / 2 + 32,
                          fullColumn,
                          out,
                          -998);
                   
      // intro section 2.b
      animateMaggieLibrary();
      
      SAGE_Pause(100);
      
      atlasBlitMaggieLibrary(0, 0);
      ditheringTransition(0,
                          0,
                          32,
                          32,
                          ATLAS_MAGGIE_LIBRARY_HEIGHT/32,
                          ATLAS_MAGGIE_LIBRARY_WIDTH/32,
                          normal,
                          MAGGIE_LIBRARY_LAYER,
                          (SCREEN_WIDTH - ATLAS_MAGGIE_LIBRARY_WIDTH) / 2,
                          (SCREEN_HEIGHT - ATLAS_MAGGIE_LIBRARY_HEIGHT) / 2 + 32,
                          worm,
                          out,
                          -998);
                          
      animatePoweredByOut();
      
      // fade in chess grid
      ditheringTransition(0,
                          0,
                          16,
                          16,
                          GRID_CHESS_LAYER_HEIGHT/16,
                          GRID_CHESS_LAYER_WIDTH/16,
                          slow,
                          GRID_CHESS_LAYER,
                          (SCREEN_WIDTH - GRID_CHESS_WIDTH) / 2,
                          (SCREEN_HEIGHT - GRID_CHESS_HEIGHT) / 2 + 46,
                          fullLine,
                          in,
                          0);
                         
      // show vampirelogo at the top                   
      animateVampireLogo(SCREEN_WIDTH / 2,
                         (int)-47,
                         (int)46);
                         
      // we need top add a half second of pause to wait
      // the logo to finish its own animation
      // or there is going to be a visual jump 
      // when starting to blit the sprite in the first section
      SAGE_Pause(25);
        
      while (!finish3DSection1) {
        updateSection1();
				renderSection1();
			}
      
      transitionTo3DSection2(32,
                             32,
                             SCREEN_HEIGHT/32,
                             SCREEN_WIDTH/32,
                             rgb888_to_rgb565(255, 255, 255));
      
			// free memory
			restore();
			
			// stop and clear the music
			SAGE_StopMusic();
      SAGE_ClearMusic();
			// show the mouse
  		SAGE_ShowMouse();
  		// close the screen
  		SAGE_CloseScreen();
		}
	}
	
  // demo ended
	SAGE_Exit();
	// exit message
	SAGE_AppliLog("Thank you for watching! ^_^");
}
