// Emacs style mode select       -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: wadgen.c 1230 2013-03-04 06:18:10Z svkaiser $
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// $Author: svkaiser $
// $Revision: 1230 $
// $Date: 2013-03-03 22:18:10 -0800 (Sun, 03 Mar 2013) $
//
// DESCRIPTION: Global stuff and main application functions
//
//-----------------------------------------------------------------------------
#ifdef RCSID
static const char rcsid[] =
    "$Id: wadgen.c 1230 2013-03-04 06:18:10Z svkaiser $";
#endif

#include "wadgen.h"
#include "files.h"
#include "rom.h"
#include "wad.h"
#include "level.h"
#include "sndfont.h"

#include <stdarg.h>

#define MAX_ARGS 256
char *ArgBuffer[MAX_ARGS + 1];
int myargc = 0;
char **myargv;

#ifdef _WIN32
HINSTANCE hAppInst = NULL;
HWND hwnd = NULL;
HWND hwndWait = NULL;
HWND hwndLoadBar = NULL;
#endif

void WGen_ShutDownApplication(void);

#ifdef _WIN32
//**************************************************************
//**************************************************************
//      LoadingDlgProc
//**************************************************************
//**************************************************************

bool __stdcall
LoadingDlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		hwndLoadBar = GetDlgItem(hWnd, IDC_PROGRESS1);
		SendMessage(hwndLoadBar, PBM_SETRANGE, 0,
			    MAKELPARAM(0, TOTALSTEPS));
		SendMessage(hwndLoadBar, PBM_SETSTEP, (WPARAM) (int)1, 0);
		return TRUE;
	case WM_PAINT:
		return TRUE;
	case WM_DESTROY:
		return TRUE;
	case WM_CLOSE:
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		}
		break;
	}
	return FALSE;
}
#endif

//**************************************************************
//**************************************************************
//      WGen_AddLumpFile
//**************************************************************
//**************************************************************

static void WGen_AddLumpFile(const char *name)
{
	path file;
	int size;
	cache data;
	char lumpname[16];

#ifdef _WIN32
	sprintf(file, "%s/Content/%s", wgenfile.basePath, name);
#else
	// on *NIX, File_Read searches a couple of places besides current dir
	sprintf(file, "Content/%s", name);
#endif
	size = File_Read(file, &data);

	strncpy(lumpname, name, 16);
	File_StripExt(lumpname);

	Wad_AddOutputLump(lumpname, size, data);

	Mem_Free((void **)&data);
}

//**************************************************************
//**************************************************************
//      WGen_AddDigest
//**************************************************************
//**************************************************************

static md5_context_t md5_context;

void WGen_AddDigest(char *name, int lump, int size)
{
	char buf[9];

	strncpy(buf, name, 8);
	buf[8] = '\0';

	MD5_UpdateString(&md5_context, buf);
	MD5_UpdateInt32(&md5_context, lump);
	MD5_UpdateInt32(&md5_context, size);
}

//**************************************************************
//**************************************************************
//      WGen_Process
//
//      The heart of the application. Opens the N64 rom and begins
//      extracting all the sprite and gfx data from it and begins
//      converting them into the ex format (Doom64 PC).
//
//      Content packages are extracted and added to the output wad.
//
//      The main IWAD is created afterwards.
//**************************************************************
//**************************************************************

void WGen_Process(void)
{
	int i = 0;
	char name[9];
	path outFile;
    path tempPath;
	md5_digest_t digest;

	Rom_Open();

	Sound_Setup();
	Sprite_Setup();
	Gfx_Setup();
	Texture_Setup();
	Level_Setup();

	Wad_CreateOutput();

	MD5_Init(&md5_context);

	// Sprites
	Wad_AddOutputLump("S_START", 0, NULL);

	for (i = 0; i < spriteExCount; i++)
		Wad_AddOutputSprite(&exSpriteLump[i]);

	Wad_AddOutputLump("S_END", 0, NULL);

	// Sprite Palettes
	for (i = 0; i < extPalLumpCount; i++)
		Wad_AddOutputPalette(&d64PaletteLump[i]);

	WGen_AddLumpFile("PALPLAY3.ACT");

	// Textures
	Wad_AddOutputLump("T_START", 0, NULL);

	for (i = 0; d64ExTexture[i].data; i++)
		Wad_AddOutputTexture(&d64ExTexture[i]);

	Wad_AddOutputLump("T_END", 0, NULL);

	// Gfx
	Wad_AddOutputLump("G_START", 0, NULL);

	for (i = 0; gfxEx[i].data; i++)
		Wad_AddOutputGfx(&gfxEx[i]);

	// Hud Sprites
	for (i = spriteExCount; i < spriteExCount + hudSpriteExCount; i++)
		Wad_AddOutputHudSprite(&exSpriteLump[i]);

	WGen_AddLumpFile("FANCRED.PNG");
	WGen_AddLumpFile("CRSHAIRS.PNG");
	WGen_AddLumpFile("BUTTONS.PNG");
	WGen_AddLumpFile("CONFONT.PNG");
	WGen_AddLumpFile("CURSOR.PNG");

	Wad_AddOutputLump("G_END", 0, NULL);

#ifndef USE_SOUNDFONTS
	// Sounds
	Wad_AddOutputLump("DOOMSND", sn64size, (byte *) sn64);
	Wad_AddOutputLump("DOOMSEQ", sseqsize, (byte *) sseq);
#endif

	// Midi tracks
	Wad_AddOutputLump("DS_START", 0, NULL);

	for (i = 0; i < (int)sseq->nentry; i++)
		Wad_AddOutputMidi(&midis[i], i);

	Wad_AddOutputLump("DS_END", 0, NULL);

#ifndef USE_SOUNDFONTS
	// Sounds
	for (i = 0; i < sn64->nsfx; i++) {
		sprintf(name, "SFX_%03d", i);
		Wad_AddOutputLump(name, sfx[i].wavsize, wavtabledata[i]);
	}
#endif

	// Maps
	for (i = 0; i < MAXLEVELWADS; i++) {
		sprintf(name, "MAP%02d", i + 1);
		Wad_AddOutputLump(name, levelSize[i], levelData[i]);
	}

	// Demo lumps
	//WGen_AddLumpFile("DEMO1LMP.LMP");
	//WGen_AddLumpFile("DEMO2LMP.LMP");
	//WGen_AddLumpFile("DEMO3LMP.LMP");

	WGen_AddLumpFile("MAPINFO.TXT");
	WGen_AddLumpFile("ANIMDEFS.TXT");
	WGen_AddLumpFile("SKYDEFS.TXT");

	MD5_Final(digest, &md5_context);

	Wad_AddOutputLump("CHECKSUM", sizeof(md5_digest_t), digest);

	// End of wad marker :)
	Wad_AddOutputLump("ENDOFWAD", 0, NULL);

	WGen_UpdateProgress("Writing IWAD File...");
    // MP2E: Copy filePath, sans the file itself into tempPath
    strncpy(tempPath, wgenfile.filePath,
            ( strlen(wgenfile.filePath)-strlen(wgenfile.fileName)-5 ) );
	sprintf(outFile, "%s/doom64.wad", tempPath);
	Wad_WriteOutput(outFile);

	// Done
#ifdef _WIN32
	EndDialog(hwndWait, FALSE);
#endif
	WGen_Printf("Sucessfully created %s", outFile);

	// Write out the soundfont file
#ifdef USE_SOUNDFONTS
	WGen_UpdateProgress("Writing Soundfont File...");
	SF_WriteSoundFont();
#endif

#ifdef _DEBUG
	{
		FILE *md5info;
		path tbuff;
		int j = 0;

		do {
			sprintf(tbuff, "md5info%02d.txt", j++);
		} while (File_Poke(tbuff));

		md5info = fopen(tbuff, "w");

		fprintf(md5info, "static const md5_digest_t <rename me> =\n");
		fprintf(md5info, "{ ");

		for (i = 0; i < 16; i++) {
			fprintf(md5info, "0x%02x", digest[i]);
			if (i < 15)
				fprintf(md5info, ",");
			else
				fprintf(md5info, " ");
		}

		fprintf(md5info, "};\n");
		fclose(md5info);
	}
#endif
}

#ifdef _WIN32
//**************************************************************
//**************************************************************
//      WinMain
//
//      Used some code from Doom's I_Main to setup arguments
//      so the base path of the executable can easily be obtained
//
//      No main dialog window is actually used for this application
//**************************************************************
//**************************************************************

int __stdcall
WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine,
	int nCmdShow)
{
	char *p;
	char *start;
	char **arg;

	hAppInst = hInstance;

	arg = ArgBuffer;
	p = lpCmdLine;
	myargc = 1;
	*(arg++) = "WadGen";

	while (*p && (myargc < MAX_ARGS)) {
		while (*p && isspace(*p))
			p++;
		if (!*p)
			break;
		start = p;
		while (*p && !isspace(*p))
			p++;
		*arg = (char *)Mem_Alloc(p - start + 1);
		(*arg)[p - start] = 0;
		strncpy(*arg, start, p - start);
		arg++;
		myargc++;
	}
	*arg = NULL;
	myargv = ArgBuffer;

	InitCommonControls();

	// Start looking for the rom..
	if (File_Dialog(&wgenfile, FILEDLGTYPE, "Select Doom64 Rom..", hwnd)) {
		hwndWait =
		    CreateDialog(hAppInst, MAKEINTRESOURCE(IDD_DIALOG1), NULL,
				 (DLGPROC) LoadingDlgProc);
		WGen_Process();
		WGen_ShutDownApplication();
	}

	return 0;
}
#else
int main(int argc, char *argv[])
{
	myargc = argc;
	myargv = argv;
	char *p;
	char *start;
	char **arg;

	if (argc < 2) {
		WGen_Printf
		    ("Provide the location of the Doom64 Rom as a command line parameter.");
		return 0;
	}
	File_Prepare(&wgenfile, argv[1]);
	WGen_Process();
	WGen_ShutDownApplication();

	return 0;
}
#endif

//**************************************************************
//**************************************************************
//      WGen_Printf
//**************************************************************
//**************************************************************

void WGen_Printf(char *s, ...)
{
	char msg[1024];
	va_list v;

	va_start(v, s);
	vsprintf(msg, s, v);
	va_end(v);

#ifdef _WIN32
	MessageBox(NULL, msg, "Info", MB_OK | MB_ICONINFORMATION);
#else
	printf("%s\n", msg);
#endif
}

//**************************************************************
//**************************************************************
//      WGen_Complain
//**************************************************************
//**************************************************************

void WGen_Complain(char *fmt, ...)
{
	va_list va;
	char buff[1024];

#ifdef _WIN32
	if (hwndWait)
		EndDialog(hwndWait, FALSE);
#endif

	va_start(va, fmt);
	vsprintf(buff, fmt, va);
	va_end(va);
	WGen_Printf("ERROR: %s", buff);
	exit(1);
}

//**************************************************************
//**************************************************************
//      WGen_UpdateProgress
//**************************************************************
//**************************************************************

void WGen_UpdateProgress(char *fmt, ...)
{
	va_list va;
	char buff[1024];

	va_start(va, fmt);
	vsprintf(buff, fmt, va);
	va_end(va);

#ifdef _WIN32
	SendMessage(hwndLoadBar, PBM_STEPIT, 0, 0);
	SetDlgItemText(hwndWait, IDC_PROGRESSTEXT, buff);
	UpdateWindow(hwndWait);
#else
	WGen_Printf("%s\n", buff);
#endif
}

//**************************************************************
//**************************************************************
//      WGen_ShutDownApplication
//**************************************************************
//**************************************************************

void WGen_ShutDownApplication(void)
{
	Rom_Close();
}

//**************************************************************
//**************************************************************
//      WGen_ConvertN64Pal
//
//      N64 stores its RGB values a bit differently than that of the
//      PC/Windows format. This algorithm will convert any 16 bit
//      N64 palette (always 512 bytes) into a 768 byte PC/Win32 palette.
//
//      palette:        Palette struct to be used to convert the N64 pal to.
//      data:           Data that contains the pointer to the N64 pal data
//                              Its important to always use words when reading
//                              the N64 palette format for simplicity sakes.
//      indexes:        How many indexes are we going to convert? Usually
//                              16 or 256
//**************************************************************
//**************************************************************
#if 1
// new algorithm contributed by Daniel Swanson (DaniJ)
void WGen_ConvertN64Pal(dPalette_t * palette, word * data, int indexes)
{
	int i;
	short val;
	cache p = (cache) data;

	for (i = 0; i < indexes; i++) {
		// Read the next packed short from the input buffer.
		val = *(short *)p;
		p += 2;
		val = WGen_Swap16(val);

		// Unpack and expand to 8bpp, then flip from BGR to RGB.
		palette[i].b = (val & 0x003E) << 2;
		palette[i].g = (val & 0x07C0) >> 3;
		palette[i].r = (val & 0xF800) >> 8;
		palette[i].a = 0xff;	// Alpha is always 255..
	}
}
#else
// old algorithm
void WGen_ConvertN64Pal(dPalette_t * palette, word * data, int indexes)
{
	short g, r, b, start;
	int i, j;

	g = r = b = 0;

	for (i = 0; i < indexes; i++) {
		g = r = b = start = 0;
		for (j = 0; j < data[i]; j++) {
			if (j & 256) {
				g += 32;
				if (g >= 256) {
					r += 8;
					g = start;
				}
				if (r >= 256) {
					b += 8;
					r = 0;
				}
				if (b >= 256) {
					start += 8;
					b = 0;
				}
			}
		}
		if (r >= 32)
			r = r + (r / 32);
		if (g >= 32)
			g = g + (g / 32);
		if (b >= 32)
			b = b + (b / 32);

		palette[i].r = (byte) r;
		palette[i].g = (byte) g;
		palette[i].b = (byte) b;
		palette[i].a = 0xff;	// Alpha is always 255..
	}
}
#endif

//**************************************************************
//**************************************************************
//      WGen_Swap16
//
//      Needed mostly for sprites
//**************************************************************
//**************************************************************

int WGen_Swap16(int x)
{
	return (((word) (x & 0xff) << 8) | ((word) x >> 8));
}

//**************************************************************
//**************************************************************
//      WGen_Swap32
//**************************************************************
//**************************************************************

uint WGen_Swap32(unsigned int x)
{
	return (((x & 0xff) << 24) |
		(((x >> 8) & 0xff) << 16) |
		(((x >> 16) & 0xff) << 8) | ((x >> 24) & 0xff)
	    );
}
