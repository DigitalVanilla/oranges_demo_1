// sc LINK oranges_demo.c CPU=68060 MATH=68882 DATA=far CODE=far IDIR=libinclude: sage:lib/sage.lib NOICONS

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <dos/dos.h>
#include <clib/dos_protos.h>

#include <sage/sage.h>

#include <proto/Maggie.h>
#include <maggie_vec.h>
#include <maggie_vertex.h>
#include <maggie_flags.h>

// GLOBAL VARIABLES

// screen definition
#define SCREEN_WIDTH							640
#define SCREEN_HEIGHT   					512
#define SCREEN_DEPTH    					16

// main layer
#define MAIN_LAYER 								0
#define MAIN_LAYER_WIDTH					SCREEN_WIDTH
#define MAIN_LAYER_HEIGHT					SCREEN_HEIGHT

// atlas layer 
#define	ATLAS_LAYER								1
#define ATLAS_FILENAME						"assets_demo/atlas.png"
#define ATLAS_WIDTH								320
#define ATLAS_HEIGHT   						384

// oranges ascii logo layer 
#define	ORANGES_LAYER							2
#define ORANGES_LOGO_FILENAME			"assets_demo/oranges_ascii_logo.png"
#define ORANGES_WIDTH							SCREEN_WIDTH
#define ORANGES_HEIGHT   					SCREEN_HEIGHT

// grid layer 
#define	GRID_CHESS_LAYER					3
#define GRID_CHESS_FILENAME				"assets_demo/grid_chess.png"
#define GRID_CHESS_WIDTH					640
#define GRID_CHESS_HEIGHT   			118

// atlas layer pieces
// running on
#define ATLAS_RUNNING_ON_X				0
#define ATLAS_RUNNING_ON_Y				64
#define ATLAS_RUNNING_ON_WIDTH		192
#define ATLAS_RUNNING_ON_HEIGHT		64

// powered by
#define ATLAS_POWERED_BY_X				0
#define ATLAS_POWERED_BY_Y				0
#define ATLAS_POWERED_BY_WIDTH		192
#define ATLAS_POWERED_BY_HEIGHT		64

// vampire logo
#define ATLAS_VAMPIRE_LOGO_X			0
#define ATLAS_VAMPIRE_LOGO_Y			128
#define ATLAS_VAMPIRE_LOGO_WIDTH	224
#define ATLAS_VAMPIRE_LOGO_HEIGHT	128

// sage
#define ATLAS_SAGE_X							0
#define ATLAS_SAGE_Y							256
#define ATLAS_SAGE_WIDTH					128
#define ATLAS_SAGE_HEIGHT					64

// maggie
#define ATLAS_MAGGIE_LIBRARY_X			0
#define ATLAS_MAGGIE_LIBRARY_Y			320
#define ATLAS_MAGGIE_LIBRARY_WIDTH	320
#define ATLAS_MAGGIE_LIBRARY_HEIGHT	64

// FONT

#define FONT_FILENAME					"assets_demo/font_16x20.png"
#define FONT_WIDTH            16
#define FONT_HEIGHT           20
#define FONT_NUM              60
#define FONTPIC_WIDTH         160
#define FONTPIC_HEIGHT        120
#define FONTPIC_TRANSPARENCY  0x000000

#define TEXTFIELD_WIDTH       SCREEN_WIDTH+FONT_WIDTH
#define TEXTFIELD_HEIGHT      FONT_HEIGHT
#define TEXTFIELD_LAYER       4

#define TEXTSCROLL_SPEED     	1
#define TEXTSCROLL_POSX       0

// TEXT MESSAGE

#define MESSAGE_FILENAME			"assets_demo/message.txt"

// GLOBAL VARIABLES

BOOL finish = FALSE;
SAGE_Picture *atlas_picture, *oranges_logo_picture, *grid_chess_picture, *font_picture;

STRPTR message = NULL;
UWORD message_pos = 0, font_posx[FONT_NUM], font_posy[FONT_NUM], char_posx = SCREEN_WIDTH, char_load = 0;
UWORD layer_posx = SCREEN_WIDTH+FONT_WIDTH, layer_posy = 0, scroll_posy = 0;


// MAIN 

BOOL createMainLayer(void) {
	if (SAGE_CreateLayer(MAIN_LAYER, MAIN_LAYER_WIDTH, MAIN_LAYER_HEIGHT)) {  
    return TRUE;
  }
}

void clearMainLayer(void) {
	if (MAIN_LAYER != NULL) {
		SAGE_ReleaseLayer(MAIN_LAYER);
	}
	createMainLayer();
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
	SAGE_BlitLayerToScreen(ORANGES_LAYER, 0, 0);
	SAGE_RefreshScreen();
}

// ATLAS

BOOL loadAtlas(void) {
	atlas_picture = SAGE_LoadPicture(ATLAS_FILENAME);

	if (atlas_picture == NULL) {
  	SAGE_DisplayError();
  	return FALSE;
  }
    
  if (SAGE_CreateLayerFromPicture(ATLAS_LAYER, atlas_picture)) { 
  	return TRUE;
 	}
  
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

BOOL createTextfieldLayer(void)
{
  if (SAGE_CreateLayer(TEXTFIELD_LAYER, TEXTFIELD_WIDTH, TEXTFIELD_HEIGHT)) {
    SAGE_SetLayerTransparency(TEXTFIELD_LAYER, FONTPIC_TRANSPARENCY);
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

// Blit helpers

void atlasBlitRunningOn(void) {
	SAGE_BlitPictureToLayer(
		atlas_picture,
	 	ATLAS_RUNNING_ON_X,
	 	ATLAS_RUNNING_ON_Y,
	 	ATLAS_RUNNING_ON_WIDTH,
	 	ATLAS_RUNNING_ON_HEIGHT,
	 	MAIN_LAYER,
	 	225,
	 	97);
	SAGE_BlitLayerToScreen(MAIN_LAYER, 0, 0);
	SAGE_RefreshScreen();
}

void atlasBlitVampireLogo(int x_, int y_) {
	SAGE_BlitPictureToLayer(
		atlas_picture,
	 	ATLAS_VAMPIRE_LOGO_X,
	 	ATLAS_VAMPIRE_LOGO_Y,
	 	ATLAS_VAMPIRE_LOGO_WIDTH,
	 	ATLAS_VAMPIRE_LOGO_HEIGHT,
	 	MAIN_LAYER,
	 	x_,
	 	y_);
	 	
	SAGE_BlitLayerToScreen(MAIN_LAYER, 0, 0);
}

void atlasBlitPoweredBy(void) {
	SAGE_BlitPictureToLayer(
		atlas_picture,
	 	ATLAS_POWERED_BY_X,
	 	ATLAS_POWERED_BY_Y,
	 	ATLAS_POWERED_BY_WIDTH,
	 	ATLAS_POWERED_BY_HEIGHT,
	 	MAIN_LAYER,
	 	225,
	 	97);
	SAGE_BlitLayerToScreen(MAIN_LAYER, 0, 0);
	SAGE_RefreshScreen();
}

void atlasBlitSage(void) {
	SAGE_BlitPictureToLayer(
		atlas_picture,
	 	ATLAS_SAGE_X,
	 	ATLAS_SAGE_Y,
	 	ATLAS_SAGE_WIDTH,
	 	ATLAS_SAGE_HEIGHT,
	 	MAIN_LAYER,
	 	257,
	 	257);
	 	
	SAGE_BlitLayerToScreen(MAIN_LAYER, 0, 0);
	SAGE_RefreshScreen();
}

void atlasBlitMaggieLibrary(void) {
	SAGE_BlitPictureToLayer(
		atlas_picture,
	 	ATLAS_MAGGIE_LIBRARY_X,
	 	ATLAS_MAGGIE_LIBRARY_Y,
	 	ATLAS_MAGGIE_LIBRARY_WIDTH,
	 	ATLAS_MAGGIE_LIBRARY_HEIGHT,
	 	MAIN_LAYER,
	 	161,
	 	257);
	 	
	SAGE_BlitLayerToScreen(MAIN_LAYER, 0, 0);
	SAGE_RefreshScreen();
}

void atlasBlitGridChess(void) {
	SAGE_BlitPictureToLayer(
		grid_chess_picture,
	 	0,
	 	0,
	 	GRID_CHESS_WIDTH,
	 	GRID_CHESS_HEIGHT,
	 	MAIN_LAYER,
	 	0,
	 	288);
	 	
	SAGE_BlitLayerToScreen(MAIN_LAYER, 0, 0);
}

// 3D

char *filename_object;
char *filename_texture;

int startface;
int values;
int maxVerts = 0;
int totalVerts = 0;
int totalFaces = 0;

struct Library *MaggieBase = NULL;

vec3 allVertexes[50000];
vec3 allNormals[50000];
vec3 allUV[50000];

static struct MaggieVertex BallVertices[2880];
UWORD BallIndices[3840];

UWORD txtr = 0xffff;
mat4 worldMatrix, viewMatrix, perspective;

// note: using 640x512 we need to use 4:3 ratio
// otherwise for wider resolution we use 16:9 ratio
float targetRatio = 3.0f / 4.0f; //9.0f / 16.0f;

int vBuffer;
int iBuffer;
float xangle = 0.0f;
float yangle = 0.0f;
vec3 cameraPosition = { 0.0f, 0.0f, 9.0f };
float cameraStepMovement = 0.05f;

// OBJ Loader helpers

static void StripWhites(char *dest, char *src) {
	int spos = 0;
  int dlen = 0;
  
	while(src[spos] && isspace(src[spos]))
		spos++;

	strcpy(dest, src + spos);

	dlen = strlen(dest) - 1;
	while(dest[dlen] && isspace(dest[dlen]))
	{
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
	while(pathLen)
	{
		if(basePath[pathLen] == '/')
		{
			basePath[pathLen] = 0;
			printf("basePath : %s\n", basePath);
			return;
		}
		pathLen--;
	}
	basePath[0] = 0;
}

vec3 vertex;
vec3 normals;
vec3 uv;
int vertexCount = 0;
int normalsCount = 0;
int uvCount = 0;
int masterCount = 0;
int indexCount = 0;
int indexInternalCount = 0;
int loop = 0;

// OBJ loader

BOOL LoadObjModel(const char *modelName) {
	FILE *objfp;
	FILE *mtrlfp;
	char basePath[256];

	objfp = fopen(modelName, "rb");

	if(!objfp) return FALSE;

	memset(basePath, 0, sizeof(basePath));

	GetBasePath(basePath, sizeof(basePath)-1, modelName);

	while(!feof(objfp))
	{
		static char line[2048];
		static char paramline[2048];
		memset(line, 0, sizeof(line));
		memset(paramline, 0, sizeof(paramline));

		fgets(line, 2048, objfp);
		
		// extract and add the vertexes x/y/z
		if(!strncmp(line, "v ", 2))
		{
			StripWhites(paramline, line + 1);
			sscanf(paramline, "%f %f %f", &vertex.x, &vertex.y, &vertex.z);
			
			allVertexes[vertexCount] = vertex;
			
			vertexCount++;
			// printf("vertexCount= %d\n", vertexCount);
			// printf("x=%f y=%f z=%f\n", vertex.x, vertex.y, vertex.z);
		}
		
		// extract and add the normals
		else if(!strncmp(line, "vn ", 3))
		{
			StripWhites(paramline, line + 2);
			sscanf(paramline, "%f %f %f", &normals.x, &normals.y, &normals.z);

			allNormals[normalsCount] = normals;
			
			normalsCount++;
		}
		
		// extract and add texture coords x/y/z
		else if(!strncmp(line, "vt ", 3))
		{
			StripWhites(paramline, line + 2);
			sscanf(paramline, "%f %f", &uv.x, &uv.y);
			
			uv.y = uv.y;// * -1.0f;
			uv.z = 0.0f;
			
			allUV[uvCount] = uv;
			
			uvCount++;
		}
		else if(!strncmp(line, "f ", 2))
		{
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
			BallVertices[masterCount].pos = allVertexes[aV-1];
			BallVertices[masterCount].normal = allNormals[aVN-1];		
			BallVertices[masterCount].colour = 0x00ffffff;
			BallVertices[masterCount].tex[0].u = allUV[aVT-1].x;
			BallVertices[masterCount].tex[0].v = allUV[aVT-1].y;
			
			masterCount++;
			
			BallVertices[masterCount].pos = allVertexes[bV-1];
			BallVertices[masterCount].normal = allNormals[bVN-1];
			BallVertices[masterCount].colour = 0x00ffffff;
			BallVertices[masterCount].tex[0].u = allUV[bVT-1].x;
			BallVertices[masterCount].tex[0].v = allUV[bVT-1].y;
							
			masterCount++;
			
			BallVertices[masterCount].pos = allVertexes[cV-1];
			BallVertices[masterCount].normal = allNormals[cVN-1];
			BallVertices[masterCount].colour = 0x00ffffff;
			BallVertices[masterCount].tex[0].u = allUV[cVT-1].x;
			BallVertices[masterCount].tex[0].v = allUV[cVT-1].y;
			
			masterCount++;
			
			if (ret == 9) {
				BallIndices[indexCount] = indexInternalCount;
				BallIndices[indexCount+1] = indexInternalCount + 1;
				BallIndices[indexCount+2] = indexInternalCount + 2;
				BallIndices[indexCount+3] = 0xffff;
				
				indexInternalCount = indexInternalCount + 3;
				indexCount = indexCount + 4;
			}
			
			// in case we have a quad, we convert it to a tris: a+b+c and b+c+d
			if (ret == 12) {
				BallVertices[masterCount].pos = allVertexes[bV-1];
				BallVertices[masterCount].normal = allNormals[bVN-1];
				BallVertices[masterCount].colour = 0x00ffffff;
				BallVertices[masterCount].tex[0].u = allUV[bVT-1].x;
				BallVertices[masterCount].tex[0].v = allUV[bVT-1].y;
				
				masterCount++;
			
				BallVertices[masterCount].pos = allVertexes[cV-1];
				BallVertices[masterCount].normal = allNormals[cVN-1];
				BallVertices[masterCount].colour = 0x00ffffff;
				BallVertices[masterCount].tex[0].u = allUV[cVT-1].x;
				BallVertices[masterCount].tex[0].v = allUV[cVT-1].y;
			
				masterCount++;	
			
			 	BallVertices[masterCount].pos = allVertexes[dV-1];
				BallVertices[masterCount].normal = allNormals[dVN-1];
				BallVertices[masterCount].colour = 0x00ffffff;
				BallVertices[masterCount].tex[0].u = allUV[dVT-1].x;
				BallVertices[masterCount].tex[0].v = allUV[dVT-1].y;
				
			  masterCount++;
				
				BallIndices[indexCount] = indexInternalCount;
				BallIndices[indexCount+1] = indexInternalCount + 1;
				BallIndices[indexCount+2] = indexInternalCount + 2;
				BallIndices[indexCount+3] = 0xffff;
				
				indexInternalCount = indexInternalCount + 3;
				indexCount = indexCount + 4;
			}
		}
	}

	printf("total tris count= %d\n", indexCount);
	printf("total vertexes count= %d\n", masterCount);

	fclose(objfp);

	return TRUE;
}

BOOL LoadTexture(const char *textureName) {
  UBYTE *data = NULL;
  FILE *fp;
  int size;

  fp = fopen(textureName, "rb");
  
  if (!fp) return FALSE;

  fseek(fp, 0, SEEK_END);
  size = ftell(fp);
  fseek(fp, 128, SEEK_SET);

  data = AllocMem(size - 128, MEMF_ANY);
  fread(data, 1, size - 128, fp);
  fclose(fp);

  txtr = magAllocateTexture(8);
  magUploadTexture(txtr, 8, data, 0);
  FreeMem(data, size - 128);

  return TRUE;
}

BOOL initMaggieEngine(void) {
  mat4_identity(&worldMatrix);
  mat4_translate(&viewMatrix, cameraPosition.x, cameraPosition.y, cameraPosition.z);
  mat4_perspective(&perspective, 60.0f, targetRatio, 0.01f, 50.0f);
 	
  vBuffer = magAllocateVertexBuffer(masterCount);
  iBuffer = magAllocateIndexBuffer(indexCount);
  
  magUploadVertexBuffer(vBuffer, BallVertices, 0, masterCount);
  magUploadIndexBuffer(iBuffer, BallIndices, 0, indexCount);

  return TRUE;
}

// CORE FUNCTIONS

void update(void) {
  UBYTE new_char;
  UWORD char_index;
  mat4 xRot, yRot;
  
  // object rotation
 	mat4_rotateX(&xRot, xangle);
  mat4_rotateY(&yRot, yangle);
  mat4_mul(&worldMatrix, &xRot, &yRot);
  
  xangle += 0.01f;
  yangle += 0.0123f;
  
 	// camera position
 	//mat4_translate(&viewMatrix, cameraPosition.x, cameraPosition.y, cameraPosition.z);
 	
  // Should we load a new char from the message
  if (char_load == 0) {
    new_char = message[message_pos];
    
    // have we reach the end of the message?
    if (new_char == 0) {
      // wrap the message;
      message_pos = 0;
      new_char = message[message_pos];
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
    
    // Copy the char to the layer
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
  
  if (layer_posx >= (SCREEN_WIDTH+FONT_WIDTH)) {
    layer_posx = 0;
  }
  
  // Y position of the text
  scroll_posy = 440;//curve[curve_idx++];
}

void render(void) {
	// Sage to get the current pixel buffer
  SAGE_Bitmap *back_bitmap;  
  back_bitmap = SAGE_GetBackBitmap();
  
	SAGE_ClearScreen();
	
	atlasBlitGridChess();
	atlasBlitVampireLogo(192, 0);
			
	// Maggie render

	// Maggie start block
  magBeginScene();

	// draw options:
	// MAG_DRAWMODE_DEPTHBUFFER | MAG_DRAWMODE_32BIT | MAG_DRAWMODE_NORMAL | MAG_DRAWMODE_BILINEAR | MAG_DRAWM ODE_LIGHTING
	magSetDrawMode(MAG_DRAWMODE_BILINEAR | MAG_DRAWMODE_DEPTHBUFFER | MAG_DRAWMODE_LIGHTING);
	
  magSetScreenMemory(back_bitmap->bitmap_buffer, SCREEN_WIDTH, SCREEN_HEIGHT);
  
  // clear options
	// MAG_CLEAR_COLOUR | MAG_CLEAR_DEPTH
	magClear(MAG_CLEAR_DEPTH);

  magSetWorldMatrix((float *)&worldMatrix);
  magSetViewMatrix((float *)&viewMatrix);
  magSetPerspectiveMatrix((float *)&perspective);

	magSetLightPosition(0, 20.0f, -20.0f, -10.0f);
	magSetLightColour(0, 0x00ffffff);
	magSetLightAttenuation(0, 800.0f);	
	magSetLightType(0, MAG_LIGHT_POINT);

	magSetLightPosition(1, -20.0f, 20.0f, 10.0f);
	magSetLightColour(1, 0x00ffffff);
	magSetLightAttenuation(1, 400.0f);	
	magSetLightType(1, MAG_LIGHT_POINT);
	
	magSetLightColour(2, 0x0f0f0f);	
	magSetLightType(2, MAG_LIGHT_AMBIENT);
		
	magSetTexture(0, txtr);
  magSetVertexBuffer(vBuffer);
  magSetIndexBuffer(iBuffer);

 	magDrawIndexedPolygons(0, masterCount, 0, indexCount);

	// Maggie end block
  magEndScene();
  
	// Set the text layer view (using the wrapping feature of layers to simulate infite scroll)
  SAGE_SetLayerView(TEXTFIELD_LAYER, layer_posx, layer_posy, SCREEN_WIDTH, TEXTFIELD_HEIGHT);
  // Blit the text layer to the screen
  SAGE_BlitLayerToScreen(TEXTFIELD_LAYER, TEXTSCROLL_POSX, scroll_posy);
  
  // Switch screen buffers
  SAGE_RefreshScreen();
}

void restore(void) {
  // show the mouse
  SAGE_ShowMouse();
  // close the screen
  SAGE_CloseScreen();
  
  magFreeTexture(txtr);
  magFreeVertexBuffer(vBuffer);
  magFreeIndexBuffer(iBuffer);
  
  if (MaggieBase != NULL) {
    CloseLibrary(MaggieBase);
  }
}

// 

void main(int argc, char* argv[]) {
	SAGE_Event *event = NULL;
	
	filename_object = "assets_demo/amiga_ball.obj";
	filename_texture = "assets_demo/grid_uv_map.dds";
	
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
  		SAGE_ErrorLog("Maggie Library is not installed in the same level of the main demo file\n");
	 	 	return;
		}
		
		if (SAGE_OpenScreen(SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_DEPTH, SSCR_STRICTRES)) {		
      SAGE_HideMouse();
		}
		
		createMainLayer();
		createTextfieldLayer();
				
		loadOrangesLogo();
		loadAtlas();
		loadGridChess();
		loadFont();
		loadMessage();
		/*
		// lock to 30fps
	 	SAGE_MaximumFPS(30);
		SAGE_VerticalSynchro(FALSE);
			
		showOrangesLogo();
		SAGE_Pause(50*2);
		
		// NB: instead of recreating the layer, let's find out how to fill the layer with a single color
		clearMainLayer();
		
		atlasBlitRunningOn();
		SAGE_Pause(50*2);
		atlasBlitVampireLogo(224, 224);
		SAGE_RefreshScreen();
		SAGE_Pause(50*2);
		
		clearMainLayer();
		
		atlasBlitPoweredBy();
		SAGE_Pause(50*2);
		atlasBlitSage();
		SAGE_Pause(50*2);
		clearMainLayer();
		atlasBlitPoweredBy();
		atlasBlitMaggieLibrary();
		
		SAGE_Pause(50*2);
		clearMainLayer();
			*/
		while (!finish) {
			update();	
			render();
			
			// read all events raised by the screen
    		while ((event = SAGE_GetEvent()) != NULL) {
    			// If we click on mouse button, we stop the loop
      		if (event->type == SEVT_MOUSEBT) {
        		finish = TRUE;
      		}
      		// If we press the ESC key, we stop the loop
					else if (event->type == SEVT_RAWKEY && event->code == SKEY_EN_ESC) {
        		finish = TRUE;
        	}
      	}
		}
		
		restore();
	}
	
	// demo ended
	SAGE_Exit();
	SAGE_AppliLog("Thank you for watching! ^_^");
}