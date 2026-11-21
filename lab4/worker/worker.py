import signal
import socket
import time
import uuid
from collections import Counter
import redis

r = redis.Redis(host="broker", decode_responses=True)
name = socket.gethostname()
stopping = False
leases = "tasks:leases"

take_task = r.register_script("""
local clock = redis.call("TIME")
local now = clock[1] * 1000 + math.floor(clock[2] / 1000)
local expired = redis.call("ZRANGEBYSCORE", KEYS[1], "-inf", now, "LIMIT", 0, 1)[1]
local task
if expired then
    task = string.sub(expired, string.find(expired, ":") + 1)
    redis.call("ZREM", KEYS[1], expired)
else
    task = redis.call("LPOP", KEYS[2]) or redis.call("LPOP", KEYS[3])
end
if task then
    local reservation = ARGV[1] .. ":" .. task
    redis.call("ZADD", KEYS[1], now + tonumber(ARGV[2]), reservation)
    return {reservation, task}
end
""")

finish_task = r.register_script("""
if redis.call("ZSCORE", KEYS[1], ARGV[1]) then
    if KEYS[2] == "tasks" then
        redis.call("LPUSH", KEYS[2], ARGV[2])
    else
        redis.call("RPUSH", KEYS[2], ARGV[2])
    end
    redis.call("ZREM", KEYS[1], ARGV[1])
end
""")


def stop(signum, frame):
    global stopping
    stopping = True


signal.signal(signal.SIGTERM, stop)

while not stopping:
    item = take_task(keys=[leases, "tasks:processing", "tasks"],
                     args=[uuid.uuid4().hex, 10000])
    if item is None:
        time.sleep(0.25)
        continue
    reservation, task = item
    if stopping:
        finish_task(keys=[leases, "tasks"], args=[reservation, task])
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
    finish_task(keys=[leases, "result:" + task_id], args=[reservation, answer])
