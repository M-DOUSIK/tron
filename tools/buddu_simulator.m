#import <Cocoa/Cocoa.h>
#import <CoreGraphics/CoreGraphics.h>

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define FB_W 800
#define FB_H 480

typedef struct {
    uint16_t width;
    uint16_t height;
    const uint16_t *palette;
    const uint8_t *rle;
    size_t rle_size;
} ui_rle_sprite_t;

/* Reuse the exact heart and diagonal capsule pixels extracted from Abirami's
 * drawings for the firmware.  The other legacy assets in this generated file
 * are linked into the desktop preview only; the simulator never draws them. */
typedef struct {
    uint16_t w, h, stride;
    const uint16_t *palette;
    const uint8_t *pixels;
} ui_sprite_t;
typedef struct {
    uint8_t w, h;
    int8_t xoff, yoff;
    uint8_t advance;
    uint16_t offset;
} ui_glyph_t;
typedef struct {
    uint8_t first, last, line_height, baseline;
    const ui_glyph_t *glyphs;
    const uint8_t *blob;
} ui_font_t;

#define __attribute__(ignored)
#include "../sessions/session_13/FSBL/Inc/ui/ui_assets_data.inc"
#undef __attribute__

#define BUDDU_ASSET
#include "../sessions/session_13/FSBL/Inc/ui/buddu_assets_data.inc"

static uint16_t fb[FB_W * FB_H];
static uint8_t rgba[FB_W * FB_H * 4];

enum {
    C_WHITE = 0xffff, C_INK = 0x18c3, C_SOFT = 0x5aeb,
    C_FRAME = 0xff1c, C_ROSE = 0xff3c, C_ROSE_DARK = 0xb1cb,
    C_GREEN = 0xe79c, C_GREEN_DARK = 0x0326, C_RED = 0xfaa8,
    C_YELLOW = 0xff35, C_BLUE = 0xd71f
};

static void fill(uint16_t color) {
    for (size_t i = 0; i < FB_W * FB_H; ++i) fb[i] = color;
}

static void rect(int x, int y, int w, int h, uint16_t c) {
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > FB_W) w = FB_W - x;
    if (y + h > FB_H) h = FB_H - y;
    if (w <= 0 || h <= 0) return;
    for (int yy = y; yy < y + h; ++yy)
        for (int xx = x; xx < x + w; ++xx) fb[yy * FB_W + xx] = c;
}

static void circle(int cx, int cy, int r, uint16_t c) {
    for (int y = -r; y <= r; ++y)
        for (int x = -r; x <= r; ++x)
            if (x*x + y*y <= r*r && cx+x >= 0 && cx+x < FB_W && cy+y >= 0 && cy+y < FB_H)
                fb[(cy+y)*FB_W + cx+x] = c;
}

static void round_rect(int x, int y, int w, int h, int r, uint16_t c) {
    rect(x+r, y, w-2*r, h, c); rect(x, y+r, w, h-2*r, c);
    circle(x+r, y+r, r, c); circle(x+w-r-1, y+r, r, c);
    circle(x+r, y+h-r-1, r, c); circle(x+w-r-1, y+h-r-1, r, c);
}

static void stroke_round(int x, int y, int w, int h, int r, int t, uint16_t c) {
    round_rect(x, y, w, h, r, c);
    round_rect(x+t, y+t, w-2*t, h-2*t, r-t > 0 ? r-t : 1, C_WHITE);
}

static const uint8_t font[][5] = {
/* space */ {0,0,0,0,0},
/* ! */ {0,0,0x5f,0,0}, /* " */ {0,7,0,7,0}, /* # */ {0x14,0x7f,0x14,0x7f,0x14},
/* $ */ {0x24,0x2a,0x7f,0x2a,0x12}, /* % */ {0x23,0x13,8,0x64,0x62},
/* & */ {0x36,0x49,0x55,0x22,0x50}, /* ' */ {0,5,3,0,0},
/* ( */ {0,0x1c,0x22,0x41,0}, /* ) */ {0,0x41,0x22,0x1c,0},
/* * */ {0x14,8,0x3e,8,0x14}, /* + */ {8,8,0x3e,8,8}, /* , */ {0,0x50,0x30,0,0},
/* - */ {8,8,8,8,8}, /* . */ {0,0x60,0x60,0,0}, /* / */ {0x20,0x10,8,4,2},
{0x3e,0x51,0x49,0x45,0x3e},{0,0x42,0x7f,0x40,0},{0x42,0x61,0x51,0x49,0x46},
{0x21,0x41,0x45,0x4b,0x31},{0x18,0x14,0x12,0x7f,0x10},{0x27,0x45,0x45,0x45,0x39},
{0x3c,0x4a,0x49,0x49,0x30},{1,0x71,9,5,3},{0x36,0x49,0x49,0x49,0x36},
{6,0x49,0x49,0x29,0x1e},
/* : ; < = > ? @ */ {0,0x36,0x36,0,0},{0,0x56,0x36,0,0},{8,0x14,0x22,0x41,0},
{0x14,0x14,0x14,0x14,0x14},{0,0x41,0x22,0x14,8},{2,1,0x51,9,6},{0x32,0x49,0x79,0x41,0x3e},
{0x7e,0x11,0x11,0x11,0x7e},{0x7f,0x49,0x49,0x49,0x36},{0x3e,0x41,0x41,0x41,0x22},
{0x7f,0x41,0x41,0x22,0x1c},{0x7f,0x49,0x49,0x49,0x41},{0x7f,9,9,9,1},
{0x3e,0x41,0x49,0x49,0x7a},{0x7f,8,8,8,0x7f},{0,0x41,0x7f,0x41,0},
{0x20,0x40,0x41,0x3f,1},{0x7f,8,0x14,0x22,0x41},{0x7f,0x40,0x40,0x40,0x40},
{0x7f,2,0x0c,2,0x7f},{0x7f,4,8,0x10,0x7f},{0x3e,0x41,0x41,0x41,0x3e},
{0x7f,9,9,9,6},{0x3e,0x41,0x51,0x21,0x5e},{0x7f,9,0x19,0x29,0x46},
{0x46,0x49,0x49,0x49,0x31},{1,1,0x7f,1,1},{0x3f,0x40,0x40,0x40,0x3f},
{0x1f,0x20,0x40,0x20,0x1f},{0x3f,0x40,0x38,0x40,0x3f},{0x63,0x14,8,0x14,0x63},
{7,8,0x70,8,7},{0x61,0x51,0x49,0x45,0x43}
};

static void text_draw(int x, int y, const char *s, int scale, uint16_t c) {
    for (; *s; ++s) {
        unsigned ch = (unsigned char)*s;
        if (ch >= 'a' && ch <= 'z') ch -= 32;
        if (ch < 32 || ch > 90) ch = 32;
        const uint8_t *g = font[ch - 32];
        for (int col=0; col<5; ++col) for (int row=0; row<7; ++row)
            if (g[col] & (1u<<row)) rect(x+col*scale, y+row*scale, scale, scale, c);
        x += 6*scale;
    }
}

static int text_width(const char *s, int scale) { return (int)strlen(s)*6*scale; }
static void centered(int y, const char *s, int scale, uint16_t c) {
    text_draw((FB_W-text_width(s,scale))/2, y, s, scale, c);
}

static void sprite(int x, int y, const ui_rle_sprite_t *sp) {
    size_t pixel = 0;
    for (size_t i=0; i+1 < sp->rle_size; i+=2) {
        uint8_t run=sp->rle[i], idx=sp->rle[i+1];
        for (uint8_t n=0; n<run; ++n, ++pixel) {
            int sx=(int)(pixel%sp->width), sy=(int)(pixel/sp->width);
            if (idx && x+sx>=0 && x+sx<FB_W && y+sy>=0 && y+sy<FB_H)
                fb[(y+sy)*FB_W+x+sx]=sp->palette[idx];
        }
    }
}

static void sprite4(int x, int y, const ui_sprite_t *sp) {
    for (uint16_t sy=0; sy<sp->h; ++sy) {
        for (uint16_t sx=0; sx<sp->w; ++sx) {
            uint8_t packed=sp->pixels[sy*sp->stride+sx/2];
            uint8_t idx=(sx&1)?(packed&0x0f):(packed>>4);
            if (idx && x+sx>=0 && x+sx<FB_W && y+sy>=0 && y+sy<FB_H)
                fb[(y+sy)*FB_W+x+sx]=sp->palette[idx];
        }
    }
}

static void frame(void) {
    fill(C_WHITE);
    round_rect(7,7,786,466,30,C_FRAME); round_rect(20,20,760,440,22,C_WHITE);
    /* Authored diagonal pairs: heart / pill, then pill / heart. */
    sprite4(34,30,&ui_sprite_heart);
    sprite4(716,27,&ui_sprite_capsule);
    sprite4(34,408,&ui_sprite_capsule);
    sprite4(716,411,&ui_sprite_heart);
}

static void button(int x,int y,int w,int h,const char *label,uint16_t fillc,uint16_t edge) {
    round_rect(x,y,w,h,18,edge); round_rect(x+4,y+4,w-8,h-8,14,fillc);
    text_draw(x+(w-text_width(label,3))/2,y+(h-21)/2,label,3,C_INK);
}

static void arrow(int x,int y,bool right) {
    round_rect(x,y,66,54,16,C_ROSE);
    for(int i=0;i<20;i++) rect(right?x+20+i:x+45-i,y+17+i/2,3,20-i,C_INK);
}

typedef enum { IDLE, INTRO, HOME, KEY_ALPHA, KEY_SYMBOL, REGISTER, REGISTERED,
               DISPENSE, COLLECT, TAKEN, ERROR_SCREEN, SCREEN_COUNT } Screen;
static Screen screen=IDLE;
static unsigned tick=0;

static void keyboard(bool symbols) {
    frame(); centered(48,"ENTER YOUR NAME",4,C_INK);
    stroke_round(120,91,560,50,14,3,C_ROSE_DARK);
    text_draw(143,108,"ABIRAMI_",3,C_INK);
    const char *rows_alpha[]={"QWERTYUIOP","ASDFGHJKL","ZXCVBNM"};
    const char *rows_symbol[]={"1234567890","@#$%&*+-=","!?.,()"};
    const char **rows=symbols?rows_symbol:rows_alpha;
    for(int r=0;r<3;r++) {
        int count=(int)strlen(rows[r]), kw=52, gap=7;
        int start=(FB_W-(count*kw+(count-1)*gap))/2;
        for(int i=0;i<count;i++) {
            round_rect(start+i*(kw+gap),165+r*66,kw,52,11,C_ROSE);
            char key[2]={rows[r][i],0};
            text_draw(start+i*(kw+gap)+(kw-text_width(key,3))/2,180+r*66,key,3,C_INK);
        }
    }
    button(82,375,170,55,symbols?"ABC":"123",C_YELLOW,C_ROSE_DARK);
    button(277,375,246,55,"SPACE",C_ROSE,C_ROSE_DARK);
    button(548,375,170,55,"NEXT",C_GREEN,C_GREEN_DARK);
}

static void render(void) {
    frame();
    switch(screen) {
    case IDLE:
        centered(49,"TAP TO WAKE BUDDU",4,C_SOFT);
        sprite(278,132,(tick/7)%2?&ui_sprite_buddu_sleep1:&ui_sprite_buddu_sleep0);
        centered(420,"MEDSIGHT",3,C_INK); break;
    case INTRO:
        centered(54,"HELLO! I AM BUDDU",5,C_INK);
        centered(101,"YOUR MEDICINE HELPER",3,C_SOFT);
        sprite(305,150,&ui_sprite_buddu_registered0);
        button(590,365,145,58,"NEXT",C_ROSE,C_ROSE_DARK); break;
    case HOME:
        centered(48,"WHAT WOULD YOU LIKE TO DO?",4,C_INK);
        sprite(302,110,&ui_sprite_buddu_register);
        button(92,350,270,68,"REGISTER",C_ROSE,C_ROSE_DARK);
        button(438,350,270,68,"DISPENSE",C_GREEN,C_GREEN_DARK); break;
    case KEY_ALPHA: keyboard(false); break;
    case KEY_SYMBOL: keyboard(true); break;
    case REGISTER:
        sprite(75,105,&ui_sprite_buddu_register);
        stroke_round(322,76,410,126,16,4,C_FRAME);
        text_draw(346,105,"HOW MANY PILLS",4,C_INK);
        text_draw(346,151,"PER DAY?",4,C_INK);
        text_draw(389,233,"COUNT: 1",5,C_INK);
        button(397,292,105,62,"-",C_ROSE,C_ROSE_DARK);
        button(552,292,105,62,"+",C_ROSE,C_ROSE_DARK);
        button(417,375,220,55,"NEXT",C_GREEN,C_GREEN_DARK); break;
    case REGISTERED:
        centered(49,"REGISTERED!",5,C_GREEN_DARK);
        sprite(278,125,(tick/5)%2?&ui_sprite_buddu_registered1:&ui_sprite_buddu_registered0);
        centered(409,"BUDDU WILL REMEMBER",3,C_INK); break;
    case DISPENSE:
        centered(50,"DISPENSING",5,C_INK);
        sprite(294,125,&ui_sprite_buddu_dispense);
        for(unsigned i=0;i<(tick/3)%4;i++) circle(543+i*30,205,8,C_ROSE_DARK);
        centered(372,"PLEASE WAIT...",4,C_SOFT); break;
    case COLLECT:
        centered(49,"COLLECT YOUR PILLS",5,C_INK);
        sprite(288,118,&ui_sprite_buddu_collect);
        button(505,353,215,62,"COLLECTED",C_GREEN,C_GREEN_DARK); break;
    case TAKEN:
        centered(48,"WELL DONE!",5,C_GREEN_DARK);
        sprite(278,123,(tick/5)%2?&ui_sprite_buddu_taken1:&ui_sprite_buddu_taken0);
        centered(409,"MEDICINE TAKEN",3,C_INK); break;
    case ERROR_SCREEN: {
        centered(49,"OH NO! TRY AGAIN",5,C_INK);
        const ui_rle_sprite_t *e[] = {&ui_sprite_buddu_error0,&ui_sprite_buddu_error1,&ui_sprite_buddu_error2};
        sprite(285,126,e[(tick/4)%3]);
        button(520,365,190,58,"RETRY",C_RED,C_ROSE_DARK); break; }
    default: break;
    }
    if(screen!=IDLE) { arrow(30,390,false); arrow(704,390,true); }
}

static void convert(void) {
    for(size_t i=0;i<FB_W*FB_H;i++) {
        uint16_t p=fb[i]; rgba[4*i+0]=(uint8_t)(((p>>11)&31)*255/31);
        rgba[4*i+1]=(uint8_t)(((p>>5)&63)*255/63); rgba[4*i+2]=(uint8_t)((p&31)*255/31);
        rgba[4*i+3]=255;
    }
}

@interface BudduView : NSView
@end

@implementation BudduView
- (BOOL)acceptsFirstResponder { return YES; }
- (void)drawRect:(NSRect)dirtyRect {
    (void)dirtyRect; render(); convert();
    CGColorSpaceRef cs=CGColorSpaceCreateDeviceRGB();
    CGDataProviderRef provider=CGDataProviderCreateWithData(NULL,rgba,sizeof(rgba),NULL);
    CGImageRef image=CGImageCreate(FB_W,FB_H,8,32,FB_W*4,cs,
        kCGBitmapByteOrderDefault|kCGImageAlphaLast,provider,NULL,false,kCGRenderingIntentDefault);
    CGContextRef ctx=[[NSGraphicsContext currentContext] CGContext];
    CGContextSetInterpolationQuality(ctx,kCGInterpolationNone);
    CGContextDrawImage(ctx,NSRectToCGRect(self.bounds),image);
    CGImageRelease(image); CGDataProviderRelease(provider); CGColorSpaceRelease(cs);
}
- (void)advance:(int)direction {
    int next=(int)screen+direction;
    if(next<0) next=SCREEN_COUNT-1; if(next>=SCREEN_COUNT) next=0;
    screen=(Screen)next; tick=0; [self setNeedsDisplay:YES];
    printf("Screen %d of %d\n",(int)screen+1,SCREEN_COUNT);
}
- (void)mouseDown:(NSEvent *)event {
    NSPoint p=[self convertPoint:event.locationInWindow fromView:nil];
    p.x=p.x*FB_W/self.bounds.size.width; p.y=FB_H-p.y*FB_H/self.bounds.size.height;
    if(screen==IDLE) screen=INTRO;
    else if(p.x<120 && p.y>360) [self advance:-1];
    else if(p.x>680 && p.y>360) [self advance:1];
    else if(screen==INTRO) screen=HOME;
    else if(screen==HOME && p.y>320) screen=p.x<400?KEY_ALPHA:DISPENSE;
    else if(screen==KEY_ALPHA && p.y>350) screen=p.x<265?KEY_SYMBOL:REGISTER;
    else if(screen==KEY_SYMBOL && p.y>350) screen=p.x<265?KEY_ALPHA:REGISTER;
    else if(screen==REGISTER) screen=REGISTERED;
    else if(screen==COLLECT) screen=TAKEN;
    else if(screen==ERROR_SCREEN) screen=HOME;
    tick=0; [self setNeedsDisplay:YES];
}
- (void)keyDown:(NSEvent *)event {
    if(event.keyCode==123) [self advance:-1];
    else if(event.keyCode==124 || event.keyCode==36 || event.keyCode==49) [self advance:1];
    else if(event.keyCode==53) [[NSApplication sharedApplication] terminate:nil];
}
@end

@interface AppDelegate : NSObject <NSApplicationDelegate>
@property NSWindow *window;
@property NSTimer *timer;
@end

@implementation AppDelegate
- (void)applicationDidFinishLaunching:(NSNotification *)notification {
    (void)notification;
    NSRect r=NSMakeRect(0,0,800,480);
    self.window=[[NSWindow alloc] initWithContentRect:r
        styleMask:NSWindowStyleMaskTitled|NSWindowStyleMaskClosable|NSWindowStyleMaskMiniaturizable|NSWindowStyleMaskResizable
        backing:NSBackingStoreBuffered defer:NO];
    self.window.title=@"Buddu UI Simulator - 800 x 480";
    self.window.aspectRatio=NSMakeSize(5,3);
    BudduView *view=[[BudduView alloc] initWithFrame:r];
    self.window.contentView=view; [self.window center]; [self.window makeKeyAndOrderFront:nil];
    [self.window makeFirstResponder:view];
    self.timer=[NSTimer scheduledTimerWithTimeInterval:0.12 repeats:YES block:^(NSTimer *t){
        (void)t; tick++; [view setNeedsDisplay:YES];
    }];
    [NSApp activateIgnoringOtherApps:YES];
}
- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)sender { (void)sender; return YES; }
@end

int main(void) {
    @autoreleasepool {
        NSApplication *app=[NSApplication sharedApplication];
        AppDelegate *delegate=[AppDelegate new]; app.delegate=delegate;
        [app setActivationPolicy:NSApplicationActivationPolicyRegular]; [app run];
    }
    return 0;
}
