#include "log.h"
#include "vospi.h"
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>
#include <assert.h>
#include <string.h>
#include <zmq.h>

#define ZMQ_SOCKET_SPEC "tcp://*:5555"

// The size of the circular frame buffer
#define FRAME_BUF_SIZE 8

// Positions of the reader and writer in the frame buffer
int reader = 0, writer = 0;

// semaphore tracking the number of frames available
sem_t count_sem;

// a lock protecting accesses to the frame buffer
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

// The frame buffer
vospi_frame_t* frame_buf[FRAME_BUF_SIZE];

/**
 * Read frames from the device into the circular buffer.
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

          pthread_mutex_lock(&lock);

          // Copy the newly-received frame into place
          memcpy(frame_buf[writer], &frame, sizeof(vospi_frame_t));

          // Move the writer ahead
          writer = (writer + 1) & (FRAME_BUF_SIZE - 1);

          // Unlock and post the space semaphore
          pthread_mutex_unlock(&lock);
          sem_post(&count_sem);

  		} while (1); // While synchronised
    } while (1);  // Forever

}

/**
 * Wait for reqests for frames on the ZMQ socket and respond with a frame each time.
 */
void* send_frames_to_socket(void* args)
{
    // Create the ZMQ context & socket
    void *context = zmq_ctx_new();
    void *responder = zmq_socket(context, ZMQ_REP);
    zmq_bind(responder, ZMQ_SOCKET_SPEC);

    // Declare a static frame
    vospi_frame_t next_frame;

    // Declare a static buffer to copy frame data into for sending
    unsigned char message_buf[VOSPI_SEGMENTS_PER_FRAME * VOSPI_PACKETS_PER_SEGMENT_NORMAL * VOSPI_PACKET_SYMBOLS];

    while (1) {

      // Receive requests
      char req_buf[10];
      zmq_recv(responder, req_buf, 10, 0);

      // Wait if there are no new frames to transmit
      sem_wait(&count_sem);

      // Lock the data structure to prevent new frames being added while we're reading this one
      pthread_mutex_lock(&lock);

      // Copy the next frame out ready to transmit
      memcpy(&next_frame, frame_buf[reader], sizeof(vospi_frame_t));

      // Move the reader ahead
      reader = (reader + 1) & (FRAME_BUF_SIZE - 1);

      // Unlock data structure
      pthread_mutex_unlock(&lock);

      // Prepare the message buffer
      void* message_buf_pos = &message_buf;
      for (int seg = 0; seg < VOSPI_SEGMENTS_PER_FRAME; seg ++) {
        for (int pkt = 0; pkt < VOSPI_PACKETS_PER_SEGMENT_NORMAL; pkt ++) {
          // Copy each packet into the message buffer
          memcpy(
            message_buf_pos,
            next_frame.segments[seg].packets[pkt].symbols,
            VOSPI_PACKET_SYMBOLS
          );
          message_buf_pos += VOSPI_PACKET_SYMBOLS;
        }
      }

      // Send the message
      zmq_send(responder, message_buf, sizeof(message_buf), 0);
    }
}

/**
 * Main entry point for example.
 *
 * This example creates a Unix Domain Socket server that listens for a connection on a socket (/tmp/lepton.sock).
 * Once a connection is created, the server frames from the Lepton to the client.
 */
int main(int argc, char *argv[])
{
  pthread_t get_frames_thread, send_frames_to_socket_thread;

  // Set the log level
	log_set_level(LOG_INFO);

  // Setup semaphores
  sem_init(&count_sem, 0, 0);

  // Check we have enough arguments to work
  if (argc < 2) {
    log_error("Can't start - SPI device file path must be specified.");
    exit(-1);
  }

	// Allocate space to receive the segments in the circular buffer
	log_info("preallocating space for segments...");
	for (int frame = 0; frame < FRAME_BUF_SIZE; frame ++) {
		frame_buf[frame] = malloc(sizeof(vospi_frame_t));
    for (int seg = 0; seg < VOSPI_SEGMENTS_PER_FRAME; seg ++) {
      frame_buf[frame]->segments[seg].packet_count = VOSPI_PACKETS_PER_SEGMENT_NORMAL;
    }
	}

  log_info("Creating get_frames_from_device thread");
  if (pthread_create(&get_frames_thread, NULL, get_frames_from_device, argv[1])) {
    log_fatal("Error creating get_frames_from_device thread");
    return 1;
  }

  log_info("Creating send_frames_to_socket thread");
  if (pthread_create(&send_frames_to_socket_thread, NULL, send_frames_to_socket, NULL)) {
    log_fatal("Error creating send_frames_to_socket thread");
    return 1;
  }

  pthread_join(get_frames_thread, NULL);
  pthread_join(send_frames_to_socket_thread, NULL);
}
