/*
 * utils/yaml.c - YAML utils
 *
 * Copyright (C) 2019 Alexandros Theodotou
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <string.h>

#include "utils/yaml.h"

#include <gtk/gtk.h>

/**
 * Gets nth line from a string.
 */
const char *sgets(char *s, int n, char **strp){
    if(**strp == '\0')return NULL;
    int i;
    for(i=0;i<n-1;++i, ++(*strp)){
        s[i] = **strp;
        if(**strp == '\0')
            break;
        if(**strp == '\n'){
            s[i+1]='\0';
            ++(*strp);
            break;
        }
    }
    if(i==n-1)
        s[i] = '\0';
    return s;
}

/**
 * Removes lines starting with control chars from the
 * YAML string.
 */
void
yaml_sanitize (char ** e)
{
  char buff[80];
  char cpy[strlen (*e)];
  int index = 0;
  while (NULL != sgets (buff,
                        sizeof (buff),
                        e))
    {
      if (buff[0] > 31)
        {
          for (int i = 0; i < strlen (buff); i++)
            cpy[index++] = buff[i];
          g_message (cpy);
        }
    }
  for (int i = 0; i < index; i++)
    {
      (*e)[i] = cpy[i];
    }
  (*e)[index] = '\0';
}
