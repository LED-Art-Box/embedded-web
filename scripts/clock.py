import argparse
import math
import time
import paho.mqtt.client as mqtt
from PIL import Image, ImageDraw
from upload import ImageUploader

DEFAULT_BROKER_ADDRESS = 'broker.emqx.io'
DEFAULT_BROKER_PORT = 1883
DEFAULT_CAPTURE_SECONDS = 60
CONNECT_TOPIC_SUFFIX = '/draw/connect'
DRAW_TOPIC_SUFFIX = '/draw'
DEFAULT_TOPIC_PREFIX = 'lieblingswelt'

DRAW_HEIGHT = 16
DRAW_WIDTH = 16
CENTER_X = DRAW_WIDTH / 2
CENTER_Y = DRAW_HEIGHT / 2

BACKGROUND_COLOR = (0x55, 0x55, 0x55)
CLOCK_FACE_COLOR = (0x0, 0x0, 0x0)
HOUR_HANDLE_LENGTH = 4
HOUR_HANDLE_COLOR = (0x50, 0x50, 0x50)
MIN_HANDLE_LENGTH = 6
MIN_HANDLE_COLOR = (0xaa, 0xaa, 0xaa)


def clockhandle(angle, length):
    radian_angle = math.pi * angle / 180.0
    y = CENTER_Y + length * -math.cos(radian_angle)
    x = CENTER_X + length * math.sin(radian_angle)

    return [(CENTER_X, CENTER_Y), (x, y)]


def run(args):

    client = mqtt.Client('upload-image')
    client.connect(args.mqtt_broker, args.mqtt_port)
    client.loop_start()

    image = Image.new('RGB', (DRAW_WIDTH, DRAW_HEIGHT))
    draw = ImageDraw.Draw(image)

    now = time.localtime()

    draw.rectangle((0, 0, DRAW_WIDTH - 1, DRAW_HEIGHT - 1), BACKGROUND_COLOR)
    draw.ellipse((1, 1, DRAW_WIDTH - 1, DRAW_HEIGHT - 1), fill=CLOCK_FACE_COLOR)
    draw.line(clockhandle((now.tm_hour % 12) * 30, HOUR_HANDLE_LENGTH), fill=HOUR_HANDLE_COLOR)
    draw.line(clockhandle(now.tm_min * 6, MIN_HANDLE_LENGTH), fill=MIN_HANDLE_COLOR)

    ImageUploader(client, args.topic + DRAW_TOPIC_SUFFIX).upload(image, full_image=args.full_image)

    client.loop_stop()
    client.disconnect()


parser = argparse.ArgumentParser(description='Upload image')
parser.add_argument('--mqtt-broker', default=DEFAULT_BROKER_ADDRESS,
                    help='MQTT broker address (default %s)' % DEFAULT_BROKER_ADDRESS)
parser.add_argument('--mqtt-port', default=DEFAULT_BROKER_PORT,
                    help='MQTT broker port (default %d)' % DEFAULT_BROKER_PORT)
parser.add_argument('--topic', default=DEFAULT_TOPIC_PREFIX,
                    help='The topic prefix (default %s)' % DEFAULT_TOPIC_PREFIX)
parser.add_argument('--full-image', dest='full_image', action='store_true',
                    help='Upload entire image in one MQTT message')


run(parser.parse_args())
