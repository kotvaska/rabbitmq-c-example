// ---===***===--- //
// AMQP SETTINGS -- прописаны как extern в AmqpConnection.h //

char const *hostname = "localhost";
int port = 5672;
char const *exchange = "amqp.example";
char const *exchange_type = "direct";
char const *bindingkey = "examplekey";
char const *amqp_user = "guest";
char const *amqp_pass = "guest";

#include <QCoreApplication>
#include <QThread>
#include <QDebug>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include <amqp_tcp_socket.h>
#include <amqp.h>
#include <amqp_framing.h>

#include "Utils.h"
#include "AmqpConnection.h"

int status;
amqp_socket_t *socket = NULL;
amqp_connection_state_t conn;

int main(int argc, char *argv[]) {
	QCoreApplication a(argc, argv);
	qDebug() << "Started..." << "\n";

	int count_try = 0;
	while (!initAmqp(conn, socket, status)) {
		if (++count_try > 10) {
			qDebug() << "Can't open AMPQ..." << "\n";
			closeAmqp(conn);
			exit(1);
		}
		closeAmqp(conn);
		QThread::msleep(1000);
	}

	for (int i = 0; i < 10; ++i) {
		QByteArray msg;
		msg.append("Test message #");
		msg.append(QString::number(i));
		send_batch(conn, bindingkey, msg);
	}

	closeAmqp(conn);

    return 0;
}// main()
