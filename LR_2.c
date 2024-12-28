#include <signal.h> // Для работы с сигналами
#include <sys/select.h> // pselect
#include <sys/types.h>  // Библиотека для типов данных
#include <sys/socket.h> // Для операций с сокетами
#include <netinet/in.h> // Библиотека для работы с интернет-адресами
#include <unistd.h> // POSIX API для системных вызовов
#include <errno.h> // Библиотека для обработки ошибок
#include <stdio.h>
#include <stdlib.h>

volatile sig_atomic_t wasSigHup = 0;         // sig_atomic_t - гарантирует, что компилятор операцию с этим типом данных всегда будет представлять в виде одной инструкции
void sigHupHandler(int r) { wasSigHup = 1; } // Объявление безопасного обработчика сигнала обработчика сигнала SigHup

int main() {
	// Регистрация обработчика сигнала
	struct sigaction sa;              // Заводим переменную типа sigaction
	sigaction(SIGHUP, NULL, &sa);     // В эту переменную сбрасываем атрибуты этого сигнала для вашего процессора из ядра операционной системы
	sa.sa_handler = sigHupHandler;    // Хранитт указатель на вашу функцию, в которой скрывается обработчик сигнала
	sa.sa_flags |= SA_RESTART;        // Устанавливаем флаг
	sigaction(SIGHUP, &sa, NULL);     // Повторный вызов функции sigaction уже с просьбой вашу модифицированную структуру сохранить в ядре
	// Выгружаем из ядра то, что было на текущий момент, модифицируем, затем возвращаем в ядро уже в модифицированном виде

	// Блокировка сигнала
	sigset_t blockedMask, origMask;                    // Заводим маску блокирования сигналов и исходную маску сигналов - маску, с которой наше приложение было запущено, она подразумевает что сигнал SigHup еще был разблокирован
	                                                   //     она идентифицирует те сигналы, которые должны быть активированы на время исполнения Pselect
	sigemptyset(&blockedMask);                         // Сбрасываем маску blockedMask
	sigaddset(&blockedMask, SIGHUP);                   // Помечаем нужный сигнал, в вашем случае это SIGHUP
	sigprocmask(SIG_BLOCK, &blockedMask, &origMask);   // Говорим, что необходимо заблокировать те сигналы, которые присутствуют в вашей множестве

	int serverSocket = socket(AF_INET, SOCK_STREAM, 0); // Создание серверного сокета (его дескриптора)

	struct sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serverAddr.sin_port = htons(2048);

	bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
	listen(serverSocket, SOMAXCONN);

	printf("Server is running\n");

	int clientSocket; // Объявление клиентского сокета (его дескриптора)
	int maxFd = serverSocket; // Переменная наибольшего дескриптора сокета, для мониторинга входящих подключений
	fd_set fds; // Заводим множество файловых дескрипторов
	bool isConnected = false; 

	while (true)
	{
		FD_ZERO(&fds); // Сбрасываем множество файловых дескрипторов
		
		FD_SET(serverSocket, &fds); // Добавляем серверный сокет во множество файловых дескрипторов
		maxFd = serverSocket; // Сохраняем значение серверного сокета в maxFd

		FD_SET(clientSocket, &fds); // Добавляем клиентский сокет во множество файловых дескрипторов
		
		if (clientSocket > maxFd) maxFd = clientSocket; // Определяем максимальный сокет
		
		if (pselect(maxFd + 1, &fds, NULL, NULL, NULL, &origMask) == -1)  // Проверяем дескрипторы начиная с нуля до maxFd, из множества файловых дескрипторов fds, без проверки на запись или ошибки(первые два NULL),
		{                                                                 // с блокировкой до тех пор пока не произойдёт событие(третий NULL), а так же передаём  указатель на маску сигналов, которая будет временно изменена во время выполнения функции.
			if (errno == EINTR)  // Проверяем ошибку на тип EINTER - то есть, была ли функция pselect() просто прервана обработкой сигнала 
			{
				if (wasSigHup) // Проверяем был ли это сигнал SigHub
				{
					wasSigHup = 0; // Условно обрабатываем сигнал SigHup
					printf("SIGHUP was received\n"); // Оповещаем о получениии сигнала SigHup
				}
				continue; // Переходим к следующей итеррации цикла, так как ошибка типа EINTER для нас по факту не является ошибкой
			}
			else 
			{
				break; // выходим, так как произошла ошибка - pselect() вернула не -1
			}
		}

		// Логика приёма нового соединения
		if (FD_ISSET(serverSocket, &fds))  // Используем макрос FD_ISSET для проверки, готов ли серверный сокет к обработке нового соединения
                {
			int clientSocketNEW = accept(serverSocket, NULL, NULL); // Заводим новый сокет и присваеваем значение функции accept(), которая принимает входящее соединение, создавая новый сокет для общения с новым клиентом
			if (clientSocketNEW != -1) // Если новое соединение есть
			{
				printf("New connection: %d\n", clientSocketNEW); // Оповещаем о новом соединении

				if (isConnected) // Если есть уже подключенный клиент, то закрываем старое соединение с оповещением
				{
					printf("Close: %d\n", clientSocket);
					close(clientSocket);
				}
				isConnected = true; // Есть подключенный клиент
				clientSocket = clientSocketNEW; // В переменную клиентского сокета сохраняем новое соединение
			}
		}

                // Логика приёма данных
		if (FD_ISSET(clientSocket, &fds)) // Используем макрос FD_ISSET для проверки, есть ли данные для чтения из клиентского сокета
		{
			char buffer[1024]; // Устанавливаем значение того, сколько нужно будет прочесть функции read()
			ssize_t readVolume = read(clientSocket, buffer, sizeof(buffer)); // Присваиваем переменной readVolume количество байтов которое удалось проситать функции read() 
			
			if (readVolume <= 0) // Если функции read() не удалось ни чего прочитать, закрываем клиентский сокет с оповещением
			{
				printf("Connection %d closed\n", clientSocket);
				close(clientSocket);
				isConnected = false;
			}
			else printf("%zd bits were read from %d\n", readVolume, clientSocket); // Оповещаем о том, сколько байтов информации прочла функция read() из текущего склиентского сокета
		}
	}

	close(serverSocket); // Закрываем серверный сокет
	return 0;
}
