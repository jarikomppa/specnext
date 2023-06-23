// Proof of concept AY midi player for zx spectrum next
// by Jari Komppa 2023
// Bits and pieces stolen from spectrum next z88dk examples
// Overall I'd say it's public domain or close enough.

#pragma output REGISTER_SP = 0xFF58
#pragma output CLIB_MALLOC_HEAP_SIZE = -0xFBFA

#include <z80.h>
#include <string.h>
#include <intrinsic.h>
#include <im2.h>
#include <arch/zxn.h>
#include <stdio.h>
#include <arch/zxn/esxdos.h>

// Generated with freqcalc.cpp
const unsigned char note_coarse[128] = {15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 14, 14, 13, 12, 11, 11, 10, 9, 9, 8, 8, 7, 7, 7, 6, 6, 5, 5, 5, 4, 4, 4, 4, 3, 3, 3, 3, 3, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
const unsigned char note_fine[128] = {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 192, 222, 8, 63, 128, 205, 35, 131, 236, 93, 215, 88, 224, 111, 4, 159, 64, 230, 145, 65, 246, 174, 107, 44, 240, 183, 130, 79, 32, 243, 200, 160, 123, 87, 53, 22, 248, 219, 193, 167, 144, 121, 100, 80, 61, 43, 26, 11, 252, 237, 224, 211, 200, 188, 178, 168, 158, 149, 141, 133, 126, 118, 112, 105, 100, 94, 89, 84, 79, 74, 70, 66, 63, 59, 56, 52, 50, 47, 44, 42, 39, 37, 35, 33, 31, 29, 28, 26, 25, 23, 22, 21, 19, 18, 17, 16, 15, 14, 14, 13, 12, 11, 11, 10, 9, 9, 8};

#pragma output CRT_ORG_CODE = 0x8184
#pragma output CLIB_MALLOC_HEAP_SIZE = 0
#pragma printf = "%d %x %c %s"

#define printCls() printf("%c", 12)
#define printAt(col, row, str) printf("\x16%c%c%s", (col), (row), (str))

#define setay(reg, val) { IO_FFFD = reg; IO_BFFD = val; }
#define setchip(val) { IO_FFFD = 0xfc | val; }

void playay(unsigned char ch, unsigned char note, unsigned char vol)
{
    unsigned char chip = 1;
    while (ch >= 3) { chip++; ch -= 3; }
    setchip(chip);
    unsigned char ch2 = ch + ch;
    setay(ch2, note_fine[note]);
    setay(ch2 + 1, note_coarse[note]);
    setay(ch + 8, vol & 15);
    
}

unsigned char chnote[9] = {255,255,255,255,255,255,255,255,255};
unsigned char chvol[9] = {0,0,0,0,0,0,0,0,0};
unsigned char nextch = 0;

void playnote(unsigned char note, unsigned char vol)
{
    // Channel allocation strategy = round robin, always overwrite oldest note.
    // Could do something smarter here and check if there are empty channels (i.e, note offed ones)
    playay(nextch, note, vol);
    chnote[nextch] = note;
    chvol[nextch] = vol;
    nextch++;
    if (nextch == 9)
        nextch = 0;
}

void stopnote(unsigned char note)
{
    // Instead of brutally silencing a note we could do a volume ramp down.

    for (char i = 0; i < 9; i++)
        if (chnote[i] == note)
        {
            playay(i, 0, 0);
            chnote[i] = 0xff;
            chvol[i] = 0;
            return;
        }
}

void aftertouchnote(unsigned char note, unsigned char vol)
{
    for (char i = 0; i < 9; i++)
        if (chnote[i] == note)
        {
            playay(i, note, vol);
            chvol[i] = vol;
            return;
        }    
}

volatile unsigned long int delay = 0;

IM2_DEFINE_ISR(isr)
{
    if (delay)
    {
        IO_FE = 1;
        delay--;
    }
    else
        IO_FE = 0;
}


#define TABLE_HIGH_BYTE        ((unsigned int)0xfc)
#define JUMP_POINT_HIGH_BYTE   ((unsigned int)0xfb)

#define UI_256                 ((unsigned int)256)

#define TABLE_ADDR             ((void*)(TABLE_HIGH_BYTE*UI_256))
#define JUMP_POINT             ((unsigned char*)( (unsigned int)(JUMP_POINT_HIGH_BYTE*UI_256) + JUMP_POINT_HIGH_BYTE ))


void init_isr()
{
  intrinsic_di();
  memset( TABLE_ADDR, JUMP_POINT_HIGH_BYTE, 257 );

  z80_bpoke( JUMP_POINT,   195 );
  z80_wpoke( JUMP_POINT+1, (unsigned int)isr );

  im2_init( TABLE_ADDR );

  intrinsic_ei();
}

void init_ay()
{    
    IO_NEXTREG_REG = REG_PERIPHERAL_3;
    IO_NEXTREG_DAT |= 2; // enable turbosound
    
    for (char i = 0; i < 9; i++)
        playay(i,0,0);
    
    setchip(1);
    setay(7, 0x38); // enable voices 1-3, disable noise 1-3
    setchip(2);
    setay(7, 0x38); // enable voices 1-3, disable noise 1-3
    setchip(3);
    setay(7, 0x38); // enable voices 1-3, disable noise 1-3
}

static unsigned char readbyte(unsigned char f)
{
    unsigned char d = 0;
    esx_f_read(f, &d, 1);
    return d;
}

// Read variable-length integer from stream
static unsigned long int readvar(unsigned char f)
{
	unsigned long int d = 0;
	unsigned long int v = 0;
	do
	{
		v = readbyte(f);
		d = (d << 7) + (v & 0x7f);
	} 
	while ((v & 0x80) != 0);
	return d;
}

#define SWAPDWORD(a) ((((a) & 0x000000ffl) << 24) | \
                      (((a) & 0x0000ff00l) << 8 ) | \
                      (((a) & 0x00ff0000l) >> 8 ) | \
                      (((a) & 0xff000000l) >> 24))

#define SWAPWORD(a) ((((a) & 0xff00) >> 8) | (((a) & 0xff) << 8))

#define FOURCC(a,b,c,d) (((unsigned long)a << 24) |((unsigned long)b << 16) | ((unsigned long)c << 8) | ((unsigned long)d << 0))

// Read doubleword from stream
static long int readdword(unsigned char f)
{
	long int d = 0;
	esx_f_read(f, &d, 4);
	d = SWAPDWORD(d);
	return d;
}

// Read word from stream
static int readword(unsigned char f)
{
	unsigned short int d = 0;
	esx_f_read(f, &d, 2);
	d = SWAPWORD(d);
	return d;
}

// Load chunk header
static long int loadchunkheader(unsigned char f, long int* length)
{
	long int id = 0;
	id = readdword(f);
	*length = readdword(f);
	return id;
}

void main(void)
{   
    const unsigned char *fn = "arabesqu.mid";
        
    printCls();
    printAt(8, 2, fn);
    printAt(1,3,">");
 
    init_ay();  
    init_isr();
 
    unsigned char f = esx_f_open(fn, ESX_MODE_OPEN_EXIST | ESX_MODE_R);
	if (f == 0xff) printf("file open failed\n");
	long int len;
	long int id = loadchunkheader(f, &len);
	if (id != FOURCC('M','T','h','d'))
	{
		printf("Bad header id\n");
		esx_f_close(f);
		while (1) {};
	}
	if (len < 6)
	{
		printf("Bad header block length\n");
		esx_f_close(f);
		while (1) {};
	}
	int format = readword(f);
	//printf("format %d\n", format);
	if (format != 1 && format != 0)
	{
		printf("midi loader: Unsupported format\n");
		esx_f_close(f);
		while (1) {};
	}
	int tracks = readword(f);
	//printf("tracks %d\n", tracks);
	unsigned int ppqn = readword(f);
	//printf("ppqn %d\n", ppqn); // pulses (clocks) per quater note
	if (ppqn <= 0)
	{
		printf("midi loader: negative ppqn formats not supported\n");
		esx_f_close(f);
		while (1) {};
	}
	if (len > 6)
	{
		while (len > 6)
		{
			readbyte(f);
			len--;
		}
	}

	long int uspertick = 500000 / ppqn;
	while (tracks)
	{
		id = loadchunkheader(f, &len);
		if (id != FOURCC('M','T','r','k'))
		{
			printf("Midi loader: Unknown chunk\n");
			esx_f_close(f);
			while (1) {};
		}
		//printf("New track, length %d\n", len);
		int trackend = 0;
		int command = 0;
		int overtime = 0;
		while (!trackend)
		{
    		unsigned long int dtime = readvar(f);
    		dtime = dtime * uspertick / 1000;
    		dtime += overtime;
			overtime = dtime % 20;
			delay = dtime / 20;
			if (delay > 2)
		    {
		        for (char i = 0; i < 9; i++)
		        {
		            printAt(1,13+i,"> ");
		            printf("%2d %3d", chvol[i], chnote[i]);
		        }
		    }
			while (delay) {}; // isr delay
			
			int data1 = readbyte(f);
			if (data1 == 0xff)
			{
				data1 = readbyte(f); // sub-command
				len = readvar(f);
				switch (data1)
				{
				case 1:
				case 2:
				case 3:
				case 4:
				case 5:
				case 6:
				case 7:
				case 8:
				case 9:
				    //printf("Text(%d %d) \"", data1, len);
					switch (data1)
					{
					case 1:
						//printf("Text:\"");
						break;
					case 2:
						//printf("Copyright:\"");
						break;
					case 3:
						//printf("Track name:\"");
						break;
					case 4:
						//printf("Instrument:\"");
						break;
					case 5:
						//printf("Lyric:\"");
						break;
					case 6:
						//printf("Marker:\"");
						break;
					case 7:
						//printf("Cue point:\"");
						break;
					case 8:
						//printf("Patch name:\"");
						break;
					case 9:
						//printf("Port name:\"");
						break;
					}
					int l = len; // compiler bug: without a local copy of the variable the loop runs forever
					while (l > 0)
					{
						/*int c = */readbyte(f);
						//printf("%c %d ", c, l);
						l--;
					}
					
					//printf("\"\n");
					break;
				case 0x2f:
				{
					trackend = 1;
					//printf("Track end\n");
				}
				break;
				case 0x58: // time signature
				{
					/*int nn = */readbyte(f);
					/*int dd = */readbyte(f);
					/*int cc = */readbyte(f);
					/*int bb = */readbyte(f);
					//printf("Time sig: %d:%d, metronome:%d, quarter:%d\n", nn, dd, cc, bb);
				}
				break;
				case 0x59: // key signature
				{
					/*int sf = */readbyte(f);
					/*int mi = */readbyte(f);
					//printf("Key sig: %d %s, %s\n", abs(sf), sf == 0 ? "c" : (sf < 0 ? "flat" : "sharp"), mi ? "minor" : "major");
				}
				break;
				case 0x51: // tempo
				{
					long int t = 0;
					t = (long int)readbyte(f) << 16;
					t |= (long int)readbyte(f) << 8;
					t |= readbyte(f);
					//printf("Tempo: quarter is %dus (%3.3fs) long - BPM = %3.3f\n", t, t / 1000000.0f, 60000000.0f / t);
					uspertick = t / ppqn;
				}
				break;
				case 0x21: // obsolete: midi port
				{
					/*int pp = */readbyte(f);
					//printf("[obsolete] midi port: %d\n", pp);
				}
				break;
				case 0x20: // obsolete: midi channel
				{
					/*int cc = */readbyte(f);
					//printf("[obsolete] midi channel: %d\n", cc);
				}
				break;
				case 0x54: // SMPTE offset
				{
					/*int hr = */readbyte(f);
					/*int mn = */readbyte(f);
					/*int se = */readbyte(f);
					/*int fr = */readbyte(f);
					/*int ff = */readbyte(f);
					//printf("SMPTE Offset: %dh %dm %ds %dfr %dff\n", hr, mn, se, fr, ff);
				}
				break;
				case 0x7f: // Proprietary event
				{
					//printf("Proprietary event ");
					while (len)
					{
						/*int d = */readbyte(f);
						//printf("%02X ", d);
						len--;
					}
				}
				break;
				default:
					//printf("meta command %02x %d\n", data1, len);
					while (len)
					{
						readbyte(f);
						len--;
					}
				}
			}
			else
			{
				if (data1 & 0x80) // new command?
				{
					command = data1;
					data1 = readbyte(f);
				}
				int data2 = 0;

				switch (command & 0xf0)
				{
				case 0x80: // note off
				{
					data2 = readbyte(f);
					stopnote(data1);
					//printf("Note off: channel %d, Oct %d Note %s Velocity %d\n", command & 0xf, (data1 / 12) - 1, note[data1 % 12], data2);
				}
				break;
				case 0x90: // note on
				{				    
				    stopnote(data1);
					data2 = readbyte(f);
					// the velocity should probably be calculated data2 >> 3, but that yields so quiet midis that we'll cheat a bit
					unsigned char vel = data2 >> 2;
					if (vel > 15) vel = 15;
					if (vel > 0) 
					    playnote(data1, vel);
					//printf("Note on: channel %d, Oct %d Note %s Velocity %d\n", command & 0xf, (data1 / 12) - 1, note[data1 % 12], data2);
				}
				break;
				case 0xa0: // Note aftertouch
				{
					data2 = readbyte(f);
					unsigned char vel = data2 >> 2;
					if (vel > 15) vel = 15;					
					aftertouchnote(data1, vel);
					//printf("Aftertouch: channel %d, Oct %d, Note %s Aftertouch %d\n", command & 0xf, (data1 / 12) - 1, note[data1 % 12], data2);
				}
				break;
				case 0xb0: // Controller
				{
					data2 = readbyte(f);
					//printf("Controller: channel %d, Controller %s Value %d\n", command & 0xf, controller[data1], data2);
				}
				break;
				case 0xc0: // program change
				{
					//printf("Program change: channel %d, program %d\n", command & 0xf, data1);
				}
				break;
				case 0xd0: // Channel aftertouch
				{
					//printf("Channel aftertouch: channel %d, Aftertouch %d\n", command & 0xf, data1);
				}
				break;
				case 0xe0: // Pitch bend
				{
					data2 = readbyte(f);
					//printf("Pitchbend: channel %d, Pitch %d\n", command & 0xf, data1 + (data2 << 7));
				}
				break;
				case 0xf0: // general / immediate
				{
					switch (command)
					{
					case 0xf0: // SysEx
					{
						//printf("SysEx ");
						while (data1 != 0xf7)
						{
							//printf("%02X ", data1);
							data1 = readbyte(f);
						}
						//printf("\n");
						// universal sysexes of note:
						// f0 (05) 7e 7f 09 01 f7 = "general midi enable"
						// f0 (05) 7e 7f 09 00 f7 = "general midi disable"
						// f0 (07) 7f 7f 04 01 ll mm f7 = "master volume", ll mm = 14bit value
						// spec doesn't say that the length byte should be there,
						// but it appears to be (the ones in brackets)
					}
					break;
					case 0xf1: // MTC quater frame
					{
						/*int dd = */readbyte(f);
						//printf("MTC quater frame %d\n", dd);
					}
					break;
					case 0xf2: // Song position pointer
					{
						/*int data1 = */readbyte(f);
						/*int data2 = */readbyte(f);
						//printf("Song position pointer %d\n", data1 + (data2 << 7));
					}
					break;
					case 0xf3: // Song select
					{
						/*int song = */readbyte(f);
						//printf("Song select %d\n", song);
					}
					break;
					case 0xf6: // Tuning request
						//printf("Tuning request\n");
						break;
					case 0xf8: // MIDI clock
						//printf("MIDI clock\n");
						break;
					case 0xf9: // MIDI Tick
						//printf("MIDI Tick\n");
						break;
					case 0xfa: // MIDI start
						//printf("MIDI start\n");
						break;
					case 0xfc:
						//printf("MIDI stop\n");
						break;
					case 0xfb:
						//printf("MIDI continue\n");
						break;
					case 0xfe:
						//printf("Active sense\n");
						break;
					case 0xff:
						//printf("Reset\n");
						break;

					default:
					{
						printf("Midi loader: Unknown: command 0x%02x, data 0x%02x\n", command, data1);
						while (1) {};
					}
					break;
					}
				}
				break;
				default:
				{
					printf("Midi loader: Unknown: command 0x%02x, data 0x%02x\n", command, data1);
					while (1) {};
				}
				break;
				}
				if ((command & 0xf0) != 0xf0)
				{
				    printAt(1,11,">");
				    printf("%02x %02x %02x   ", command, data1, data2);
				}
			}
		}

		tracks--;
	}
	esx_f_close(f);
}
