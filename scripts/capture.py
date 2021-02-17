import argparse
import paho.mqtt.client as mqtt
import time
from PIL import Image

DEFAULT_BROKER_ADDRESS = 'broker.emqx.io'
DEFAULT_BROKER_PORT = 1883
DEFAULT_CAPTURE_SECONDS = 60
CONNECT_TOPIC_SUFFIX = '/draw/connect'
DRAW_TOPIC_SUFFIX = '/draw'
DEFAULT_TOPIC_PREFIX = 'lieblingswelt'


class ImageCapturer(object):
    def __init__(self, client, topic_prefix):
        self._client = client
        self._img = Image.new('RGB', (16, 16))
        self._topic_prefix = topic_prefix

    def capture(self, output_file, capture_window_seconds=DEFAULT_CAPTURE_SECONDS):
        draw_topic = self._topic_prefix + DRAW_TOPIC_SUFFIX
        self._client.subscribe(draw_topic, 1)
        self._client.message_callback_add(draw_topic, lambda _, userdata, msg: self._on_message_sub(userdata, msg))

        self._trigger_sync()

        time.sleep(capture_window_seconds)

        self._client.message_callback_remove(draw_topic)

        self._img.save(output_file)

    def _trigger_sync(self):
        self._client.publish(self._topic_prefix + CONNECT_TOPIC_SUFFIX, '1')

    def _on_message_sub(self, _userdata, message):
        coord, rgb = self._payload_to_pixel_color(message.payload)
        self._img.putpixel(coord, rgb)

    @staticmethod
    def _payload_to_pixel_color(payload):
        x = payload[0]
        y = payload[1]
        r = payload[2]
        g = payload[3]
        b = payload[4]
        return (x, y), (r, g, b)


def run(args):
    client = mqtt.Client('image-snapshot')
    client.connect(args.mqtt_broker, args.mqtt_port)
    client.loop_start()

    ImageCapturer(client, args.topic).capture(args.image, capture_window_seconds=args.capture_time)

    client.loop_stop()
    client.disconnect()


parser = argparse.ArgumentParser(description='Save current art to file')
parser.add_argument('image', metavar='image', type=str,
                    help='the target image file')
parser.add_argument('--capture-time', metavar='secs', type=int,
                    help='The capture time window (default %ds)' % DEFAULT_CAPTURE_SECONDS,
                    default=DEFAULT_CAPTURE_SECONDS)
parser.add_argument('--mqtt-broker', default=DEFAULT_BROKER_ADDRESS,
                    help='MQTT broker address (default %s)' % DEFAULT_BROKER_ADDRESS)
parser.add_argument('--mqtt-port', default=DEFAULT_BROKER_PORT,
                    help='MQTT broker port (default %d)' % DEFAULT_BROKER_PORT)
parser.add_argument('--topic', default=DEFAULT_TOPIC_PREFIX,
                    help='The topic prefix (default %s)' % DEFAULT_TOPIC_PREFIX)


run(parser.parse_args())
