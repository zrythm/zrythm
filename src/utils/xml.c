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
 * XML parser for the project file.
 *
 * See http://xmlsoft.org/examples/testWriter.c for details.
 */

#include <stdio.h>
#include <string.h>

#include "project.h"
#include "audio/channel.h"
#include "audio/mixer.h"
#include "utils/xml.h"

#include <gtk/gtk.h>

#include <libxml/encoding.h>

#define MY_ENCODING "ISO-8859-1"

/**
 * Already serialized/deserialized ports
 */
static Port * ports[2000];
static int num_ports = 0;


/**
 * ConvertInput:
 * @in: string in a given encoding
 * @encoding: the encoding used
 *
 * Converts @in into UTF-8 for processing with libxml2 APIs
 *
 * Returns the converted UTF-8 string, or NULL in case of error.
 */
static xmlChar *
convert_input(const char *in, const char *encoding)
{
  xmlChar *out;
  int ret;
  int size;
  int out_size;
  int temp;
  xmlCharEncodingHandlerPtr handler;

  if (in == 0)
      return 0;

  handler = xmlFindCharEncodingHandler(encoding);

  if (!handler) {
      printf("ConvertInput: no encoding handler found for '%s'\n",
             encoding ? encoding : "");
      return 0;
  }

  size = (int) strlen(in) + 1;
  out_size = size * 2 - 1;
  out = (unsigned char *) xmlMalloc((size_t) out_size);

  if (out != 0) {
      temp = size - 1;
      ret = handler->input(out, &out_size, (const xmlChar *) in, &temp);
      if ((ret < 0) || (temp - size + 1)) {
          if (ret < 0) {
              printf("ConvertInput: conversion wasn't successful.\n");
          } else {
              printf
                  ("ConvertInput: conversion wasn't successful. converted: %i octets.\n",
                   temp);
          }

          xmlFree(out);
          out = 0;
      } else {
          out = (unsigned char *) xmlRealloc(out, out_size + 1);
          out[out_size] = 0;  /*null terminating out */
      }
  } else {
      printf("ConvertInput: no mem\n");
  }

  return out;
}

static void
write_gdk_rgba (xmlTextWriterPtr writer, GdkRGBA * color)
{
  int rc;
  rc = xmlTextWriterStartElement (writer, BAD_CAST "Color");
  rc = xmlTextWriterWriteFormatElement (writer,
                                        BAD_CAST "red",
                                        "%f",
                                        color->red);
  rc = xmlTextWriterWriteFormatElement (writer,
                                        BAD_CAST "green",
                                        "%f",
                                        color->green);
  rc = xmlTextWriterWriteFormatElement (writer,
                                        BAD_CAST "blue",
                                        "%f",
                                        color->blue);
  rc = xmlTextWriterWriteFormatElement (writer,
                                        BAD_CAST "alpha",
                                        "%f",
                                        color->alpha);
  rc = xmlTextWriterEndElement(writer);
}

static void
write_port (xmlTextWriterPtr writer, Port * port)
{
  int rc;
  rc = xmlTextWriterStartElement (writer, BAD_CAST "Port");
  rc = xmlTextWriterWriteFormatAttribute (writer,
                                        BAD_CAST "id",
                                        "%d",
                                        port->id);
  rc = xmlTextWriterWriteFormatElement (writer,
                                        BAD_CAST "label",
                                        "%s",
                                        port->label);
  rc = xmlTextWriterWriteFormatElement (writer,
                                        BAD_CAST "type",
                                        "%d",
                                        port->type);
  rc = xmlTextWriterWriteFormatElement (writer,
                                        BAD_CAST "flow",
                                        "%d",
                                        port->flow);

  /* write source port ids */
  rc = xmlTextWriterStartElement (writer, BAD_CAST "Srcs");
  for (int i = 0; i < port->num_srcs; i++)
    {
      rc = xmlTextWriterWriteFormatElement (writer,
                                            BAD_CAST "id",
                                            "%d",
                                            port->srcs[i]->id);
    }
  rc = xmlTextWriterEndElement(writer);

  /* write dest port ids */
  rc = xmlTextWriterStartElement (writer, BAD_CAST "Dests");
  for (int i = 0; i < port->num_dests; i++)
    {
      rc = xmlTextWriterWriteFormatElement (writer,
                                            BAD_CAST "id",
                                            "%d",
                                            port->dests[i]->id);
    }
  rc = xmlTextWriterEndElement(writer);

  rc = xmlTextWriterWriteFormatElement (writer,
                                        BAD_CAST "internal_type",
                                        "%d",
                                        port->internal_type);
  rc = xmlTextWriterWriteFormatElement (writer,
                                        BAD_CAST "owner_jack",
                                        "%d",
                                        port->owner_jack);
  rc = xmlTextWriterWriteFormatElement (writer,
                                        BAD_CAST "owner_pl",
                                        "%d",
                                        port->owner_pl->id);
  rc = xmlTextWriterWriteFormatElement (writer,
                                        BAD_CAST "owner_ch",
                                        "%d",
                                        port->owner_ch->id);

  rc = xmlTextWriterEndElement(writer);
}

static void
write_track (xmlTextWriterPtr writer, Track * track)
{

}

static void
write_channel (xmlTextWriterPtr writer, Channel * channel)
{
  int rc;
  rc = xmlTextWriterStartElement (writer, BAD_CAST "Channel");
  rc = xmlTextWriterWriteFormatAttribute(writer, BAD_CAST "id",
                                         "%d", channel->id);
  rc = xmlTextWriterWriteFormatElement (writer,
                                        BAD_CAST "name",
                                        "%s",
                                        channel->name);
  rc = xmlTextWriterStartElement (writer, BAD_CAST "Plugins");
  for (int i = 0; i < MAX_PLUGINS; i++)
    {
      Plugin * plugin = channel->strip[i];
      if (plugin)
        {
          rc = xmlTextWriterStartElement (writer, BAD_CAST "Plugin");
          rc = xmlTextWriterWriteFormatAttribute(writer,
                                                 BAD_CAST "pos",
                                                 "%d",
                                                 i);
          rc = xmlTextWriterWriteFormatAttribute(writer,
                                                 BAD_CAST "id",
                                                 "%d",
                                                 plugin->id);
          rc = xmlTextWriterWriteFormatElement (writer,
                                                BAD_CAST "name",
                                                "%s",
                                                channel->name);


          rc = xmlTextWriterEndElement(writer);
        }

    }
  rc = xmlTextWriterEndElement(writer);

  rc = xmlTextWriterWriteFormatElement (writer,
                                        BAD_CAST "type",
                                        "%d",
                                        channel->type);
  rc = xmlTextWriterWriteFormatElement (writer,
                                        BAD_CAST "volume",
                                        "%f",
                                        channel->volume);
  rc = xmlTextWriterWriteFormatElement (writer,
                                        BAD_CAST "muted",
                                        "%d",
                                        channel->muted);
  rc = xmlTextWriterWriteFormatElement (writer,
                                        BAD_CAST "soloed",
                                        "%d",
                                        channel->soloed);
  write_gdk_rgba (writer, &channel->color);
  rc = xmlTextWriterWriteFormatElement (writer,
                                        BAD_CAST "phase",
                                        "%f",
                                        channel->phase);
  rc = xmlTextWriterStartElement (writer, BAD_CAST "stereo_in");
  write_port (writer, channel->stereo_in->l);
  write_port (writer, channel->stereo_in->r);
  rc = xmlTextWriterEndElement(writer);
  write_port (writer, channel->midi_in);
  write_port (writer, channel->piano_roll);
  rc = xmlTextWriterStartElement (writer, BAD_CAST "stereo_out");
  write_port (writer, channel->stereo_in->l);
  write_port (writer, channel->stereo_in->r);
  rc = xmlTextWriterEndElement(writer);
  rc = xmlTextWriterWriteFormatElement (writer,
                                        BAD_CAST "recording",
                                        "%d",
                                        channel->recording);
  rc = xmlTextWriterWriteFormatElement (writer,
                                        BAD_CAST "output",
                                        "%d",
                                        channel->output->id);
  write_track (writer, channel->track);
  rc = xmlTextWriterWriteFormatElement (writer,
                                        BAD_CAST "enabled",
                                        "%d",
                                        channel->enabled);

  rc = xmlTextWriterEndElement(writer);
}

/**
 * Writes the project to an XML file.
 */
void
xml_write_project (const char * file)
{
  g_message ("Writing %s...", file);

  /*
   * this initialize the library and check potential ABI mismatches
   * between the version it was compiled for and the actual shared
   * library used.
   */
  LIBXML_TEST_VERSION

  int rc;
  xmlTextWriterPtr writer;
  xmlChar *tmp;

  /* Create a new XmlWriter for uri, with no compression. */
  writer = xmlNewTextWriterFilename(file, 0);
  if (writer == NULL) {
      g_warning ("Error creating the xml writer\n");
      return;
  }

  /* prettify */
  xmlTextWriterSetIndent (writer, 1);

  /* Start the document with the xml default for the version,
   * encoding ISO 8859-1 and the default for the standalone
   * declaration. */
  rc = xmlTextWriterStartDocument(writer, NULL, MY_ENCODING, NULL);
  if (rc < 0)
    {
      g_warning ("Error at xmlTextWriterStartDocument\n");
      return;
    }

  /* write project */
  rc = xmlTextWriterWriteComment(writer, "The project");
  rc = xmlTextWriterStartElement(writer, BAD_CAST "Project");
  rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "ver",
                                   BAD_CAST PROJECT_XML_VER);

  /* write channels */
  rc = xmlTextWriterStartElement(writer, BAD_CAST "Channels");
  for (int i = 0; i < MIXER->num_channels; i++)
    {
      write_channel (writer, MIXER->channels[i]);
    }


  /* Write an element named "X_ORDER_ID" as child of HEADER. */
  /*rc = xmlTextWriterWriteFormatElement(writer, BAD_CAST "X_ORDER_ID",*/
                                       /*"%010d", 53535);*/
  /*if (rc < 0) {*/
      /*return;*/
  /*}*/

  /* Write an element named "CUSTOMER_ID" as child of HEADER. */
  /*rc = xmlTextWriterWriteFormatElement(writer, BAD_CAST "CUSTOMER_ID",*/
                                       /*"%d", 1010);*/
  /*if (rc < 0) {*/
      /*return;*/
  /*}*/

  /*[> Write an element named "NAME_1" as child of HEADER. <]*/
  /*tmp = convert_input("M\xFCller", MY_ENCODING);*/
  /*rc = xmlTextWriterWriteElement(writer, BAD_CAST "NAME_1", tmp);*/
  /*if (rc < 0) {*/
      /*return;*/
  /*}*/
  /*if (tmp != NULL) xmlFree(tmp);*/

  /*[> Write an element named "NAME_2" as child of HEADER. <]*/
  /*tmp = convert_input("J\xF6rg", MY_ENCODING);*/
  /*rc = xmlTextWriterWriteElement(writer, BAD_CAST "NAME_2", tmp);*/
  /*if (rc < 0) {*/
      /*return;*/
  /*}*/
  /*if (tmp != NULL) xmlFree(tmp);*/

  /* Close the element named FOOTER. */
  rc = xmlTextWriterEndElement(writer);
  if (rc < 0) {
      printf
          ("testXmlwriterFilename: Error at xmlTextWriterEndElement\n");
      return;
  }

  /* Here we could close the elements ORDER and EXAMPLE using the
   * function xmlTextWriterEndElement, but since we do not want to
   * write any other elements, we simply call xmlTextWriterEndDocument,
   * which will do all the work. */
  rc = xmlTextWriterEndDocument(writer);
  if (rc < 0) {
      printf
          ("testXmlwriterFilename: Error at xmlTextWriterEndDocument\n");
      return;
  }

  xmlFreeTextWriter(writer);

  /*
   * Cleanup function for the XML library.
   */
  xmlCleanupParser();
}


