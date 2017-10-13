(function($) {
  $(function () {

      var socket = io();
      var canvas = document.getElementById('canvas');
      var ctx = canvas.getContext('2d');
      ctx.fillRect(0, 0, canvas.width, canvas.height);
      var imageData = ctx.getImageData(0, 0, canvas.width, canvas.height);

      socket.on('frame', function (msg) {

        // Create an array of unsigned integers from the array
        var intData = new Uint16Array(msg);

        // Take the range of the values
        var max = Math.max.apply(Math, intData);
        var min = Math.min.apply(Math, intData);
        var range = max - min;

        // Build some image data with the integers
        for (var c = 0; c < intData.length; c ++) {
          // Normalise each pixel value first
          var pixelValue = parseInt((intData[c] - min) / range * 255);
          // var pixelValue = parseInt(intData[c] / 16384 * 255);
          imageData.data[(c * 4) + 0] = pixelValue;
          imageData.data[(c * 4) + 1] = pixelValue;
          imageData.data[(c * 4) + 2] = pixelValue;
        }

        ctx.putImageData(imageData, 0, 0);

      });

  });
})(jQuery);
