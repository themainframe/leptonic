#include "log.h"
#include "vospi.h"
#include <time.h>
#include <stdlib.h>
#include <gtk/gtk.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <pthread.h>
#include <assert.h>
#include <string.h>

static GtkWidget* window;
static GtkWidget* image;

/* A lock protecting accesses to the current frame */
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

/* The current frame, ready to be displayed. */
static vospi_frame_t current_frame;

/* The Lepton resolution */
#define LEP_WIDTH 160
#define LEP_HEIGHT 120

/* The default window size */
#define WIND_WIDTH 320
#define WIND_HEIGHT 240

/**
 * See if there's a frame available and draw it to the screen if possible.
 */
static gboolean refresh(gpointer data)
{
  vospi_frame_t frame;

  // Obtain the mutex on the current frame for reading, copy out the current frame for rendering
  pthread_mutex_lock(&lock);
  memcpy(&frame, &current_frame, sizeof(vospi_frame_t));

  // Release the current frame mutex
  pthread_mutex_unlock(&lock);

  GdkPixbuf* pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, 0, 8, LEP_WIDTH, LEP_HEIGHT);
  int rowstride = gdk_pixbuf_get_rowstride(pixbuf);
  guchar *pixels = gdk_pixbuf_get_pixels(pixbuf);

  uint16_t values[LEP_WIDTH * LEP_HEIGHT];

  // Perform autoscaling of values and produce a maximum and minimum, as well as the bitmap
  uint16_t offset = 0, max = 0, min = UINT16_MAX;
  for (uint8_t seg = 0; seg < VOSPI_SEGMENTS_PER_FRAME; seg ++) {
    for (uint8_t pkt = 0; pkt < VOSPI_PACKETS_PER_SEGMENT_NORMAL; pkt ++) {
      for (uint8_t sym = 0; sym < VOSPI_PACKET_SYMBOLS; sym += 2) {
        values[offset] = frame.segments[seg].packets[pkt].symbols[sym] << 8 | frame.segments[seg].packets[pkt].symbols[sym + 1];
        if (min >= values[offset]) {
          min = values[offset];
        }
        if (max <= values[offset]) {
          max = values[offset];
        }
        offset ++;
      }
    }
  }

  // Calculate autoscale range
  uint16_t range = max - min, p_value;

  // Draw the frame into the pixbuf
  offset = 0;
  for (uint16_t val_i = 0; val_i < LEP_WIDTH * LEP_HEIGHT; val_i ++) {
    p_value = (int)(((float)values[val_i] - (float)min) / (float)range * 254);
    pixels[offset] = p_value;
    pixels[offset + 1] = p_value;
    pixels[offset + 2] = p_value;
    offset += 3;
  }

  // Scale the pixbuf up to the window size
  GdkPixbuf* w_pixbuf = gdk_pixbuf_scale_simple(pixbuf, WIND_WIDTH, WIND_HEIGHT, GDK_INTERP_BILINEAR);
  gtk_image_set_from_pixbuf(GTK_IMAGE(image), w_pixbuf);

  return TRUE;
}


/**
 * Read frames from the device.
 */
void* get_frames_from_device(void* spidev_path_ptr)
{

  char* spidev_path = (char*)spidev_path_ptr;
  int spi_fd;

  // Declare a static frame to use as a scratch space to avoid locking the framebuffer while
  // we're waiting for a new frame
  vospi_frame_t frame;

  // Initialise the segments
  for (int seg = 0; seg < VOSPI_SEGMENTS_PER_FRAME; seg ++) {
    frame.segments[seg].packet_count = VOSPI_PACKETS_PER_SEGMENT_NORMAL;
  }

  // Open the spidev device
  log_info("opening SPI device... %s", spidev_path);
  if ((spi_fd = open(spidev_path, O_RDWR)) < 0) {
    log_fatal("SPI: failed to open device - check permissions & spidev enabled");
    exit(-1);
  }

  // Initialise the VoSPI interface
  if (vospi_init(spi_fd, 20000000) == -1) {
      log_fatal("SPI: failed to condition SPI device for VoSPI use.");
      exit(-1);
  }

  // Synchronise, then receive frames forever
  do {

    log_info("aquiring VoSPI synchronisation");

    if (0 == sync_and_transfer_frame(spi_fd, &frame)) {
      log_error("failed to obtain frame from device.");
      exit(-10);
    }

    log_info("VoSPI stream synchronised");

    do {

        if (!transfer_frame(spi_fd, &frame)) {
          break;
        }

        // Lock the current frame, we're writing to it
        pthread_mutex_lock(&lock);

        // Copy the newly-received frame into place
        memcpy(&current_frame, &frame, sizeof(vospi_frame_t));

        // Unlock the current frame, it's ready for use
        pthread_mutex_unlock(&lock);

    } while (1); // While synchronised
  } while (1);  // Forever


}

/**
 * Initialise everything.
 */
int main (int argc, char** argv)
{
  pthread_t get_frames_thread, send_frames_to_socket_thread;

  // Set the log level
  log_set_level(LOG_INFO);

  // Check we have enough arguments to work
  if (argc < 2) {
    log_error("Can't start - SPI device file path must be specified.");
    exit(-1);
  }

  log_info("Creating get_frames_from_device thread");
  if (pthread_create(&get_frames_thread, NULL, get_frames_from_device, argv[1])) {
    log_fatal("Error creating get_frames_from_device thread");
    return 1;
  }

  srand(time(NULL));
  gtk_init(&argc, &argv);
  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_default_size(GTK_WINDOW(window), WIND_WIDTH, WIND_HEIGHT);
  image = gtk_image_new();
  gtk_container_add(GTK_CONTAINER(window), image);
  gtk_widget_show_all(window);
  g_timeout_add(100, (GSourceFunc) refresh, NULL);
  gtk_main();

  pthread_join(get_frames_thread, NULL);
  pthread_join(send_frames_to_socket_thread, NULL);
}
