/*
 * Copyright (c) 2008, 2009, 2010 Joseph Gaeddert
 * Copyright (c) 2008, 2009, 2010 Virginia Polytechnic
 *                                Institute & State University
 *
 * This file is part of liquid.
 *
 * liquid is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * liquid is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with liquid.  If not, see <http://www.gnu.org/licenses/>.
 */

//
// qtype.s.gen.c : automatic assembly source code generator
//
// This script generates assembly routines to help improve the
// performance of certain low-level arithmetic operations (such
// as multiplication and division) of fixed-point numbers.  It
// creates a shell assembly file which defines some global
// constants and includes the appropriate
//
// The constants defined in the .text section are:
//      qint        the number of integer bits in the fixed-
//                  point data type (including the sign)
//
//      qfrac       the number of fractional bits in the
//                  fixed-point data type
//
//      QTYPE_mul   type-specific multiplication routine,
//                  where QTYPE represents the name of the
//                  data type (e.g. 'q32')
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

void usage(void)
{
    printf("qtype.s.gen usage:\n");
    printf("  u/h   :   print this help file\n");
    printf("    a   :   target architecture: ppc, x86, x86_64, intelmac\n");
    printf("    n   :   name (e.g. q32b16)\n");
    printf("    i   :   intbits (including sign bit)\n");
    printf("    f   :   fracbits\n");
    printf("    o   :   output filename [default: standard output]\n");
}

int main(int argc, char *argv[])
{
    // target architecture specifics
    char name_mangler[8];       // name-mangling string (mainly for darwin)
    enum {  ARCH_UNKNOWN=0,
            ARCH_PPC,
            ARCH_X86,
            ARCH_X86_64,
            ARCH_INTELMAC } arch=0;
    char typename[64] = "";     // data type name
    int intbits = -1;           // number of integer bits
    unsigned int fracbits = -1; // number of fractional bits
    char outputfile[64] = "";   // output filename

    char archname[64];          // architecture name ppc|x86|x86_64
    char comment_char = '#';    // comment character

    int dopt;
    while ((dopt = getopt(argc,argv,"ua:n:i:f:o:")) != EOF) {
        switch (dopt) {
        case 'u':
        case 'h':   usage(); return 0;
        case 'a':
            if (strcmp(optarg,"ppc")==0) {
                // Building for PowerPC
                arch = ARCH_PPC;
            } else if (strcmp(optarg,"x86")==0) {
                // Building for x86
                arch = ARCH_X86;
            } else if (strcmp(optarg,"x86_64")==0) {
                // Building for x86_64
                arch = ARCH_X86_64;
            } else if (strcmp(optarg,"intelmac")==0) {
                // Building for Intel mac (x86_64 with slightly different syntax)
                arch = ARCH_INTELMAC;
            } else {
                fprintf(stderr,"error: unknown architecture: %s\n", optarg);
                exit(1);
            }
            break;
        case 'n':
            if (strlen(optarg) >= 64) {
                fprintf(stderr,"error: %s, typename too long\n", argv[0]);
                return -1;
            }
            strcpy(typename,optarg); 
            break;
        case 'i':   intbits = atoi(optarg);     break;
        case 'f':   fracbits = atoi(optarg);    break;
        case 'o':
            if (strlen(optarg) > 64) {
                fprintf(stderr,"error: %s, output filename too long\n", argv[0]);
                return -1;
            }
            strcpy(outputfile,optarg); 
            break;
        default:
            usage();
            return -1;
        }
    }

    int totalbits = intbits + fracbits;
    if ( strcmp(typename,"") == 0 ) {
        fprintf(stderr,"error: %s, must specify typename\n", argv[0]);
        usage();
        return -1;
    } else if ( arch == ARCH_UNKNOWN ) {
        fprintf(stderr,"error: %s, must specify architecture type\n", argv[0]);
        usage();
        return -1;
    } else if ( intbits < 0 ) {
        fprintf(stderr,"error: %s, unspecified or negative intbits \n", argv[0]);
        usage();
        return -1;
    } else if ( intbits < 1 ) {
        fprintf(stderr,"error: %s, intbits must be at least 1 (sign bit)\n", argv[0]);
        return -1;
    } else if ( fracbits < 0 ) {
        fprintf(stderr,"error: %s, unspecified or negative fracbits \n", argv[0]);
        usage();
        return -1;
    } else if ( totalbits != 8 && totalbits != 16 && totalbits != 32) {
        fprintf(stderr,"error: intbits (%d) + fracbits (%d) must equal 8|16|32\n",intbits,fracbits);
        return -1;
    }

    // mangling...
    if (arch == ARCH_PPC) {
        comment_char = ';';
        strcpy(name_mangler,"_");
        strcpy(archname,"ppc");
    } else if (arch == ARCH_X86) {
        comment_char = '#';
        strcpy(name_mangler,"");
        strcpy(archname,"x86");
    } else if (arch == ARCH_X86_64) {
        comment_char = '#';
        strcpy(name_mangler,"");
        strcpy(archname,"x86_64");
    } else if (arch == ARCH_INTELMAC) {
        comment_char = '#';
        strcpy(name_mangler,"_");
        //strcpy(archname,"intelmac");
        strcpy(archname,"x86_64");  // intelmac is x86_64
    } else {
        comment_char = '#';
        strcpy(name_mangler,"");
        strcpy(archname,"[unknown]");
    }

    // open output file
    FILE * fid;
    if ( strcmp(outputfile,"") != 0 ) {
        // no output file set
        fid = fopen(outputfile,"w");
        if (fid == NULL) {
            printf("error: could not open %s for writing\n",outputfile);
            return -1;
        }
    } else {
        fid = stdout;
    }

    // run the assembly source code generator
    // TODO : clean up this mess!

    fprintf(fid,"%c auto-generated file : do not edit\n", comment_char);
    fprintf(fid,"%c invoked as: ", comment_char);
    unsigned int i;
    for (i=0; i<argc; i++)
        fprintf(fid," %s", argv[i]);
    fprintf(fid,"\n");
    fprintf(fid,"%c \n", comment_char);
    fprintf(fid,"%c   target architecture : %s\n", comment_char, archname);
    fprintf(fid,"    .text\n");
    fprintf(fid,"    .set qint,  %3d   %c intbits\n",intbits,comment_char);
    fprintf(fid,"    .set qfrac, %3d   %c fracbits\n",fracbits,comment_char);
    fprintf(fid,"    .globl %s%s_mul\n", name_mangler,typename);
    if (arch == ARCH_X86)
        fprintf(fid,"    .type %s%s_mul,@function\n", name_mangler,typename);
    fprintf(fid,"\n");
    fprintf(fid,"%c multiplication\n", comment_char);
    fprintf(fid,"%s%s_mul:\n", name_mangler,typename);
    fprintf(fid,".include \"qtype.mul.%s.s\"\n", archname);

    // close the file (if not printing to stdout)
    if ( strcmp(outputfile,"") != 0 )
        fclose(fid);

    return 0;
}

