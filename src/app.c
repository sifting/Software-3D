#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <SDL2/SDL.h>
#include "r3d.h"


/*video*/
static SDL_Window *window;
void 
vid_init (const char *title, int width, int height)
{
    SDL_InitSubSystem (SDL_INIT_VIDEO);
    window = SDL_CreateWindow
	(
		title,
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		width,
		height,
		SDL_WINDOW_RESIZABLE
    );
    if (NULL == window) 
	{
		return;
	}
}
void
vid_shutdown (void)
{
	SDL_DestroyWindow (window);
}
/*draw*/
static SDL_Renderer *renderer;
static SDL_Texture *texture;
void
draw_sys_init (void)
{
	renderer = SDL_CreateRenderer 
	(
		window, 
		-1, 
		SDL_RENDERER_ACCELERATED
	);
	if (NULL == renderer)
	{
		return;
	}
	SDL_RenderSetLogicalSize (renderer, 320, 240);
	texture = SDL_CreateTexture
	(
		renderer,
		SDL_PIXELFORMAT_ARGB8888,
		SDL_TEXTUREACCESS_STREAMING,
		320,
		240
	);
	if (NULL == texture)
	{
		return;
	}
}
void
draw_sys_shutdown (void)
{
	SDL_DestroyTexture (texture);
	SDL_DestroyRenderer (renderer);
}
unsigned char *
draw_lock (void)
{
	unsigned char *px;
	int pt;
	SDL_LockTexture (texture, NULL, (void **)&px, &pt);
	return px;
}
void
draw_unlock (void)
{
	SDL_UnlockTexture (texture);
}
void
draw_present (void)
{
	SDL_RenderClear (renderer);
	SDL_RenderCopy (renderer, texture, NULL, NULL);
	SDL_RenderPresent (renderer);
}
void
draw_init (void)
{
	vid_init ("r3d", 320, 240);
	draw_sys_init ();
}
void
draw_shutdown (void)
{
	draw_sys_shutdown ();
	vid_shutdown ();
}

int 
main (int argc, char **argv) 
{
	unsigned short *zb = malloc (320*240*2);
	if (!zb)
	{
		printf ("Failed to allocate Z buffer\n");
		return EXIT_FAILURE;
	}
	SDL_Init (SDL_INIT_TIMER);
	draw_init ();
	if (r3d_init ())
	{
		printf ("Failed to initialise r3d\n");
		return EXIT_FAILURE;
	}
	r3d_perspective (50, 4.0/3.0, 1, 256);
	//r3d_viewport (32, 24, 256, 192);
	r3d_viewport (0, 0, 320, 240);
	//r3d_viewport (80, 60, 320>>1, 240>>1);
	r3d_depth_buffer (zb);
	while (1) 
	{
		SDL_Event ev;
		while (SDL_PollEvent (&ev)) 
		{
			switch (ev.type)
			{
			case SDL_KEYDOWN:
			{
				int a = 0;
    				a = 1;
				break;
			}
			case SDL_QUIT: goto Shutdown;
			default: break;
			}
		}
		
		static int time = 0;
		r3d_colour_buffer (draw_lock ());
		memset (_c.c.cb, 0, 320*240*4);
		/*Create modelview matrix*/
		float th = SDL_GetTicks ()*0.001;
		float C = cos (th);
		float S = sin (th);
		
		Matrix x, z, y, acc, mv;
		matrix_identity (z);
		z[0][0] = C; z[0][1] =-S;
		z[1][0] = S; z[1][1] = C;
		
		matrix_identity (x);
		x[1][1] = C; x[1][2] =-S;
		x[2][1] = S; x[2][2] = C;
		
		C *= 0.8;
		S *= 0.8;
		
		matrix_identity (y);
		y[0][0] = C; y[0][2] =-S;
		y[2][0] = S; y[2][2] = C;
		matrix_multiply (acc, z, x);
		
		/*Draw Cube*/
		float vtx[] =
		{
			-30,-30, 30,
			 30,-30, 30,
			 30, 30, 30,
			-30, 30, 30,
			
			-30, 30,-30,
			 30, 30,-30,
			 30,-30,-30,
			-30,-30,-30,

			-30,-30,-30,
			 30,-30,-30,
			 30,-30, 30,
			-30,-30, 30,

			-30, 30, 30,
			 30, 30, 30,
			 30, 30,-30,
			-30, 30,-30,

			 30,-30,-30,
			 30, 30,-30,
			 30, 30, 30,
			 30,-30, 30,

			-30,-30, 30,
			-30, 30, 30,
			-30, 30,-30,
			-30,-30,-30,		
		};
		unsigned short shades[] = 
		{
			0x0000,
			0x0000,
			0x0000,
			0x0000,
			
			0x3000,
			0x3000,
			0x3000,
			0x3000,
			
			0x2000,
			0x2000,
			0x2000,
			0x2000,
			
			0x1000,
			0x1000,
			0x1000,
			0x1000,
			
			0x0500,
			0x0500,
			0x0500,
			0x0500,
			
			0x1500,
			0x1500,
			0x1500,
			0x1500,
		};
		r3d_clear ();
#if 1		
		matrix_identity (r3d_pmv);
		matrix_translate (r3d_pmv, 76, 50, -200);
		r3d_colour (30);
		r3d_vertex_pointer (vtx);
		r3d_shade_pointer (shades);
		r3d_draw (DM_TRIANGLE_FAN, 4);
		
		matrix_identity (r3d_pmv);
		matrix_translate (r3d_pmv, -76, -50, -200);
		r3d_colour (30);
		r3d_vertex_pointer (vtx);
		r3d_shade_pointer (shades);
		r3d_draw (DM_TRIANGLE_FAN, 4);
#endif	
		matrix_multiply (r3d_pmv, acc, y);
		//matrix_identity (r3d_pmv);
		//memcpy (r3d_pmv, z, sizeof (r3d_pmv));
		//matrix_translate (r3d_pmv, 0, 0, -200);
		matrix_translate (r3d_pmv, C*160, S*110, -200);
		r3d_colour (140);
		r3d_vertex_pointer (vtx);
		r3d_shade_pointer (shades);
		r3d_draw (DM_TRIANGLE_FAN, 4);
	
#if 1
		r3d_colour (192);
		r3d_vertex_pointer (vtx + 12);
		r3d_shade_pointer (shades + 4);
		r3d_draw (DM_TRIANGLE_FAN, 4);

		r3d_colour (64);
		r3d_vertex_pointer (vtx + 24);
		r3d_shade_pointer (shades + 8);
		r3d_draw (DM_TRIANGLE_FAN, 4);

		r3d_colour (112);
		r3d_vertex_pointer (vtx + 36);
		r3d_shade_pointer (shades + 12);
		r3d_draw (DM_TRIANGLE_FAN, 4);
		
		r3d_colour (220);
		r3d_vertex_pointer (vtx + 48);
		r3d_shade_pointer (shades + 16);
		r3d_draw (DM_TRIANGLE_FAN, 4);
		
		r3d_colour (250);
		r3d_vertex_pointer (vtx + 60);
		r3d_shade_pointer (shades + 20);
		r3d_draw (DM_TRIANGLE_FAN, 4);
#endif

#if 1		
		matrix_identity (r3d_pmv);
		matrix_multiply (r3d_pmv, acc, y);
		matrix_translate (r3d_pmv, 0, 0, -200);
		r3d_colour (140);
		r3d_vertex_pointer (vtx);
		r3d_shade_pointer (shades);
		r3d_draw (DM_TRIANGLE_FAN, 4);
		
		r3d_colour (192);
		r3d_vertex_pointer (vtx + 12);
		r3d_shade_pointer (shades + 4);
		r3d_draw (DM_TRIANGLE_FAN, 4);

		r3d_colour (64);
		r3d_vertex_pointer (vtx + 24);
		r3d_shade_pointer (shades + 8);
		r3d_draw (DM_TRIANGLE_FAN, 4);

		r3d_colour (112);
		r3d_vertex_pointer (vtx + 36);
		r3d_shade_pointer (shades + 12);
		r3d_draw (DM_TRIANGLE_FAN, 4);
		
		r3d_colour (220);
		r3d_vertex_pointer (vtx + 48);
		r3d_shade_pointer (shades + 16);
		r3d_draw (DM_TRIANGLE_FAN, 4);
		
		r3d_colour (250);
		r3d_vertex_pointer (vtx + 60);
		r3d_shade_pointer (shades + 20);
		r3d_draw (DM_TRIANGLE_FAN, 4);
#endif
		draw_unlock ();
		draw_present ();
		{
			int a = 0;
			a = 1;
		}
	}
Shutdown:
	r3d_shutdown ();
	draw_shutdown ();
	SDL_Quit ();
	free (zb);
	return EXIT_SUCCESS;
}
