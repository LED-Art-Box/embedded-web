import MQTT from 'paho-mqtt'

const broker_address = "wss://broker.emqx.io:8084/mqtt";
const draw_topic = "lieblingswelt/draw";
const connect_topic = "lieblingswelt/draw/connect";

function toHexString(byteArray) {
    return Array.from(byteArray, function (byte) {
        return ("0" + (byte & 0xff).toString(16)).slice(-2);
    }).join("");
}

export default class MQTTService {
    constructor() {
        this.broker_address = broker_address;
        this.draw_topic = draw_topic;
        this.connect_topic = connect_topic;
        this.mqtt = null;

        this.messageHandlers = [];
    }

    connect() {
        var clientId = Math.random().toString(36).substring(20);
        this.mqtt = new MQTT.Client(this.broker_address, clientId);

        this.mqtt.onConnectionLost = this.onConnectionLost.bind(this);
        this.mqtt.onMessageArrived = this.onMessageArrived.bind(this);

        this.mqtt.connect({ onSuccess: this.onConnect.bind(this) });
    }

    registerPixelEventHandler(handler) {
        this.messageHandlers.push(handler);
    }

    onConnect() {
        console.log("Successfully connected to MQTT broker");

        this.mqtt.subscribe(this.draw_topic);

        this._triggerSync();
    }

    _triggerSync() {
        var message = new MQTT.Message("1");
        message.destinationName = this.connect_topic;
        this.mqtt.send(message);
    }

    onMessageArrived(message) {
        console.log("onMessageArrived: " + toHexString(message.payloadBytes));
        if (message.payloadBytes.length == 5) {
            var x = message.payloadBytes[0];
            var y = message.payloadBytes[1];
            var color = '#' + toHexString(message.payloadBytes.slice(2, 5));

            this.messageHandlers.forEach(handler => handler(x, y, color));
        }
    }

    onConnectionLost(response) {
        if (response.errorCode !== 0) {
            console.log("onConnectionLost:" + response.errorMessage);
        }
        // Reconnect
        console.log("Reconnecting");
        this.mqtt.connect({ onSuccess: () => this.onConnect() });
    }

    publishPixel(x, y, color) {
        var payload = new Uint8Array(5);
        payload[0] = x;
        payload[1] = y;
        payload[2] = Number("0x" + color.substr(1, 2)).toString(10);
        payload[3] = Number("0x" + color.substr(3, 2)).toString(10);
        payload[4] = Number("0x" + color.substr(5, 2)).toString(10);

        var message = new MQTT.Message(payload);
        message.destinationName = draw_topic;
        this.mqtt.send(message);
    }
}
