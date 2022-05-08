#include <stdlib.h>
#include <string.h>

// include NESLIB header
#include "neslib.h"

// include CC65 NES Header (PPU)
#include <nes.h>


#include "BG_Game_3.h"

#define NES_MIRRORING 1

// link the pattern table into CHR ROM
//#link "chr_generic.s"

// BCD arithmetic support
#include "bcd.h"
//#link "bcd.c"

// VRAM update buffer
#include "vrambuf.h"
//#link "vrambuf.c"

//#link "famitone2.s"
void __fastcall__ famitone_update(void);
//#link "game2sound.s"
extern char game2sound_data[];

const unsigned char player[]={ // Describes the MetaSprite of our player.
  // x-offset, y-offset, tile,   attributes
     0,        0,        0xC4,   0x03, // Tile 1 upper left
     0,        8,        0xC5,   0x03, // Tile 2 lower left
     8,        0,        0xC6,   0x03, // Tile 3 upper right
     8,        8,        0xC7,   0x03, // Tile 4 lower right
        128};
#define DEF_METASPRITE_2x2(name,code,pal)\
const unsigned char name[]={\
        0,      0,      (code)+0,   pal, \
        0,      8,      (code)+1,   pal, \
        8,      0,      (code)+2,   pal, \
        8,      8,      (code)+3,   pal, \
        128};

#define DEF_METASPRITE_2x2_FLIP(name,code,pal)\
const unsigned char name[]={\
        8,      0,      (code)+0,   (pal)|OAM_FLIP_H, \
        8,      8,      (code)+1,   (pal)|OAM_FLIP_H, \
        0,      0,      (code)+2,   (pal)|OAM_FLIP_H, \
        0,      8,      (code)+3,   (pal)|OAM_FLIP_H, \
        128};

DEF_METASPRITE_2x2(playerStand,0xD8,2);
DEF_METASPRITE_2x2(playerRRun1, 0xdc, 2);
DEF_METASPRITE_2x2(playerRRun2, 0xe0, 2);
DEF_METASPRITE_2x2(playerup, 0xec, 2);
DEF_METASPRITE_2x2(playerup2, 0xf0, 2);

DEF_METASPRITE_2x2(Enemyrun, 0xc4, 2);
DEF_METASPRITE_2x2(Enemyrun2, 0xc8, 2);


DEF_METASPRITE_2x2_FLIP(playerStandL,0xD8,2);
DEF_METASPRITE_2x2_FLIP(playerLRun1, 0xdc, 2);
DEF_METASPRITE_2x2_FLIP(playerLRun2, 0xe0, 2);
DEF_METASPRITE_2x2_FLIP(playerdown, 0xec, 2);
DEF_METASPRITE_2x2_FLIP(playerdown2, 0xf0, 2);

DEF_METASPRITE_2x2_FLIP(EnemyrunL, 0xc4, 2);
DEF_METASPRITE_2x2_FLIP(Enemyrun2L, 0xc8, 2);



#define TITLE_GAME_STATE 0
#define SPAWN_GAME_STATE 1
#define PLAY_GAME_STATE 2
#define NEW_LEVEL_STATE 3
#define GAME_OVER_STATE 4
#define RETRY_STATE 5
#define LEVEL_TWO_STATE 6
#define CH_COIN 0x18
#define CH_LASER 0xCE
#define CH_BLOCK_D 0xCD
#define CH_BLOCK_U 0xCC
#define CH_LADDER_L 0xd4
#define CH_LADDER_R 0xd5



static unsigned char controller, oam_id = 0, temp_index, i=0; // Poll for our controller, oam_id, iterator
static unsigned char player_x,player_y,screen_x,screen_y,coins_x[5],coins_y[5], laser_flag[4],coin_flag[4];//Player x/y coordinates
static unsigned char game_state = 2, level = 1; // Game State
static unsigned char scroll_speed = 2, sprite_speed = 1;
static unsigned char coins_collected = 0, total_coins_collected = 0;
static unsigned char stars_x[10] = {70,50,35,85,20,180,220,205,175,200};
static unsigned char stars_y[10] = {150,120,80,60,170,75,170,140,160,100};
static unsigned char color = 2;
char number[20] = "0";
struct Laser{
unsigned char start_x; // Laser x coordinate
unsigned char start_y,end_y; // Laser y coordinates
unsigned char blocks_x[13], blocks_y[13];
unsigned char num_blocks;
};
struct Laser lasers[5];
//////////////////////////////
const unsigned char* const Running[16] = {
  playerStandL,playerLRun1,playerLRun2,playerStandL,
  playerStandL,playerLRun1,playerLRun2,playerStandL,
  playerStand,playerRRun1,playerRRun2,playerStand,
  playerStand,playerRRun1,playerRRun2,playerStand,
  
};

const unsigned char* const RunUpDown[16] = {

 playerup,playerup2,playerup2,playerup,
playerup,playerup2,playerup2,playerup,
  playerdown,playerdown2,playerdown2,playerdown,
playerdown,playerdown2,playerdown2,playerdown,

};



void player_movement(){
  	if(controller&PAD_UP && player_y > 32)
        {
          player_y-=scroll_speed;
          oam_id = oam_meta_spr(player_x,player_y,oam_id,player);
        }
        if(controller&PAD_DOWN && player_y < 182)
        {
          player_y+=scroll_speed; 
          oam_id = oam_meta_spr(player_x,player_y,oam_id,player);
        }
  
  	 if(controller&PAD_LEFT && player_x > 32)
        {
          player_x-=2; 
          oam_id = oam_meta_spr(player_x,player_y,oam_id,player);
        }
  
   	if(controller&PAD_RIGHT && player_x < 112)
        {
          player_x+=2; 
          oam_id = oam_meta_spr(player_x,player_y,oam_id,player);
        }
}


void keep_everything_right(){
// Make all blocks x and y stay right until the first bar goes through once that bar goes, u can
  // make some sort of flag now true. allowing everything else to follow. Coin will need seperate flag.
  if(laser_flag[0] == 1){
  for(i = 0; i < lasers[0].num_blocks; i++)
    lasers[0].blocks_x[i] = 255;
  laser_flag[0] = 0;
    }
  if(laser_flag[1] == 1)
  for(i = 0; i < lasers[1].num_blocks; i++){
    lasers[1].blocks_x[i] = 255;
    //lasers[1].blocks_x[i] += sprite_speed;
    }
  if(laser_flag[2] == 1)
  for(i = 0; i < lasers[2].num_blocks; i++){
    lasers[2].blocks_x[i] = 255;
    //lasers[2].blocks_x[i] += sprite_speed;
    }
  if(laser_flag[3] == 1)
   for(i = 0; i < lasers[3].num_blocks; i++){
    lasers[3].blocks_x[i] = 255;
    //lasers[3].blocks_x[i] += sprite_speed;
    }
  
  if(coin_flag[0] == 1) {
      coins_x[0] = 0;
      coins_y[0] = 255;
  }
  if(coin_flag[1] == 1) {
      coins_x[1] = 0;
      coins_y[1] = 255;
  }
  if(coin_flag[2] == 1) {
      coins_x[2] = 0;
      coins_y[2] = 255;
  }
  if(coin_flag[3] == 1) {
      coins_x[3] = 0;
      coins_y[3] = 255;
  }
    
    
}
void make_coins(char num){
if(num == 0) {
     // random coin assignment
     // usual x and y start
     coin_flag[0] = 0;
     coins_x[0] = 256;
     coins_y[0] =  (rand() & 69) + 42;
   }
if(num == 1) {
     // random coin assignment
     // usual x and y start
     coin_flag[1] = 0;
     coins_x[1] = 256;
     coins_y[1] =  (rand() & 35) + 105;
   }
if(num == 2) {
     // random coin assignment
     // usual x and y start
     coin_flag[2] = 0;
     coins_x[2] = 256;
     coins_y[2] =  (rand() & 86) + 56;
   }
if(num == 3) {
     // random coin assignment
     // usual x and y start
     coin_flag[3] = 0;
     coins_x[3] = 256;
     coins_y[3] =  (rand() & 56) + 86;
   }
  
}
void spawn_coins(){
  if(lasers[0].blocks_x[0] == 224 || lasers[0].blocks_x[0] == 223)
    make_coins(0);
  if(lasers[1].blocks_x[1] == 224 || lasers[1].blocks_x[1] == 223)
    make_coins(1);
  if(lasers[2].blocks_x[2] == 224 || lasers[2].blocks_x[2] == 223)
    make_coins(2);
  if(lasers[3].blocks_x[3] == 224 || lasers[3].blocks_x[3] == 223)
    make_coins(3);  
}
void move_coins(){
  coins_x[0] -= sprite_speed;
  oam_id = oam_spr(coins_x[0],coins_y[0],CH_COIN,3,oam_id);  
  coins_x[1] -= sprite_speed;
  oam_id = oam_spr(coins_x[1],coins_y[1],CH_COIN,3,oam_id);  
  coins_x[2] -= sprite_speed;
  oam_id = oam_spr(coins_x[2],coins_y[2],CH_COIN,3,oam_id);  
  coins_x[3] -= sprite_speed;
  oam_id = oam_spr(coins_x[3],coins_y[3],CH_COIN,3,oam_id);  
}
void coin_collision(){
  if((player_x + 12 >= coins_x[0]) && (player_x + 12 <= coins_x[0] + 8))
    if((player_y >= coins_y[0] - 16) && (player_y <= coins_y[0] + 8))
    {
      coins_collected +=1;
      coins_x[0] = 0;
      coins_y[0] = 255;
    }
  if((player_x + 12 >= coins_x[1]) && (player_x + 12 <= coins_x[1] + 8))
    if((player_y >= coins_y[1] - 16) && (player_y <= coins_y[1] + 8))
    {
      coins_collected +=1;
      coins_x[1] = 0;
      coins_y[1] = 255;
    }
   if((player_x + 12 >= coins_x[2]) && (player_x + 12 <= coins_x[2] + 8))
    if((player_y >= coins_y[2] - 16) && (player_y <= coins_y[2] + 8))
    {
      coins_collected +=1;
      coins_x[2] = 0;
      coins_y[2] = 255;
    }
   if((player_x + 12 >= coins_x[3]) && (player_x + 12 <= coins_x[3] + 8))
    if((player_y >= coins_y[3] - 16) && (player_y <= coins_y[3] + 8))
    {
      coins_collected +=1;
      coins_x[3] = 0;
      coins_y[3] = 255;
    }
  
return;}
void make_lasers(char num) {
// Depending on the num given will, mean that the laser is offscreen ann
// needs to be remade to spawn on screen recycle. We could also check
// each time to see if those sprites are placed on zero
// Make if else statements checking if num == 0/1/2/3/4/5 and update that laser
   if(num == 0) {
     // randomize number of blocks for our laser 6 - 12
     temp_index = (rand() & 3) + 9;
     // assign the randomized number to the # of blocks our laser will have
     lasers[0].num_blocks = temp_index; 
     // usual x and y start
     lasers[0].start_x = 256;
     lasers[0].start_y =  (rand() & 69) + 42;
     // give our first block the start location, then for loop to make other blocks
     lasers[0].blocks_x[0] = lasers[0].start_x;
     lasers[0].blocks_y[0] = lasers[0].start_y;
     for(i = 1; i < lasers[0].num_blocks ; i++){
     lasers[0].blocks_x[i] = lasers[0].start_x;
     lasers[0].blocks_y[i] = lasers[0].blocks_y[i-1] + 8;
     }
     lasers[0].end_y = lasers[0].blocks_y[lasers[0].num_blocks - 1];
   }
   if(num == 1) {
     // randomize number of blocks for our laser 5 - 12
     temp_index = (rand() & 4) + 6;
     // assign the randomized number to the # of blocks our laser will have
     lasers[1].num_blocks = temp_index; 
     // usual x and y start
     lasers[1].start_x = 256;
     lasers[1].start_y = (rand() & 35) + 105;
     // give our first block the start location, then for loop to make other blocks
     lasers[1].blocks_x[0] = lasers[1].start_x;
     lasers[1].blocks_y[0] = lasers[1].start_y;
     for(i = 1; i < lasers[1].num_blocks ; i++){
     	lasers[1].blocks_x[i] = lasers[1].start_x;
     	lasers[1].blocks_y[i] = lasers[1].blocks_y[i-1] + 8;
     }
     lasers[1].end_y = lasers[1].blocks_y[lasers[1].num_blocks - 1];
   }
  if(num == 2) {
     // randomize number of blocks for our laser 5 - 12
     temp_index = (rand() & 4) + 6;
     // assign the randomized number to the # of blocks our laser will have
     lasers[2].num_blocks = temp_index; 
     // usual x and y start
     lasers[2].start_x = 256;
     lasers[2].start_y = (rand() & 86) + 56;
     // give our first block the start location, then for loop to make other blocks
     lasers[2].blocks_x[0] = lasers[2].start_x;
     lasers[2].blocks_y[0] = lasers[2].start_y;
     for(i = 1; i < lasers[2].num_blocks ; i++){
     	lasers[2].blocks_x[i] = lasers[2].start_x;
     	lasers[2].blocks_y[i] = lasers[2].blocks_y[i-1] + 8;
     }
    lasers[2].end_y = lasers[2].blocks_y[lasers[2].num_blocks - 1];
   }
  
  if(num == 3) {
     // randomize number of blocks for our laser 5 - 12
     temp_index = (rand() & 5) + 6;
     // assign the randomized number to the # of blocks our laser will have
     lasers[3].num_blocks = temp_index; 
     // usual x and y start
     lasers[3].start_x = 256;
     lasers[3].start_y = (rand() & 56) + 86;
     // give our first block the start location, then for loop to make other blocks
     lasers[3].blocks_x[0] = lasers[3].start_x;
     lasers[3].blocks_y[0] = lasers[3].start_y;
     for(i = 1; i < lasers[3].num_blocks ; i++){
     	lasers[3].blocks_x[i] = lasers[3].start_x;
     	lasers[3].blocks_y[i] = lasers[3].blocks_y[i-1] + 8;
     }
    lasers[3].end_y = lasers[3].blocks_y[lasers[3].num_blocks - 1];
   }
   
}
void move_lasers() {
//Concerns movement on screen, gotta make lasers progressively move towards the left
  oam_id = oam_meta_spr(player_x,player_y,oam_id,player);
    
  for(i = 0; i < lasers[0].num_blocks; i++){
    lasers[0].blocks_x[i] -= sprite_speed;
    if(i == 0)
      oam_id = oam_spr(lasers[0].blocks_x[i],lasers[0].blocks_y[i],CH_BLOCK_D,2,oam_id);
    if(i == lasers[0].num_blocks - 1)
      oam_id = oam_spr(lasers[0].blocks_x[i],lasers[0].blocks_y[i],CH_BLOCK_U,2,oam_id);
    else
      oam_id = oam_spr(lasers[0].blocks_x[i],lasers[0].blocks_y[i],CH_LASER,color,oam_id);
    }
  
  for(i = 0; i < lasers[1].num_blocks; i++){
    lasers[1].blocks_x[i] -= sprite_speed;
    if(i == 0)
      oam_id = oam_spr(lasers[1].blocks_x[i],lasers[1].blocks_y[i],CH_BLOCK_D,2,oam_id);
    if(i == lasers[1].num_blocks - 1)
      oam_id = oam_spr(lasers[1].blocks_x[i],lasers[1].blocks_y[i],CH_BLOCK_U,2,oam_id);
    else
      oam_id = oam_spr(lasers[1].blocks_x[i],lasers[1].blocks_y[i],CH_LASER,color,oam_id);
    }
  
  for(i = 0; i < lasers[2].num_blocks; i++){
    lasers[2].blocks_x[i] -= sprite_speed;
    if(i == 0)
      oam_id = oam_spr(lasers[2].blocks_x[i],lasers[2].blocks_y[i],CH_BLOCK_D,2,oam_id);
    if(i == lasers[2].num_blocks - 1)
      oam_id = oam_spr(lasers[2].blocks_x[i],lasers[2].blocks_y[i],CH_BLOCK_U,2,oam_id);
    else
      oam_id = oam_spr(lasers[2].blocks_x[i],lasers[2].blocks_y[i],CH_LASER,color,oam_id);
    }
  
   for(i = 0; i < lasers[3].num_blocks; i++){
    lasers[3].blocks_x[i] -= sprite_speed;
    if(i == 0)
      oam_id = oam_spr(lasers[3].blocks_x[i],lasers[3].blocks_y[i],CH_BLOCK_D,2,oam_id);
    if(i == lasers[3].num_blocks - 1)
      oam_id = oam_spr(lasers[3].blocks_x[i],lasers[3].blocks_y[i],CH_BLOCK_U,2,oam_id);
    else
      oam_id = oam_spr(lasers[3].blocks_x[i],lasers[3].blocks_y[i],CH_LASER,color,oam_id);
    }
}

void spawn_remake_lasers(){
  if(lasers[0].blocks_x[0] == 0)
    make_lasers(0);
  if(lasers[0].blocks_x[0] == 192){
    laser_flag[1] = 0;
    make_lasers(1);}
  if(lasers[1].blocks_x[1] == 192){
    laser_flag[2] = 0;
    make_lasers(2);}
  if(lasers[2].blocks_x[2] == 192){
    laser_flag[3] = 0;
    make_lasers(3);}
}

void detect_laser_collision(){
  if((player_x + 12 >= lasers[0].blocks_x[0]) && (player_x + 12 <= lasers[0].blocks_x[0] + 8))
    if((player_y >= lasers[0].start_y-14) && (player_y <= lasers[0].end_y + 7))
      game_state = GAME_OVER_STATE;
  if((player_x + 12 >= lasers[1].blocks_x[0]) && (player_x + 12 <= lasers[1].blocks_x[0] + 8))
    if((player_y >= lasers[1].start_y-14) && (player_y <= lasers[1].end_y + 7))
      game_state = GAME_OVER_STATE;
  if((player_x + 12 >= lasers[2].blocks_x[0]) && (player_x + 12 <= lasers[2].blocks_x[0] + 8))
    if((player_y >= lasers[2].start_y-14) && (player_y <= lasers[2].end_y + 7))
      game_state = GAME_OVER_STATE;
  if((player_x + 12 >= lasers[3].blocks_x[0]) && (player_x + 12 <= lasers[3].blocks_x[0] + 8))
    if((player_y >= lasers[3].start_y-14) && (player_y <= lasers[3].end_y + 7))
      game_state = GAME_OVER_STATE;
  
return;}

void fade_out(){
byte x;
  for(x=4;x>=1;x--)
  {
	pal_bright(x);
   ppu_wait_frame();
    ppu_wait_frame();
    ppu_wait_frame();
    ppu_wait_frame();
    ppu_wait_frame();
    ppu_wait_frame();
    ppu_wait_frame();
    ppu_wait_frame();
    ppu_wait_frame();
    ppu_wait_frame();
    ppu_wait_frame();
    ppu_wait_frame();
   
  }
  pal_bright(x);
}

void fade_in(){
  byte x;
  for(x=0;x<=4;x++)
  {
  
  pal_bright(x);
    ppu_wait_frame();
    ppu_wait_frame();
    ppu_wait_frame();
    ppu_wait_frame();
    ppu_wait_frame();
    ppu_wait_frame();
    ppu_wait_frame();
    ppu_wait_frame();
    ppu_wait_frame();
    ppu_wait_frame();
    ppu_wait_frame();
    ppu_wait_frame();
   
  }

}


void setup_graphics() {
  // clear sprites
  oam_clear();
  // set palette colors
  
  pal_spr(palette);
  pal_bg(palette);
}


void main(void)
{
  famitone_init(game2sound_data);
  nmi_set_callback(famitone_update);
  game_state = TITLE_GAME_STATE;
  vram_adr(NTADR_A(0,0));
  vram_unrle(Title_Game_3);
  for(i = 0; i < 4; i++){
    laser_flag[i] = 0;
    coin_flag[i] = 0;}
  setup_graphics();
  ppu_on_all();
  fade_in();
	 //enable rendering
  	//setup sound
  	//famitone_init(game2sound_data);
  	//nmi_set_callback(famitone_update);

	while(1) {
        oam_id = 0;  	
	controller=pad_poll(0);
        //oam_id = oam_meta_spr(player_x,player_y,oam_id,Running[0]);
         
          if(game_state == TITLE_GAME_STATE)
          {
             for(i=0;i<10;i++)
 	     oam_id = oam_spr(stars_x[i],stars_y[i],0xB9,2,oam_id);
            
                controller=pad_poll(0);
  	  	ppu_wait_frame();
   		oam_id=0;
  		for(i=0;i<10;i++) {
    		  stars_x[i] -=1;
    		  oam_id = oam_spr(stars_x[i],stars_y[i],0xB9,2,oam_id);
    		}
                if(controller&PAD_START){
                  game_state = SPAWN_GAME_STATE;
                  fade_out();
                  ppu_off();
                }
          
            }
            
          
          if(game_state == SPAWN_GAME_STATE)
          {
              oam_clear_fast();
              player_x = 56; // player position
	      player_y = 56;
              screen_x = 0;   // x scroll position
  	      screen_y = 0;   // y scroll position
              scroll_speed = 2;
              sprite_speed = 1;
              level = 1;
              color = 3;
              for(i = 0; i < 4; i++){
    		laser_flag[i] = 1;
    		coin_flag[i] = 1;}
              coins_collected = 0;
              vram_adr(NTADR_A(0,0));
  	      vram_unrle(Background_Game_3);
  	      vram_adr(NTADR_B(0,0));
  	      vram_unrle(Background_Game_3);
  	      setup_graphics();
              ppu_on_all();
              make_lasers(0);
            
              //reset_all_sprites();
              fade_in();
              music_play(0);
              game_state = PLAY_GAME_STATE;
            continue;
          }
          
          if(game_state == PLAY_GAME_STATE)
          {
            	
            	oam_clear(); // must oam_clear or sprites leave trail behind
                keep_everything_right(); // only at the start, here we use our flags
            	player_movement(); // Checking player movement, and spawning player
    		spawn_remake_lasers();
            	move_lasers(); // Moving our laser sprites, and spawning them
            	spawn_coins();
            	move_coins();
            	coin_collision();
            	detect_laser_collision();
            	scroll(screen_x, screen_y); // scrolling screen
            	screen_x += scroll_speed; // adding one to our scroll coordinate x
            	
            	if (coins_collected == 25 && level !=2){
		game_state = NEW_LEVEL_STATE;
                total_coins_collected += coins_collected;
                music_stop();
                }
            	if (coins_collected == 12){
                scroll_speed = 3;
                sprite_speed = 2;
                }
            	
          }
         if(game_state == LEVEL_TWO_STATE)
          {
            oam_clear();
            music_stop();
            fade_out();
           oam_clear_fast();
              player_x = 56; // player position
	      player_y = 56;
              //screen_x = 0;   // x scroll position
  	      //screen_y = 0;   // y scroll position
              scroll_speed = 2;
              sprite_speed = 1;
              level = 2;
              color = 0;
              for(i = 0; i < 4; i++){
    		laser_flag[i] = 1;
    		coin_flag[i] = 1;}
              coins_collected = 0;
              vram_adr(NTADR_A(0,0));
  	      vram_unrle(Background_2_Game_3);
  	      vram_adr(NTADR_B(0,0));
  	      vram_unrle(Background_2_Game_3);
  	      setup_graphics();
              ppu_on_all();
              make_lasers(0);
            
              //reset_all_sprites();
              fade_in();
              game_state = PLAY_GAME_STATE;
            continue;
            
            
          }
          if(game_state == NEW_LEVEL_STATE)
          {
            oam_clear(); 
            
            oam_id = oam_meta_spr(player_x,player_y,oam_id,player);
           
            
              controller = PAD_RIGHT;
            if(player_y > 0){
          	player_y-=1;
          	oam_id = oam_meta_spr(player_x,player_y,oam_id,player);
        	}
       	    
   	    if(controller&PAD_RIGHT && player_x <= 242){
          	player_x+=1; 
          	oam_id = oam_meta_spr(player_x,player_y,oam_id,player);
        	}
            
            if (player_y == 0)
            {
            	game_state = LEVEL_TWO_STATE;
            	fade_out();
              	ppu_off();
            }
            
          }
          
           if(game_state == GAME_OVER_STATE)
          {
            music_stop();
            fade_out();
            scroll(0, 0);
            ppu_off();
            vram_adr(NTADR_A(0,0));
            vram_fill(0x00,32*30);
            vram_adr(NTADR_A(11,10));
            vram_write("Game Over",9);
            vram_adr(NTADR_A(11,14));
            total_coins_collected += coins_collected;
            itoa(total_coins_collected*4,number,10);
            vram_write("Your Score:",10);
            vram_adr(NTADR_A(22,14));
            vram_write(number,10);
            vram_adr(NTADR_A(11,18));
            vram_write("Try again?",10);
            fade_in();
            pal_bright(3);
            oam_clear();
            ppu_on_bg();
            game_state = RETRY_STATE;

            
          }
           if(game_state == RETRY_STATE)
          {
           if(controller&PAD_START)
           {
             game_state = TITLE_GAME_STATE;
             fade_out();
             ppu_off();
             vram_adr(NTADR_A(0,0));
  	     vram_unrle(Title_Game_3);
             setup_graphics();
             ppu_on_all();
             fade_in();
           }
          }
          
        ppu_wait_frame();
	}
}