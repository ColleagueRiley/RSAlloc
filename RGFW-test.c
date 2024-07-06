#define RGFW_IMPLEMENTATION

#define RSA_DEFAULT_ARENA 400000 /* ~ 400 KB */

#define RSALLOC_IMPLEMENTATION
#include "RSAlloc.h"

#include <stdlib.h>

#define RGFW_MALLOC RSA_alloc
#define RGFW_FREE RSA_free
#define RGFW_CALLOC RSA_calloc
#include "RGFW.h"

void drawLoop(RGFW_window* w); /* I seperate the draw loop only because it's run twice */

#ifdef RGFW_WINDOWS
DWORD loop2(void* args);
#else
void* loop2(void* args);
#endif


unsigned char icon[4 * 3 * 3] = {0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF};
unsigned char running = 1, running2 = 1;

/* callbacks are another way you can handle events in RGFW */
void refreshCallback(RGFW_window* win) {
    drawLoop(win);
}


int main(void) {    
    RSA_init(400000);
    RGFW_window* win = RGFW_createWindow("RGFW Example Window", RGFW_RECT(500, 500, 500, 500), RGFW_ALLOW_DND | RGFW_CENTER);
    RGFW_window_makeCurrent(win);
    
    RGFW_window_setIcon(win, icon, RGFW_AREA(3, 3), 4);

    RGFW_setWindowRefreshCallback(refreshCallback);

    RGFW_window_swapInterval(win, 1);

    unsigned char i;

    #ifndef RGFW_VULKAN
    glEnable(GL_BLEND);
    #endif

    glEnable(GL_BLEND);             
    glClearColor(0, 0, 0, 0);

    RGFW_window_setMouseStandard(win, RGFW_MOUSE_RESIZE_NESW);

    while (running && !RGFW_isPressed(win, RGFW_Escape)) {   
        #ifdef __APPLE__
        RGFW_window_checkEvent(win2);
        #endif

        while (RGFW_window_checkEvent(win)) {
            if (win->event.type == RGFW_windowMoved) {
                printf("window moved\n");
            }
            else if (win->event.type == RGFW_windowResized) {
                printf("window resized\n");
            }
            if (win->event.type == RGFW_quit) {
                running = 0;  
                break;
            }
            if (RGFW_isPressed(win, RGFW_Up)) {
                char* str = RGFW_readClipboard(NULL);
                printf("Pasted : %s\n", str);
                RGFW_FREE(str);
            }
            else if (RGFW_isPressed(win, RGFW_Down))
                RGFW_writeClipboard("DOWN", 4);
            else if (RGFW_isPressed(win, RGFW_Space))
                printf("fps : %i\n", win->event.fps);
            else if (RGFW_isPressed(win, RGFW_w))
                RGFW_window_setMouseDefault(win);
            else if (RGFW_isPressed(win, RGFW_q))
                RGFW_window_showMouse(win, 0);
            else if (RGFW_isPressed(win, RGFW_t)) {
                RGFW_window_setMouse(win, icon, RGFW_AREA(3, 3), 4);
            }   

            if (win->event.type == RGFW_dnd) {
                for (i = 0; i < win->event.droppedFilesCount; i++)
                    printf("dropped : %s\n", win->event.droppedFiles[i]);
            }

            else if (win->event.type == RGFW_jsButtonPressed)
                printf("pressed %i\n", win->event.button);

            else if (win->event.type == RGFW_jsAxisMove && !win->event.button)
                printf("{%i, %i}\n", win->event.axis[0].x, win->event.axis[0].y);
        }
        
        drawLoop(win);
    }

    RGFW_window_close(win);
    RSA_deInit();
}

void drawLoop(RGFW_window *w) {
    RGFW_window_makeCurrent(w);

    #ifndef RGFW_VULKAN
    glClearColor(255, 255, 255, 255);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
    
    glBegin(GL_TRIANGLES);
        glColor3f(1, 0, 0); glVertex2f(-0.6, -0.75);
        glColor3f(0, 1, 0); glVertex2f(0.6, -0.75);
        glColor3f(0, 0, 1); glVertex2f(0, 0.75);
    glEnd();
    
    #else

    #endif

    
    RGFW_window_swapBuffers(w); /* NOTE(EimaMei): Rendering should always go: 1. Clear everything 2. Render 3. Swap buffers. Based on https://www.khronos.org/opengl/wiki/Common_Mistakes#Swap_Buffers */
}
