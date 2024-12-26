#include <stdio.h>
#include <pthread.h>

pthread_cond_t cond_for_provider = PTHREAD_COND_INITIALIZER; // Условная переменная для сигнализации потоку поставщику о том, что пришло время просыпаться 
pthread_cond_t cond_for_consumer = PTHREAD_COND_INITIALIZER; // Условная переменная для сигнализации потоку потребителю о том, что пришло время просыпаться

pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER; // Инициализируем мьютекс

int event = 0; // Буферная переменная события: 1 - событие произошло/не обработано, 0 - событие обработано/не произошло

int i = 0; // Итерратор

void provider() {
    while(1) {
        sleep(1);
        pthread_mutex_lock(&mtx); // Лочим мьютекс чтобы исключить наступление нового события
        
        while (event == 1) // Проверка токго, что осталось необработанным предидущее событие
        {
            pthread_cond_wait(&cond_for_provider, &mtx); // Временно и атомарно разлочиваем мьютекс на время ожидания сигнала для потока поставщика, и лочим при получении
            printf("Provider woke up\n"); 
        }
        
        i += 1;
        event = 1; // Событие наступило!
        printf("Message %d sent\n", i);
        
        pthread_cond_signal(&cond_for_consumer); // Сигнализируем потребителю о наступлении события
        pthread_mutex_unlock(&mtx); // Окончательно разлочиваем мьютекс
    }
}

void consumer() {
    while(1) {
        pthread_mutex_lock(&mtx); // Лочим мьютекс пока не обработаем текущее событие
        
        while (event == 0) // Ждём наступления события
        {
            pthread_cond_wait(&cond_for_consumer, &mtx); // Временно и атомарно разлочиваем мьютекс на время ожидания сигнала для потока потребителя, и лочим при получении
            printf("consumer woke up\n");
        }
        
        event = 0; // Событие обработано!
        printf("Message %d accepted\n", i);
        
        pthread_cond_signal(&cond_for_provider); // Сигнализируем поставщику
        pthread_mutex_unlock(&mtx); // Окончательно разлочиваем мьютекс 
    }
}

int main() {
    pthread_t prov, cons; // Объявляем две переменные типа pthread_t для хранения ID потоков
    
    pthread_create(&prov, NULL, provider, NULL); // Создаём новый поток с атрибутами по умолчанию и запускаем в нём функцию provider, которая не требует аргументов
    pthread_create(&cons, NULL, consumer, NULL); // Создаём новый поток с атрибутами по умолчанию и запускаем в нём функцию consumer, которая не требует аргументов
    
    pthread_join(prov, NULL); // Ждём результатов от птока с ID prov
    pthread_join(cons, NULL); // Ждём результатов от птока с ID cons
    
    return 0;
}
