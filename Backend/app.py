from flask import Flask, jsonify, render_template, request
from config import FLASK_HOST, FLASK_PORT, DEBUG
from mongo import collection
from mqtt_listener import start_mqtt_thread, init_mqtt_publisher, publish_control
from datetime import timedelta

app = Flask(__name__)


def serialize_doc(doc):
    if not doc:
        return None

    doc["_id"] = str(doc["_id"])

    if "received_at" in doc and doc["received_at"] is not None:
        utc_time = doc["received_at"]
        local_time = utc_time - timedelta(hours=5)
        doc["received_at"] = local_time.strftime("%d %b %Y, %I:%M:%S %p")

    return doc


@app.route("/")
def home():
    latest = collection.find_one(sort=[("_id", -1)])
    if latest:
        latest = serialize_doc(latest)
    return render_template("index.html", latest=latest)


@app.route("/api/latest", methods=["GET"])
def api_latest():
    latest = collection.find_one(sort=[("_id", -1)])
    if not latest:
        return jsonify({"status": "no data"}), 404
    return jsonify(serialize_doc(latest))


@app.route("/api/readings", methods=["GET"])
def api_readings():
    docs = collection.find().sort("_id", -1).limit(50)
    readings = [serialize_doc(doc) for doc in docs]
    return jsonify(readings)


@app.route("/api/control/<device>", methods=["POST"])
def api_control(device):
    if device not in ["ac", "sprinkler", "dehumidifier"]:
        return jsonify({"error": "invalid device"}), 400

    data = request.get_json()
    mode = data.get("mode")
    state = data.get("state", False)

    if mode not in ["auto", "manual"]:
        return jsonify({"error": "invalid mode"}), 400

    payload = publish_control(device, mode, state)
    return jsonify({
        "status": "sent",
        "device": device,
        "mode": mode,
        "state": state,
        "payload": payload
    })


if __name__ == "__main__":
    start_mqtt_thread()
    init_mqtt_publisher()
    app.run(host=FLASK_HOST, port=FLASK_PORT, debug=DEBUG)