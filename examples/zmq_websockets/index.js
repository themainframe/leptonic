const express = require('express');
const app = express();
const path = require('path');
const http = require('http').Server(app);
const io = require('socket.io')(http);
const zmq = require('zeromq');

// Define a requester
let requester = zmq.socket('req');
requester.connect('tcp://10.10.1.13:5555')

// Upon Lepton data arriving, build up a frame
requester.on('message', (data) => {
  data.swap16();
  io.volatile.emit('frame', data, {for: 'everyone'});
});

// Request a single frame now
setInterval(() => {
  requester.send('hello');
}, 1000/9);

app.get('/', (req, res) => {
  res.sendFile(__dirname + '/index.html');
});

app.use('/assets', express.static(path.join(__dirname, 'assets')))

http.listen(3000);
