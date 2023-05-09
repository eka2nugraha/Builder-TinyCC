#ifndef dL_5601
#define dL_5601
static const char L_5601[]="usage: tcc -ar [rcsv] lib file...\n";
static const char L_5602[]="create library ([abdioptxN] not supported).\n";
static const char L_5603[]="habdioptxN";
static const char L_5604[]=".";
static const char L_5605[]=".";
static const char L_5606[]="v";
static const char L_5607[]="wb";
static const char L_5608[]="tcc: ar: can't open file %s \n";
static const char L_5609[]="%s.tmp";
static const char L_5610[]="wb+";
static const char L_5611[]="tcc: ar: can't create temporary file %s\n";
static const char L_5612[]="100666";
static const char L_5613[]="rb";
static const char L_5614[]="tcc: ar: can't open file %s \n";
static const char L_5615[]="a - %s\n";
static const char L_5616[]="tcc: ar: Unsupported Elf Class: %s\n";
static const char L_5617[]=".strtab";
static const char L_5618[]="%-10d";
static const char L_5619[]="!<arch>\n";
static const char L_5620[]="%-10d";
static const char L_5621[]="";
static const char L_5622[]="-v";
static const char L_5623[]="-o";
static const char L_5624[]="usage: tcc -impdef library.dll [-v] [-o outputfile]\ncreate export definition file (.def) from dll\n";
static const char L_5625[]=".def";
static const char L_5626[]=".dll";
static const char L_5627[]="tcc: impdef: %s '%s'\n";
static const char L_5628[]="can't find file";
static const char L_5629[]="can't read symbols";
static const char L_5630[]="no symbols found in";
static const char L_5631[]="unknown file type";
static const char L_5632[]="-> %s\n";
static const char L_5633[]="wb";
static const char L_5634[]="tcc: impdef: could not create output file: %s\n";
static const char L_5635[]="LIBRARY %s\n\nEXPORTS\n";
static const char L_5636[]="%s\n";
static const char L_5637[]="<- %s (%d symbol%s)\n";
static const char L_5638[]="s";
static const char L_5639[]="-m%d not implemented.";
static const char L_5640[]="\"";
static const char L_5641[]="\\\"";
static const char L_5642[]="%.*s%s"
#ifdef TCC_TARGET_PE
        "-win32"
#endif
        "-tcc"
#ifdef _WIN32
        ".exe"
#endif
;
static const char L_5643[]="x86_64";
static const char L_5644[]="i386";
static const char L_5645[]="could not run '%s'";
static const char L_5646[]="%.*s.d";
static const char L_5647[]="<- %s\n";
static const char L_5648[]="w";
static const char L_5649[]="could not open '%s'";
static const char L_5650[]="%s: \\\n";
static const char L_5651[]=" %s \\\n";
static const char L_5652[]="\n";
#endif
/* -------------------------------------------------------------- */
/*
 *  TCC - Tiny C Compiler
 *
 *  tcctools.c - extra tools and and -m32/64 support
 *
 */

/* -------------------------------------------------------------- */
/*
 * This program is for making libtcc1.a without ar
 * tiny_libmaker - tiny elf lib maker
 * usage: tiny_libmaker [lib] files...
 * Copyright (c) 2007 Timppa
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "tcc.h"

//#define ARMAG  "!<arch>\n"
#define ARFMAG "`\n"

typedef struct {
    char ar_name[16];
    char ar_date[12];
    char ar_uid[6];
    char ar_gid[6];
    char ar_mode[8];
    char ar_size[10];
    char ar_fmag[2];
} ArHdr;

static unsigned long le2belong(unsigned long ul) {
    return ((ul & 0xFF0000)>>8)+((ul & 0xFF000000)>>24) +
        ((ul & 0xFF)<<24)+((ul & 0xFF00)<<8);
}

/* Returns 1 if s contains any of the chars of list, else 0 */
static int contains_any(const char *s, const char *list) {
  const char *l;
  for (; *s; s++) {
      for (l = list; *l; l++) {
          if (*s == *l)
              return 1;
      }
  }
  return 0;
}

static int ar_usage(int ret) {
    fprintf(stderr, L_5601);
    fprintf(stderr, L_5602);
    return ret;
}

ST_FUNC int tcc_tool_ar(TCCState *s1, int argc, char **argv)
{
    static ArHdr arhdr = {
        "/               ",
        "            ",
        "0     ",
        "0     ",
        "0       ",
        "          ",
        ARFMAG
        };

    static ArHdr arhdro = {
        "                ",
        "            ",
        "0     ",
        "0     ",
        "0       ",
        "          ",
        ARFMAG
        };

    FILE *fi, *fh = NULL, *fo = NULL;
    ElfW(Ehdr) *ehdr;
    ElfW(Shdr) *shdr;
    ElfW(Sym) *sym;
    int i, fsize, i_lib, i_obj;
    char *buf, *shstr, *symtab = NULL, *strtab = NULL;
    int symtabsize = 0;//, strtabsize = 0;
    char *anames = NULL;
    int *afpos = NULL;
    int istrlen, strpos = 0, fpos = 0, funccnt = 0, funcmax, hofs;
    char tfile[260], stmp[20];
    char *file, *name;
    int ret = 2;
    const char *ops_conflict = L_5603;  // unsupported but destructive if ignored.
    int verbose = 0;

    i_lib = 0; i_obj = 0;  // will hold the index of the lib and first obj
    for (i = 1; i < argc; i++) {
        const char *a = argv[i];
        if (*a == '-' && strstr(a, L_5604))
            ret = 1; // -x.y is always invalid (same as gnu ar)
        if ((*a == '-') || (i == 1 && !strstr(a, L_5605))) {  // options argument
            if (contains_any(a, ops_conflict))
                ret = 1;
            if (strstr(a, L_5606))
                verbose = 1;
        } else {  // lib or obj files: don't abort - keep validating all args.
            if (!i_lib)  // first file is the lib
                i_lib = i;
            else if (!i_obj)  // second file is the first obj
                i_obj = i;
        }
    }

    if (!i_obj)  // i_obj implies also i_lib. we require both.
        ret = 1;

    if (ret == 1)
        return ar_usage(ret);

    if ((fh = fopen(argv[i_lib], L_5607)) == NULL)
    {
        fprintf(stderr, L_5608, argv[i_lib]);
        goto the_end;
    }

    sprintf(tfile, L_5609, argv[i_lib]);
    if ((fo = fopen(tfile, L_5610)) == NULL)
    {
        fprintf(stderr, L_5611, tfile);
        goto the_end;
    }

    funcmax = 250;
    afpos = tcc_realloc(NULL, funcmax * sizeof *afpos); // 250 func
    memcpy(&arhdro.ar_mode, L_5612, 6);

    // i_obj = first input object file
    while (i_obj < argc)
    {
        if (*argv[i_obj] == '-') {  // by now, all options start with '-'
            i_obj++;
            continue;
        }
        if ((fi = fopen(argv[i_obj], L_5613)) == NULL) {
            fprintf(stderr, L_5614, argv[i_obj]);
            goto the_end;
        }
        if (verbose)
            printf(L_5615, argv[i_obj]);

        fseek(fi, 0, SEEK_END);
        fsize = ftell(fi);
        fseek(fi, 0, SEEK_SET);
        buf = tcc_malloc(fsize + 1);
        fread(buf, fsize, 1, fi);
        fclose(fi);

        // elf header
        ehdr = (ElfW(Ehdr) *)buf;
        if (ehdr->e_ident[4] != ELFCLASSW)
        {
            fprintf(stderr, L_5616, argv[i_obj]);
            goto the_end;
        }

        shdr = (ElfW(Shdr) *) (buf + ehdr->e_shoff + ehdr->e_shstrndx * ehdr->e_shentsize);
        shstr = (char *)(buf + shdr->sh_offset);
        for (i = 0; i < ehdr->e_shnum; i++)
        {
            shdr = (ElfW(Shdr) *) (buf + ehdr->e_shoff + i * ehdr->e_shentsize);
            if (!shdr->sh_offset)
                continue;
            if (shdr->sh_type == SHT_SYMTAB)
            {
                symtab = (char *)(buf + shdr->sh_offset);
                symtabsize = shdr->sh_size;
            }
            if (shdr->sh_type == SHT_STRTAB)
            {
                if (!strcmp(shstr + shdr->sh_name, L_5617))
                {
                    strtab = (char *)(buf + shdr->sh_offset);
                    //strtabsize = shdr->sh_size;
                }
            }
        }

        if (symtab && symtabsize)
        {
            int nsym = symtabsize / sizeof(ElfW(Sym));
            //printf("symtab: info size shndx name\n");
            for (i = 1; i < nsym; i++)
            {
                sym = (ElfW(Sym) *) (symtab + i * sizeof(ElfW(Sym)));
                if (sym->st_shndx &&
                    (sym->st_info == 0x10
                    || sym->st_info == 0x11
                    || sym->st_info == 0x12
                    )) {
                    //printf("symtab: %2Xh %4Xh %2Xh %s\n", sym->st_info, sym->st_size, sym->st_shndx, strtab + sym->st_name);
                    istrlen = strlen(strtab + sym->st_name)+1;
                    anames = tcc_realloc(anames, strpos+istrlen);
                    strcpy(anames + strpos, strtab + sym->st_name);
                    strpos += istrlen;
                    if (++funccnt >= funcmax) {
                        funcmax += 250;
                        afpos = tcc_realloc(afpos, funcmax * sizeof *afpos); // 250 func more
                    }
                    afpos[funccnt] = fpos;
                }
            }
        }

        file = argv[i_obj];
        for (name = strchr(file, 0);
             name > file && name[-1] != '/' && name[-1] != '\\';
             --name);
        istrlen = strlen(name);
        if (istrlen >= sizeof(arhdro.ar_name))
            istrlen = sizeof(arhdro.ar_name) - 1;
        memset(arhdro.ar_name, ' ', sizeof(arhdro.ar_name));
        memcpy(arhdro.ar_name, name, istrlen);
        arhdro.ar_name[istrlen] = '/';
        sprintf(stmp, L_5618, fsize);
        memcpy(&arhdro.ar_size, stmp, 10);
        fwrite(&arhdro, sizeof(arhdro), 1, fo);
        fwrite(buf, fsize, 1, fo);
        tcc_free(buf);
        i_obj++;
        fpos += (fsize + sizeof(arhdro));
    }
    hofs = 8 + sizeof(arhdr) + strpos + (funccnt+1) * sizeof(int);
    fpos = 0;
    if ((hofs & 1)) // align
        hofs++, fpos = 1;
    // write header
    fwrite(L_5619, 8, 1, fh);
    sprintf(stmp, L_5620, (int)(strpos + (funccnt+1) * sizeof(int)));
    memcpy(&arhdr.ar_size, stmp, 10);
    fwrite(&arhdr, sizeof(arhdr), 1, fh);
    afpos[0] = le2belong(funccnt);
    for (i=1; i<=funccnt; i++)
        afpos[i] = le2belong(afpos[i] + hofs);
    fwrite(afpos, (funccnt+1) * sizeof(int), 1, fh);
    fwrite(anames, strpos, 1, fh);
    if (fpos)
        fwrite(L_5621, 1, 1, fh);
    // write objects
    fseek(fo, 0, SEEK_END);
    fsize = ftell(fo);
    fseek(fo, 0, SEEK_SET);
    buf = tcc_malloc(fsize + 1);
    fread(buf, fsize, 1, fo);
    fwrite(buf, fsize, 1, fh);
    tcc_free(buf);
    ret = 0;
the_end:
    if (anames)
        tcc_free(anames);
    if (afpos)
        tcc_free(afpos);
    if (fh)
        fclose(fh);
    if (fo)
        fclose(fo), remove(tfile);
    return ret;
}

/* -------------------------------------------------------------- */
/*
 * tiny_impdef creates an export definition file (.def) from a dll
 * on MS-Windows. Usage: tiny_impdef library.dll [-o outputfile]"
 *
 *  Copyright (c) 2005,2007 grischka
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifdef TCC_TARGET_PE

ST_FUNC int tcc_tool_impdef(TCCState *s1, int argc, char **argv)
{
    int ret, v, i;
    char infile[260];
    char outfile[260];

    const char *file;
    char *p, *q;
    FILE *fp, *op;

#ifdef _WIN32
    char path[260];
#endif

    infile[0] = outfile[0] = 0;
    fp = op = NULL;
    ret = 1;
    p = NULL;
    v = 0;

    for (i = 1; i < argc; ++i) {
        const char *a = argv[i];
        if ('-' == a[0]) {
            if (0 == strcmp(a, L_5622)) {
                v = 1;
            } else if (0 == strcmp(a, L_5623)) {
                if (++i == argc)
                    goto usage;
                strcpy(outfile, argv[i]);
            } else
                goto usage;
        } else if (0 == infile[0])
            strcpy(infile, a);
        else
            goto usage;
    }

    if (0 == infile[0]) {
usage:
        fprintf(stderr,
            L_5624
            );
        goto the_end;
    }

    if (0 == outfile[0]) {
        strcpy(outfile, tcc_basename(infile));
        q = strrchr(outfile, '.');
        if (NULL == q)
            q = strchr(outfile, 0);
        strcpy(q, L_5625);
    }

    file = infile;
#ifdef _WIN32
    if (SearchPath(NULL, file, L_5626, sizeof path, path, NULL))
        file = path;
#endif
    ret = tcc_get_dllexports(file, &p);
    if (ret || !p) {
        fprintf(stderr, L_5627,
            ret == -1 ? L_5628 :
            ret ==  1 ? L_5629 :
            ret ==  0 ? L_5630 :
            L_5631, file);
        ret = 1;
        goto the_end;
    }

    if (v)
        printf(L_5632, file);

    op = fopen(outfile, L_5633);
    if (NULL == op) {
        fprintf(stderr, L_5634, outfile);
        goto the_end;
    }

    fprintf(op, L_5635, tcc_basename(file));
    for (q = p, i = 0; *q; ++i) {
        fprintf(op, L_5636, q);
        q += strlen(q) + 1;
    }

    if (v)
        printf(L_5637, outfile, i, &L_5638[i<2]);

    ret = 0;

the_end:
    /* cannot free memory received from tcc_get_dllexports
       if it came from a dll */
    /* if (p)
        tcc_free(p); */
    if (fp)
        fclose(fp);
    if (op)
        fclose(op);
    return ret;
}

#endif /* TCC_TARGET_PE */

/* -------------------------------------------------------------- */
/*
 *  TCC - Tiny C Compiler
 *
 *  Copyright (c) 2001-2004 Fabrice Bellard
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/* re-execute the i386/x86_64 cross-compilers with tcc -m32/-m64: */

#if !defined TCC_TARGET_I386 && !defined TCC_TARGET_X86_64

ST_FUNC void tcc_tool_cross(TCCState *s, char **argv, int option)
{
    tcc_error(L_5639, option);
}

#else
#ifdef _WIN32
#include <process.h>

static char *str_replace(const char *str, const char *p, const char *r)
{
    const char *s, *s0;
    char *d, *d0;
    int sl, pl, rl;

    sl = strlen(str);
    pl = strlen(p);
    rl = strlen(r);
    for (d0 = NULL;; d0 = tcc_malloc(sl + 1)) {
        for (d = d0, s = str; s0 = s, s = strstr(s, p), s; s += pl) {
            if (d) {
                memcpy(d, s0, sl = s - s0), d += sl;
                memcpy(d, r, rl), d += rl;
            } else
                sl += rl - pl;
        }
        if (d) {
            strcpy(d, s0);
            return d0;
        }
    }
}

static int execvp_win32(const char *prog, char **argv)
{
    int ret; char **p;
    /* replace all " by \" */
    for (p = argv; *p; ++p)
        if (strchr(*p, '"'))
            *p = str_replace(*p, L_5640, L_5641);
#if defined(__BORLANDC__)
	ret = spawnvp(P_NOWAIT, prog, (char *const*)argv);
	if (-1 == ret)
		return ret;
	cwait(&ret, ret, WAIT_CHILD);
#else
	ret = _spawnvp(P_NOWAIT, prog, (const char *const*)argv);
	if (-1 == ret)
		return ret;
	_cwait(&ret, ret, WAIT_CHILD);
#endif
    exit(ret);
}
#define execvp execvp_win32
#endif /* _WIN32 */

ST_FUNC void tcc_tool_cross(TCCState *s, char **argv, int target)
{
    char program[4096];
    char *a0 = argv[0];
    int prefix = tcc_basename(a0) - a0;

    snprintf(program, sizeof program,
        L_5642
        , prefix, a0, target == 64 ? L_5643 : L_5644);

    if (strcmp(a0, program))
        execvp(argv[0] = program, argv);
    tcc_error(L_5645, program);
}

#endif /* TCC_TARGET_I386 && TCC_TARGET_X86_64 */
/* -------------------------------------------------------------- */
/* enable commandline wildcard expansion (tcc -o x.exe *.c) */

#ifdef _WIN32
int _CRT_glob = 1;
#ifndef _CRT_glob
int _dowildcard = 1;
#endif
#endif

/* -------------------------------------------------------------- */
/* generate xxx.d file */

ST_FUNC void gen_makedeps(TCCState *s, const char *target, const char *filename)
{
    FILE *depout;
    char buf[1024];
    int i;

    if (!filename) {
        /* compute filename automatically: dir/file.o -> dir/file.d */
        snprintf(buf, sizeof buf, L_5646,
            (int)(tcc_fileextension(target) - target), target);
        filename = buf;
    }

    if (s->verbose)
        printf(L_5647, filename);

    /* XXX return err codes instead of error() ? */
    depout = fopen(filename, L_5648);
    if (!depout)
        tcc_error(L_5649, filename);

    fprintf(depout, L_5650, target);
    for (i=0; i<s->nb_target_deps; ++i)
        fprintf(depout, L_5651, s->target_deps[i]);
    fprintf(depout, L_5652);
    fclose(depout);
}

/* -------------------------------------------------------------- */
