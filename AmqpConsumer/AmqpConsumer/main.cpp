// ---===***===--- //
// AMQP SETTINGS -- exist in AmqpConnection.h //

char const *hostname = "localhost";
int port = 5672;
char const *exchange = "amqp.example";
char const *exchange_type = "direct";
char const *bindingkey = "examplekey";
char const *amqp_user = "guest";
char const *amqp_pass = "guest";
char const *consumer_name = "example.consumer";

#define DUMP_PACKETS 1

#include <QtCore/QCoreApplication>
#include <QDebug>
#include <QThread>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include <amqp_tcp_socket.h>
#include <amqp.h>
#include <amqp_framing.h>

#include <assert.h>
#include "Utils.h"
#include "AmqpConnection.h"

int status;
amqp_socket_t *socket = NULL;
amqp_connection_state_t conn;
amqp_bytes_t queuename;

void start() {
	int count_try = 0;
	while (!initAmqp(conn, socket, status, queuename)) {
		if (++count_try > 10) {
			qDebug() << "Can't open AMPQ..." << "\n";
			exit(1);
		}
		closeAmqp(conn);
		QThread::msleep(1000);
	}
}

int main(int argc, char *argv[]) {
	QCoreApplication a(argc, argv);
	start(); // creating conn

	int tmpLen;
	bool ackEnvelope; // true - delete a packet from queue
	bool parsed;

	unsigned long errorPauseSec = 1;

	bool ok = true;
	for (int i = 0; i < 10; ++i) {
		if (!ok) {
			closeAmqp(conn);
			QThread::msleep(1000);
			start();
		}// if no ack ...

		amqp_rpc_reply_t res;
		amqp_envelope_t envelope;

		amqp_maybe_release_buffers(conn);

		// waiting for a packet
		res = amqp_consume_message(conn, &envelope, NULL, 0);

		if (res.reply_type != AMQP_RESPONSE_NORMAL) {
			qDebug() << amqp_error_string(res.library_error) << "\n";
			try { amqp_destroy_envelope(&envelope); } catch (...) { /* nothing */ }
			ok = false;
			continue;
		}

		#ifdef DUMP_PACKETS
		
		printf("Delivery %u, exchange %.*s routingkey %.*s\n",
			(unsigned) envelope.delivery_tag,
			(int) envelope.exchange.len, (char *) envelope.exchange.bytes,
			(int) envelope.routing_key.len, (char *) envelope.routing_key.bytes);

		if (envelope.message.properties._flags & AMQP_BASIC_CONTENT_TYPE_FLAG) {
			printf("Content-type: %.*s\n",
				(int) envelope.message.properties.content_type.len,
				(char *) envelope.message.properties.content_type.bytes);
		}
		printf("----\n");
		amqp_dump(envelope.message.body.bytes, envelope.message.body.len);

		#endif
		
		// envelope.message.body.bytes - пакет (всё, что положили в очередь)
		// envelope.message.body.len - длина пакета в байтах

		char *data = (char*)envelope.message.body.bytes;
		char *dataEnd = data + envelope.message.body.len;

		ackEnvelope = i % 2 == 0;	

		if (ackEnvelope) {
			ok = (amqp_basic_ack(conn, 1, envelope.delivery_tag, 0) == 0);
		} else {
			amqp_basic_nack(conn, 1, envelope.delivery_tag, 0, true);
			ok = false;
		} 

		amqp_destroy_envelope(&envelope);
	}// while ...

	closeAmqp(conn);

	return 0;
}
