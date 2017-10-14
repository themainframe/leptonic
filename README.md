# Leptonic

**A web GUI and several utilities for working with FLIR® Lepton® 3 LWIR camera modules.**

<img src="https://i.imgur.com/uULTPun.png">

---

Leptonic can:

* Stream a live view from a Lepton® 3 camera to any modern web browser.
* Be integrated into your own C projects to work with Lepton® 3 cameras.

Currently supported features:

* VoSPI interface (RAW14 format only, code could be adapted to support RGB888)
* Telemetry (header-location only)
* I2C CCI (partial)

Currently supported hardware:

* FLIR® Lepton® 3 (160x120 resolution) [[link](http://www.flir.com/uploadedFiles/OEM/Products/LWIR-Cameras/Lepton/Lepton-3-Engineering-Datasheet.pdf)]

The code is written in such a way that adapting it to work with the original 80x60 camera modules should be fairly straightforward. Unfortunately I do not currently own one to develop with, so haven't embarked upon this endeavor just yet.

Pull requests are of course welcomed!

## Dependencies

Recommended platform is Raspberry Pi with Raspbian. That's currently the only hardware I've tested this out on, but others should work fine too.

_Generally:_

* Linux, SPI support (via `spidev`)
* [ØMQ/ZeroMQ](http://zeromq.org/) (`libzmq3-dev`) for the camera interface/frontend IPC [[guide](http://zeromq.org/intro:get-the-software)]
* I2C support enabled for CCI interface use.
* NodeJS for the frontend stuff (`yarn`/`npm` to install dependencies)

_Specifically:_

* `spidev`'s `bufsiz` module parameter must be set to a value large enough to receive an entire Lepton® 3 VoSPI segment (10004 bytes with telemetry enabled, 9840 without). By default this isn't the case, so please do check before running the code.

## Building

* Check out the codebase.
* Make sure you've satisfied the dependencies, namely libzmq3-dev
* Run `make` to build the Leptonic IPC server. Run `make examples` to build the examples in the `examples` directory.
* Install the NodeJS dependencies with `yarn install` or `npm install` in the `frontend` subdirectory.

## Running

* Start the IPC server with `./bin/leptonic /dev/spidev0.0` (switching out the name of your `spidev` device file as appropriate). You may optionally also supply a socket address to bind to as a second argument (I.e. `tcp://127.0.0.1:5555`).
* Start the frontend app with `yarn start` or `npm start` from the `frontend` subdirectory. You may optionally also supply a socket address to connect to as a second argument (I.e. `tcp://127.0.0.1:5555`).
* The Web UI should now be running on port 3000.

## Performance

The camera communication process is extremely time-sensitive. There are strict parameters pertaining to how quickly frames and segments must be clocked out of the camera's SPI interface. Any slowdowns/scheduling caused by a master based on a multitasking OS such as Linux can cause the code to lose VoSPI synchronisation. While my code does reacquire synchronisation immediately, this does cause a visible amount of frame-drop in the output.

Empirically, I've found that the Raspberry Pi 3 Model B struggles a little running _both_ the camera interface and the frontend together. You might find it best to run the frontend server on a separate machine and have the ØMQ traffic go over the network.
