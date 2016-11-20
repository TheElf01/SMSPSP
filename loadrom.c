/*
    loadrom.c --
    File loading and management.
*/

#include "shared.h"

char game_name[PATH_MAX];
int juegopal;
int vcodies;
int paddlec;
extern int contador;
int spidermanhack;


typedef struct {
    uint32 crc;
    int mapper;
    int display;
    int territory;
    char *name;
} rominfo_t;

rominfo_t game_list[] = {
    {0x29822980, MAPPER_CODIES, DISPLAY_PAL, TERRITORY_EXPORT, "Cosmic Spacehead"},
    {0xB9664AE1, MAPPER_CODIES, DISPLAY_PAL, TERRITORY_EXPORT, "Fantastic Dizzy"},
    {0xA577CE46, MAPPER_CODIES, DISPLAY_PAL, TERRITORY_EXPORT, "Micro Machines"},
    {0x8813514B, MAPPER_CODIES, DISPLAY_PAL, TERRITORY_EXPORT, "Excellent Dizzy (Proto)"},
    {0xAA140C9C, MAPPER_CODIES, DISPLAY_PAL, TERRITORY_EXPORT, "Excellent Dizzy (Proto - GG)"},   
    {0xc8718d40, MAPPER_SEGA, DISPLAY_PAL, TERRITORY_EXPORT, "Aladdin"}, 
    {0x72420f38, MAPPER_SEGA, DISPLAY_PAL, TERRITORY_EXPORT, "Addams Family"},
    {0x414f1db3, MAPPER_SEGA, DISPLAY_PAL, TERRITORY_EXPORT, "Back to the Future Part III"},  
    {0x6c1433f9, MAPPER_SEGA, DISPLAY_PAL, TERRITORY_EXPORT, "Desert Strike"}, 
    {0x0047b615, MAPPER_SEGA, DISPLAY_PAL, TERRITORY_EXPORT, "Predator 2"},   
    {0xf42e145c, MAPPER_SEGA, DISPLAY_PAL, TERRITORY_EXPORT, "Quest for the Shaven Yak "}, 
    {0xc9dbf936, MAPPER_SEGA, DISPLAY_PAL, TERRITORY_EXPORT, "Home Alone"},   
    {0x5c0b1f0f, MAPPER_NONE, DISPLAY_PAL, TERRITORY_EXPORT, "Pworld"},    
    {0x85cfc9c9, MAPPER_SEGA, DISPLAY_PAL, TERRITORY_EXPORT, "Chase HQ"},
    {0xd4b8f66d, MAPPER_SEGA, DISPLAY_PAL, TERRITORY_EXPORT, "Star Wars"},
    {0x13ac9023, MAPPER_SEGA, DISPLAY_PAL, TERRITORY_EXPORT, "Cool Spot"},      
    {0xebe45388, MAPPER_SEGA, DISPLAY_PAL, TERRITORY_EXPORT, "Spiderman sinister"},  
    {0x77efe84a, MAPPER_KOREA_MSX, DISPLAY_NTSC, TERRITORY_EXPORT, "Cyborg Z"},
    {0x06965ed9, MAPPER_KOREA_MSX, DISPLAY_NTSC, TERRITORY_EXPORT, "F1 Spirits"}, 
    {0x0a77fa5e, MAPPER_KOREA_MSX, DISPLAY_NTSC, TERRITORY_EXPORT, "Nemesis 2"},
    {0x83f0eede, MAPPER_KOREA_MSX, DISPLAY_NTSC, TERRITORY_EXPORT, "Street Master"},
    {0xA05258F5, MAPPER_KOREA_MSX, DISPLAY_NTSC, TERRITORY_EXPORT, "Wonshin"},
    {0x5ac99fc4, MAPPER_KOREA_MSX, DISPLAY_NTSC, TERRITORY_EXPORT, "Penguin Adventure"},    
    {0x97D03541, MAPPER_KOREA, DISPLAY_NTSC, TERRITORY_EXPORT, "Sangokushi 3"},  
    {0x18FB98A3, MAPPER_KOREA, DISPLAY_NTSC, TERRITORY_EXPORT, "Jang Pung 3"},
    {0x89B79E77, MAPPER_KOREA, DISPLAY_NTSC, TERRITORY_EXPORT, "Dallyeora Pigu-Wang"},
    {0x17AB6883, MAPPER_KOREA, DISPLAY_NTSC, TERRITORY_EXPORT, "FA Tetris"},
    {0x61E8806F, MAPPER_KOREA, DISPLAY_NTSC, TERRITORY_EXPORT, "Flash Point"},
    {0x929222c4, MAPPER_KOREA, DISPLAY_NTSC, TERRITORY_EXPORT, "Jang Pung 2"},
//    {0xdd4a661b, MAPPER_CASTLE, DISPLAY_NTSC, TERRITORY_DOMESTIC, "Terebi"},
    {0x092f29d6, MAPPER_CASTLE, DISPLAY_NTSC, TERRITORY_DOMESTIC, "Castle"},    
    {0xc671c8a6, MAPPER_KOREA, DISPLAY_NTSC, TERRITORY_EXPORT, "Super Boy 3"},
    {0xf89af3cc, MAPPER_KOREA_MSX, DISPLAY_NTSC, TERRITORY_EXPORT, "Knightmare II"},                                                       
    {-1        , -1  , -1, -1, NULL},
};

int load_rom(char *filename)
{
    int i;
    int size;

    if(cart.rom)
    {
        free(cart.rom);
        cart.rom = NULL;
    }

    if(check_zip(filename))
    {
        char name[PATH_MAX];
        cart.rom = loadFromZipByName(filename, name, &size);
        if(!cart.rom) return 0;
        strcpy(game_name, name);
    }
    else
    {
        FILE *fd = NULL;

        fd = fopen(filename, "rb");
        if(!fd) return 0;

        /* Seek to end of file, and get size */
        fseek(fd, 0, SEEK_END);
        size = ftell(fd);
        fseek(fd, 0, SEEK_SET);

        cart.rom = malloc(size);
        if(!cart.rom) return 0;
        fread(cart.rom, size, 1, fd);

        fclose(fd);
    }

    /* Don't load games smaller than 16K 
    if(size < 0x4000) return 0;*/

    /* Take care of image header, if present */
    if((size / 512) & 1)
    {
        size -= 512;
        memmove(cart.rom, cart.rom + 512, size);
    }

    cart.pages = (size / 0x4000);
    cart.crc = crc32(0L, cart.rom, size);

    /* Assign default settings (US NTSC machine) */
    cart.mapper     = MAPPER_SEGA;
    sms.display     = DISPLAY_NTSC;
    sms.territory   = TERRITORY_EXPORT;
    juegopal=0;
    vcodies=0;
    
    
    /* Look up mapper in game list */
    for(i = 0; game_list[i].name != NULL; i++)
    {
        if(cart.crc == game_list[i].crc)
        {
            juegopal=1;
            cart.mapper     = game_list[i].mapper;
           // sms.display     = game_list[i].display;
          // sms.territory   = game_list[i].territory;
        }
    }
    
    if (cart.mapper == MAPPER_CODIES)
    {
    vcodies=1;
    }

    /* Figure out game image type */
    if(stricmp(strrchr(game_name, '.'), ".gg") == 0)
        sms.console = CONSOLE_GG;
    else
        sms.console = CONSOLE_SMS;
        
    paddlec=0; 
    spidermanhack=0;
    
    if (cart.crc == 0xf9dbb533 || cart.crc == 0xa6fa42d0 || cart.crc == 0x29bc7fad || cart.crc ==  0x315917d4)    
    {paddlec=1;
    //contador=14;
    }  
    if (cart.crc == 0xebe45388 || cart.crc == 0x2d367c43  || cart.crc == 0xbc240779   || cart.crc == 0x13ac9023)
    {spidermanhack=1;
    }      

    
    system_assign_device(PORT_A, DEVICE_PAD2B);
    system_assign_device(PORT_B, DEVICE_PAD2B);
    
   
    return 1;
}

