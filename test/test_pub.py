# import paho.mqtt.client as mqtt
import paho.mqtt.client as mqtt

def on_connect(client, userdata, flags, rc):
    print("Connected with result code " + str(rc))
    client.publish('test/topic', payload='hello world expiry', qos=0, retain=True, properties={'message_expiry_interval': 10})

client = mqtt.Client(protocol=mqtt.MQTTv5)
client.on_connect = on_connect

client.connect("127.0.0.1", 1883, 60)
client.loop_forever()

# import paho.mqtt.client as mqtt

# def on_connect(client, userdata, flags, rc):
#     print("Connected with result code " + str(rc))

# client = mqtt.Client()
# client.on_connect = on_connect

# client.connect("broker.hivemq.com", 1883, 60)

# # Publish a message to the specific user's topic
# client.publish("user/user123", "Hello user123!")

# client.loop_forever()

