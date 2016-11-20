#include "emumain.h"

#include <psptypes.h>
#include "time.h"
#include <psprtc.h>

#include "pl_rewind.h"
#include "pl_file.h"
#include "pl_snd.h"
#include "pl_perf.h"
#include "pl_util.h"
#include "pl_psp.h"
#include "video.h"
#include "ctrl.h"
#include "ui.h"
#include "shared.h"
#include "sound.h"
#include "system.h"

#include "emumain.h"

PspImage *Screen;
extern PspImage *borderimg;
pl_rewind Rewinder;

static pl_perf_counter FpsCounter;
static int ClearScreen;
static int ScreenX, ScreenY, ScreenW, ScreenH;
static int TicksPerUpdate;
static u32 TicksPerSecond;
static u64 LastTick;
static u64 CurrentTick;
static int Frame;
static int fondo;
extern int bilinear;
extern int changeborder;
extern int gameonram;
extern int vcodies;

extern char *bordername;
extern int customborder;

static int Rewinding;

extern pl_file_path CurrentGame;
extern EmulatorOptions Options;
extern const u64 ButtonMask[];
extern const int ButtonMapId[];
extern struct ButtonConfig ActiveConfig;
extern char *ScreenshotPath;

static inline int ParseInput();
static inline void RenderVideo();
static void AudioCallback(pl_snd_sample* buf,
                          unsigned int samples,
                          void *userdata);
void MixerCallback(int16 **stream, int16 **output, int length);

void InitEmulator()
{

  ClearScreen = 0;

  /* Initialize screen buffer */
  Screen = pspImageCreateVram(256, 192, PSP_IMAGE_INDEXED);

  // pspImageClear(Screen, 0x8000);

  /* Set up bitmap structure */
  memset(&bitmap, 0, sizeof(bitmap_t));
  bitmap.width  = Screen->Width;
  bitmap.height = Screen->Height;
  bitmap.depth  = Screen->Depth;
  bitmap.granularity = (bitmap.depth >> 3);
  bitmap.pitch  = bitmap.width * bitmap.granularity;
  bitmap.data   = (uint8 *)Screen->Pixels;
  bitmap.viewport.x = 0;
  bitmap.viewport.y = 0;
  bitmap.viewport.w = 256;
  bitmap.viewport.h = 192;

 
  /* Initialize sound structure */
  snd.fm_which = Options.SoundEngine;
  snd.fps = FPS_NTSC;
  snd.fm_clock = CLOCK_NTSC;
  snd.psg_clock = CLOCK_NTSC;
  snd.sample_rate = 44100;
  snd.mixer_callback = MixerCallback;

 // sms.use_fm = 1;
 // sms.territory = TERRITORY_EXPORT;

  /* Initialize rewinder */
  pl_rewind_init(&Rewinder,
    save_state_to_mem,
    load_state_from_mem,
    get_save_state_size);

  pl_snd_set_callback(0, AudioCallback, NULL);
}

void RunEmulator()
{
  float ratio;
 
  /* Reset viewport */
  if (IS_GG)
  {
  if (Options.DisplayMode==0 || Options.DisplayMode==1)
  { bitmap.viewport.x = Screen->Viewport.X = 48;
    bitmap.viewport.y = Screen->Viewport.Y = Options.posx+22;
    bitmap.viewport.w = Screen->Viewport.Width = 160;
    bitmap.viewport.h = Screen->Viewport.Height = 136;}
  if (Options.DisplayMode==2)
  { bitmap.viewport.x = Screen->Viewport.X = 8;
    bitmap.viewport.y = Screen->Viewport.Y = (Options.posx*2)+16;
    bitmap.viewport.w = Screen->Viewport.Width = 240;
    bitmap.viewport.h = Screen->Viewport.Height = 136;}
  if (Options.DisplayMode==3 || Options.DisplayMode==4)
  { bitmap.viewport.x = Screen->Viewport.X = 48;
    bitmap.viewport.y = Screen->Viewport.Y = 24;
    bitmap.viewport.w = Screen->Viewport.Width = 160;
    bitmap.viewport.h = Screen->Viewport.Height = 144;}
  }  
  else
  {
    
    if (Options.DisplayMode==0){
    bitmap.viewport.x = Screen->Viewport.X = 0;
    bitmap.viewport.y = Screen->Viewport.Y = 0;
    
    bitmap.viewport.w = Screen->Viewport.Width = 256;
    bitmap.viewport.h = Screen->Viewport.Height = 192;
    
    if (!Options.VertStrip)
    {
      Screen->Viewport.X += 8;
      Screen->Viewport.Width -= 8;}
    if (vcodies==1){
    bitmap.viewport.h = Screen->Viewport.Height = 224; }}
    
    if (Options.DisplayMode==1){
    bitmap.viewport.x = Screen->Viewport.X = 8;
    bitmap.viewport.y = Screen->Viewport.Y = Options.posx;
    bitmap.viewport.w = Screen->Viewport.Width = 240;
    bitmap.viewport.h = Screen->Viewport.Height = 180;}
    
    if (Options.DisplayMode==2){
    bitmap.viewport.x = Screen->Viewport.X = 8;
    bitmap.viewport.y = Screen->Viewport.Y = 0;
    bitmap.viewport.w = Screen->Viewport.Width = 240;
    bitmap.viewport.h = Screen->Viewport.Height = 192;
        if (vcodies==1){
    bitmap.viewport.h = Screen->Viewport.Height = 224; }}
         
    
         
    if (Options.DisplayMode==3 || Options.DisplayMode==4){
    bitmap.viewport.x = Screen->Viewport.X = 0;
    bitmap.viewport.y = Screen->Viewport.Y = 0;
    bitmap.viewport.w = Screen->Viewport.Width = 256;
    bitmap.viewport.h = Screen->Viewport.Height = 192;
    if (!Options.VertStrip)
    {
      Screen->Viewport.X += 8;
      Screen->Viewport.Width -= 8;
    }
    if (vcodies==1){
    bitmap.viewport.h = Screen->Viewport.Height = 224;} }
    
    }

  pspImageClear(Screen, 0);

  /* Recompute screen size/position */
  switch (Options.DisplayMode)
  {
  default:
  case DISPLAY_MODE_UNSCALED:
  
  if (changeborder == 1){
    borderimg = pspImageLoadPng(bordername);
    gameonram = 1;
    changeborder = 0;}

  if (IS_GG){
    ScreenW = 320;
    ScreenH = 272;}
  else {
    ScreenW = Screen->Viewport.Width;
    ScreenH = Screen->Viewport.Height;} 

    fondo=0;
    bilinear = 0;
    break;

  
    
  case DISPLAY_MODE_FIT_HEIGHT: 
  
  
  if (changeborder == 1){
    borderimg = pspImageLoadPng(bordername);
    gameonram = 1;
    changeborder = 0;}

  if (IS_GG){
    ScreenW = 480;
    ScreenH = 272;}
  else {
    ScreenW = Screen->Viewport.Width*2;
    ScreenH = Screen->Viewport.Height*1.5;} 

    fondo=0;
    bilinear = 0;
    break;
    
    
  case DISPLAY_MODE_FILL_SCREEN:

  if (changeborder == 1){
    borderimg = pspImageLoadPng(bordername);
    gameonram = 1;
    changeborder = 0;}

  if (IS_GG){
    ScreenW = 480;
    ScreenH = 272;}
  else {
    ScreenW = Screen->Viewport.Width*2;
    ScreenH = Screen->Viewport.Height;} 

    fondo=0;
    bilinear = 0;
    break;


  case DISPLAY_MODE_ASPECT:

  if (changeborder == 1){
    borderimg = pspImageLoadPng(bordername);
    gameonram = 1;
    changeborder = 0;}

  if (IS_GG){
    ScreenW = 384;
    ScreenH = 272;}
  else {
    ScreenW = 362;
    ScreenH = 272;} 

    fondo=0;
    bilinear = 1;
  break;   

  case DISPLAY_MODE_FULLSCREEN:

  if (changeborder == 1){
    borderimg = pspImageLoadPng(bordername);
    gameonram = 1;
    changeborder = 0;}

    ScreenW = 480;
    ScreenH = 272; 

    fondo=0;
    bilinear = 1;    
    
    break;    
    }

  ScreenX = (SCR_WIDTH / 2) - (ScreenW / 2);
  ScreenY = (SCR_HEIGHT / 2) - (ScreenH / 2);

  /* Init performance counter */
  pl_perf_init_counter(&FpsCounter);

  /* Recompute update frequency */
  TicksPerSecond = sceRtcGetTickResolution();
  if (Options.UpdateFreq)
  {
    TicksPerUpdate = TicksPerSecond
      / (Options.UpdateFreq / (Options.Frameskip + 1));
    sceRtcGetCurrentTick(&LastTick);
  }
  Frame = 0;
  ClearScreen = 1;
  Rewinding = 0;

//pl_rewind_realloc(&Rewinder);

  int frames_until_save = 0;

  /* Resume sound */
  pl_snd_resume(0);

  /* Wait for V. refresh */
  pspVideoWaitVSync();

  /* Main emulation loop */
  while (!ExitPSP)
  {
    /* Check input */
    if (ParseInput()) break;

    /* Rewind/save state */
    if (!Rewinding)
    {
      if (--frames_until_save <= 0)
      {
        frames_until_save = Options.RewindSaveRate;
        pl_rewind_save(&Rewinder);
      }
    }
    else
    {
      frames_until_save = Options.RewindSaveRate;
      if (!pl_rewind_restore(&Rewinder))
        continue;  /* At starting point, cannot rewind */
    }

    /* Run the system emulation for a frame */
    if (++Frame <= Options.Frameskip)
    {
      /* Skip frame */
      system_frame(1);
    }
    else
    {
      system_frame(0);
      Frame = 0;

      /* Display */
      RenderVideo();
    }
  }

  /* Stop sound */
  pl_snd_pause(0);
}

void TrashEmulator()
{
  pl_rewind_destroy(&Rewinder);

  /* Trash screen */
  if (Screen) pspImageDestroy(Screen);

  if (CurrentGame[0] != '\0')
  {
    /* Release emulation resources */
    system_poweroff();
    system_shutdown();
  }
}

int ParseInput()
{
  /* Reset input */
  input.system    = 0;
  input.pad[0]    = 0;
  input.pad[1]    = 0;
 // input.analog[0][0] = 0x7F;
 // input.analog[1][1] = 0x7F;

  static SceCtrlData pad;
  static int autofire_status = 0;

  /* Check the input */
  if (pspCtrlPollControls(&pad))
  {
    if (--autofire_status < 0)
      autofire_status = Options.AutoFire;

    /* Parse input */
    int i, on, code;
    for (i = 0; ButtonMapId[i] >= 0; i++)
    {
      code = ActiveConfig.ButtonMap[ButtonMapId[i]];
      on = (pad.Buttons & ButtonMask[i]) == ButtonMask[i];

      /* Check to see if a button set is pressed. If so, unset it, so it */
      /* doesn't trigger any other combination presses. */
      if (on) pad.Buttons &= ~ButtonMask[i];

      if (!Rewinding)
      {
        if (code & AFI)
        {
          if (on && (autofire_status == 0)) 
            input.pad[0] |= CODE_MASK(code);
          continue;
        }
        else if (code & JOY)
        {
          if (on) input.pad[0] |= CODE_MASK(code);
          continue;
        }
        else if (code & SYS)
        {
          if (on)
          {
            if (CODE_MASK(code) == (INPUT_START | INPUT_PAUSE))
              input.system |= (IS_GG) ? INPUT_START : INPUT_PAUSE;
          }
          continue;
        }
      }

      if (code & SPC)
      {
        switch (CODE_MASK(code))
        {
        case SPC_MENU:
          if (on) return 1;
          break;
        case SPC_REWIND:
          Rewinding = on;
          break;
        }
      }
    }
  }

  return 0;
}

void RenderVideo()
{

  /* Update the display */
  pspVideoBegin();

  /* Clear the buffer first, if necessary */
  if (ClearScreen >= 0)
  {
    ClearScreen--;
    pspVideoClearScreen();
  }

	if (Screen->Depth == PSP_IMAGE_INDEXED && bitmap.pal.update)
	{
    unsigned char r, g, b;
    unsigned short c, i;

		for(i = 0; i < PALETTE_SIZE; i++)
		{
      if (bitmap.pal.dirty[i])
      {
        r = bitmap.pal.color[i][0];
        g = bitmap.pal.color[i][1];
        b = bitmap.pal.color[i][2];
        c = MAKE_PIXEL(r,g,b);

        Screen->Palette[i] = c;
        Screen->Palette[i|0x20] = c;
        Screen->Palette[i|0x40] = c;
        Screen->Palette[i|0x60] = c;
      }
    }
  }

  /* NOTA Muestra PNG*/
  pspVideoPutImage(Screen, ScreenX, ScreenY, ScreenW, ScreenH);
  
  if (fondo<10) {
  pspVideoPutImage(borderimg, 0, 0, 480, 272);
  fondo++;
  }

  /* Show FPS counter */
  if (Options.ShowFps)
  {
    static char fps_display[32];
    sprintf(fps_display, " %3.02f", pl_perf_update_counter(&FpsCounter));

    int width = pspFontGetTextWidth(&PspStockFont, fps_display);
    int height = pspFontGetLineHeight(&PspStockFont);

    pspVideoFillRect(SCR_WIDTH - width, 0, SCR_WIDTH, height, PSP_COLOR_BLACK);
    pspVideoPrint(&PspStockFont, SCR_WIDTH - width, 0, fps_display, PSP_COLOR_WHITE);
  }

  pspVideoEnd();

  /* Wait if needed */
  if (Options.UpdateFreq)
  {
    do { sceRtcGetCurrentTick(&CurrentTick); }
    while (CurrentTick - LastTick < TicksPerUpdate);
    LastTick = CurrentTick;
  }

  /* Wait for VSync signal */
  if (Options.VSync) 
    pspVideoWaitVSync();

  /* Swap buffers */
  pspVideoSwapBuffers();
}

/* Generic FM+PSG stereo mixer callback */
void MixerCallback(int16 **stream, int16 **output, int length)
{
  /* Set up buffer pointers */
  int16 **fm_buffer = (int16 **)&stream[STREAM_FM_MO];
  int16 **psg_buffer = (int16 **)&stream[STREAM_PSG_L];
  int i;

  for(i = 0; i < length; i++)
  {
    int16 temp = (fm_buffer[0][i] + fm_buffer[1][i]) >> 1;
    output[0][i] = (temp + psg_buffer[0][i]) << Options.SoundBoost;
    output[1][i] = (temp + psg_buffer[1][i]) << Options.SoundBoost;
  }
}

static void AudioCallback(pl_snd_sample* buf,
                          unsigned int samples,
                          void *userdata)
{
  int i;
  if (!Rewinding)
  {
    for (i = 0; i < samples; i++) 
    {
      buf[i].stereo.l = snd.output[0][i];
      buf[i].stereo.r = snd.output[1][i];
    }
  }
  else /* Render silence */
    for (i = 0; i < samples; i++) 
      buf[i].stereo.l = buf[i].stereo.r = 0;
}

