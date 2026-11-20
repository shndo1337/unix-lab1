import signal
import socket
import time
from collections import Counter
import redis

r = redis.Redis(host="broker", decode_responses=True)
name = socket.gethostname()
stopping = False


def stop(signum, frame):
    global stopping
    stopping = True


signal.signal(signal.SIGTERM, stop)

while not stopping:
    task = r.blmove("tasks", "tasks:processing", timeout=1)
    if task is None:
        continue
    if stopping:
        with r.pipeline() as pipe:
            pipe.lpush("tasks", task)
            pipe.lrem("tasks:processing", 1, task)
            pipe.execute()
        break

    task_id, text = task.split(":", 1)
    time.sleep(1)
    words = [w.strip(".,!?;:\"()") for w in text.lower().split()]
    words = [w for w in words if w != ""]
    top = Counter(words).most_common(3)
    answer = ""
    for word, count in top:
        answer = answer + word + " - " + str(count) + "; "
    answer = answer + "посчитал " + name
    with r.pipeline() as pipe:
        pipe.rpush("result:" + task_id, answer)
        pipe.lrem("tasks:processing", 1, task)
        pipe.execute()
