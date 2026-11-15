import socket
import time
from collections import Counter
import redis

r = redis.Redis(host="broker", decode_responses=True)
name = socket.gethostname()

while True:
    task = r.blpop("tasks")[1]
    task_id, text = task.split(":", 1)
    time.sleep(1)
    words = [w.strip(".,!?;:\"()") for w in text.lower().split()]
    words = [w for w in words if w != ""]
    top = Counter(words).most_common(3)
    answer = ""
    for word, count in top:
        answer = answer + word + " - " + str(count) + "; "
    answer = answer + "посчитал " + name
    r.rpush("result:" + task_id, answer)
