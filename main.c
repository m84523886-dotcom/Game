#include <pspkernel.h>
#include <pspdisplay.h>
#include <pspgu.h>
#include <pspdebug.h>
#include <pspctrl.h>
#include <stdlib.h>
#include <string.h>

PSP_MODULE_INFO("NinjaDaFolha", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);

#define W 480
#define H 272
#define GROUND 226
#define WORLD_W 1800

static unsigned int __attribute__((aligned(16))) list[262144];
typedef struct { unsigned int color; short x, y, z; } Vertex;
static Vertex verts[6];

static int px=42, py=GROUND-34, pv=0, face=1, hp=3, score=0;
static int enemy_x[4]={330,650,990,1350};
static int enemy_alive[4]={1,1,1,1};
static int orb_x[9]={120,205,390,490,720,830,1080,1240,1580};
static int orb_y[9]={175,145,180,150,175,145,170,150,175};
static int orb_taken[9]={0};
static int cam=0, attack=0, invuln=0, won=0, lost=0;
static int tick=0;

static void rect(int x,int y,int w,int h,unsigned int color) {
    int sx=x-cam;
    if(sx+w<0 || sx>=W || y+h<0 || y>=H) return;
    verts[0]=(Vertex){color,(short)sx,(short)y,0};
    verts[1]=(Vertex){color,(short)(sx+w),(short)y,0};
    verts[2]=(Vertex){color,(short)sx,(short)(y+h),0};
    verts[3]=(Vertex){color,(short)(sx+w),(short)y,0};
    verts[4]=(Vertex){color,(short)(sx+w),(short)(y+h),0};
    verts[5]=(Vertex){color,(short)sx,(short)(y+h),0};
    sceGuDrawArray(GU_TRIANGLES, GU_COLOR_8888|GU_VERTEX_16BIT|GU_TRANSFORM_2D, 6, 0, verts);
}
static void circleish(int x,int y,int r,unsigned int color) {
    rect(x-r,y-r,r*2,r*2,color);
}
static void draw_scene(void) {
    sceGuStart(GU_DIRECT,list);
    sceGuClearColor(0xfff2d48b);
    sceGuClear(GU_COLOR_BUFFER_BIT|GU_DEPTH_BUFFER_BIT);
    // céu, colinas e chão
    rect(0,0,W,H,0xfff2d48b);
    for(int i=0;i<7;i++) rect(i*100+((cam/5)%100)-80,145+(i%3)*13,120,75,0xff8abf76);
    rect(0,GROUND,W,H-GROUND,0xff4e8b43);
    rect(0,GROUND,W,8,0xff7ed05d);
    for(int x=0;x<WORLD_W;x+=42) rect(x,GROUND+18,24,4,0xff376e3b);
    // casinhas da vila
    for(int i=0;i<12;i++){
        int x=70+i*155;
        rect(x,GROUND-63,65,63,0xffc9a477);
        rect(x-5,GROUND-77,75,19,0xffb95c49);
        rect(x+23,GROUND-38,18,24,0xff6d8792);
    }
    // orbes
    for(int i=0;i<9;i++) if(!orb_taken[i]) {
        int bob=(tick/8+i)%5;
        circleish(orb_x[i],orb_y[i]+bob,7,0xff55ddff);
        rect(orb_x[i]-2,orb_y[i]-5+bob,4,10,0xffffffc0);
    }
    // inimigos
    for(int i=0;i<4;i++) if(enemy_alive[i]) {
        rect(enemy_x[i],GROUND-30,28,30,0xff4545a8);
        rect(enemy_x[i]+4,GROUND-24,20,8,0xffd9c9b8);
        rect(enemy_x[i]+6,GROUND-22,4,4,0xff202030);
        rect(enemy_x[i]+17,GROUND-22,4,4,0xff202030);
    }
    // jogador ninja
    if(invuln==0 || (tick/4)%2==0) {
        rect(px,py+12,24,22,0xff293c58);
        rect(px+3,py+2,18,15,0xffe9b58e);
        rect(px,py,24,8,0xff202b43);
        rect(px+2,py+5,20,4,0xffe8eaf0);
        rect(px+(face>0?15:3),py+9,6,3,0xffdc4949);
        rect(px+2,py+31,8,6,0xff172033);
        rect(px+15,py+31,8,6,0xff172033);
    }
    if(attack>0) rect(px-cam+(face>0?23:-18),py+13,20,10,0xff55e8ff);
    // HUD
    rect(8,8,174,38,0xcc15243a);
    // small text via debug screen is drawn separately below
    sceGuFinish();
    sceGuSync(0,0);
    sceDisplayWaitVblankStart();
    sceGuSwapBuffers();
}
static void reset_game(void) {
    px=42; py=GROUND-34; pv=0; face=1; hp=3; score=0; cam=0;
    attack=0; invuln=0; won=0; lost=0; tick=0;
    for(int i=0;i<4;i++) enemy_alive[i]=1;
    for(int i=0;i<9;i++) orb_taken[i]=0;
}
int main(void) {
    sceCtrlSetSamplingCycle(0);
    sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);
    sceGuInit();
    sceGuStart(GU_DIRECT,list);
    sceGuDrawBuffer(GU_PSM_8888,(void*)0,512);
    sceGuDispBuffer(W,H,(void*)0x88000,512);
    sceGuDepthBuffer((void*)0x110000,512);
    sceGuOffset(2048-W/2,2048-H/2);
    sceGuViewport(2048,2048,W,H);
    sceGuDepthRange(0xc350,0x2710);
    sceGuScissor(0,0,W,H);
    sceGuEnable(GU_SCISSOR_TEST);
    sceGuDisable(GU_DEPTH_TEST);
    sceGuFinish();
    sceGuSync(0,0);
    sceDisplayWaitVblankStart();
    sceGuDisplay(GU_TRUE);
    pspDebugScreenInit();

    while(1) {
        SceCtrlData pad;
        sceCtrlReadBufferPositive(&pad,1);
        if(pad.Buttons & PSP_CTRL_START) { reset_game(); }
        if(!(won||lost)) {
            int move=0;
            if(pad.Buttons & PSP_CTRL_LEFT) move=-3;
            if(pad.Buttons & PSP_CTRL_RIGHT) move=3;
            if(pad.Buttons & PSP_CTRL_CROSS) { if(py>=GROUND-34) pv=-9; }
            if(pad.Buttons & PSP_CTRL_SQUARE) { if(attack==0) attack=12; }
            if(move) face=move>0?1:-1;
            px+=move; if(px<0)px=0; if(px>WORLD_W-24)px=WORLD_W-24;
            py+=pv; pv+=1;
            if(py>=GROUND-34){py=GROUND-34;pv=0;}
            if(attack>0) attack--;
            if(invuln>0)invuln--;
            for(int i=0;i<9;i++) if(!orb_taken[i] && abs((px+12)-orb_x[i])<20 && abs((py+17)-orb_y[i])<28){orb_taken[i]=1;score++;}
            for(int i=0;i<4;i++) if(enemy_alive[i]) {
                if(abs((px+12)-(enemy_x[i]+14))<28 && py+34>GROUND-30) {
                    if(attack>0) enemy_alive[i]=0;
                    else if(invuln==0){hp--;invuln=65;pv=-5;if(hp<=0)lost=1;}
                }
                enemy_x[i] += ((tick/70+i)%2)?1:-1;
            }
            if(px>WORLD_W-70)won=1;
            cam=px-150;if(cam<0)cam=0;if(cam>WORLD_W-W)cam=WORLD_W-W;
        }
        tick++;
        draw_scene();
        pspDebugScreenSetXY(2,1);
        pspDebugScreenSetTextColor(0xffffffff);
        pspDebugScreenPrintf("NINJA DA FOLHA  |  ORBES %d/9  |  VIDAS %d",score,hp);
        pspDebugScreenSetXY(2,3);
        pspDebugScreenPrintf("D-PAD: mover  X: pular  QUADRADO: atacar  START: reiniciar");
        if(won||lost) {
            pspDebugScreenSetXY(14,10);
            pspDebugScreenPrintf(won?"MISSAO CONCLUIDA!":"FIM DE JOGO");
            pspDebugScreenSetXY(10,12);
            pspDebugScreenPrintf("Pressione START para jogar novamente");
        }
        sceDisplayWaitVblankStart();
    }
    sceGuTerm();
    sceKernelExitGame();
    return 0;
}
