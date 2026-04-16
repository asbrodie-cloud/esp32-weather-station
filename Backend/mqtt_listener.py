import json
import threading
from datetime import datetime
from paho.mqtt import client as mqtt_client

from config import MQTT_BROKER, MQTT_PORT, MQTT_TOPIC, MQTT_CLIENT_ID
from mongo import collection

CONTROL_TOPIC = "weatherstation/control"
mqtt_pub_client = None


def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("Connected to MQTT Broker")
        client.subscribe(MQTT_TOPIC)
        print(f"Subscribed to topic: {MQTT_TOPIC}")
    else:
        print(f"Failed to connect to MQTT Broker, return code {rc}")


def on_message(client, userdata, msg):
    try:
        payload = msg.payload.decode()
        print(f"Received MQTT message: {payload}")

        data = json.loads(payload)

        document = {
            "device_id": data.get("device_id"),
            "temperature": data.get("temperature"),
            "humidity": data.get("humidity"),
            "pressure": data.get("pressure"),
            "soil_moisture": data.get("soil_moisture"),
            "soil_raw": data.get("soil_raw"),
            "heat_index": data.get("heat_index"),
            "altitude": data.get("altitude"),
            "ac_state": data.get("ac_state"),
            "ac_manual": data.get("ac_manual"),
            "sprinkler_state": data.get("sprinkler_state"),
            "sprinkler_manual": data.get("sprinkler_manual"),
            "dehumidifier_state": data.get("dehumidifier_state"),
            "dehumidifier_manual": data.get("dehumidifier_manual"),
            "timestamp": data.get("timestamp"),
            "received_at": datetime.utcnow()
        }

        collection.insert_one(document)
        print("Inserted into MongoDB")

    except Exception as e:
        print("Error processing MQTT message:", e)


def start_mqtt_listener():
    client = mqtt_client.Client(client_id=MQTT_CLIENT_ID)
    client.on_connect = on_connect
    client.on_message = on_message
    client.connect(MQTT_BROKER, MQTT_PORT)
    client.loop_forever()


def start_mqtt_thread():
    thread = threading.Thread(target=start_mqtt_listener, daemon=True)
    thread.start()
    return thread


def init_mqtt_publisher():
    global mqtt_pub_client
    mqtt_pub_client = mqtt_client.Client(client_id="flask-control-publisher")
    mqtt_pub_client.connect(MQTT_BROKER, MQTT_PORT)
    mqtt_pub_client.loop_start()


def publish_control(device, mode, state):
    global mqtt_pub_client

    if mqtt_pub_client is None:
        init_mqtt_publisher()

    payload = json.dumps({
        "device": device,
        "mode": mode,
        "state": state
    })

    mqtt_pub_client.publish(CONTROL_TOPIC, payload)
    return payload