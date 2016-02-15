#include <QCoreApplication>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include <amqp_tcp_socket.h>
#include <amqp.h>
#include <amqp_framing.h>

#include "AmqpConnection.h"
#include "Utils.h"

// ---===***===--- //

bool initAmqp(amqp_connection_state_t &conn, amqp_socket_t *socket, int &status) {
	amqp_bytes_t queuename;
	conn = amqp_new_connection();

	socket = amqp_tcp_socket_new(conn);
	if (!socket) {
		die("creating TCP socket");
		return false;
	}

	status = amqp_socket_open(socket, hostname, port);
	if (status) {
		die("opening TCP socket");
		return false;
	}

	if (!die_on_amqp_error(amqp_login(conn, "/", 0, 131072, 0, AMQP_SASL_METHOD_PLAIN, amqp_user, amqp_pass), "Logging in")) {
		return false;
	}
	amqp_channel_open(conn, 1);
	if (!die_on_amqp_error(amqp_get_rpc_reply(conn), "Opening channel")) {
		return false;
	}

	// passive 0 durable 1 auto-delete 0 internal 0 exchange
	amqp_exchange_declare_ok_t_ *er = amqp_exchange_declare(conn, 1, amqp_cstring_bytes(exchange), amqp_cstring_bytes(exchange_type), 0, 1, 0, 0, amqp_empty_table);
	if (!die_on_amqp_error(amqp_get_rpc_reply(conn), "Declaring exchange")) {
		return false;
	}
	
	// passive 0 durable 1 exclusive 0 auto-delete 0 queue
    amqp_queue_declare_ok_t *r = amqp_queue_declare(conn, 1, amqp_cstring_bytes(bindingkey), 0, 1, 0, 0, amqp_empty_table);
    die_on_amqp_error(amqp_get_rpc_reply(conn), "Declaring queue");
    queuename = amqp_bytes_malloc_dup(r->queue);
    if (queuename.bytes == NULL) {
		fprintf(stderr, "Out of memory while copying queue name");
		return false;
    }

    amqp_queue_bind(conn, 1, queuename, amqp_cstring_bytes(exchange), amqp_cstring_bytes(bindingkey), amqp_empty_table);
	if (!die_on_amqp_error(amqp_get_rpc_reply(conn), "Binding queue")) {
		return false;
	}

	return true;
}


void closeAmqp(amqp_connection_state_t &conn) {
	die_on_amqp_error(amqp_channel_close(conn, 1, AMQP_REPLY_SUCCESS), "Closing channel");
	die_on_amqp_error(amqp_connection_close(conn, AMQP_REPLY_SUCCESS), "Closing connection");
	die_on_error(amqp_destroy_connection(conn), "Ending connection");
}

void send_batch(amqp_connection_state_t conn, char const *queue_name, const QByteArray &datagram) {
	int i;
	int length = datagram.size();
	//char *message = new char[length + 1];	//! memoryleaks without delete[]
	amqp_bytes_t message_bytes;

	//for (i = 0; i < length; i++) {
	//	message[i] = datagram.data()[i];
	//}

	message_bytes.len = length;
	message_bytes.bytes = (void *)datagram.data();//message;

	amqp_basic_properties_t props;
    props._flags = AMQP_BASIC_CONTENT_TYPE_FLAG |
                   AMQP_BASIC_DELIVERY_MODE_FLAG;
    props.content_type = amqp_cstring_bytes("text/plain");
    props.delivery_mode = 2; /* persistent delivery mode */

	// mandatory 1 immediate 0 publisher
	//! if you want to make delete[], you should divide die and publish functions
	die_on_error(
		amqp_basic_publish(conn,
			1,
			amqp_cstring_bytes(exchange),
			amqp_cstring_bytes(queue_name),
			1,
			0,
			&props,
			message_bytes
		),
		"Publishing"
	);

}