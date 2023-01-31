#include <windows.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <math.h>

//------------------------------------------------------------------------------------
//------------------------------   DEFS   --------------------------------------------
//------------------------------------------------------------------------------------

#define PI 3.14159265358979323846

static bool quit = false;
static volatile bool mouse_down;

#define dRand() (rand()/(double)RAND_MAX)

#define PRINTF "%f\n"
#define PRINTI "%i\n"
#define PRINTL "%lu\n"

typedef struct _FRAME{
    int width;
    int height;
    uint32_t *pixels;
} FRAME, *PFRAME;

LRESULT CALLBACK wndProcess(HWND, UINT, WPARAM, LPARAM);

void UpdateVis(PFRAME);

void SetFPixel(int, int, uint32_t, PFRAME);
void DrawLine(int, int, int, int, uint32_t, int, PFRAME);

const LPCSTR wndClass = "wndClass";
const LPCSTR wndTitle = "Collatz Visualization";

static DWORD scrWidth;
static DWORD scrHeight;

static HWND root;
static FRAME frame = {0};

static BITMAPINFO bitmapInfo;
static HBITMAP frameBitmap = 0;
static HDC frameDC = 0;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR pCmdLine, int nCmdShow) {

    scrWidth = GetSystemMetrics(SM_CXSCREEN);
    scrHeight = GetSystemMetrics(SM_CYSCREEN);

    static WNDCLASS wndc = { 0 };
    wndc.lpfnWndProc = wndProcess;
    wndc.hInstance = hInstance;
    wndc.lpszClassName = (PCSTR)wndClass;
    RegisterClass(&wndc);
    
    bitmapInfo.bmiHeader.biSize = sizeof(bitmapInfo.bmiHeader);
    bitmapInfo.bmiHeader.biPlanes = 1;
    bitmapInfo.bmiHeader.biBitCount = 32;
    bitmapInfo.bmiHeader.biCompression = BI_RGB;
    frameDC = CreateCompatibleDC(0);
    
    root = CreateWindow((PCSTR)wndClass, wndTitle, WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                                 0, 0, scrWidth, scrHeight, NULL, NULL, hInstance, NULL);
    if(root == NULL) { 
        return -1; 
    }
    
    while(!quit) {
        static MSG message = { 0 };
        while(PeekMessage(&message, NULL, 0, 0, PM_REMOVE)) { DispatchMessage(&message); }
        
        InvalidateRect(root, NULL, FALSE);
        UpdateWindow(root);
    }
    
    return 0;
}


LRESULT CALLBACK wndProcess(HWND window_handle, UINT message, WPARAM wParam, LPARAM lParam) {

    switch(message) {
        case WM_QUIT:
        case WM_DESTROY: {
            quit = true;
        } break;
        
        case WM_LBUTTONDOWN:{
            mouse_down = true;
            break;
        }

        case WM_LBUTTONUP:{
            mouse_down = false;
            break;
        }

        case WM_PAINT: {
            UpdateVis(&frame);

            static PAINTSTRUCT paint;
            static HDC device_context;
            device_context = BeginPaint(window_handle, &paint);
            BitBlt(device_context,
                   paint.rcPaint.left, paint.rcPaint.top,
                   paint.rcPaint.right - paint.rcPaint.left, paint.rcPaint.bottom - paint.rcPaint.top,
                   frameDC,
                   paint.rcPaint.left, paint.rcPaint.top,
                   SRCCOPY);
            EndPaint(window_handle, &paint);

            //clear frame
            // memset(frame.pixels, 0, 4 * frame.width*frame.height);
            
        } break;
        
        case WM_SIZE: {
            bitmapInfo.bmiHeader.biWidth  = LOWORD(lParam);
            bitmapInfo.bmiHeader.biHeight = HIWORD(lParam);
            
            if(frameBitmap) DeleteObject(frameBitmap);
            frameBitmap = CreateDIBSection(NULL, &bitmapInfo, DIB_RGB_COLORS, &frame.pixels, 0, 0);
            SelectObject(frameDC, frameBitmap);
            
            frame.width =  LOWORD(lParam);
            frame.height = HIWORD(lParam);
        } break;
        
        default: {
            return DefWindowProc(window_handle, message, wParam, lParam);
        }
    }
    return 0;
}


//------------------------------------------------------------------------------------
//------------------------------   CONJECTURE FILE  ----------------------------------
//------------------------------------------------------------------------------------

static long count = 1;
static long max = MAXLONG;

long* _collatz(long);

void UpdateVis(PFRAME frame){

    int lnmg = 7;

    double turn = PI/4;

    if(count > max){
        return;
    }
    
    printf(PRINTI, count);

    long* clist = _collatz(count);

    int cx = 20;
    int cy = 3*frame->height/4;
    
    uint32_t color = 0;

    for(int i = 1; i+1 < clist[0]; i++){
        if(clist[i] < clist[i+1]){
            turn-=PI/24;
        } else {
            turn+=PI/14;
        }

        int ncx = (int)(cx + lnmg*cos(turn));
        int ncy = (int)(cy + lnmg*sin(turn));

        DrawLine(cx, cy, ncx, ncy, color, 1, frame);

        color+=0xff;

        cx = ncx;
        cy = ncy;
    }
    
    free(clist);

    count++;
}

long* _collatz(long n){
    
    long len = 1;
    long* nlist = (long*)malloc(2*sizeof(long));
    nlist[0] = len;
    nlist[1] = n;

    while(n != 1){
        if(n % 2 == 0){
            n/=2;
        } else {
            n*=3;
            n++;
        }
        len++;
        nlist = (long*)realloc(nlist, (len+1)*sizeof(long));
        nlist[len] = n; 
        nlist[0] = len;
    }
    len++;
    nlist[0]++;

    //reverse the array
    for(int i = 1; i < len/2; i++){
        long tmp = nlist[i];
        nlist[i] = nlist[len-i];
        nlist[len-i] = tmp;
    }

    return nlist;
}

//------------------------------------------------------------------------------------
//-------------------------------   GRAPHICS FILE ------------------------------------
//------------------------------------------------------------------------------------

void SetFPixel(int x, int y, uint32_t color, PFRAME frame){
    if(x < 0 || y < 0 || x >= frame->width || y >= frame->height)
        return;
    frame->pixels[x + y*frame->width] = color;
}

void DrawLine(int x1, int y1, int x2, int y2, uint32_t color, int stroke, PFRAME frame){
    //vertically aligned
    if(x1 == x2){

        int start = (y1 >= y2) ? y2 : y1;
        int end = (y1 >= y2) ? y1 : y2;

        for(int off = 0; off < stroke; off++){
            for(int yy = start; yy < end; yy++){
                SetFPixel(x1+off, yy, color, frame);
            }
        }
        return;
    }

    double m = ((y1-y2)*1.0)/(x1-x2);
    int b = (int)(-1*(m*x1-y1));

    int start = (x1 >= x2) ? x2 : x1;
    int end = (x1 >= x2) ? x1 : x2;

    for(int off = 0; off < stroke; off++){
        for(double xx = start; xx < end; xx += (1.0/frame->width)){
            SetFPixel(round(xx), round(m*xx + b)-off, color, frame);
        }
    }
}