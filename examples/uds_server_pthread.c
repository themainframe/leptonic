#include "log.h"
#include "vospi.h"
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <pthread.h>
#include <semaphore.h>

#define SOCKET_PATH "/tmp/lepton.sock"

// The size of the circular frame buffer
#define FRAME_BUF_SIZE 16

// Positions of the reader and writer in the frame buffer
int reader = 0, writer = 0;

// semaphores tracking the available spaces and number of frames
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
  	if (vospi_init(spi_fd, 18000000) == -1) {
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

          // Verify the frame
          int need_resync = 0;
          for (int seg = 0; seg < VOSPI_SEGMENTS_PER_FRAME; seg ++) {
            if (frame.segments[seg].packets[20].id >> 12 != seg + 1) {
              // Skip this entire frame
              log_info("Received an invalid frame - seg %d TTT was %d", seg, frame.segments[seg].packets[20].id >> 12);
              need_resync = 1;
              break;
            }
          }

          // Resync if any part of the frame is broken
          if (need_resync) {
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
 * Listen for connections on a unix domain socket, stream frames out once we obtain a connection.
 */
void* send_frames_to_socket(void* args)
{
    int sock, client;
    struct sockaddr_un addr;

    // Declare a static frame to use as a scratch space to avoid locking the framebuffer while
    // we're transmitting our frame
    vospi_frame_t frame;

    // Unlink the existing socket
    unlink(SOCKET_PATH);

    // Open the socket file
    sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock < 0) {
    		log_fatal("Socket: failed to open socket");
    		exit(-1);
    }

    // Set up the socket
  	log_info("creating socket...");
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);
    bind(sock, (struct sockaddr*)&addr, sizeof(addr));

    // Start listening
    if (listen(sock, 5) == -1) {
      log_fatal("error listening on socket");
      exit(-1);
    }

    log_info("waiting for a connection...");
    while (1) {

      // Accept the client connection
      if ((client = accept(sock, NULL, NULL)) == -1) {
        log_warn("failed to accept connection");
        continue;
      }

      log_info("Got connection: %d", client);

      // While we're connected
      while (client > 0) {

        // Wait if there are no new frames to transmit
        sem_wait(&count_sem);

        // Lock the data structure to prevent new frames being added while we're reading this one
        pthread_mutex_lock(&lock);

        // Copy the next frame out ready to transmit
        memcpy(&frame, frame_buf[reader], sizeof(vospi_frame_t));

        // Move the reader ahead
        reader = (reader + 1) & (FRAME_BUF_SIZE - 1);

        // Unlock data structure
        pthread_mutex_unlock(&lock);

        // Transmit the frame
        for (int seg = 0; seg < VOSPI_SEGMENTS_PER_FRAME; seg ++) {

          // Send each packet from the segment
          for (int pkt = 0; pkt < VOSPI_PACKETS_PER_SEGMENT_NORMAL; pkt ++) {
            if (send(client, frame.segments[seg].packets[pkt].symbols, VOSPI_PACKET_SYMBOLS, MSG_NOSIGNAL) < 0) {
              log_warn("socket disconnected.");
              client = 0;
              break;
            }
          }

          // If the client disconnected, don't send them any more segments
          if (!client) {
            break;
          }
        }

      }

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
