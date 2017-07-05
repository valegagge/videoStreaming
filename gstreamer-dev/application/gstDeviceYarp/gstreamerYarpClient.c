#include <gst/gst.h>
#include <glib.h>


static gboolean
bus_call (GstBus     *bus,
          GstMessage *msg,
          gpointer    data)
{
  GMainLoop *loop = (GMainLoop *) data;

  switch (GST_MESSAGE_TYPE (msg)) {

    case GST_MESSAGE_EOS:
      g_print ("End of stream\n");
      g_main_loop_quit (loop);
      break;

    case GST_MESSAGE_ERROR: {
      gchar  *debug;
      GError *error;

      gst_message_parse_error (msg, &error, &debug);
      g_free (debug);

      g_printerr ("Error: %s\n", error->message);
      g_error_free (error);

      g_main_loop_quit (loop);
      break;
    }
    default:
      break;
  }

  return TRUE;
}


static gboolean link_videosrc2convert(GstElement *e1, GstElement *e2)
{
    gboolean link_ok;
    GstCaps *caps;

    caps = gst_caps_new_simple("video/x-raw",
                               "format", G_TYPE_STRING, "I420",
                                "width", G_TYPE_INT, 640,
                                "height", G_TYPE_INT,480,
                                NULL);
//"framerate", GST_TYPE_FRACTION, 30, 1

    link_ok = gst_element_link_filtered(e1, e2, caps);
    if(!link_ok)
    {
        g_print("failed link videosrc2convert with caps!!\n");
    }
    else
    {
        g_print("link videosrc2convert with caps OK!!!!!!\n");
    }

    return (link_ok);
}


static gboolean link_convert2yarp(GstElement *e1, GstElement *e2)
{
    gboolean link_ok;
    GstCaps *caps;

    caps = gst_caps_new_simple("video/x-raw",
                               "format", G_TYPE_STRING, "RGB",
                              NULL);


    link_ok = gst_element_link_filtered(e1, e2, caps);
    if(!link_ok)
    {
        g_print("failed link convert2yarp with caps!!\n");
    }
    else
    {
        g_print("link convert2yarp with caps OK!!!!!!\n");
    }

    return (link_ok);
}



int
main (int   argc,
      char *argv[])
{
  GMainLoop *loop;

  GstElement *pipeline, *source, *sink, *my_yarpdevice, *convert;
  GstBus *bus;
  guint bus_watch_id;

  /* Initialisation */
  gst_init (&argc, &argv);

  loop = g_main_loop_new (NULL, FALSE);


  /* Create gstreamer elements */
  pipeline = gst_pipeline_new ("video-player");
  source   = gst_element_factory_make ("autovideosrc",      "video-source");
  convert  = gst_element_factory_make ("videoconvert",       "pippo-convert");
  my_yarpdevice =  gst_element_factory_make ("yarpdevice",  "yarp-device");
//  sink     = gst_element_factory_make ("autovideosink", "video-output");
  sink     = gst_element_factory_make ("glimagesink", "video-output"); //because use RGB space
 
  if (!pipeline || !source || !convert || !my_yarpdevice || !sink) {
    g_printerr ("One element could not be created. Exiting.\n");
    return -1;
  }

  /* ================== Set up the pipeline ==========================*/


  /* we add a message handler */
  bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));
  bus_watch_id = gst_bus_add_watch (bus, bus_call, loop);
  gst_object_unref (bus);

  g_print("try to add elements to pipeline..... \n");
  /* we add all elements into the pipeline */
  gst_bin_add_many (GST_BIN (pipeline),
                    source, convert, my_yarpdevice, sink, NULL);

  g_print("Elements were been added to pipeline!");


  /* autovideosrc ! "video/x-raw, width=640, height=480, format=(string)I420" ! videoconvert ! 'video/x-raw, format=(string)RGB'  ! yarpdevice ! glimagesink */
    g_print("try to link_videosrc2convert..... \n");
  gboolean result = link_videosrc2convert (source, convert);
  if(!result)
    {
        return -1;
    }

    g_print("try to link_convert2yarp..... \n");
    result = link_convert2yarp(convert, my_yarpdevice);
    if(!result)
    {
        return -1;
    }

    gst_element_link(my_yarpdevice, sink);


  /* Set the pipeline to "playing" state*/
  g_print ("Now playing...\n");
  gst_element_set_state (pipeline, GST_STATE_PLAYING);


  /* Iterate */
  g_print ("Running...\n");
  g_main_loop_run (loop);


  /* Out of the main loop, clean up nicely */
  g_print ("Returned, stopping playback\n");
  gst_element_set_state (pipeline, GST_STATE_NULL);

  g_print ("Deleting pipeline\n");
  gst_object_unref (GST_OBJECT (pipeline));
  g_source_remove (bus_watch_id);
  g_main_loop_unref (loop);

  return 0;
}

