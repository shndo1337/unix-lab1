FROM python:3.12-slim
WORKDIR /app
RUN pip install redis
COPY worker.py .
CMD ["python", "worker.py"]
