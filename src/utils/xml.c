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
#include "audio/region.h"
#include "audio/track.h"
#include "plugins/lv2_plugin.h"
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
write_port_id (xmlTextWriterPtr writer, Port * port)
{
  int rc;
  rc = xmlTextWriterWriteFormatElement (writer,
                                        BAD_CAST "port_id",
                                        "%d",
                                        port->id);
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
  if (port->owner_pl)
    {
      rc = xmlTextWriterWriteFormatElement (writer,
                                            BAD_CAST "owner_pl_id",
                                            "%d",
                                            port->owner_pl->id);
    }
  if (port->owner_ch)
    {
      rc = xmlTextWriterWriteFormatElement (writer,
                                            BAD_CAST "owner_ch_id",
                                            "%d",
                                            port->owner_ch->id);
    }

  rc = xmlTextWriterEndElement(writer);
}

static void
write_plugin_descr (xmlTextWriterPtr writer, Plugin * plugin)
{
  int rc;
  Plugin_Descriptor * descr = plugin->descr;
  rc = xmlTextWriterStartElement (writer, BAD_CAST "Desriptor");
  rc = xmlTextWriterWriteFormatElement (writer,
                                        BAD_CAST "author",
                                        "%s",
                                        descr->author);
  rc = xmlTextWriterWriteFormatElement (writer,
                                        BAD_CAST "name",
                                        "%s",
                                        descr->name);
  rc = xmlTextWriterWriteFormatElement (writer,
                                        BAD_CAST "website",
                                        "%s",
                                        descr->website);
  rc = xmlTextWriterWriteFormatElement (writer,
                                        BAD_CAST "category",
                                        "%s",
                                        descr->category);
  rc = xmlTextWriterWriteFormatElement (writer,
                                        BAD_CAST "num_audio_ins",
                                        "%d",
                                        descr->num_audio_ins);
  rc = xmlTextWriterWriteFormatElement (writer,
                                        BAD_CAST "num_midi_ins",
                                        "%d",
                                        descr->num_midi_ins);
  rc = xmlTextWriterWriteFormatElement (writer,
                                        BAD_CAST "num_audio_outs",
                                        "%d",
                                        descr->num_audio_outs);
  rc = xmlTextWriterWriteFormatElement (writer,
                                        BAD_CAST "num_midi_outs",
                                        "%d",
                                        descr->num_midi_outs);
  rc = xmlTextWriterWriteFormatElement (writer,
                                        BAD_CAST "num_ctrl_ins",
                                        "%d",
                                        descr->num_ctrl_ins);
  rc = xmlTextWriterWriteFormatElement (writer,
                                        BAD_CAST "num_ctrl_outs",
                                        "%d",
                                        descr->num_ctrl_outs);
  rc = xmlTextWriterWriteFormatElement (writer,
                                        BAD_CAST "arch",
                                        "%d",
                                        descr->arch);
  rc = xmlTextWriterWriteFormatElement (writer,
                                        BAD_CAST "protocol",
                                        "%d",
                                        descr->protocol);
  rc = xmlTextWriterWriteFormatElement (writer,
                                        BAD_CAST "path",
                                        "%s",
                                        descr->path);
  rc = xmlTextWriterWriteFormatElement (writer,
                                        BAD_CAST "uri",
                                        "%s",
                                        descr->uri);

  rc = xmlTextWriterEndElement(writer);
}

static void
write_lv2_port (xmlTextWriterPtr writer, LV2_Port * port)
{
  int rc;
  rc = xmlTextWriterStartElement (writer, BAD_CAST "LV2Port");
  rc = xmlTextWriterWriteFormatAttribute (writer,
                                        BAD_CAST "index",
                                        "%d",
                                        port->index);
  if (port->port)
    {
      write_port_id (writer, port->port);
    }

  rc = xmlTextWriterEndElement(writer);
}

static void
write_lv2_plugin (xmlTextWriterPtr writer, LV2_Plugin * plugin)
{
  int rc;
  rc = xmlTextWriterStartElement (writer, BAD_CAST "LV2Plugin");
  rc = xmlTextWriterStartElement (writer, BAD_CAST "LV2Ports");
  for (int i = 0; i < plugin->num_ports; i++)
    {
      write_lv2_port (writer, &plugin->ports[i]);
    }
  rc = xmlTextWriterEndElement(writer);

  rc = xmlTextWriterEndElement(writer);
}

static void
write_plugin (xmlTextWriterPtr writer, Plugin * plugin, int pos)
{
  int rc;
  rc = xmlTextWriterStartElement (writer, BAD_CAST "Plugin");
  rc = xmlTextWriterWriteFormatAttribute(writer,
                                         BAD_CAST "pos",
                                         "%d",
                                         pos);
  rc = xmlTextWriterWriteFormatAttribute(writer,
                                         BAD_CAST "id",
                                         "%d",
                                         plugin->id);
  write_plugin_descr (writer, plugin);
  rc = xmlTextWriterStartElement (writer, BAD_CAST "in_ports");
  for (int i = 0; i < plugin->num_in_ports; i++)
    {
      write_port_id (writer, plugin->in_ports[i]);
    }
  rc = xmlTextWriterEndElement(writer);
  rc = xmlTextWriterStartElement (writer, BAD_CAST "out_ports");
  for (int i = 0; i < plugin->num_out_ports; i++)
    {
      write_port_id (writer, plugin->out_ports[i]);
    }
  rc = xmlTextWriterEndElement(writer);
  rc = xmlTextWriterStartElement (writer, BAD_CAST "unknown_ports");
  for (int i = 0; i < plugin->num_unknown_ports; i++)
    {
      write_port_id (writer, plugin->unknown_ports[i]);
    }
  rc = xmlTextWriterEndElement(writer);
  rc = xmlTextWriterWriteFormatElement (writer,
                                        BAD_CAST "enabled",
                                        "%d",
                                        plugin->enabled);
  if (plugin->descr->protocol == PROT_LV2)
    {
      write_lv2_plugin (writer, (LV2_Plugin *) plugin->original_plugin);
    }

  rc = xmlTextWriterEndElement(writer);
}

static void
write_position (xmlTextWriterPtr writer, Position * pos, char * name)
{
  int rc;
  rc = xmlTextWriterStartElement (writer, BAD_CAST "Position");
  rc = xmlTextWriterWriteFormatAttribute(writer, BAD_CAST "id",
                                         "%s", name);
  rc = xmlTextWriterWriteFormatElement (writer,
                                        BAD_CAST "bars",
                                        "%d",
                                        pos->bars);
  rc = xmlTextWriterWriteFormatElement (writer,
                                        BAD_CAST "beats",
                                        "%d",
                                        pos->beats);
  rc = xmlTextWriterWriteFormatElement (writer,
                                        BAD_CAST "quarter_beats",
                                        "%d",
                                        pos->quarter_beats);
  rc = xmlTextWriterWriteFormatElement (writer,
                                        BAD_CAST "ticks",
                                        "%d",
                                        pos->ticks);
  rc = xmlTextWriterWriteFormatElement (writer,
                                        BAD_CAST "frames",
                                        "%d",
                                        pos->frames);

  rc = xmlTextWriterEndElement(writer);

}

static void
write_region_id (xmlTextWriterPtr writer, Region * region)
{
  int rc;
  rc = xmlTextWriterWriteFormatElement (writer,
                                        BAD_CAST "Region",
                                        "",
                                        NULL);
  rc = xmlTextWriterWriteFormatAttribute(writer, BAD_CAST "id",
                                         "%d", region->id);
  rc = xmlTextWriterEndElement(writer);
}

static void
write_region (xmlTextWriterPtr writer, Region * region)
{
  int rc;
  rc = xmlTextWriterStartElement (writer, BAD_CAST "Region");
  rc = xmlTextWriterWriteFormatElement (writer,
                                        BAD_CAST "name",
                                        "%s",
                                        region->name);
  write_position (writer, &region->start_pos, "start_pos");
  write_position (writer, &region->start_pos, "end_pos");
  if (region->track->channel->type == CT_AUDIO)
    {
      //
    }
  else if (region->track->channel->type == CT_MIDI)
    {
      if (region->linked_region)
        write_region_id (writer, region->linked_region);
      else
        {
          rc = xmlTextWriterWriteFormatElement (writer,
                                                BAD_CAST "num_midi_notes",
                                                "%d",
                                                region->num_midi_notes);

          rc = xmlTextWriterStartElement (writer, BAD_CAST "MidiNotes");
          /* TODO write midi notes */
          rc = xmlTextWriterEndElement(writer);
        }
    }

  rc = xmlTextWriterEndElement(writer);

}

static void
write_track (xmlTextWriterPtr writer, Track * track)
{
  int rc;
  rc = xmlTextWriterStartElement (writer, BAD_CAST "Track");
  rc = xmlTextWriterWriteFormatElement (writer,
                                        BAD_CAST "num_regions",
                                        "%d",
                                        track->num_regions);
  rc = xmlTextWriterStartElement (writer, BAD_CAST "Regions");
  for (int i = 0; i < track->num_regions; i++)
    {
      write_region (writer, track->regions[i]);
    }
  rc = xmlTextWriterEndElement(writer);

  rc = xmlTextWriterEndElement(writer);

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
     if (channel->strip[i])
       write_plugin (writer, channel->strip[i], i);
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
  rc = xmlTextWriterStartElement (writer, BAD_CAST "StereoIn");
  write_port_id (writer, channel->stereo_in->l);
  write_port_id (writer, channel->stereo_in->r);
  rc = xmlTextWriterEndElement(writer);
  rc = xmlTextWriterStartElement (writer, BAD_CAST "MidiIn");
  write_port_id (writer, channel->midi_in);
  rc = xmlTextWriterEndElement(writer);
  rc = xmlTextWriterStartElement (writer, BAD_CAST "PianoRoll");
  write_port_id (writer, channel->piano_roll);
  rc = xmlTextWriterEndElement(writer);
  rc = xmlTextWriterStartElement (writer, BAD_CAST "StereoOut");
  write_port_id (writer, channel->stereo_in->l);
  write_port_id (writer, channel->stereo_in->r);
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

void
xml_write_ports (const char * file)
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

  /* write ports */
  rc = xmlTextWriterStartElement(writer, BAD_CAST "Ports");
  rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "ver",
                                   BAD_CAST PROJECT_XML_VER);

  /* write port */
  for (int i = 0; i < PROJECT->num_ports; i++)
    {
      write_port (writer, PROJECT->ports[i]);
    }

  /* Close the element named FOOTER. */
  rc = xmlTextWriterEndElement(writer);
  if (rc < 0) {
      g_warning ("Error at xmlTextWriterEndElement\n");
      return;
  }

  /* Here we could close the elements ORDER and EXAMPLE using the
   * function xmlTextWriterEndElement, but since we do not want to
   * write any other elements, we simply call xmlTextWriterEndDocument,
   * which will do all the work. */
  rc = xmlTextWriterEndDocument(writer);
  if (rc < 0) {
      g_warning ("Error at xmlTextWriterEndDocument\n");
      return;
  }

  xmlFreeTextWriter(writer);

  /*
   * Cleanup function for the XML library.
   */
  xmlCleanupParser();

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
  rc = xmlTextWriterWriteComment(writer, BAD_CAST "The project");
  rc = xmlTextWriterStartElement(writer, BAD_CAST "Project");
  rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "ver",
                                   BAD_CAST PROJECT_XML_VER);

  /* write channels */
  rc = xmlTextWriterStartElement(writer, BAD_CAST "Channels");
  for (int i = 0; i < MIXER->num_channels; i++)
    {
      write_channel (writer, MIXER->channels[i]);
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


