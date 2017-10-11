const app = require('express')();
const http = require('http').Server(app);
const io = require('socket.io')(http);
const net = require('net');

// Define a buffer to store the current frame
const FRAME_SIZE = 38400;
let frameBuffer = Buffer.alloc(FRAME_SIZE);
let frameBufferSize = 0;

// Connect to the Unix Socket and await Lepton data
const client = net.createConnection({ path: "/tmp/lepton.sock" });

// Upon Lepton data arriving, build up a frame
client.on('data', (data) => {

  frameBufferSize += data.copy(frameBuffer, frameBufferSize);

  // When we've received enough bytes
  if (frameBufferSize == frameBuffer.length) {

    // Byteswap the frame in-place
    frameBuffer.swap16();

    // Build the frame and dispatch it to everyone
    io.volatile.emit('frame', frameBuffer, {for: 'everyone'});

    // Start working on a new frame
    frameBufferSize = 0;
    frameBuffer = Buffer.alloc(FRAME_SIZE);
  }

});

app.get('/', (req, res) => {
  res.sendFile(__dirname + '/index.html');
});

http.listen(3000);
