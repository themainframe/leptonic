(function($) {

  function setupNavbarHandlers() {

    // Get all "navbar-burger" elements
    var $navbarBurgers = Array.prototype.slice.call(document.querySelectorAll('.navbar-burger'), 0);

    // Check if there are any navbar burgers
    if ($navbarBurgers.length > 0) {

      // Add a click event on each of them
      $navbarBurgers.forEach(function ($el) {
        $el.addEventListener('click', function () {

          // Get the target from the "data-target" attribute
          var target = $el.dataset.target;
          var $target = document.getElementById(target);

          // Toggle the class on both the "navbar-burger" and the "navbar-menu"
          $el.classList.toggle('is-active');
          $target.classList.toggle('is-active');

        });
      });
    }

  }

  $(function () {

      setupNavbarHandlers();

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
