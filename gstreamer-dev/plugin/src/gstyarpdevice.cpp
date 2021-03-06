/*
 * GStreamer
 * Copyright (C) 2005 Thomas Vander Stichele <thomas@apestaart.org>
 * Copyright (C) 2005 Ronald S. Bultje <rbultje@ronald.bitfreak.net>
 * Copyright (C) 2017 icub <<user@hostname.org>>
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Alternatively, the contents of this file may be used under the
 * GNU Lesser General Public License Version 2.1 (the "LGPL"), in
 * which case the following provisions apply instead of the ones
 * mentioned above:
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/**
 * SECTION:element-yarpdevice
 *
 * FIXME:Describe yarpdevice here.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch -v -m fakesrc ! yarpdevice ! fakesink silent=TRUE
 * ]|
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gst/gst.h>

#include "gstyarpdevice.h"

//provo a creare un'immagine a partire da un frame
#include <gdk-pixbuf/gdk-pixbuf.h>

#include <yarp/os/Stamp.h>
#include <yarp/os/Time.h>

#include <string.h>

using namespace yarp::os;
using namespace yarp::sig;

GST_DEBUG_CATEGORY_STATIC (gst_yarp_device_debug);
#define GST_CAT_DEFAULT gst_yarp_device_debug




#define FRAME_SIZE_WIDTH   640
#define FRAME_SIZE_HEIGHT  480

//#define FRAME_SIZE_WIDTH   2560
//#define FRAME_SIZE_HEIGHT  720


/* Filter signals and args */
enum
{
  /* FILL ME */
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_SILENT
};

/* the capabilities of the inputs and outputs.
 *
 * describe the real formats here.
 */
static GstStaticPadTemplate sink_factory = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,// input
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("ANY")
    );

static GstStaticPadTemplate src_factory = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC, //output
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("ANY")
    );

#define gst_yarp_device_parent_class parent_class
G_DEFINE_TYPE (GstyarpDevice, gst_yarp_device, GST_TYPE_ELEMENT);

static void gst_yarp_device_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_yarp_device_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);

static gboolean gst_yarp_device_sink_event (GstPad * pad, GstObject * parent, GstEvent * event);
static GstFlowReturn gst_yarp_device_chain (GstPad * pad, GstObject * parent, GstBuffer * buf);

/* GObject vmethod implementations */

/* initialize the yarpdevice's class */
static void
gst_yarp_device_class_init (GstyarpDeviceClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;

  gobject_class->set_property = gst_yarp_device_set_property;
  gobject_class->get_property = gst_yarp_device_get_property;

  g_object_class_install_property (gobject_class, PROP_SILENT,
      g_param_spec_boolean ("silent", "Silent", "Produce verbose output ?",
          FALSE, G_PARAM_READWRITE));

  gst_element_class_set_details_simple(gstelement_class,
    "yarpDevice",
    "FIXME:Generic",
    "FIXME:Generic Template Element",
    "icub <<user@hostname.org>>");

  gst_element_class_add_pad_template (gstelement_class,
      gst_static_pad_template_get (&src_factory));
  gst_element_class_add_pad_template (gstelement_class,
      gst_static_pad_template_get (&sink_factory));
}

/* initialize the new element
 * instantiate pads and add them to element
 * set pad calback functions
 * initialize instance structure
 */
static void
gst_yarp_device_init (GstyarpDevice * filter)
{
  filter->sinkpad = gst_pad_new_from_static_template (&sink_factory, "sink");
  gst_pad_set_event_function (filter->sinkpad,
                              GST_DEBUG_FUNCPTR(gst_yarp_device_sink_event));
  gst_pad_set_chain_function (filter->sinkpad,
                              GST_DEBUG_FUNCPTR(gst_yarp_device_chain));
  GST_PAD_SET_PROXY_CAPS (filter->sinkpad);
  gst_element_add_pad (GST_ELEMENT (filter), filter->sinkpad);

  filter->srcpad = gst_pad_new_from_static_template (&src_factory, "src");
  GST_PAD_SET_PROXY_CAPS (filter->srcpad);
  gst_element_add_pad (GST_ELEMENT (filter), filter->srcpad);

  filter->silent = FALSE;


  //***************** INIT YARP OBJS *******************************
  g_print("create network");
  filter->ynet_ptr = new yarp::os::Network();
  g_print("checking network ....");
  if(!NetworkBase::checkNetwork())
  {
        g_print("network down!!!");
        return;
  }

  g_print("network OK!!!");

  g_print("try to open yarp port ....");
  filter->count = 0;
  filter->yport_ptr = new yarp::os::BufferedPort <yarp::sig::ImageOf<yarp::sig::PixelRgb> >();
  if(!filter->yport_ptr->open("/gstreamer_yarp_port"))
  {
      g_print("error opening yarp port");
  }
  else
  {
      g_print("yarp port opened successfully!!");
  }
  //*****************************************************************

}

static void
gst_yarp_device_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstyarpDevice *filter = GST_YARPDEVICE (object);

  switch (prop_id) {
    case PROP_SILENT:
      filter->silent = g_value_get_boolean (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_yarp_device_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstyarpDevice *filter = GST_YARPDEVICE (object);

  switch (prop_id) {
    case PROP_SILENT:
      g_value_set_boolean (value, filter->silent);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

/* GstElement vmethod implementations */

/* this function handles sink events */
static gboolean
gst_yarp_device_sink_event (GstPad * pad, GstObject * parent, GstEvent * event)
{
  GstyarpDevice *filter;
  gboolean ret;

  filter = GST_YARPDEVICE (parent);

  GST_LOG_OBJECT (filter, "Received %s event: %" GST_PTR_FORMAT,
      GST_EVENT_TYPE_NAME (event), event);

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_CAPS:
    {
      GstCaps * caps;

      gst_event_parse_caps (event, &caps);
      /* do something with the caps */

      /* and forward */
      ret = gst_pad_event_default (pad, parent, event);
      break;
    }
    default:
      ret = gst_pad_event_default (pad, parent, event);
      break;
  }
  return ret;
}

/* chain function
 * this function does the actual processing
 */
static GstFlowReturn
gst_yarp_device_chain (GstPad * pad, GstObject * parent, GstBuffer * buf)
{
  GstyarpDevice *filter;
  GstMapInfo map;

  filter = GST_YARPDEVICE (parent);

  if (filter->silent == FALSE)
    g_print ("I'm plugged, therefore I'm in.\n");


  /*
  GstCaps * caps = gst_buffer_get_caps(buf); //questa funzione non vien trovata quando carico il plugin
  if(NULL == caps)
  {
        g_print("Caps are NULL !\n");
  }
  else
  {
      GST_LOG ("caps are %" GST_PTR_FORMAT, caps);
  }*/

  gsize size = gst_buffer_get_size(buf);

  gst_buffer_map (buf, &map, GST_MAP_READ);
  u_int8_t data_is_null = 1;

  if(map.data != NULL)
      data_is_null = 0;

  g_print("buf_size=%lu! start_offset=%lu, data_is_null=%d, map.size=%lu\n", size, buf->offset, data_is_null, map.size);

///////////////////////////////////////////////////////////////////////////
//////////////// USO PIXBUF LIB //////////////////////////////////////////
//   GError *error = NULL;
//   GdkPixbuf * pixbuf = gdk_pixbuf_new_from_data (map.data,
//         GDK_COLORSPACE_RGB, FALSE, 8, FRAME_SIZE_WIDTH,FRAME_SIZE_HEIGHT,
//         GST_ROUND_UP_4 (FRAME_SIZE_WIDTH * 3), NULL, NULL);
//
//     /* save the pixbuf */
//     gdk_pixbuf_save (pixbuf, "snapshot.png", "png", &error, NULL);
////////////////////////////////////////////////////////////////////////////////



//*****************************************************************************
//*********************** USO YARP *******************************************

    ImageOf<PixelRgb> &yframebuff = filter->yport_ptr->prepare();
    yframebuff.resize(FRAME_SIZE_WIDTH, FRAME_SIZE_HEIGHT);
    unsigned char *ydata_ptr = yframebuff.getRawImage();
    memcpy(ydata_ptr, map.data, FRAME_SIZE_WIDTH*FRAME_SIZE_HEIGHT*3);

    Stamp s = Stamp(filter->count,Time::now());
    //filter->yport_ptr->setStrict(0);
    filter->yport_ptr->setEnvelope(s);
    filter->yport_ptr->write();
    filter->count++;

//******************************************************************************

    gst_buffer_unmap (buf, &map);


  /* just push out the incoming buffer without touching it */
  return gst_pad_push (filter->srcpad, buf);
}


/* entry point to initialize the plug-in
 * initialize the plug-in itself
 * register the element factories and other features
 */
static gboolean
yarpdevice_init (GstPlugin * yarpdevice)
{
  /* debug category for fltering log messages
   *
   * exchange the string 'Template yarpdevice' with your description
   */
  GST_DEBUG_CATEGORY_INIT (gst_yarp_device_debug, "yarpdevice",
      0, "transforms each frame in yar::image");

  return gst_element_register (yarpdevice, "yarpdevice", GST_RANK_NONE,
      GST_TYPE_YARPDEVICE);
}

/* PACKAGE: this is usually set by autotools depending on some _INIT macro
 * in configure.ac and then written into and defined in config.h, but we can
 * just set it ourselves here in case someone doesn't use autotools to
 * compile this code. GST_PLUGIN_DEFINE needs PACKAGE to be defined.
 */
#ifndef PACKAGE
#define PACKAGE "myfirstyarpdevice"
#endif

/* gstreamer looks for this structure to register yarpdevices
 *
 * exchange the string 'Template yarpdevice' with your yarpdevice description
 */
GST_PLUGIN_DEFINE (
    GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    yarpdevice,
    "transforms each frame in yar::image",
    yarpdevice_init,
    //VERSION,
    "1.8.3",
    "LGPL",
    "GStreamer",
    "http://gstreamer.net/"
)
