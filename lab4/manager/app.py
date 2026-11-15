import uuid
import redis
from flask import Flask, request

app = Flask(__name__)
r = redis.Redis(host="broker", decode_responses=True)


@app.route("/words", methods=["POST"])
def words():
    text = request.form.get("text", "")
    task_id = str(uuid.uuid4())
    r.rpush("tasks", task_id + ":" + text)
    result = r.blpop("result:" + task_id, timeout=30)
    if result is None:
        return "timeout\n", 504
    return result[1] + "\n"


app.run(host="0.0.0.0", port=5000)
