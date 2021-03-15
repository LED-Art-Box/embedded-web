<template>
  <div class="canvas">
    <canvas
      id="canvas"
      ref="canvas"
      width="16"
      height="16"
      @click="onCanvasClick"
      @mousedown="onCanvasMouseDown"
      @mousemove="onCanvasMouseMove"
      @mouseup="onCanvasMouseUp"
    >
    </canvas>
    <p>color is {{ color }}</p>
    <form>
      <label for="draw_color">Choose color: </label>
      <input v-model="color" type="color" id="draw_color" />
    </form>
  </div>
</template>

<script>
import MQTTService from '../services/MQTTService.js'

function initBuffer(size, color) {
  return Array.from({ length: size }, () =>
    Array.from({ length: size }, () => color)
  );
}

export default {
  name: "Canvas",
  data() {
    return {
      color: "#ffffff",
      buffer: initBuffer(16, "#000000"),
      mqtt: null,
      mouseDrawing: false,
    };
  },
  created() {
    this.mqtt = new MQTTService();

    this.mqtt.registerPixelEventHandler((x, y, color) => this.onMessageArrived(x, y, color));

    this.mqtt.connect();
  },
  mounted() {
    var canvas = this.$refs.canvas;
    var context = canvas.getContext("2d");

    context.fillStyle = "black";
    context.fillRect(0, 0, canvas.width, canvas.height);
  },
  methods: {
    onCanvasClick(e) {
      var [x, y] = this.canvasCoordinates(e);

      if (this.buffer[x][y] != this.color) {
        this.buffer[x][y] = this.color;
        this.drawBlock(x, y, this.color);
        this.publishMessage(x, y, this.color);
      }
    },
    onCanvasMouseDown() {
      this.mouseDrawing = true;
    },
    onCanvasMouseMove(e) {
      if (this.mouseDrawing) {
        var [x, y] = this.canvasCoordinates(e);

        if (this.buffer[x][y] != this.color) {
          this.buffer[x][y] = this.color;
          this.drawBlock(x, y, this.color);
          this.publishMessage(x, y, this.color);
        }
      }
    },
    onCanvasMouseUp() {
      this.mouseDrawing = false;
    },
    onMessageArrived(x, y, color) {
      if (this.buffer[x][y] != color) {
        this.buffer[x][y] = color;
        this.drawBlock(x, y, color);
      }
    },
    drawBlock(x, y, color) {
      var context = this.$refs.canvas.getContext("2d");
      console.log("drawing with " + color);

      context.fillStyle = color;
      context.fillRect(x, y, 1, 1);
    },
    canvasPixels() {
      var canvas = this.$refs.canvas;
      var context = canvas.getContext("2d");

      var pixel_width = context.canvas.clientWidth / canvas.width;
      var pixel_height = context.canvas.clientHeight / canvas.height;

      return [pixel_width, pixel_height];
    },
    canvasCoordinates(event) {
      var rect = this.$refs.canvas.getBoundingClientRect();
      var [pixel_width, pixel_height] = this.canvasPixels();
      var x = Math.floor((event.clientX - rect.left) / pixel_width);
      var y = Math.floor((event.clientY - rect.top) / pixel_height);

      return [x, y];
    },
    publishMessage(x, y, color) {
      this.mqtt.publishPixel(x, y, color);
    },
  },
};
</script>

<!-- Add "scoped" attribute to limit CSS to this component only -->
<style scoped>
canvas {
  margin-top: 20px;
  outline: white 3px solid;
  width: 160mm;
  height: 160mm;
  image-rendering: -moz-crisp-edges;
  image-rendering: pixelated;
  margin-bottom: 10px;
  border: 1px solid black;
}
</style>
