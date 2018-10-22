/*
 * utils/xml.c - XML serializer for parsing/writing project file
 *
 * Copyright (C) 2018 Alexandros Theodotou
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

/** \file
 */
#ifndef __UTILS_XML_H__
#define __UTILS_XML_H__

#include <libxml/xmlwriter.h>


/**
 * Writes the project to an XML file.
 */
void
xml_write_project ();

void
xml_write_ports ();

void
xml_write_regions ();

void
xml_load_ports ();

void
xml_load_regions ();

/**
 * Loads the project data.
 */
void
xml_load_project ();

#endif
