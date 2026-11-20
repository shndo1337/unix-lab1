import signal
import uuid
import redis
from flask import Flask, request
from werkzeug.serving import ThreadedWSGIServer

app = Flask(__name__)
r = redis.Redis(host="broker", decode_responses=True)
stopping = False


class Server(ThreadedWSGIServer):
    daemon_threads = False


def stop(signum, frame):
    global stopping
    stopping = True


@app.route("/words", methods=["POST"])
def words():
    text = request.form.get("text", "")
    if stopping:
        return "shutting down\n", 503
    task_id = str(uuid.uuid4())
    r.rpush("tasks", task_id + ":" + text)
    result = r.blpop("result:" + task_id, timeout=30)
    if result is None:
        return "timeout\n", 504
    return result[1] + "\n"


if __name__ == "__main__":
    signal.signal(signal.SIGTERM, stop)
    with Server("0.0.0.0", 5000, app) as server:
        server.timeout = 0.2
        while not stopping:
            server.handle_request()
