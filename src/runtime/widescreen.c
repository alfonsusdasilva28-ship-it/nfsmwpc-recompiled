/* Resolution and HUD adaptation from ThirteenAG's MIT WidescreenFix.
 * Update the original HUD constants and centers, not the x87 camera stack. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "recomp_types.h"
#include "nfs_widescreen.h"
static unsigned render_width,render_height;
static float display_aspect;
static float hud_center=320.f;
static void put_float(uint32_t address,float value){uint32_t bits;memcpy(&bits,&value,sizeof(value));MEM32(address)=bits;}
static void update_hud(void){
    float aspect=display_aspect>0.f?display_aspect:4.f/3.f;
    hud_center=240.f*aspect;
    put_float(0x008AF9A4u,1.f/hud_center);
    put_float(0x00894B40u,hud_center);
}

void nfs_widescreen_init(const char *game_root) { (void)game_root; }
void nfs_widescreen_set_display_aspect(float aspect) { display_aspect=aspect>1.f&&aspect<8.f?aspect:0.f; }
void nfs_widescreen_set_resolution(unsigned width, unsigned height) { render_width=width;render_height=height; }
int nfs_widescreen_hud_entries(void) { return 0; }
/* Minimap vertical move (Android: the bottom-left corner sits under the
 * steering thumb). Minimap::AdjustForWidescreen (0x5678A0) moves the map
 * horizontally through its local X (+0xB8) and five FE children (+0x3C, +0x40,
 * +0x44, +0x48, +0x90; object data +0x2C, X at +0x1C); the same objects carry Y
 * at +0xBC and data +0x20, and the radar icons add the pivot pair at 0x91CF04
 * (X) / 0x91CF08 (Y, otherwise unused and 0). */
static float minimap_offset;
static uint32_t minimap_object;
static float minimap_applied,minimap_written_y;
static float get_float(uint32_t address){float value;uint32_t bits=MEM32(address);memcpy(&value,&bits,sizeof(value));return value;}
static void move_minimap(uint32_t minimap,float dy){
    static const uint32_t children[]={0x3cu,0x40u,0x44u,0x48u,0x90u};
    for(unsigned i=0;i<sizeof(children)/sizeof(children[0]);++i){
        uint32_t child=MEM32(minimap+children[i]),data=child?MEM32(child+0x2cu):0;
        if(data&&children[i]!=0x90u)put_float(data+0x20u,get_float(data+0x20u)+dy);
    }
    put_float(minimap+0xbcu,get_float(minimap+0xbcu)+dy);
    { uint32_t child=MEM32(minimap+0x90u),data=child?MEM32(child+0x2cu):0;
      if(data)MEM32(data+0x20u)=MEM32(minimap+0xbcu); }
}
/* Once per HUD update (FEngHud::SetWideScreenMode entry, ecx = FEngHud). A new
 * Minimap, or one at a reused address whose Y no longer holds what we wrote,
 * starts from the original layout. */
static void update_minimap(uint32_t hud){
    uint32_t minimap=hud?MEM32(hud+0x320u):0;
    if(!minimap){minimap_object=0;return;}
    if(minimap!=minimap_object||get_float(minimap+0xbcu)!=minimap_written_y){minimap_object=minimap;minimap_applied=0.f;}
    if(minimap_applied!=minimap_offset){move_minimap(minimap,minimap_offset-minimap_applied);minimap_applied=minimap_offset;}
    minimap_written_y=get_float(minimap+0xbcu);
    put_float(0x0091CF08u,minimap_applied);
}
void nfs_widescreen_set_minimap_offset(float dy) { minimap_offset=dy; }
void nfs_widescreen_set_frame_rate(unsigned fps) { (void)fps; }
void nfs_widescreen_apply_frame_rate(void) {}
void nfs_frontend_cursor_units(int on) { (void)on; }
float nfs_widescreen_hud_pos_x(void) { return hud_center; }
void recomp_hook(uint32_t va) {
    switch(va){
    case 0x006C27F9u:case 0x006C2810u:case 0x006C2827u:
    case 0x006C283Eu:case 0x006C2855u:case 0x006C286Cu:
        if(render_width&&render_height){
            MEM32(MEM32(g_esp+4))=render_width;
            MEM32(MEM32(g_esp+8))=render_height;
        }
        update_hud();
        return;
    case 0x005696C0u:update_minimap(g_ecx);return;
    /* After each original mov [esp+offset],320, before its following Y write. */
    case 0x0056FED8u:put_float(g_esp+0x19c,hud_center);return;
    case 0x00584EFAu:put_float(g_esp+0x90,hud_center);return;
    case 0x00599E84u:put_float(g_esp+0x84,hud_center);return;
    case 0x0059A12Bu:put_float(g_esp+0xc4,hud_center);return;
    case 0x0059A5B6u:case 0x0059A849u:put_float(g_esp+0x94,hud_center);return;
    case 0x005A44D0u:put_float(g_esp+0x64,hud_center);return;
    /* Original GetCursorPos conversion returns 0..640. HUD coordinates now
     * span 0..2*center; change the result before the game computes deltas. */
    case 0x006C9C5Cu:MEM32(g_esi+4)=(uint32_t)((float)(int32_t)MEM32(g_esi+4)*(hud_center/320.f));return;
    }
    fprintf(stderr, "Unexpected disabled MW render hook at 0x%08X\n", (unsigned)va);
    abort();
}
