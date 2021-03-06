import argparse
import paho.mqtt.client as mqtt
from PIL import Image
from upload import ImageUploader

DEFAULT_BROKER_ADDRESS = 'broker.emqx.io'
DEFAULT_BROKER_PORT = 1883
DEFAULT_CAPTURE_SECONDS = 60
CONNECT_TOPIC_SUFFIX = '/draw/connect'
DRAW_TOPIC_SUFFIX = '/draw'
DEFAULT_TOPIC_PREFIX = 'lieblingswelt'


def _validate_image(filename, auto_convert=True):
    img = Image.open(filename)
    if not auto_convert:
        if img.size != (16, 16):
            raise Exception('Expected image of size 16x16')
        if img.getbands() != ('R', 'G', 'B'):
            raise Exception('Expected image with RGB colors but got %s' % str(img.getbands()))
        return img
    else:
        return img.convert('RGB').resize((16, 16))


def run(args):
    img = _validate_image(args.image, auto_convert=args.auto_convert)

    client = mqtt.Client('upload-image')
    client.connect(args.mqtt_broker, args.mqtt_port)
    client.loop_start()

    ImageUploader(client, args.topic + DRAW_TOPIC_SUFFIX).upload(img, full_image=args.full_image)

    client.loop_stop()
    client.disconnect()


parser = argparse.ArgumentParser(description='Upload image')
parser.add_argument('image', metavar='image', type=str,
                    help='the image to upload')
parser.add_argument('--mqtt-broker', default=DEFAULT_BROKER_ADDRESS,
                    help='MQTT broker address (default %s)' % DEFAULT_BROKER_ADDRESS)
parser.add_argument('--mqtt-port', default=DEFAULT_BROKER_PORT,
                    help='MQTT broker port (default %d)' % DEFAULT_BROKER_PORT)
parser.add_argument('--topic', default=DEFAULT_TOPIC_PREFIX,
                    help='The topic prefix (default %s)' % DEFAULT_TOPIC_PREFIX)
parser.add_argument('--auto-convert', dest='auto_convert', action='store_true',
                    help='Auto convert image to 16x16 RGB')
parser.add_argument('--full-image', dest='full_image', action='store_true',
                    help='Upload entire image in one MQTT message')


run(parser.parse_args())
