/*
 * The new sysinstall program.
 *
 * This is probably the last program in the `sysinstall' line - the next
 * generation being essentially a complete rewrite.
 *
 * $Id: menus.c,v 1.5 1995/05/04 03:51:19 jkh Exp $
 *
 * Copyright (c) 1995
 *	Jordan Hubbard.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer, 
 *    verbatim and that no modifications are made prior to this 
 *    point in the file.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Jordan Hubbard
 *	for the FreeBSD Project.
 * 4. The name of Jordan Hubbard or the FreeBSD project may not be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY JORDAN HUBBARD ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL JORDAN HUBBARD OR HIS PETS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, LIFE OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#include "sysinstall.h"

void
lang_set_Danish(char *str)
{
    systemChangeFont("iso-8x14");
    systemChangeLang("da_DK.ISO8859-1");
    systemChangeTerminal("cons25l1", "cons25l1_m");
}

void
lang_set_Dutch(char *str)
{
    systemChangeFont("iso-8x14");
    systemChangeLang("nl_NL.ISO8859-1");
    systemChangeTerminal("cons25l1", "cons25l1_m");
}

void
lang_set_English(char *str)
{
    systemChangeFont("cp850-8x14");
    systemChangeLang("en_US.ISO8859-1");
    systemChangeTerminal("cons25", "cons25_m");
}

void
lang_set_French(char *str)
{
    systemChangeFont("iso-8x14");
    systemChangeLang("fr_FR.ISO8859-1");
    systemChangeTerminal("cons25l1", "cons25l1_m");
}

void
lang_set_German(char *str)
{
    systemChangeFont("iso-8x14");
    systemChangeLang("de_DE.ISO8859-1");
    systemChangeTerminal("cons25l1", "cons25l1_m");
}

void
lang_set_Italian(char *str)
{
    systemChangeFont("iso-8x14");
    systemChangeLang("it_IT.ISO8859-1");
    systemChangeTerminal("cons25l1", "cons25l1_m");
}

/* Someday we will have to do a lot better than this */
void
lang_set_Japanese(char *str)
{
    systemChangeFont("iso-8x14");
    systemChangeLang("ja_JP.ROMAJI");
    systemChangeTerminal("cons25", "cons25_m");
}

void
lang_set_Russian(char *str)
{
    systemChangeFont("iso-8x14");
    systemChangeLang("ru_SU.KOI8-R");
    systemChangeTerminal("cons25r", "cons25r_m");
    systemChangeScreenmap("koi8-r2cp866");
}

void
lang_set_Spanish(char *str)
{
    systemChangeFont("iso-8x14");
    systemChangeLang("es_ES.ISO8859-1");
    systemChangeTerminal("cons25l1", "cons25l1_m");
}

void
lang_set_Swedish(char *str)
{
    systemChangeFont("iso-8x14");
    systemChangeLang("sv_SV.ISO8859-1");
    systemChangeTerminal("cons25l1", "cons25l1_m");
}
