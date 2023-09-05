/*
 *   This file is part of nes_emu.
 *   Copyright (c) 2019 Franz Flasch.
 *
 *   nes_emu is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   nes_emu is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with nes_emu.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <vprintf.h>
#include <sys/kernel_tic.h>
#include <sys/klog.h>
#include <x++/X.h>

#include <ferox.h>
#include "pic.h"

using namespace Ewok;
   /* NES part */

#define KEY_TIMEOUT 2	

#define MAX_WALL_COUNT  4
#define BALL_RADIUS     16

xscreen_t scr;

class ScreenSaver : public XWin {
private:
	frWorld *world = 0;
	frBody *walls[MAX_WALL_COUNT];
	uint32_t lastSec;
    uint64_t lastUsec;
	graph_t  *particle;
	int width = 0;
	int height = 0;
	
	const frMaterial MATERIAL_BRICK = {
	    1.0f,		//density        
	    0.9f,		//restitution    
	    0.05f,		//staticFriction 
	    0.035f		//dynamicFriction
	};
	
	const frMaterial MATERIAL_WALL = {
	    1.25f,
	    0.9f,
	    0.075f,
	    0.05f
	};

public:
	inline ScreenSaver() {
        kernel_tic(&lastSec, &lastUsec);
		particle = graph_new((uint32_t*)object, 32, 32);
 	}
	
	inline ~ScreenSaver() {
		if(world)
			frReleaseWorld(world);
	}

protected:
	void Init(int w, int h) {
		 const Rectangle bounds = {
   		     .x = -0.05f * frNumberPixelsToMeters(w),
   		     .y = -0.05f * frNumberPixelsToMeters(h),
   		     .width = 1.1f * frNumberPixelsToMeters(w),
   		     .height = 1.1f * frNumberPixelsToMeters(h)
   		 };

   		 world = frCreateWorld(frVec2ScalarMultiply(FR_WORLD_DEFAULT_GRAVITY, 0.000005f), bounds);
		//left
   		 walls[0] = frCreateBodyFromShape(
   		     FR_BODY_STATIC,
   		     FR_FLAG_NONE,
   		     frVec2PixelsToMeters((Vector2) { -0.05f * w, 0.5f * h }),
   		     frCreateRectangle(
   		         MATERIAL_WALL,
   		         frNumberPixelsToMeters(0.1f * w),
   		         frNumberPixelsToMeters(1.1f * h)
   		     )
   		 );
		//down
   		 walls[1] = frCreateBodyFromShape(
   		     FR_BODY_STATIC,
   		     FR_FLAG_NONE,
   		     frVec2PixelsToMeters((Vector2) { 0.5f * w,  h + 25 }),
   		     frCreateRectangle(
   		         MATERIAL_WALL,
   		         frNumberPixelsToMeters(1.1f * w),
   		         frNumberPixelsToMeters(50)
   		     )
   		 );
		//right
   		 walls[2] = frCreateBodyFromShape(
   		     FR_BODY_STATIC,
   		     FR_FLAG_NONE,
   		     frVec2PixelsToMeters((Vector2) { 1.05f * w, 0.5f * (h  - 3*BALL_RADIUS)}),
   		     frCreateRectangle(
   		         MATERIAL_WALL,
   		         frNumberPixelsToMeters(0.1f * w),
   		         frNumberPixelsToMeters(h - 3*BALL_RADIUS)
   		     )
   		 );

		//down
   		 walls[3] = frCreateBodyFromShape(
   		     FR_BODY_STATIC,
   		     FR_FLAG_NONE,
   		     frVec2PixelsToMeters((Vector2) { 0.5f * w, h + 50 }),
   		     frCreateRectangle(
   		         MATERIAL_WALL,
   		         frNumberPixelsToMeters(1.1f * w),
   		         frNumberPixelsToMeters(100)
   		     )
   		 );

   		 for (int i = 0; i < MAX_WALL_COUNT; i++)
   		     frAddToWorld(world, walls[i]);
		
		for(int i = 0; i < 6; i++){
              frBody *body = frCreateBodyFromShape(FR_BODY_DYNAMIC,FR_FLAG_NONE,
                 frVec2PixelsToMeters((Vector2) { random()%w, 0 }),
                 frCreateCircle(MATERIAL_BRICK, frNumberPixelsToMeters(BALL_RADIUS))
             );
             frSetBodyVelocity(body, (Vector2){(random()%10 -5)/1000.0f, 0});
			 frSetBodyGravityScale(body, 0.2);
             frAddToWorld(world, body);
		}
	}

    void waitForNextFrame(){
        uint32_t sec;
        uint64_t usec;
        int wait;

        kernel_tic(&sec, &usec);
        wait = 1000000/60 - ((sec - lastSec) * 1000000 + (usec - lastUsec));
        if(wait > 0)
            usleep(wait);


        kernel_tic(&lastSec, &lastUsec);
    }

	void onRepaint(graph_t* g) {
		if(width != g->w || height != g->h){
			if(world)
				frReleaseWorld(world);
			
			Init(g->w, g->h);
			width = g->w;
			height = g->h;
		}

	    frSimulateWorld(world, (1.0f / 60.0f) * 100.0f);
		graph_clear(g, 0);
        const int bodyCount = frGetWorldBodyCount(world);
        for (int i = MAX_WALL_COUNT; i < bodyCount; i++) {
            frBody *body = frGetWorldBody(world, i);
            const Vector2 bodyPos = frGetBodyPosition(body);
            int x = (int)frNumberMetersToPixels(bodyPos.x);
            int y = (int)frNumberMetersToPixels(bodyPos.y);
            if(y > g->h * 1.2f){
                frSetBodyPosition(body, frVec2PixelsToMeters((Vector2){ random()%g->w, 0}));
				frSetBodyVelocity(body, (Vector2){(random()%10 -5)/1000.0f, 0});
            }
			graph_blt_alpha(particle, 0, 0, 2*BALL_RADIUS, 2*BALL_RADIUS, 
							 g, x, y, 2*BALL_RADIUS, 2*BALL_RADIUS, 0xff);	
        }	
		waitForNextFrame();
	}
};

static void loop(void* p) {
	XWin* xwin = (XWin*)p;
	xwin->repaint();
}

int main(int argc, char *argv[])
{
	const char* path;

	ScreenSaver saver;
    /*init window*/

	X x;
	x.screenInfo(scr, 0);
	x.open(&saver, 30, 30, scr.size.w/2, scr.size.h/2, "ScreenSaver", X_STYLE_NORMAL);
	saver.setVisible(true);

	x.run(loop, &saver);

    return 0;
}
