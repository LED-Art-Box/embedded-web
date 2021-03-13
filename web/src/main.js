import Vue from 'vue'
import App from './App.vue'
import MQTT from 'paho-mqtt'

Vue.config.productionTip = false

new Vue({
  render: h => h(App),
}).$mount('#app')


const draw_topic = 'lieblingswelt/draw';
const connect_topic = 'lieblingswelt/draw/connect';
var canvas, context, client, active;

/* exported init */
function init() {
    client = new MQTT.Client('wss://broker.emqx.io:8084/mqtt', Math.random().toString(36).substring(20));

    canvas = document.getElementById("canvas");
    context = canvas.getContext("2d");

    client.onConnectionLost = onConnectionLost;
    client.onMessageArrived = onMessageArrived;

    client.connect({ onSuccess: onConnect });

    var pixel_width = context.canvas.clientWidth / canvas.width;
    var pixel_height = context.canvas.clientHeight / canvas.height;

    //var rect = canvas.getBoundingClientRect();

    //Background
    context.fillStyle = "black";
    context.fillRect(0, 0, canvas.width, canvas.height);

    canvas.addEventListener("click", e => {
        var rect = canvas.getBoundingClientRect();
        var x = Math.floor((e.clientX - rect.left) / pixel_width);
        var y = Math.floor((e.clientY - rect.top) / pixel_height);
        draw(x, y);
    });

    canvas.addEventListener("mousemove", e => {
        if (active) {
            var rect = canvas.getBoundingClientRect();
            var x = Math.floor((e.clientX - rect.left) / pixel_width);
            var y = Math.floor((e.clientY - rect.top) / pixel_height);
            draw(x, y)
        }
    });

    canvas.addEventListener("mousedown", () => {
        active = true;
    });
    canvas.addEventListener("mouseup", () => {
        active = false;
    });
}

function onConnect() {
    // Once a connection has been made, make a subscription and send a message.
    console.log("onConnect");
    client.subscribe(draw_topic);

    var message = new MQTT.Message("1");
    message.destinationName = connect_topic;
    client.send(message);
}

// called when the client loses its connection
function onConnectionLost(responseObject) {
    if (responseObject.errorCode !== 0) {
        console.log("onConnectionLost:" + responseObject.errorMessage);
    }
    // Reconnect
    console.log("Reconnecting");
    client.connect({ onSuccess: onConnect });
}

// called when a message arrives
function onMessageArrived(message) {
    console.log("onMessageArrived:" + toHexString(message.payloadBytes));
    var x = message.payloadBytes[0];
    var y = message.payloadBytes[1];

    context.fillStyle = '#' + toHexString(message.payloadBytes.slice(2, 5));
    context.fillRect(x, y, 1, 1);
}

function toHexString(byteArray) {
    return Array.from(byteArray, function (byte) {
        return ('0' + (byte & 0xFF).toString(16)).slice(-2);
    }).join('')
}

function sendMessage(x, y, color) {
    var payload = new Uint8Array(5);
    payload[0] = x;
    payload[1] = y;
    payload[2] = Number('0x' + color.substr(1, 2)).toString(10);
    payload[3] = Number('0x' + color.substr(3, 2)).toString(10);
    payload[4] = Number('0x' + color.substr(5, 2)).toString(10);

    var message = new MQTT.Message(payload);
    message.destinationName = draw_topic;
    client.send(message);
}

function draw(x, y) {
    context.fillStyle = document.getElementById("draw_color").value;
    context.fillRect(x, y, 1, 1);
    sendMessage(x, y, context.fillStyle)
}

init();