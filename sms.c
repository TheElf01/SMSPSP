/*
    sms.c --
    Sega Master System console emulation.
*/
#include "shared.h"

/* SMS context */
sms_t sms;


/* BIOS/CART ROM */
bios_t bios;
slot_t slot;

uint8 dummy_write[0x400];
uint8 dummy_read[0x400];

void writemem_mapper_none(int offset, int data)
{
    cpu_writemap[offset >> 10][offset & 0x03FF] = data;
}

void writemem_mapper_sega(int offset, int data)
{
    cpu_writemap[offset >> 10][offset & 0x03FF] = data;
    if(offset >= 0xFFFC)
        sms_mapper_w(offset & 3, data);
}

void writemem_mapper_codies(int offset, int data)
{
    switch(offset & 0xC000)
    {
        case 0x0000:
            sms_mapper_w(1, data);
            return;
        case 0x4000:
            sms_mapper_w(2, data);
            return;
        case 0x8000:
            sms_mapper_w(3, data);
            return;
        case 0xC000:
            cpu_writemap[offset >> 10][offset & 0x03FF] = data;
            return;
    }

}


static void writemem_mapper_korea_msx(int offset, int data)
{
  if (offset <= 0x0003)
  {
    mapper_8k_w(offset,data);
    //sms_mapper_w(offset & 3, data);
    return;
  }
  cpu_writemap[offset >> 10][offset & 0x03FF] = data;
}

static void writemem_mapper_korea(int offset, int data)
{
  if (offset == 0xA000)
  {
    mapper_16k_w(3,data);
    //sms_mapper_w(offset & 3, data);
    return;
  }
  cpu_writemap[offset >> 10][offset & 0x03FF] = data;
}

static void writemem_mapper_castle(int offset, int data)
{
  if (offset == 0xA000)
  {
   sms_mapper_w(offset & 3, data);
    return;
  }
  cpu_writemap[offset >> 10][offset & 0x03FF] = data;
}


/*static void writemem_mapper_koreasega(int offset, int data)
{
  if (offset == 0xA000)
  {
    //mapper_16k_w(3,data);
    sms_mapper_w(offset & 3, data);
    return;
  }
  cpu_writemap[offset >> 10][offset & 0x03FF] = data;
}*/



void sms_init(void)
{
  
    z80_init();
    sms_reset();

  
    /* Default: open bus */
    data_bus_pullup     = 0x00;
    data_bus_pulldown   = 0x00;

    /* Assign mapper */
    cpu_writemem16 = writemem_mapper_sega;
    if(cart.mapper == MAPPER_CODIES)
        cpu_writemem16 = writemem_mapper_codies;
    if(cart.mapper == MAPPER_KOREA)
        cpu_writemem16 = writemem_mapper_korea;
    if(cart.mapper == MAPPER_KOREA_MSX)
        cpu_writemem16 = writemem_mapper_korea_msx;
    if(cart.mapper == MAPPER_CASTLE)
        cpu_writemem16 = writemem_mapper_castle;        

    /* Force SMS (J) console type if FM sound enabled */
    if(sms.use_fm)
    {
        sms.console = CONSOLE_SMSJ;
        sms.territory = TERRITORY_DOMESTIC;
        sms.display = DISPLAY_NTSC;
    }

    /* Initialize selected console emulation */
    switch(sms.console)
    {
        case CONSOLE_SMS:
            cpu_writeport16 = sms_port_w;
            cpu_readport16 = sms_port_r;
            break;

        case CONSOLE_SMSJ:
            cpu_writeport16 = smsj_port_w;
            cpu_readport16 = smsj_port_r;
            break;
  
        case CONSOLE_SMS2:
            cpu_writeport16 = sms_port_w;
            cpu_readport16 = sms_port_r;
            data_bus_pullup = 0xFF;
            break;

        case CONSOLE_GG:
            cpu_writeport16 = gg_port_w;
            cpu_readport16 = gg_port_r;
            data_bus_pullup = 0xFF;
            break;

        case CONSOLE_GGMS:
            cpu_writeport16 = ggms_port_w;
            cpu_readport16 = ggms_port_r;
            data_bus_pullup = 0xFF;
            break;

        case CONSOLE_GEN:
        case CONSOLE_MD:
            cpu_writeport16 = md_port_w;
            cpu_readport16 = md_port_r;
            break;

        case CONSOLE_GENPBC:
        case CONSOLE_MDPBC:
            cpu_writeport16 = md_port_w;
            cpu_readport16 = md_port_r;
            data_bus_pullup = 0xFF;
            break;
    }
}

void sms_shutdown(void)
{
    /* Nothing to do */
}

void sms_reset(void)
{
    int i;

    
    z80_reset(NULL);
    z80_set_irq_callback(sms_irq_callback);

    /* Clear SMS context */
    memset(dummy_write, 0, sizeof(dummy_write));
    memset(dummy_read,  0, sizeof(dummy_read));
    memset(sms.wram,    0, sizeof(sms.wram));
    memset(cart.sram,    0, sizeof(cart.sram));

    sms.paused      = 0x00;
    sms.save        = 0x00;
    sms.fm_detect   = 0x00;
    sms.memctrl     = 0xAB;
    sms.ioctrl      = 0xFF;

    for(i = 0x00; i <= 0x2F; i++)
    {
        cpu_readmap[i]  = &cart.rom[(i & 0x1F) << 10];
        cpu_writemap[i] = dummy_write;
    }

    for(i = 0x30; i <= 0x3F; i++)
    {
        cpu_readmap[i] = &sms.wram[(i & 0x07) << 10];
        cpu_writemap[i] = &sms.wram[(i & 0x07) << 10];
    }

    cart.fcr[0] = 0x00;
    cart.fcr[1] = 0x00;
    cart.fcr[2] = 0x01;
    cart.fcr[3] = 0x00;
}


void sms_mapper_w(int address, int data)
{
    int i;

    /* Calculate ROM page index */
    uint8 page = (data % cart.pages);

    /* Save frame control register data */
    cart.fcr[address] = data;

    switch(address)
    {
        case 0:
            if(data & 8)
            {
                uint32 offset = (data & 4) ? 0x4000 : 0x0000;
                sms.save = 1;

                for(i = 0x20; i <= 0x2F; i++)
                {
                    cpu_writemap[i] = cpu_readmap[i]  = &cart.sram[offset + ((i & 0x0F) << 10)];
                }
            }
            else
            {
                for(i = 0x20; i <= 0x2F; i++)
                {          
                    cpu_readmap[i] = &cart.rom[((cart.fcr[3] % cart.pages) << 14) | ((i & 0x0F) << 10)];
                    cpu_writemap[i] = dummy_write;
                }
            }
            break;

        case 1:
            for(i = 0x01; i <= 0x0F; i++)
            {
                cpu_readmap[i] = &cart.rom[(page << 14) | ((i & 0x0F) << 10)];
            }
            break;

        case 2:
            for(i = 0x10; i <= 0x1F; i++)
            {
                cpu_readmap[i] = &cart.rom[(page << 14) | ((i & 0x0F) << 10)];
            }
            break;

        case 3:
            if(!(cart.fcr[0] & 0x08))
            {
                for(i = 0x20; i <= 0x2F; i++)
                {
                    cpu_readmap[i] = &cart.rom[(page << 14) | ((i & 0x0F) << 10)];
                }
            }
            break;
    }
}

void mapper_8k_w(int address, int data)
{
  int i;

  /* cartridge ROM page (8k) index */
  uint8 page = data % (cart.pages << 1);
  
  /* Save frame control register data */
  cart.fcr[address] = data;

  switch (address & 3)
  {
    case 0: /* cartridge ROM bank (16k) at $8000-$9FFF */
    {
      for(i = 0x20; i <= 0x27; i++)
      {
        cpu_readmap[i] = &cart.rom[(page << 13) | ((i & 0x07) << 10)];
      }
      break;
    }
    
    case 1: /* cartridge ROM bank (16k) at $A000-$BFFF */
    {
      for(i = 0x28; i <= 0x2F; i++)
      {
        cpu_readmap[i] = &cart.rom[(page << 13) | ((i & 0x07) << 10)];
      }
      break;
    }
    
    case 2: /* cartridge ROM bank (16k) at $4000-$5FFF */
    {
      for(i = 0x10; i <= 0x17; i++)
      {
        cpu_readmap[i] = &cart.rom[(page << 13) | ((i & 0x07) << 10)];
      }
      break;
    }
    
    case 3: /* cartridge ROM bank (16k) at $6000-$7FFF */
    {
      for(i = 0x18; i <= 0x1F; i++)
      {
        cpu_readmap[i] = &cart.rom[(page << 13) | ((i & 0x07) << 10)];
      }
      break;
    }
  }
}
    
void mapper_16k_w(int address, int data)
{
  int i;

  /* cartridge ROM page (16k) index */
  uint8 page = data % cart.pages;
  
  /* page index increment (SEGA mapper) */
  if (cart.fcr[0] & 0x03)
  {
    page = (page + ((4 - (cart.fcr[0] & 0x03)) << 3)) % cart.pages;
  }

  /* save frame control register data */
  cart.fcr[address] = data;

  switch (address)
  {
    case 0: /* control register (SEGA mapper) */
    {
      if(data & 0x08)
      {
        /* external RAM (upper or lower 16K) mapped at $8000-$BFFF */
        int offset = (data & 0x04) ? 0x4000 : 0x0000;
        for(i = 0x20; i <= 0x2F; i++)
        {
          cpu_readmap[i] = cpu_writemap[i] = &cart.sram[offset + ((i & 0x0F) << 10)];
        }
        sms.save = 1;
      }
      else
      {
        page = cart.fcr[3] % cart.pages;
        
        /* page index increment (SEGA mapper) */
        if (cart.fcr[0] & 0x03)
        {
          page = (page + ((4 - (cart.fcr[0] & 0x03)) << 3)) % cart.pages;
        }

        /* cartridge ROM mapped at $8000-$BFFF */
        for(i = 0x20; i <= 0x2F; i++)
        {
          cpu_readmap[i] = &cart.rom[(page << 14) | ((i & 0x0F) << 10)];
          cpu_writemap[i] = dummy_write;
        }
      }

      if(data & 0x10)
      {
        /* external RAM (lower 16K) mapped at $C000-$FFFF */
        for(i = 0x30; i <= 0x3F; i++)
        {
          cpu_writemap[i] = cpu_readmap[i]  = &cart.sram[(i & 0x0F) << 10];
        }
        sms.save = 1;
      }
      else
      {
        /* internal RAM (8K mirrorred) mapped at $C000-$FFFF */
        for(i = 0x30; i <= 0x3F; i++)
        {
          cpu_writemap[i] = cpu_readmap[i] = &sms.wram[(i & 0x07) << 10];
        }
      }
      break;
    }

    case 1: /* cartridge ROM bank (16k) at $0000-$3FFF */
    {
      /* first 1k is not fixed (CODEMASTER mapper) */
      if (cart.mapper == MAPPER_CODIES)
      {
        cpu_readmap[0] = &cart.rom[(page << 14)];
      }

      for(i = 0x01; i <= 0x0F; i++)
      {
        cpu_readmap[i] = &cart.rom[(page << 14) | ((i & 0x0F) << 10)];
      }
      break;
    }

    case 2: /* cartridge ROM bank (16k) at $4000-$7FFF */
    {
      for(i = 0x10; i <= 0x1F; i++)
      {
        cpu_readmap[i] = &cart.rom[(page << 14) | ((i & 0x0F) << 10)];
      }

      /* Ernie Elf's Golf external RAM switch */
      if (cart.mapper == MAPPER_CODIES)
      {
        if (data & 0x80)
        {
          /* external RAM (8k) mapped at $A000-$BFFF */
          for(i = 0x28; i <= 0x2F; i++)
          {
            cpu_writemap[i] = cpu_readmap[i]  = &cart.sram[(i & 0x0F) << 10];
          }
          sms.save = 1;
        }
        else
        {
          /* cartridge ROM mapped at $A000-$BFFF */
          for(i = 0x28; i <= 0x2F; i++)
          {
            cpu_readmap[i] = &cart.rom[((cart.fcr[3] % cart.pages) << 14) | ((i & 0x0F) << 10)];
            cpu_writemap[i] = dummy_write;
          }
        }
      }
      break;
    }

    case 3: /* cartridge ROM bank (16k) at $8000-$BFFF */
    {
      /* check that external RAM (16k) is not mapped at $8000-$BFFF (SEGA mapper) */
      if ((cart.fcr[0] & 0x08)) break;

      /* first 8k */
      for(i = 0x20; i <= 0x27; i++)
      {
        cpu_readmap[i] = &cart.rom[(page << 14) | ((i & 0x0F) << 10)];
      }

      /* check that external RAM (8k) is not mapped at $A000-$BFFF (CODEMASTER mapper) */
      if ((cart.mapper == MAPPER_CODIES) && (cart.fcr[2] & 0x80)) break;

      /* last 8k */
      for(i = 0x28; i <= 0x2F; i++)
      {
        cpu_readmap[i] = &cart.rom[(page << 14) | ((i & 0x0F) << 10)];
      }
      break;
    }
  }
}


int sms_irq_callback(int param)
{
    return 0xFF;
}



