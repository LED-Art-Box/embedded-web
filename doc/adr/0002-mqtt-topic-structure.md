# 2. MQTT Topic Structure

Date: 2021-03-19

## Status

Proposed

## Context

We currently use a channel naming that is `<prefix>/draw` to send images to the boxes and `<prefix>/draw/update` to publish software updates.

Subscribing to a different `prefix` currently requires a recompilation. We are looking for a topic structure that allows:

- draw on an individual box
- support multiple channels
- configure the channel subscription of a box remotely
- sending a software update to an individual box

The following sources have been considered as inspiration for the topic structure:

* https://raspberry-valley.azurewebsites.net/MQTT-Topic-Trees/
* https://github.com/mqtt-smarthome/mqtt-smarthome
* https://homieiot.github.io/

## Decision


The new channel structure will have two different topic trees:

* `box/[box-id]`: Communication with individual boxes
* `channel/[channel]`. Channels that provide different art content

This allows the following topics:

- `<prefix>/box/[box-id]/status`  The status of the box (e.g. software version)
- `<prefix>/box/[box-id]/update`  Send software update information
- `<prefix>/box/[box-id]/draw`    Draw onto box
- `<prefix>/box/[box-id]/channel` Set channel subscription for box

- `<prefix>/channel/[channel-id]` Draw onto all box subscribing to channel

The `led-art/` prefix could be omitted if the project would host it's own MQTT broker

The new prefix is `led-art`.
The default channel is `led-art/channel/default.`
The website will draw onto the default channel.

## Consequences

What becomes easier or more difficult to do and any risks introduced by the change that will need to be mitigated.

There are to general ways of drawing onto a device: Either via the `box/[box-id]/draw` topic or via `channel/[channel-id]` if the box is subscribed. This might cause confusion, if a box is subscribing a channel and a direct draw onto the box is eventually overriden. No solution yet.


It is possible to send software updates to all boxes sharing the same prefix. If this is a security concern, a different prefix has to be used.

It is possible to use `<prefix>/box/+/draw` to draw onto all boxes ignoring all subscriptions.

The software running on the box needs to listen to multiple topics. The subscription for a box can be a retained message, so configuration is not lost during restarts.

