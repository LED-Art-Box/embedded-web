import time


class ImageUploader(object):
    def __init__(self, client, topic):
        self._client = client
        self._topic = topic

    def upload(self, image, full_image=False):
        if full_image:
            self._upload_full_image(image)
        else:
            self._upload_pixel_by_pixel(image)

    def _upload_pixel_by_pixel(self, image):
        for y in range(16):
            for x in range(16):
                rgb = image.getpixel((x, y))
                self._client.publish(self._topic, bytearray([x, y, rgb[0], rgb[1], rgb[2]]))
                time.sleep(0.2)

    def _upload_full_image(self, image):
        data = []
        for y in range(16):
            for x in range(16):
                rgb = image.getpixel((x, y))
                color565 = self._rgb_to_rgb565(rgb)
                data.extend(color565.to_bytes(2, byteorder='little'))

        self._client.publish(self._topic, payload=bytearray(data))
        print('Uploading %d bytes to %s' % (len(data), self._topic))

    @staticmethod
    def _rgb_to_rgb565(rgb):
        r5 = (rgb[0] * 249 + 1014) >> 11
        g6 = (rgb[1] * 253 + 505) >> 10
        b5 = (rgb[2] * 249 + 1014) >> 11
        color565 = ((r5 & 0x1F) << 11) | ((g6 & 0x3F) << 5) | (b5 & 0x1F)
        return color565
