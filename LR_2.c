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
	struct sigaction sa;              //
	sigaction(SIGHUP, NULL, &sa);     // В эту переменную сбрасываем атрибуты этого сигнала для вашего процессора из ядра операционной системы
	sa.sa_handler = sigHupHandler;    // Хранитт указатель на вашу функцию, в которой скрывается обработчик сигнала
	sa.sa_flags |= SA_RESTART;        // Устанавливаем ылаг
	sigaction(SIGHUP, &sa, NULL);     // Повторный вызов функции sigaction уже с просьбой вашу модифицированную структуру сохранить в ядре
	// Выгружаем из ядра то, что было на текущий момент, модифицируем, затем возвращаем в ядро уже в модифицированном виде

	sigset_t blockedMask, origMask;                    //
	sigemptyset(&blockedMask);                         // Блокировка сигнала
	sigaddset(&blockedMask, SIGHUP);                   //
	sigprocmask(SIG_BLOCK, &blockedMask, &origMask);   //

	int serverSocket = socket(AF_INET, SOCK_STREAM, 0);

	struct sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serverAddr.sin_port = htons(2048);

	bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
	listen(serverSocket, SOMAXCONN);

	printf("Server is running\n");

	int clientSocket;
	int maxFd = serverSocket;
	fd_set fds; // Из работы основного цикла
	bool client_is_connected = false;

	while (true)
	{
		FD_ZERO(&fds); // Из работы основного цикла
		FD_SET(serverSocket, &fds); // Из работы основного цикла
		maxFd = serverSocket;

		FD_SET(clientSocket, &fds);
		if (clientSocket > maxFd) maxFd = clientSocket;
		
		if (pselect(maxFd + 1, &fds, NULL, NULL, NULL, &origMask) == -1)
		{
			if (errno == EINTR)  // Из работы основного цикла
			{
				if (wasSigHup) {
					wasSigHup = 0;
					printf("SIGHUP is received\n");
				}
				continue;
			}
			else 
			{
				break;
			}
		}

		if (FD_ISSET(serverSocket, &fds))  // Из работы основного цикла
    {
			int clientNewSocket = accept(serverSocket, NULL, NULL);
			if (clientNewSocket != -1) {
				printf("New connection: %d\n", clientNewSocket);

				if (client_is_connected) {
					printf("Close: %d\n", clientSocket);
					close(clientSocket);
				}
				client_is_connected = true;
				clientSocket = clientNewSocket;
			}
		}


		if (FD_ISSET(clientSocket, &fds)) {
			char buffer[1024];
			ssize_t bytesRead = read(clientSocket, buffer, sizeof(buffer));
			if (bytesRead <= 0) {
				printf("Connection %d closed\n", clientSocket);
				close(clientSocket);
			}
			else printf("Got %zd byte from %d\n", bytesRead, clientSocket);
		}
	}

	close(serverSocket);
	return 0;
}
