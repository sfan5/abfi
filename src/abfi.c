/*
ABFI ; Advanced Brainfuck Interpreter
Copyright (C) 2012-13  sfan5 <sfan5@live.de>

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#define NUM_CELLS 65535

int exec_bf(char* cmds, size_t length, FILE* in, FILE* out, int caps[2]);

int main(int argc, char** argv)
{
    int i;
    int caps[3];
    caps[0] = 0; // Debug
    caps[1] = 0; // string Extension
    caps[2] = 0; // pstderr Extension
    char *a, *p, *filename, *infile, *outfile, *extensions;
    infile = outfile = filename = NULL;
    extensions = "";
    for(i = 1;i < argc;i++)
    {
        a = argv[i];
        
        p = strstr(a, "-i"); // -i [stdin file]
        if(p != NULL)
        {
            infile = (char*) p+2;
            continue;
        }
        
        p = strstr(a, "-o"); // -o [stdout file]
        if(p != NULL)
        {
            outfile = (char*) p+2;
            continue;
        }
        
        p = strstr(a, "-e"); // -e [extensions seperated by comma]
        if(p != NULL)
        {
            extensions = (char*) p+2;
            continue;
        }
        
        p = strstr(a, "-d"); // -d
        if(p != NULL)
        {
            caps[0] = 1;
            continue;
        }
        
        filename = a; // Filename
        break;
    }
    if(!filename)
    {
        printf("%s\n", PACKAGE_STRING);
        printf("Usage: %s [switches] <filename>\n",argv[0]);
        printf(" -i [filename]  Input (defaults to stdin)\n");
        printf(" -o [filename]  Output (defaults to stdout)\n");
        printf(" -e [list seperated by ,]  activated Extensions\n");
        printf(" -d Debugging Mode\n");
        printf("Supported Extensions:\n");
#ifdef ENABLE_EXT_STRING
        printf(" string   Use \"string\" to write the null-terminated chars into the cells beginning at current cell index, leaves pointer at end of string.\n");
        printf("          Use 'string' to write the null-terminated chars into the cells beginning at current cell index, leaves pointer at start of string.\n");
#endif // ENABLE_EXT_STRING
#ifdef ENABLE_EXT_PSTDERR
        printf(" pstderr  Use ! to print the current char to stderr.\n");
#endif // ENABLE_EXT_PSTDERR
        printf(" *        Will enable all available extensions.\n");
        printf("");
        printf("%s version %s, Copyright (C) 2013 sfan5 \n\
%s comes with ABSOLUTELY NO WARRANTY.  This is free software, and you are welcome \n\
to redistribute it under certain conditions.\n", PACKAGE_NAME, PACKAGE_VERSION, PACKAGE_NAME);
        return 0;
    }
    else
    {
        FILE* f = fopen(filename, "r");
        if(!f)
        {
            fprintf(stderr, "Error opening File");
            return 1;
        }
        long fsize;
        char* buffer;
        size_t r;
        fseek(f, 0, SEEK_END);   // may not work
        fsize = ftell(f);
        fseek(f, 0, SEEK_SET);
        buffer = (char*) malloc(sizeof(char) * fsize);
        if(buffer == NULL)
        {
            fprintf(stderr, "Could not allocate Memory for File");
            return 1;
        }
        r = fread(buffer, 1, fsize, f);
        if(r != fsize)
        {
            fprintf(stderr, "Reading Error");
            return 1;
        }
        fclose(f);
        
        FILE *in, *out;
        if(infile) in = fopen(infile, "rb"); else in = stdin;
        if(outfile) out = fopen(outfile, "wb"); else out = stdout;
        
        int ret;
        char* pch;
        pch = strtok(extensions, ",");
        while(pch != NULL)
        {
#ifdef ENABLE_EXT_STRING
            if(!strcmp(pch, "string"))
            {
                caps[1] = 1;
            }
#endif // ENABLE_EXT_STRING
#ifdef ENABLE_EXT_PSTDERR
            if(!strcmp(pch, "pstderr"))
            {
                caps[2] = 1;
            }
#endif // ENABLE_EXT_PSTDERR
            if(!strcmp(pch, "*"))
            {
#ifdef ENABLE_EXT_STRING
                caps[1] = 1;
#endif // ENABLE_EXT_STRING
#ifdef ENABLE_EXT_PSTDERR
                caps[2] = 1;
#endif // ENABLE_EXT_PSTDERR
            }
            pch = strtok(NULL, ",");
        }
        ret = exec_bf(buffer, fsize, in, out, caps);
        
        free(buffer);
        return ret;
    }
}

int exec_bf(char* cmds, size_t length, FILE* in, FILE* out, int caps[2])
{
#define DBGMSG if(caps[0]) printf
    char* ptr;
    int t;
    ptr = malloc(sizeof(char) * NUM_CELLS);
    if(ptr == NULL)
    {
        fprintf(stderr, "Could not allocate Memory for Cells");
        return 1;
    }
    for(t = 0;t <= NUM_CELLS;t++)
    {
        ptr[t] = 0;
    }
    int i = 0, r;
    int counter = 0;
    int cellbase = (int) ptr;
#define CELLOFFSET ptr-cellbase
    while(i < length - 1)
    {
            switch(cmds[i])
            {
                case '>':
                    ++ptr;
                    DBGMSG("Cell Index changed to %i (inc.)\n",(int) CELLOFFSET);
                    break;
                case '<':
                    --ptr;
                    DBGMSG("Cell Index changed to %i (dec.)\n",(int) CELLOFFSET);
                    break;
                case '+':
                    ++*ptr;
                    DBGMSG("Value %i at Cell %i (inc.)\n",(int) *ptr,(int) CELLOFFSET);
                    break;
                case '-':
                    --*ptr;
                    DBGMSG("Value %i at Cell %i (dec.)\n",(int) *ptr,(int) CELLOFFSET);
                    break;
                case '.':
                    fputc(*ptr, out);
                    break;
                case ',':
                    *ptr = fgetc(in);
                    break;
                case '[':
                    if(*ptr == 0)
                    {
                        counter = 1;
                        i++;
                        while(counter > 0)
                        {
                            if(cmds[i] == '[') counter++;
                            if(cmds[i] == ']') counter--;
                            i++;
                        }
                    }
                    else
                    {
                        DBGMSG("Not executing/exiting Loop because %i in Cell %i\n",(int) *ptr,(int) CELLOFFSET);
                    }
                    break;
                case ']':
                    if(*ptr != 0)
                    {
                        counter = 1;
                        i--;
                        while(counter > 0)
                        {
                            if(cmds[i] == '[') counter--;
                            if(cmds[i] == ']') counter++ ;
                            i--;
                        }
                    }
                    else
                    {
                        DBGMSG("Exiting Loop because 0 in Cell %i\n",(int) CELLOFFSET);
                    }
                    break;
#ifdef ENABLE_EXT_STRING
                case '"':
                    if(!caps[1]) break;
                    i++;
                    r = 1;
                    while(r)
                    {
                        if(cmds[i] == '"')
                        {
                            ++ptr;
                            *ptr = 0; // 0 at End of String
                            DBGMSG("Value 0 at Cell %i\n",(int) CELLOFFSET);
                            r = 0;
                        }
                        else
                        {
                            ++ptr;
                            *ptr = cmds[i];
                            DBGMSG("Value %i at Cell %i\n",(int) cmds[i],(int) CELLOFFSET);
                        }
                        i++;
                    }
                    DBGMSG("Exiting String at Cell %i\n",(int) CELLOFFSET);
                    i--; // Do not remove
                    break;
                case '\'':
                    if(!caps[1]) break;
                    counter = (int) ptr; // counter is currently unused, so we can use it
                    i++;
                    r = 1;
                    while(r)
                    {
                        if(cmds[i] == '\'')
                        {
                            ++ptr;
                            *ptr = 0; // 0 at End of String
                            DBGMSG("Value 0 at Cell %i\n",(int) CELLOFFSET);
                            r = 0;
                        }
                        else
                        {
                            ++ptr;
                            *ptr = cmds[i];
                            DBGMSG("Value %i at Cell %i\n",(int) cmds[i],(int) CELLOFFSET);
                        }
                        i++;
                    }
                    ptr = (char*) counter;
                    counter = 0;
                    DBGMSG("Exiting String at Cell %i\n",(int) CELLOFFSET);
                    i--; // Do not remove
                    break;
#endif // ENABLE_EXT_STRING
#ifdef ENABLE_EXT_PSTDERR
                case '!':
                    if(!caps[2]) break;
                    fputc(*ptr, stderr);
                    break;
#endif // ENABLE_EXT_PSTDERR
                default:
                    break;
            }
            i++;
    }
    return 0;
}
