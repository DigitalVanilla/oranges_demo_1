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
#include <sys/time.h>
#include <dos/dos.h>
#include <clib/dos_protos.h>
#include <string.h>
#include <unistd.h>

#include <sage/sage.h>

#include <proto/Maggie.h>
#include <maggie_vec.h>
#include <maggie_vertex.h>
#include <maggie_flags.h>

// ******************************************
// GLOBAL VARIABLES
// ******************************************

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
#define ORANGES_LOGO_FILENAME			"assets/oranges_logo.png"
#define ORANGES_WIDTH							SCREEN_WIDTH
#define ORANGES_HEIGHT   					SCREEN_HEIGHT

// grid layer 
#define GRID_CHESS_FILENAME				"assets/grid_chess.png"
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

// Sprites

#define SPRITE_BANK             0                           
#define RUNNING_ON_SPRITE       0
#define POWERED_BY_SPRITE       1
#define VAMPIRE_LOGO_SPRITE     2
#define SAGE_SPRITE             3
#define MAGGIE_LIBRARY_SPRITE   4

// FONT

#define FONT_FILENAME			    "assets/font_16x20.png"
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

// TRANSITION Layer 1st > 2nd 3D section

#define	TRANSITION_LAYER		  10
//#define	SAGE_3D_LAYER		      11  

// TEXT MESSAGE

#define M_PI                  3.14159265358979323846
#define MESSAGE_FILENAME			"assets/message.txt"
#define RAD(x)                ((x)*PI/180.0)
#define CURVE_SCROLL          112 // smaller numbers create faster movement
FLOAT curve[CURVE_SCROLL];
UWORD curve_idx = 0;

// ******************************************
// MUSIC
// ******************************************

SAGE_Music *music = NULL;
#define MUSIC_SLOT 10
char *filename_music = "assets/funky_colors_fusion.mod";

// ******************************************
// ENUMS
// ******************************************

enum direction { towardLeft, towardRight, towardUp, towardDown };
enum ballXBoundary { left = 0.0f , right = 0.0f };
enum ditherTransitionType { worm, fullLine, fullColumn };
enum ditherTransitionFlow { in, out };
enum ditherTransitionSpeed { slow, normal, fast };

// ******************************************
// GLOBAL VARIABLES
// ******************************************

BOOL mainFinish       = FALSE;
BOOL finish3DSection1 = FALSE;
BOOL isFinishSection1 = FALSE;
BOOL finish3DSection2 = FALSE;
BOOL isFinishSection2 = FALSE;
BOOL finish3DSection3 = FALSE;
BOOL isFinishSection3 = FALSE;
BOOL startTransitionToSection2 = FALSE;
BOOL startTransitionToSection3 = FALSE;
BOOL calledVampireLogoOutro = FALSE;
BOOL calledGridChessOutro = FALSE;
BOOL calledBallsOutro = FALSE;

SAGE_Picture *atlas_picture, *oranges_logo_picture, *grid_chess_picture, *font_picture;

STRPTR message = NULL;
UWORD message_pos = 0, font_posx[FONT_NUM], font_posy[FONT_NUM];
UWORD char_posx = SCREEN_WIDTH, char_load = 0;
UWORD layer_posx = SCREEN_WIDTH+FONT_WIDTH, layer_posy = 0, scroll_posy = 0;

// ******************************************
// Sage 3D related section 3
// ******************************************

#define SAGE_3D_LAYER   11
#define MAIN_CAMERA     1

/*
  Prism centered:
  v 0.000000 -0.429397 -1.000000
  v 0.866025 -0.429397 0.500000
  v -0.866025 -0.429397 0.500000
  v 0.000000 0.809264 0.000000
  
  Prism offset:
  v 0.000000 -0.429397 1.500000
  v 0.866025 -0.429397 0.000000
  v -0.866025 -0.429397 0.000000
  v 0.000000 0.809264 0.500000
*/

// prism 3d object
SAGE_Vertex PrismVertices[4] = {
  { 0.000000, -0.429397, 1.500000 },
  { 0.866025, -0.429397, 0.500000 },
  { -0.866025, -0.429397, 0.500000 },
  { 0.000000, 0.809264, 0.500000 }
};

SAGE_Face PrismFaces[4] = {
  { FALSE, FALSE,
    S3DE_NOCLIP, -1,
    0, 3, 1, 0,
    0xff0000,
    0,0, 0,0, 0,0, 0,0 },
    
  { FALSE, FALSE,
    S3DE_NOCLIP, -1,
    0, 1, 2, 0,
    0xff0000,
    0,0, 0,0, 0,0, 0,0 },
    
  { FALSE, FALSE,
    S3DE_NOCLIP, -1,
    1, 3, 2, 0,
    0xff0000,
    0,0, 0,0, 0,0, 0,0 },
    
  { FALSE, FALSE,
    S3DE_NOCLIP, -1,
    2, 3, 0, 0,
    0xff0000,
    0,0, 0,0, 0,0, 0,0 }
};

SAGE_Vector PrismNormals[4];

SAGE_Entity Prism = {
  0, 0, 0,
  0.0, 0.0, 0.0, 0.0,
  FALSE, FALSE, FALSE,
  4, 4, 0,
  PrismVertices,
  PrismFaces,
  PrismNormals
};

SAGE_Vertex Path1[589] = {
  { -14.512112, 6.685640, 0.000000 },
  { -14.501267, 6.623427, 0.000000 },
  { -14.490294, 6.561475, 0.000000 },
  { -14.479189, 6.499784, 0.000000 },
  { -14.467949, 6.438357, 0.000000 },
  { -14.456571, 6.377193, 0.000000 },
  { -14.445050, 6.316295, 0.000000 },
  { -14.433386, 6.255662, 0.000000 },
  { -14.421574, 6.195296, 0.000000 },
  { -14.409611, 6.135199, 0.000000 },
  { -14.397493, 6.075370, 0.000000 },
  { -14.385219, 6.015812, 0.000000 },
  { -14.372784, 5.956525, 0.000000 },
  { -14.360185, 5.897510, 0.000000 },
  { -14.347419, 5.838768, 0.000000 },
  { -14.334483, 5.780302, 0.000000 },
  { -14.321375, 5.722110, 0.000000 },
  { -14.308090, 5.664196, 0.000000 },
  { -14.294626, 5.606558, 0.000000 },
  { -14.280979, 5.549200, 0.000000 },
  { -14.267147, 5.492121, 0.000000 },
  { -14.253125, 5.435322, 0.000000 },
  { -14.238912, 5.378806, 0.000000 },
  { -14.224503, 5.322572, 0.000000 },
  { -14.209895, 5.266623, 0.000000 },
  { -14.195086, 5.210958, 0.000000 },
  { -14.180073, 5.155580, 0.000000 },
  { -14.164851, 5.100488, 0.000000 },
  { -14.149419, 5.045685, 0.000000 },
  { -14.133772, 4.991170, 0.000000 },
  { -14.117908, 4.936946, 0.000000 },
  { -14.101823, 4.883014, 0.000000 },
  { -14.085515, 4.829373, 0.000000 },
  { -14.068980, 4.776027, 0.000000 },
  { -14.052215, 4.722974, 0.000000 },
  { -14.035216, 4.670218, 0.000000 },
  { -14.017982, 4.617757, 0.000000 },
  { -14.000507, 4.565595, 0.000000 },
  { -13.982790, 4.513731, 0.000000 },
  { -13.964828, 4.462167, 0.000000 },
  { -13.946615, 4.410903, 0.000000 },
  { -13.928151, 4.359942, 0.000000 },
  { -13.909431, 4.309284, 0.000000 },
  { -13.890453, 4.258929, 0.000000 },
  { -13.871214, 4.208880, 0.000000 },
  { -13.851709, 4.159137, 0.000000 },
  { -13.831937, 4.109701, 0.000000 },
  { -13.811893, 4.060573, 0.000000 },
  { -13.791575, 4.011754, 0.000000 },
  { -13.770980, 3.963246, 0.000000 },
  { -13.750104, 3.915049, 0.000000 },
  { -13.728944, 3.867165, 0.000000 },
  { -13.707497, 3.819594, 0.000000 },
  { -13.685760, 3.772337, 0.000000 },
  { -13.663729, 3.725396, 0.000000 },
  { -13.641401, 3.678772, 0.000000 },
  { -13.618774, 3.632466, 0.000000 },
  { -13.595845, 3.586478, 0.000000 },
  { -13.572610, 3.540811, 0.000000 },
  { -13.549066, 3.495464, 0.000000 },
  { -13.525208, 3.450439, 0.000000 },
  { -13.501036, 3.405737, 0.000000 },
  { -13.476544, 3.361359, 0.000000 },
  { -13.451732, 3.317306, 0.000000 },
  { -13.426594, 3.273580, 0.000000 },
  { -13.401128, 3.230180, 0.000000 },
  { -13.375331, 3.187109, 0.000000 },
  { -13.349199, 3.144367, 0.000000 },
  { -13.322730, 3.101956, 0.000000 },
  { -13.295920, 3.059876, 0.000000 },
  { -13.268766, 3.018129, 0.000000 },
  { -13.241265, 2.976715, 0.000000 },
  { -13.213414, 2.935636, 0.000000 },
  { -13.185209, 2.894892, 0.000000 },
  { -13.156648, 2.854486, 0.000000 },
  { -13.127727, 2.814417, 0.000000 },
  { -13.098443, 2.774687, 0.000000 },
  { -13.068793, 2.735297, 0.000000 },
  { -13.038774, 2.696248, 0.000000 },
  { -13.008382, 2.657540, 0.000000 },
  { -12.977614, 2.619176, 0.000000 },
  { -12.946468, 2.581156, 0.000000 },
  { -12.914941, 2.543481, 0.000000 },
  { -12.883028, 2.506153, 0.000000 },
  { -12.850727, 2.469171, 0.000000 },
  { -12.818034, 2.432538, 0.000000 },
  { -12.784947, 2.396255, 0.000000 },
  { -12.751463, 2.360322, 0.000000 },
  { -12.717577, 2.324740, 0.000000 },
  { -12.683287, 2.289511, 0.000000 },
  { -12.648589, 2.254636, 0.000000 },
  { -12.613482, 2.220115, 0.000000 },
  { -12.577961, 2.185950, 0.000000 },
  { -12.542024, 2.152142, 0.000000 },
  { -12.505666, 2.118692, 0.000000 },
  { -12.468885, 2.085601, 0.000000 },
  { -12.431679, 2.052870, 0.000000 },
  { -12.394043, 2.020499, 0.000000 },
  { -12.355974, 1.988491, 0.000000 },
  { -12.317470, 1.956846, 0.000000 },
  { -12.278526, 1.925565, 0.000000 },
  { -12.239141, 1.894650, 0.000000 },
  { -12.199311, 1.864100, 0.000000 },
  { -12.159033, 1.833918, 0.000000 },
  { -12.118302, 1.804105, 0.000000 },
  { -12.077117, 1.774661, 0.000000 },
  { -12.035474, 1.745587, 0.000000 },
  { -11.993370, 1.716885, 0.000000 },
  { -11.950802, 1.688556, 0.000000 },
  { -11.907766, 1.660600, 0.000000 },
  { -11.864261, 1.633020, 0.000000 },
  { -11.820281, 1.605815, 0.000000 },
  { -11.775825, 1.578986, 0.000000 },
  { -11.730888, 1.552536, 0.000000 },
  { -11.685469, 1.526465, 0.000000 },
  { -11.639563, 1.500774, 0.000000 },
  { -11.593167, 1.475464, 0.000000 },
  { -11.546280, 1.450536, 0.000000 },
  { -11.498897, 1.425991, 0.000000 },
  { -11.451014, 1.401831, 0.000000 },
  { -11.402629, 1.378056, 0.000000 },
  { -11.353739, 1.354667, 0.000000 },
  { -11.304340, 1.331666, 0.000000 },
  { -11.254430, 1.309053, 0.000000 },
  { -11.204005, 1.286829, 0.000000 },
  { -11.153063, 1.264997, 0.000000 },
  { -11.101599, 1.243556, 0.000000 },
  { -11.049610, 1.222507, 0.000000 },
  { -10.997094, 1.201853, 0.000000 },
  { -10.944048, 1.181593, 0.000000 },
  { -10.890468, 1.161729, 0.000000 },
  { -10.836350, 1.142262, 0.000000 },
  { -10.781693, 1.123193, 0.000000 },
  { -10.726493, 1.104524, 0.000000 },
  { -10.670746, 1.086254, 0.000000 },
  { -10.614450, 1.068385, 0.000000 },
  { -10.557600, 1.050919, 0.000000 },
  { -10.500195, 1.033856, 0.000000 },
  { -10.442230, 1.017197, 0.000000 },
  { -10.383704, 1.000944, 0.000000 },
  { -10.324613, 0.985097, 0.000000 },
  { -10.264953, 0.969657, 0.000000 },
  { -10.204720, 0.954626, 0.000000 },
  { -10.143913, 0.940005, 0.000000 },
  { -10.082528, 0.925795, 0.000000 },
  { -10.020562, 0.911996, 0.000000 },
  { -9.958012, 0.898610, 0.000000 },
  { -9.894874, 0.885637, 0.000000 },
  { -9.831144, 0.873080, 0.000000 },
  { -9.766822, 0.860939, 0.000000 },
  { -9.701902, 0.849214, 0.000000 },
  { -9.636382, 0.837908, 0.000000 },
  { -9.570259, 0.827021, 0.000000 },
  { -9.503530, 0.816554, 0.000000 },
  { -9.436190, 0.806508, 0.000000 },
  { -9.368237, 0.796885, 0.000000 },
  { -9.299669, 0.787684, 0.000000 },
  { -9.230481, 0.778909, 0.000000 },
  { -9.160671, 0.770558, 0.000000 },
  { -9.090236, 0.762635, 0.000000 },
  { -9.019172, 0.755138, 0.000000 },
  { -8.947475, 0.748071, 0.000000 },
  { -8.875144, 0.741433, 0.000000 },
  { -8.802175, 0.735226, 0.000000 },
  { -8.728564, 0.729450, 0.000000 },
  { -8.654309, 0.724108, 0.000000 },
  { -8.579407, 0.719199, 0.000000 },
  { -8.503853, 0.714725, 0.000000 },
  { -8.427645, 0.710688, 0.000000 },
  { -8.350780, 0.707087, 0.000000 },
  { -8.273254, 0.703925, 0.000000 },
  { -8.195065, 0.701202, 0.000000 },
  { -8.116210, 0.698919, 0.000000 },
  { -8.036685, 0.697077, 0.000000 },
  { -7.956487, 0.695678, 0.000000 },
  { -7.875612, 0.694722, 0.000000 },
  { -7.794059, 0.694210, 0.000000 },
  { -7.711823, 0.694144, 0.000000 },
  { -7.628901, 0.694525, 0.000000 },
  { -7.545291, 0.695353, 0.000000 },
  { -7.460989, 0.696630, 0.000000 },
  { -7.375991, 0.698357, 0.000000 },
  { -7.290295, 0.700534, 0.000000 },
  { -7.203898, 0.703164, 0.000000 },
  { -7.116796, 0.706246, 0.000000 },
  { -7.028986, 0.709782, 0.000000 },
  { -6.940466, 0.713774, 0.000000 },
  { -6.851231, 0.718221, 0.000000 },
  { -6.761279, 0.723126, 0.000000 },
  { -6.670607, 0.728488, 0.000000 },
  { -6.579211, 0.734310, 0.000000 },
  { -6.487088, 0.740593, 0.000000 },
  { -6.394235, 0.747336, 0.000000 },
  { -6.300649, 0.754543, 0.000000 },
  { -6.206327, 0.762212, 0.000000 },
  { -6.111266, 0.770346, 0.000000 },
  { -6.015460, 0.778940, 0.000000 },
  { -5.902927, 0.788993, 0.000002 },
  { -5.791578, 0.798376, 0.000009 },
  { -5.681400, 0.807096, 0.000021 },
  { -5.572382, 0.815161, 0.000037 },
  { -5.464512, 0.822578, 0.000058 },
  { -5.357779, 0.829353, 0.000083 },
  { -5.252171, 0.835495, 0.000112 },
  { -5.147676, 0.841009, 0.000146 },
  { -5.044285, 0.845903, 0.000184 },
  { -4.941984, 0.850184, 0.000227 },
  { -4.840763, 0.853858, 0.000274 },
  { -4.740609, 0.856934, 0.000325 },
  { -4.641512, 0.859419, 0.000380 },
  { -4.543461, 0.861318, 0.000439 },
  { -4.446443, 0.862639, 0.000502 },
  { -4.350447, 0.863390, 0.000569 },
  { -4.255462, 0.863577, 0.000640 },
  { -4.161476, 0.863208, 0.000715 },
  { -4.068478, 0.862289, 0.000794 },
  { -3.976456, 0.860827, 0.000877 },
  { -3.885399, 0.858830, 0.000963 },
  { -3.795296, 0.856305, 0.001054 },
  { -3.706134, 0.853258, 0.001148 },
  { -3.617903, 0.849697, 0.001245 },
  { -3.530591, 0.845629, 0.001346 },
  { -3.444187, 0.841061, 0.001451 },
  { -3.358679, 0.836000, 0.001559 },
  { -3.274055, 0.830453, 0.001670 },
  { -3.190305, 0.824426, 0.001785 },
  { -3.107416, 0.817928, 0.001904 },
  { -3.025378, 0.810965, 0.002025 },
  { -2.944178, 0.803544, 0.002150 },
  { -2.863806, 0.795673, 0.002278 },
  { -2.784249, 0.787357, 0.002410 },
  { -2.705497, 0.778605, 0.002544 },
  { -2.627538, 0.769424, 0.002681 },
  { -2.550360, 0.759820, 0.002822 },
  { -2.473953, 0.749800, 0.002965 },
  { -2.398304, 0.739372, 0.003112 },
  { -2.323402, 0.728543, 0.003261 },
  { -2.249236, 0.717320, 0.003413 },
  { -2.175795, 0.705709, 0.003568 },
  { -2.103065, 0.693718, 0.003726 },
  { -2.031038, 0.681354, 0.003886 },
  { -1.959700, 0.668624, 0.004049 },
  { -1.889040, 0.655535, 0.004215 },
  { -1.819047, 0.642094, 0.004383 },
  { -1.749710, 0.628308, 0.004553 },
  { -1.681017, 0.614184, 0.004727 },
  { -1.612956, 0.599730, 0.004902 },
  { -1.545516, 0.584952, 0.005080 },
  { -1.478686, 0.569857, 0.005260 },
  { -1.412454, 0.554452, 0.005443 },
  { -1.346809, 0.538745, 0.005628 },
  { -1.281739, 0.522743, 0.005815 },
  { -1.217232, 0.506452, 0.006004 },
  { -1.153278, 0.489879, 0.006195 },
  { -1.089865, 0.473033, 0.006389 },
  { -1.026981, 0.455919, 0.006584 },
  { -0.964615, 0.438544, 0.006781 },
  { -0.902755, 0.420917, 0.006980 },
  { -0.841391, 0.403044, 0.007181 },
  { -0.780510, 0.384931, 0.007384 },
  { -0.720101, 0.366587, 0.007589 },
  { -0.660153, 0.348018, 0.007795 },
  { -0.600653, 0.329231, 0.008003 },
  { -0.541592, 0.310233, 0.008213 },
  { -0.482957, 0.291031, 0.008424 },
  { -0.424736, 0.271632, 0.008637 },
  { -0.366919, 0.252044, 0.008852 },
  { -0.309493, 0.232274, 0.009067 },
  { -0.252448, 0.212328, 0.009285 },
  { -0.195772, 0.192213, 0.009503 },
  { -0.139454, 0.171937, 0.009723 },
  { -0.083481, 0.151507, 0.009944 },
  { -0.027843, 0.130929, 0.010167 },
  { 0.027472, 0.110211, 0.010390 },
  { 0.082475, 0.089360, 0.010615 },
  { 0.137179, 0.068383, 0.010841 },
  { 0.191593, 0.047286, 0.011067 },
  { 0.245731, 0.026078, 0.011295 },
  { 0.299602, 0.004765, 0.011524 },
  { 0.353219, -0.016646, 0.011753 },
  { 0.406594, -0.038148, 0.011984 },
  { 0.459737, -0.059734, 0.012215 },
  { 0.512660, -0.081396, 0.012447 },
  { 0.565374, -0.103129, 0.012680 },
  { 0.617892, -0.124923, 0.012913 },
  { 0.670224, -0.146774, 0.013147 },
  { 0.722382, -0.168672, 0.013382 },
  { 0.774377, -0.190613, 0.013617 },
  { 0.826221, -0.212587, 0.013852 },
  { 0.877926, -0.234589, 0.014088 },
  { 0.929502, -0.256612, 0.014325 },
  { 0.980962, -0.278647, 0.014562 },
  { 1.032316, -0.300689, 0.014799 },
  { 1.083576, -0.322729, 0.015036 },
  { 1.134754, -0.344762, 0.015273 },
  { 1.185861, -0.366780, 0.015511 },
  { 1.236908, -0.388775, 0.015749 },
  { 1.287907, -0.410742, 0.015987 },
  { 1.338870, -0.432672, 0.016224 },
  { 1.389808, -0.454559, 0.016462 },
  { 1.440732, -0.476396, 0.016700 },
  { 1.491654, -0.498176, 0.016937 },
  { 1.542585, -0.519891, 0.017175 },
  { 1.593537, -0.541534, 0.017412 },
  { 1.644521, -0.563099, 0.017649 },
  { 1.695549, -0.584579, 0.017886 },
  { 1.746632, -0.605965, 0.018122 },
  { 1.797782, -0.627253, 0.018358 },
  { 1.849009, -0.648433, 0.018593 },
  { 1.900326, -0.669499, 0.018828 },
  { 1.951745, -0.690445, 0.019063 },
  { 2.003275, -0.711263, 0.019296 },
  { 2.054929, -0.731946, 0.019529 },
  { 2.106719, -0.752486, 0.019762 },
  { 2.158656, -0.772878, 0.019994 },
  { 2.210751, -0.793113, 0.020225 },
  { 2.263016, -0.813186, 0.020455 },
  { 2.315461, -0.833088, 0.020684 },
  { 2.368100, -0.852812, 0.020913 },
  { 2.420943, -0.872353, 0.021140 },
  { 2.474001, -0.891701, 0.021367 },
  { 2.527286, -0.910852, 0.021592 },
  { 2.580810, -0.929797, 0.021816 },
  { 2.634583, -0.948529, 0.022039 },
  { 2.688618, -0.967041, 0.022261 },
  { 2.742926, -0.985327, 0.022482 },
  { 2.797519, -1.003379, 0.022701 },
  { 2.852407, -1.021190, 0.022920 },
  { 2.907602, -1.038753, 0.023136 },
  { 2.963116, -1.056062, 0.023352 },
  { 3.018960, -1.073108, 0.023566 },
  { 3.075146, -1.089885, 0.023778 },
  { 3.131685, -1.106386, 0.023989 },
  { 3.188589, -1.122604, 0.024198 },
  { 3.245869, -1.138532, 0.024405 },
  { 3.303536, -1.154162, 0.024611 },
  { 3.361602, -1.169488, 0.024815 },
  { 3.420079, -1.184503, 0.025018 },
  { 3.478977, -1.199199, 0.025218 },
  { 3.538309, -1.213570, 0.025417 },
  { 3.598085, -1.227608, 0.025613 },
  { 3.658318, -1.241307, 0.025808 },
  { 3.719018, -1.254658, 0.026001 },
  { 3.780198, -1.267656, 0.026191 },
  { 3.841868, -1.280294, 0.026380 },
  { 3.904041, -1.292563, 0.026566 },
  { 3.966727, -1.304457, 0.026750 },
  { 4.029938, -1.315970, 0.026932 },
  { 4.093685, -1.327093, 0.027111 },
  { 4.157980, -1.337821, 0.027289 },
  { 4.222835, -1.348145, 0.027463 },
  { 4.288261, -1.358059, 0.027636 },
  { 4.354269, -1.367556, 0.027806 },
  { 4.420871, -1.376628, 0.027973 },
  { 4.488078, -1.385269, 0.028138 },
  { 4.555902, -1.393472, 0.028300 },
  { 4.624353, -1.401229, 0.028459 },
  { 4.693445, -1.408533, 0.028616 },
  { 4.763187, -1.415378, 0.028770 },
  { 4.833592, -1.421757, 0.028921 },
  { 4.904672, -1.427661, 0.029070 },
  { 4.976436, -1.433085, 0.029215 },
  { 5.048898, -1.438021, 0.029357 },
  { 5.122068, -1.442462, 0.029497 },
  { 5.195958, -1.446401, 0.029633 },
  { 5.270579, -1.449831, 0.029767 },
  { 5.345943, -1.452745, 0.029897 },
  { 5.422061, -1.455136, 0.030024 },
  { 5.498945, -1.456997, 0.030148 },
  { 5.576606, -1.458321, 0.030268 },
  { 5.655056, -1.459100, 0.030385 },
  { 5.734305, -1.459328, 0.030499 },
  { 5.814367, -1.458997, 0.030610 },
  { 5.895251, -1.458102, 0.030717 },
  { 5.976970, -1.456633, 0.030820 },
  { 6.059535, -1.454585, 0.030920 },
  { 6.142957, -1.451951, 0.031016 },
  { 6.227248, -1.448722, 0.031109 },
  { 6.312419, -1.444893, 0.031198 },
  { 6.398482, -1.440456, 0.031283 },
  { 6.485448, -1.435405, 0.031365 },
  { 6.573328, -1.429731, 0.031442 },
  { 6.662136, -1.423428, 0.031516 },
  { 6.751880, -1.416490, 0.031586 },
  { 6.842574, -1.408908, 0.031652 },
  { 6.934228, -1.400676, 0.031714 },
  { 7.026854, -1.391787, 0.031772 },
  { 7.120464, -1.382234, 0.031825 },
  { 7.215068, -1.372009, 0.031875 },
  { 7.310679, -1.361106, 0.031920 },
  { 7.407308, -1.349518, 0.031961 },
  { 7.504966, -1.337237, 0.031998 },
  { 7.603687, -1.324255, 0.032031 },
  { 7.695716, -1.311362, 0.032064 },
  { 7.786779, -1.297552, 0.032107 },
  { 7.876879, -1.282832, 0.032159 },
  { 7.966023, -1.267213, 0.032222 },
  { 8.054214, -1.250701, 0.032293 },
  { 8.141459, -1.233306, 0.032374 },
  { 8.227763, -1.215037, 0.032465 },
  { 8.313130, -1.195901, 0.032564 },
  { 8.397567, -1.175908, 0.032673 },
  { 8.481077, -1.155065, 0.032790 },
  { 8.563666, -1.133382, 0.032917 },
  { 8.645340, -1.110868, 0.033052 },
  { 8.726103, -1.087529, 0.033196 },
  { 8.805960, -1.063376, 0.033348 },
  { 8.884916, -1.038416, 0.033509 },
  { 8.962977, -1.012659, 0.033678 },
  { 9.040149, -0.986112, 0.033856 },
  { 9.116435, -0.958784, 0.034041 },
  { 9.191841, -0.930685, 0.034235 },
  { 9.266373, -0.901822, 0.034436 },
  { 9.340034, -0.872204, 0.034646 },
  { 9.412831, -0.841839, 0.034863 },
  { 9.484769, -0.810736, 0.035088 },
  { 9.555852, -0.778905, 0.035320 },
  { 9.626085, -0.746352, 0.035560 },
  { 9.695475, -0.713087, 0.035807 },
  { 9.764026, -0.679118, 0.036061 },
  { 9.831742, -0.644454, 0.036323 },
  { 9.898630, -0.609103, 0.036591 },
  { 9.964695, -0.573075, 0.036867 },
  { 10.029941, -0.536377, 0.037149 },
  { 10.094374, -0.499017, 0.037438 },
  { 10.157998, -0.461006, 0.037733 },
  { 10.220819, -0.422350, 0.038035 },
  { 10.282843, -0.383060, 0.038344 },
  { 10.344073, -0.343142, 0.038658 },
  { 10.404516, -0.302606, 0.038979 },
  { 10.464176, -0.261461, 0.039307 },
  { 10.523059, -0.219714, 0.039640 },
  { 10.581169, -0.177375, 0.039979 },
  { 10.638513, -0.134452, 0.040323 },
  { 10.695094, -0.090954, 0.040674 },
  { 10.750918, -0.046888, 0.041030 },
  { 10.805990, -0.002264, 0.041391 },
  { 10.860315, 0.042909, 0.041758 },
  { 10.913899, 0.088624, 0.042130 },
  { 10.966746, 0.134872, 0.042508 },
  { 11.018863, 0.181644, 0.042890 },
  { 11.070252, 0.228931, 0.043277 },
  { 11.120921, 0.276726, 0.043670 },
  { 11.170874, 0.325018, 0.044067 },
  { 11.220116, 0.373801, 0.044469 },
  { 11.268652, 0.423066, 0.044875 },
  { 11.316487, 0.472803, 0.045286 },
  { 11.363627, 0.523004, 0.045701 },
  { 11.410077, 0.573661, 0.046120 },
  { 11.455841, 0.624766, 0.046544 },
  { 11.500925, 0.676309, 0.046971 },
  { 11.545335, 0.728282, 0.047403 },
  { 11.589074, 0.780676, 0.047838 },
  { 11.632149, 0.833484, 0.048277 },
  { 11.674563, 0.886696, 0.048720 },
  { 11.716324, 0.940304, 0.049166 },
  { 11.757435, 0.994299, 0.049616 },
  { 11.797902, 1.048674, 0.050069 },
  { 11.837730, 1.103418, 0.050525 },
  { 11.876925, 1.158524, 0.050985 },
  { 11.915489, 1.213983, 0.051447 },
  { 11.953430, 1.269787, 0.051913 },
  { 11.990753, 1.325927, 0.052381 },
  { 12.027462, 1.382394, 0.052852 },
  { 12.063562, 1.439180, 0.053325 },
  { 12.099060, 1.496276, 0.053801 },
  { 12.133960, 1.553675, 0.054280 },
  { 12.168266, 1.611366, 0.054761 },
  { 12.201984, 1.669343, 0.055244 },
  { 12.235120, 1.727595, 0.055729 },
  { 12.267678, 1.786116, 0.056216 },
  { 12.299664, 1.844895, 0.056705 },
  { 12.331082, 1.903924, 0.057196 },
  { 12.361938, 1.963196, 0.057688 },
  { 12.392238, 2.022701, 0.058183 },
  { 12.421985, 2.082431, 0.058678 },
  { 12.451185, 2.142378, 0.059175 },
  { 12.479843, 2.202532, 0.059674 },
  { 12.507965, 2.262885, 0.060173 },
  { 12.535555, 2.323429, 0.060674 },
  { 12.562618, 2.384155, 0.061175 },
  { 12.589161, 2.445055, 0.061678 },
  { 12.615188, 2.506120, 0.062181 },
  { 12.640703, 2.567341, 0.062685 },
  { 12.665712, 2.628710, 0.063189 },
  { 12.690221, 2.690218, 0.063694 },
  { 12.714234, 2.751857, 0.064199 },
  { 12.737757, 2.813619, 0.064705 },
  { 12.760794, 2.875494, 0.065211 },
  { 12.783351, 2.937474, 0.065716 },
  { 12.805433, 2.999551, 0.066222 },
  { 12.827045, 3.061716, 0.066728 },
  { 12.848192, 3.123960, 0.067233 },
  { 12.868879, 3.186276, 0.067738 },
  { 12.889112, 3.248653, 0.068243 },
  { 12.908895, 3.311085, 0.068747 },
  { 12.928234, 3.373562, 0.069250 },
  { 12.947134, 3.436075, 0.069753 },
  { 12.965599, 3.498617, 0.070254 },
  { 12.983636, 3.561179, 0.070755 },
  { 13.001248, 3.623751, 0.071255 },
  { 13.018442, 3.686326, 0.071753 },
  { 13.035222, 3.748895, 0.072251 },
  { 13.051594, 3.811450, 0.072747 },
  { 13.067562, 3.873981, 0.073241 },
  { 13.083132, 3.936481, 0.073734 },
  { 13.098309, 3.998941, 0.074225 },
  { 13.113097, 4.061351, 0.074714 },
  { 13.127503, 4.123705, 0.075202 },
  { 13.141532, 4.185993, 0.075687 },
  { 13.155188, 4.248206, 0.076171 },
  { 13.168476, 4.310336, 0.076652 },
  { 13.181402, 4.372375, 0.077131 },
  { 13.193971, 4.434313, 0.077608 },
  { 13.206187, 4.496143, 0.078082 },
  { 13.218057, 4.557856, 0.078553 },
  { 13.229584, 4.619443, 0.079022 },
  { 13.240774, 4.680896, 0.079488 },
  { 13.251633, 4.742206, 0.079951 },
  { 13.262165, 4.803364, 0.080411 },
  { 13.272376, 4.864363, 0.080868 },
  { 13.282270, 4.925193, 0.081321 },
  { 13.291854, 4.985846, 0.081772 },
  { 13.301131, 5.046314, 0.082219 },
  { 13.310108, 5.106587, 0.082662 },
  { 13.318789, 5.166657, 0.083102 },
  { 13.327180, 5.226517, 0.083538 },
  { 13.335285, 5.286156, 0.083970 },
  { 13.343110, 5.345568, 0.084398 },
  { 13.350659, 5.404742, 0.084823 },
  { 13.357939, 5.463671, 0.085243 },
  { 13.364953, 5.522346, 0.085659 },
  { 13.371707, 5.580759, 0.086070 },
  { 13.378206, 5.638900, 0.086477 },
  { 13.384456, 5.696762, 0.086880 },
  { 13.390461, 5.754335, 0.087278 },
  { 13.396227, 5.811612, 0.087671 },
  { 13.401758, 5.868583, 0.088059 },
  { 13.407061, 5.925241, 0.088443 },
  { 13.412139, 5.981576, 0.088821 },
  { 13.416999, 6.037580, 0.089194 },
  { 13.421644, 6.093245, 0.089562 },
  { 13.426082, 6.148561, 0.089925 },
  { 13.430315, 6.203522, 0.090282 },
  { 13.434350, 6.258117, 0.090633 },
  { 13.438191, 6.312338, 0.090979 },
  { 13.441845, 6.366177, 0.091319 },
  { 13.445315, 6.419625, 0.091653 },
  { 13.448608, 6.472673, 0.091981 },
  { 13.451729, 6.525314, 0.092303 },
  { 13.454681, 6.577538, 0.092619 },
  { 13.457471, 6.629338, 0.092929 },
  { 13.460103, 6.680704, 0.093232 },
  { 13.462584, 6.731627, 0.093529 },
  { 13.464916, 6.782101, 0.093819 },
  { 13.467107, 6.832115, 0.094103 },
  { 13.469161, 6.881661, 0.094379 },
  { 13.471084, 6.930731, 0.094649 },
  { 13.472879, 6.979316, 0.094912 },
  { 13.474553, 7.027407, 0.095167 },
  { 13.476110, 7.074996, 0.095416 },
  { 13.477557, 7.122076, 0.095657 },
  { 13.478897, 7.168635, 0.095891 },
  { 13.480136, 7.214668, 0.096117 },
  { 13.481279, 7.260164, 0.096336 },
  { 13.482332, 7.305115, 0.096546 },
  { 13.483299, 7.349513, 0.096749 },
  { 13.484185, 7.393349, 0.096945 },
  { 13.484996, 7.436615, 0.097132 },
  { 13.485737, 7.479302, 0.097311 },
  { 13.486412, 7.521401, 0.097481 },
  { 13.487027, 7.562905, 0.097644 },
  { 13.487588, 7.603804, 0.097798 },
  { 13.488099, 7.644089, 0.097943 },
  { 13.488565, 7.683753, 0.098080 },
  { 13.488992, 7.722787, 0.098208 },
  { 13.489384, 7.761182, 0.098327 },
  { 13.489747, 7.798929, 0.098438 },
  { 13.490086, 7.836021, 0.098539 },
  { 13.490405, 7.872448, 0.098631 },
  { 13.490711, 7.908202, 0.098714 },
  { 13.491008, 7.943275, 0.098787 },
  { 13.491302, 7.977657, 0.098851 },
  { 13.491596, 8.011341, 0.098905 },
  { 13.491898, 8.044318, 0.098950 },
  { 13.492210, 8.076579, 0.098985 },
  { 13.492540, 8.108116, 0.099010 },
  { 13.492892, 8.138920, 0.099025 },
  { 13.493297, 8.168991, 0.099030 }
};

// ******************************************
// GENERIC HELPERS
// ******************************************

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
  
  return rgb565;
}

// ******************************************
// FILL areas
// ******************************************

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

// ******************************************
// CLEAR areas
// ******************************************

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

// ******************************************
// COLORS HELPERS
// ******************************************

/*The lower limit for R, G, B (real version), S, I*/
#define PER_LOWER_LIMIT (0.0)
/*The upper limit for R, G, B (real version), S, I*/
#define PER_UPPER_LIMIT (1.0)
/*The lower limit for H*/
#define HUE_LOWER_LIMIT (0.0)
/*The upper limit for H*/
#define HUE_UPPER_LIMIT (360.0)
/*The lower limit for R, G, B (integer version)*/
#define RGBI_LOWER_LIMIT (0U)
/*The upper limit for R, G, B (integer version)*/
#define RGBI_UPPER_LIMIT (255U)
/*The upper limit for I in YIQ*/
#define YIQ_I_UPPER_LIMIT (0.5957)
/*The lower limit for I in YIQ*/
#define YIQ_I_LOWER_LIMIT (-0.5957)
/*The upper limit for Q in YIQ*/
#define YIQ_Q_UPPER_LIMIT (0.5226)
/*The lower limit for Q in YIQ*/
#define YIQ_Q_LOWER_LIMIT (-0.5226)
/*The upper limit for U in YUV*/
#define YUV_U_UPPER_LIMIT (0.436)
/*The lower limit for U in YUV*/
#define YUV_U_LOWER_LIMIT (-0.436)
/*The upper limit for V in YUV*/
#define YUV_V_UPPER_LIMIT (0.615)
/*The lower limit for V in YUV*/
#define YUV_V_LOWER_LIMIT (-0.615)

typedef struct
{
    double R;
    double G;
    double B;
} RgbFColor;

typedef struct
{
    int R;
    int G;
    int B;
} RgbIColor;

typedef struct
{
    double H;
    double S;
    double L;
} HslColor;

BOOL RealIsWithinBounds(double value, double lowerLimit, double upperLimit)
{
    if (value >= lowerLimit && value <= upperLimit)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

BOOL IntegerIsWithinBounds(int value, int lowerLimit, int upperLimit) {
  if (value >= lowerLimit && value <= upperLimit) {
    return TRUE;
  }
  else {
    return FALSE;
  }
}

double Double_GetMinimum(double r, double g, double b) {
  if (r < g) {
    if (r < b) {
      return r;
    }
    else {
      return b;
    }
  }
  else {
    if (g < b) {
      return g;
    }
    else {
      return b;
    }
  }
  return 0;
}

double Double_GetMaximum(double r, double g, double b) {
  if (r > g) {
    if (r > b) {
      return r;
    }
    else {
      return b;
    }
  }
  else {
    if (g > b) {
      return g;
    }
    else {
      return b;
    }
  }
  return 0;
}

BOOL Hsl_IsValid(double h, double s, double l) {
  BOOL isValid = TRUE;
  if ((RealIsWithinBounds(h, HUE_LOWER_LIMIT, HUE_UPPER_LIMIT) == FALSE)
          || (RealIsWithinBounds(s, PER_LOWER_LIMIT, PER_UPPER_LIMIT) == FALSE)
          || (RealIsWithinBounds(l, PER_LOWER_LIMIT, PER_UPPER_LIMIT) == FALSE)) {
    isValid = FALSE;
  }
  return isValid;
}

BOOL RgbF_IsValid(double r, double g, double b) {
  BOOL isValid = TRUE;
  if ((RealIsWithinBounds(r, PER_LOWER_LIMIT, PER_UPPER_LIMIT) == FALSE) ||
      (RealIsWithinBounds(g, PER_LOWER_LIMIT, PER_UPPER_LIMIT) == FALSE) ||
      (RealIsWithinBounds(b, PER_LOWER_LIMIT, PER_UPPER_LIMIT) == FALSE)) {
    isValid = FALSE;
  }
  return isValid;
}

BOOL RgbI_IsValid(int r, int g, int b) {
  BOOL isValid = TRUE;
  if ((IntegerIsWithinBounds(r, RGBI_LOWER_LIMIT, RGBI_UPPER_LIMIT) == FALSE)
          || (IntegerIsWithinBounds(g, RGBI_LOWER_LIMIT, RGBI_UPPER_LIMIT)
                  == FALSE)
          || (IntegerIsWithinBounds(b, RGBI_LOWER_LIMIT, RGBI_UPPER_LIMIT)
                  == FALSE)) {
    isValid = FALSE;
  }
  return isValid;
}

RgbFColor* RgbF_Create(double r, double g, double b) {
  RgbFColor* color = NULL;
  if (RgbF_IsValid(r, g, b) == TRUE) {  
    color = (RgbFColor*) malloc(sizeof(RgbFColor));
    if (color != NULL) {
      color->R = r;
      color->G = g;
      color->B = b;
    }
  }
  return color;
}

RgbIColor* RgbI_Create(int r, int g, int b) {
  RgbIColor* color = NULL;
  if (RgbI_IsValid(r, g, b) == TRUE) {
    color = (RgbIColor*) malloc(sizeof(RgbIColor));
    if (color != NULL) {
      color->R = r;
      color->G = g;
      color->B = b;
    }
  }
  return color;
}

RgbFColor* RgbF_CreateFromIntegerForm(int r, int g, int b) {
  RgbFColor* color = NULL;
  if (RgbI_IsValid(r, g, b) == TRUE) {
    color = (RgbFColor*) malloc(sizeof(RgbFColor));
    if (color != NULL) {
      color->R = (double) (r) / (double) (RGBI_UPPER_LIMIT);
      color->G = (double) (g) / (double) (RGBI_UPPER_LIMIT);
      color->B = (double) (b) / (double) (RGBI_UPPER_LIMIT);
    }
  }
  return color;
}

HslColor* Hsl_Create(double h, double s, double l) {
  HslColor* color = NULL;
  if (Hsl_IsValid(h, s, l) == TRUE) {
    color = (HslColor*) malloc(sizeof(HslColor));
    if (color != NULL) {
      color->H = h;
      color->S = s;
      color->L = l;
    }
  }
  return color;
}

HslColor* Hsl_CreateFromRgbF(double r, double g, double b)
{
    double M = 0.0, m = 0.0, c = 0.0;
    HslColor* color = NULL;
    if (RgbF_IsValid(r, g, b) == TRUE)
    {
        M = Double_GetMaximum(r, g, b);
        m = Double_GetMinimum(r, g, b);
        c = M - m;
        color = Hsl_Create(0.0, 0.0, 0.0);
        color->L = 0.5 * (M + m);
        if (c != 0.0)
        {
            if (M == r)
            {
                color->H = fmod(((g - b) / c), 6.0);
            }
            else if (M == g)
            {
                color->H = ((b - r) / c) + 2.0;
            }
            else
            {
                color->H = ((r - g) / c) + 4.0;
            }
            color->H *= 60.0;
            color->S = c / (1.0 - fabs(2.0 * color->L - 1.0));
        }
    }
    return color;
}


RgbFColor* RgbF_CreateFromHsl(double h, double s, double l) {
  RgbFColor* color = NULL;
  
  double c = 0.0, m = 0.0, x = 0.0;
  
  if (Hsl_IsValid(h, s, l) == TRUE) {
    c = (1.0 - fabs(2 * l - 1.0)) * s;
    m = 1.0 * (l - 0.5 * c);
    x = c * (1.0 - fabs(fmod(h / 60.0, 2) - 1.0));
    
    if (h >= 0.0 && h < (HUE_UPPER_LIMIT / 6.0))
    {
        color = RgbF_Create(c + m, x + m, m);
    }
    else if (h >= (HUE_UPPER_LIMIT / 6.0) && h < (HUE_UPPER_LIMIT / 3.0))
    {
        color = RgbF_Create(x + m, c + m, m);
    }
    else if (h < (HUE_UPPER_LIMIT / 3.0) && h < (HUE_UPPER_LIMIT / 2.0))
    {
        color = RgbF_Create(m, c + m, x + m);
    }
    else if (h >= (HUE_UPPER_LIMIT / 2.0) && h < (2.0f * HUE_UPPER_LIMIT / 3.0))
    {
        color = RgbF_Create(m, x + m, c + m);
    }
    else if (h >= (2.0 * HUE_UPPER_LIMIT / 3.0) && h < (5.0 * HUE_UPPER_LIMIT / 6.0))
    {
        color = RgbF_Create(x + m, m, c + m);
    }
    else if (h >= (5.0 * HUE_UPPER_LIMIT / 6.0) && h < HUE_UPPER_LIMIT)
    {
        color = RgbF_Create(c + m, m, x + m);
    }
    else if (h >= (HUE_UPPER_LIMIT / 3.0) && h <= ((HUE_UPPER_LIMIT / 3.0) * 2.0))
    {
        color = RgbF_Create(m, c + m, x + m);
    }
    else if (h == HUE_UPPER_LIMIT)
    {
        color = RgbF_Create(c, m, m);
    }
    else
    {
        color = RgbF_Create(m, m, m);
    }
  }
  return color;
}

RgbIColor* RgbI_CreateFromRealForm(double r, double g, double b)
{
    RgbIColor* color = NULL;
    if (RgbF_IsValid(r, g, b) == TRUE)
    {
        color = (RgbIColor*) malloc(sizeof(RgbIColor));
        if (color != NULL)
        {
            color->R = (int) (r * (double) RGBI_UPPER_LIMIT + 0.5);
            color->G = (int) (g * (double) RGBI_UPPER_LIMIT + 0.5);
            color->B = (int) (b * (double) RGBI_UPPER_LIMIT + 0.5);
        }
    }
    return color;
}

// ******************************************
// Layers
// ******************************************

BOOL createAllLayers(void) {
  SAGE_Picture *oranges_logo;

  if (!SAGE_CreateLayer(MAIN_LAYER, MAIN_LAYER_WIDTH, MAIN_LAYER_HEIGHT)) {
    SAGE_SetLayerTransparency(MAIN_LAYER, GLOBAL_BLACK_TRANSPARENCY);
    return FALSE;
  }
  
  if (!SAGE_CreateLayer(ATLAS_LAYER, ATLAS_WIDTH, ATLAS_HEIGHT)) {  
    return FALSE;
  }
  
  oranges_logo = SAGE_LoadPicture(ORANGES_LOGO_FILENAME);
  if (!SAGE_CreateLayerFromPicture(ORANGES_LAYER, oranges_logo)) {
    return FALSE;
  }
  SAGE_ReleasePicture(oranges_logo);
  
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
  
  if (!SAGE_CreateLayer(TEXTFIELD_LAYER, TEXTFIELD_WIDTH, TEXTFIELD_HEIGHT)) {
    return FALSE;
  }
  
  if (!SAGE_CreateLayer(SAGE_3D_LAYER, SCREEN_WIDTH, SCREEN_HEIGHT)) {
    return FALSE;
  }
  
  SAGE_DisplayError();
  return TRUE;
}

void releaseAllLayers(void) {
  int i;
  
  for (i=0; i<11; i++) {
    SAGE_ReleaseLayer(i);
  }
}

void clearMainLayer(void) {
	if (MAIN_LAYER != NULL) {
		clearBackScreen(rgb888_to_rgb565(0,0,0));
	}
}

// ******************************************
// Sprites
// ******************************************

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

// ******************************************
// ORANGES LOGO
// ******************************************

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

// ATLAS

BOOL loadAtlas(void) {
	atlas_picture = SAGE_LoadPicture(ATLAS_FILENAME);
	
	SAGE_SetPictureTransparency(atlas_picture, GLOBAL_TRANSPARENCY);
  
	if (atlas_picture == NULL) {
  	SAGE_DisplayError();
  	return FALSE;
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

// ******************************************
// Blit
// ******************************************

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

// ******************************************
// Ease animation functions
// ******************************************

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

// ******************************************
// Animating the initial intro part (logos and sprites)
// ******************************************

void showOrangesLogo(void) {
	SAGE_BlitLayerToScreen(ORANGES_LAYER, 0, 0);
	SAGE_RefreshScreen();
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

// ******************************************
// Dithering effect transition
// ******************************************

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

// ******************************************
// 3D maggie LIbrary related to section 1
// ******************************************

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

// ******************************************
// OBJ LOADER
// ******************************************

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

// ******************************************
// CORE FUNCTIONS
// ******************************************

void visualDebug(void) {
	// Draw the fps counter
  SAGE_PrintFText(10, 10, "%d fps", SAGE_GetFps());
}

void updateKeyboardKeysListener(void) {
  SAGE_Event * event = NULL;

  // read all events raised by the screen
	while ((event = SAGE_GetEvent()) != NULL) {
		// If we click on mouse button, we stop the loop
  	if (event->type == SEVT_MOUSEBT) {
   		mainFinish = TRUE;
   		finish3DSection1 = TRUE;
   		finish3DSection2 = TRUE;
   		finish3DSection3 = TRUE;
  	}
  	// If we press the ESC key, we stop the loop
		else if (event->type == SEVT_RAWKEY && event->code == SKEY_EN_ESC) {
   		mainFinish = TRUE;
   		finish3DSection1 = TRUE;
   		finish3DSection2 = TRUE;
   		finish3DSection3 = TRUE;
   	}
  }
}

// ******************************************
// 1st section
// ******************************************

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
      // this starts the transition to middle section between section 1 and 2
      startTransitionToSection2 = TRUE;
      calledVampireLogoOutro = TRUE;
      
      // wrap the message and make the loop
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
  //updateKeyboardKeysListener();
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

// ******************************************
// Transition 1st>2nd sections
// ******************************************

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

// ******************************************
// 2nd section
// ******************************************

HslColor *hslUpdateStart, *hslUpdate;
int positionIndex = 0, totalEntities = 0, indexEntity = 0;

void addEntity() {
  int p, count, i;
  RgbFColor* rgbF = NULL;
  RgbIColor* rgbI = NULL;
  SAGE_Entity *onFlyEntity;
     
  count = SAGE_GetVblCount();
  
  if (count >= 20) {
    for (i=0; i<6; i++) {
      onFlyEntity = SAGE_CloneEntity(&Prism);
    
      hslUpdate->L = 0.4;        
      
      for (p=0; p<4; p++) {
        rgbF = RgbF_CreateFromHsl(hslUpdate->H, hslUpdate->S, hslUpdate->L);
        rgbI = RgbI_CreateFromRealForm(rgbF->R, rgbF->G, rgbF->B);
        
        onFlyEntity->faces[p].color = rgb888_to_rgb565(rgbI->R, rgbI->G, rgbI->B);
        
        hslUpdate->L += 0.085;
      }
      
      SAGE_AddEntity(indexEntity, onFlyEntity);
      //SAGE_SetEntityPosition(indexEntity, Path1[0].x - (15.0 + (i * 1.5)), Path1[0].y - (2.0 * i), Path1[0].z);
      SAGE_SetEntityPosition(indexEntity, Path1[0].x - 15.0, Path1[0].y - (2.0 * i), Path1[0].z);
      indexEntity ++;
      
      if (indexEntity >= S3DE_MAX_ENTITIES) {
        indexEntity = 0;
      }
      
      totalEntities ++;
      
      SAGE_ResetVblCount();
    }
    
    hslUpdate->H += 8;
      
    if (hslUpdate->H > HUE_UPPER_LIMIT) {
      hslUpdate->H = 0.0;
    }
  }
}

void updatePrismLocation() {
  int i, index;
  
  for (i=0; i<1024; i++) {
    //index = checkEntityCurrentPosition(i);
    
    //SAGE_SetEntityPosition(i, Path1[index+1].x, Path1[index+1].y, Path1[index+1].z);
    SAGE_MoveEntity(i, +0.1, 0, 0);
      
    SAGE_RotateEntity(i,
                      S3DE_ONEDEGREE * 2,
                      S3DE_ONEDEGREE * 2,
                      S3DE_ONEDEGREE * 2);
  }
}

void removeEntity() {
  int i, total;
  SAGE_Entity *onFlyEntity;

  total = totalEntities;
  for (i=0; i<1024; i++) {
    onFlyEntity = SAGE_GetEntity(i);
    
    if (onFlyEntity->posx >= 15.0) {
       SAGE_RemoveEntity(i);
       //totalEntities --;
    }
    
    /*if (onFlyEntity->posx == Path1[589].x &&
        onFlyEntity->posy == Path1[589].y &&
        onFlyEntity->posz == Path1[589].z) {
       SAGE_RemoveEntity(i);
       //totalEntities --;
    }*/
  }
}

void updateEntities() {
  addEntity();
  updatePrismLocation();
  removeEntity();  
}

int checkEntityCurrentPosition(int index) {
  int i;
  SAGE_Entity *onFlyEntity;

  onFlyEntity = SAGE_GetEntity(index);
    
  /*for (i=0; i<=589; i++) {  
    if (onFlyEntity->posx == Path1[i].x &&
        onFlyEntity->posy == Path1[i].y &&
        onFlyEntity->posz == Path1[i].z) {
       return i;
    }
  }*/

  return 0;
}

#define ZOOM_STEP 0.5
WORD rotate = S3DE_ONEDEGREE * 180;
FLOAT zoom = -3.0, camX = 5.0, camY = -4.0, camRotX = -130.0, camRotY = -121;

void initSage3DWorld() {
  RgbFColor* rgbF = NULL;
  RgbIColor* rgbI = NULL;
  
  SAGE_EnableFrameCount(TRUE);
  
  SAGE_Init3DEngine();
  SAGE_Set3DRenderMode(S3DR_RENDER_TEXT);
  
  if (!SAGE_AddCamera(MAIN_CAMERA, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT)) {
    return FALSE;
  }
  
  SAGE_SetActiveCamera(MAIN_CAMERA);
  SAGE_SetCameraAngle(MAIN_CAMERA, camRotX, camRotY, 0);
  SAGE_SetCameraPlane(MAIN_CAMERA, (FLOAT)-50.0, (FLOAT)100.0);
  SAGE_SetCameraPosition(MAIN_CAMERA, camX, camY, zoom);
  
  SAGE_InitEntity(&Prism);
  
  // start with full red
  rgbI = RgbI_Create(255, 0, 0);
  rgbF = RgbF_CreateFromIntegerForm(rgbI->R, rgbI->G, rgbI->B);
    
  // convert the initial RGB color into HSL
  hslUpdate = Hsl_CreateFromRgbF(rgbF->R, rgbF->G, rgbF->B);
  hslUpdate->L = 0.5;
}

void _update(void) {
  int i=0, x_ = 0, y_ = 0;
  SAGE_Event * event = NULL;
  
  while ((event = SAGE_GetEvent()) != NULL) {
    // If we click on mouse button, we stop the loop
  	if (event->type == SEVT_MOUSEBT) {
   		mainFinish = TRUE;
   		finish3DSection1 = TRUE;
   		finish3DSection2 = TRUE;
   		finish3DSection3 = TRUE;
  	}
  	// If we press the ESC key, we stop the loop
		else if (event->type == SEVT_RAWKEY && event->code == SKEY_EN_ESC) {
   		mainFinish = TRUE;
   		finish3DSection1 = TRUE;
   		finish3DSection2 = TRUE;
   		finish3DSection3 = TRUE;
   	}
   	
    if (event->type == SEVT_RAWKEY) {
      if (event->code == SKEY_EN_W) {
        zoom += ZOOM_STEP;
      }
      else if (event->code == SKEY_EN_S) {
        zoom -= ZOOM_STEP;
      }
  
      if (event->code == SKEY_EN_UP) {
        camY += S3DE_ONEDEGREE / 4;
      }
      else if (event->code == SKEY_EN_DOWN) {
        camY -= S3DE_ONEDEGREE / 4;
      }
      
      if (event->code == SKEY_EN_LEFT) {
        camX -= S3DE_ONEDEGREE / 4;
      }
      else if (event->code == SKEY_EN_RIGHT) {
        camX += S3DE_ONEDEGREE / 4;
      }
      
      if (event->code == SKEY_EN_A) {
        y_ -= S3DE_ONEDEGREE / 4;
        camRotY += y_;
      }
      else if (event->code == SKEY_EN_D) {
        y_ += S3DE_ONEDEGREE / 4;
        camRotY += y_;
      }
      
      if (event->code == SKEY_EN_Z) {
        x_ -= S3DE_ONEDEGREE / 2;
        camRotX += x_;
      }
      else if (event->code == SKEY_EN_X) {
        x_ += S3DE_ONEDEGREE / 2;
        camRotX += x_;
      }
    }
  }
  
  SAGE_SetCameraPosition(MAIN_CAMERA, camX, camY, zoom);
  SAGE_RotateCamera(MAIN_CAMERA, x_, y_, 0);
}

void renderSection2(void) {
  clearLayerBitmap(SAGE_3D_LAYER, rgb888_to_rgb565(255, 255, 255));
  
  addEntity();
  updatePrismLocation();
  removeEntity(); 
  
  //_update();
  
  SAGE_BlitLayerToScreen(SAGE_3D_LAYER, 0, 0);
  SAGE_RenderWorld();
  
  //debugSage3D();
  
  SAGE_RefreshScreen();
}

// ******************************************
// 3rd Section
// ******************************************
void renderSection3(void) {
  
}

void debugSage3D() {
  // Draw the fps counter
  SAGE_PrintFText(10, 15, "%d fps", SAGE_GetFps());
 
  SAGE_SetCameraPosition(MAIN_CAMERA, camX, camY, zoom);
  
  SAGE_PrintFText(10, 236,
    "camX=%.2f camY=%.2f zoom=%.2f",
    camX, camY, zoom
  );
  
  SAGE_PrintFText(10, 246,
    "x_=%.2f y_=%.2f",
    camRotX, camRotY
  );
}

// ******************************************
// RESTORE > EXIT
// ******************************************

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
	
	SAGE_FlushEntities();
  SAGE_FlushMaterials();
  SAGE_FlushCameras();
  SAGE_Release3DEngine();
}

// ******************************************
// MAIN
// ******************************************

void main(int argc, char* argv[]) {
  // generate random X position for each ball and also random intial Y position
  int i, x = ballYPositionsTotal;
  float rand, startXPosition = 40.0;
  float* randomFloatArray;

  srand((unsigned int)time(NULL));
  
  randomFloatArray = randomFloatNumbers(totalBalls, leftXBoundary, rightXBoundary);

  // INITIALIZE THE DEMO
  // the demo will only run on an Apollo Vampire 080
	if (SAGE_ApolloCore() == FALSE) {
		SAGE_AppliLog("Apollo Vampire not found! o_O");
		return FALSE;
	}
	else {
		SAGE_AppliLog("Apollo Vampire found! ^_^");
	}
	
	// everything is fine, let's proceed with all the default initializations
	
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
	    
	// init the SAGE system
  if (SAGE_Init(SMOD_VIDEO|SMOD_AUDIO|SMOD_3D|SMOD_INTERRUPTION)) {
  	
  	// maggie library must be present
  	MaggieBase = OpenLibrary((UBYTE *)"maggie.library", 0);
		if (!MaggieBase) {
    	SAGE_ErrorLog("Can't open maggie.library");
    	return FALSE;
  	}
  		
		// load the 3d model
  	if (!LoadObjModel(filename_object)) {
  		SAGE_ErrorLog("Cannot load 3D object model with name %s\n", filename_object);
	  	return FALSE;
		}

	 	// load the texture
  	if (!LoadTexture(filename_texture)) {
  		SAGE_ErrorLog("Cannot load texture with name %s\n", filename_texture);
   		return FALSE;
  	}
  
		if (!initMaggieEngine()) {
  		SAGE_ErrorLog("Cannot initiate Maggie engine\n");
	 	 	return FALSE;
		}
		
		music = SAGE_LoadMusic(filename_music);
  	if (music) {
      if (!SAGE_AddMusic(MUSIC_SLOT, music)) {
        SAGE_ErrorLog("Cannot load music\n");
        SAGE_DisplayError();
        return FALSE;
      }
    }
    else {
      SAGE_ErrorLog("Cannot initiate music\n");
  		SAGE_DisplayError();
  		return FALSE;
  	}
				
		if (SAGE_OpenScreen(SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_DEPTH, SSCR_STRICTRES)) {	
  		SAGE_HideMouse();
      SAGE_SetTextColor(0,255);
  	 	SAGE_SetDrawingMode(SSCR_TXTTRANSP);
  	 	
  		// fps counter
      if (SAGE_EnableFrameCount(TRUE)) {
        SAGE_MaximumFPS(60);
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
      
  		// load all the assets
  		loadGridChess();
  		loadFont();
  		loadMessage();
  		initSage3DWorld();
  		
  		SAGE_Pause(100);
      
  		// start playing the music
  		SAGE_PlayMusic(MUSIC_SLOT);

			// intro pre-sections
			showOrangesLogo();
			/*ditheringTransition(0,
                          0,
                          32,
                          32,
                          SCREEN_WIDTH/32,
                          SCREEN_WIDTH/32,
                          fast,
                          ORANGES_LAYER,
                          0,
                          0,
                          worm,
                          in,
                          -999);
                          */
			SAGE_Pause(200);
			
			ditheringTransition(0,
                          0,
                          32,
                          32,
                          SCREEN_WIDTH/32,
                          SCREEN_WIDTH/32,
                          fast,
                          ORANGES_LAYER,
                          0,
                          0,
                          fullLine,
                          out,
                          -999);
                          
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
      
      while (!mainFinish) {
        updateKeyboardKeysListener();
        
        // first 3D section (bouncing balls)
        while (!finish3DSection1) {
          updateKeyboardKeysListener();
          updateSection1();
  				renderSection1();
  			}
       
        transitionTo3DSection2(32,
                               32,
                               SCREEN_HEIGHT/32,
                               SCREEN_WIDTH/32,
                               rgb888_to_rgb565(255, 255, 255));
        
        // second 3D section (3D sage with prisms objects effects)
        while (!finish3DSection2) {
          updateKeyboardKeysListener();
          renderSection2();
  			}
			}
			
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
