#include <pthread.h>
#include <unistd.h>
#include <iostream>

class Event {
public:
    int id;
};

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t condition = PTHREAD_COND_INITIALIZER;
bool ready = false;
Event *data = NULL;

void *provider(void *arg)
{
    int i = 1;
    while (i <= 5) {
        sleep(1);
        pthread_mutex_lock(&mutex);
        if (!ready) {
            data = new Event;
            data->id = i;
            ready = true;
            std::cout << "Поставщик: событие " << data->id << " отправлено" << std::endl;
            pthread_cond_signal(&condition);
            i++;
        }
        pthread_mutex_unlock(&mutex);
    }
    return NULL;
}

void *consumer(void *arg)
{
    for (int i = 0; i < 5; i++) {
        pthread_mutex_lock(&mutex);
        while (!ready)
            pthread_cond_wait(&condition, &mutex);
        Event *event = data;
        ready = false;
        std::cout << "Потребитель: событие " << event->id << " получено" << std::endl;
        delete event;
        pthread_mutex_unlock(&mutex);
    }
    return NULL;
}

int main()
{
    pthread_t p;
    pthread_t c;
    pthread_create(&c, NULL, consumer, NULL);
    pthread_create(&p, NULL, provider, NULL);
    pthread_join(p, NULL);
    pthread_join(c, NULL);
    pthread_cond_destroy(&condition);
    pthread_mutex_destroy(&mutex);
    return 0;
}
