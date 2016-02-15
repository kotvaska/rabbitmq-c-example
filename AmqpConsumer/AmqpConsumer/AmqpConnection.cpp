#include <QCoreApplication>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include <amqp_tcp_socket.h>
#include <amqp.h>
#include <amqp_framing.h>

#include <assert.h>

#include "AmqpConnection.h"
#include "Utils.h"

// ---===***===--- //

bool initAmqp(amqp_connection_state_t &conn, amqp_socket_t *socket, int &status, amqp_bytes_t &queuename) {
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
	};
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
	if (!die_on_amqp_error(amqp_get_rpc_reply(conn), "Declaring queue")) {
		return false;
	}
	queuename = amqp_bytes_malloc_dup(r->queue);
	if (queuename.bytes == NULL) {
	  fprintf(stderr, "Out of memory while copying queue name");
	  return false;
	}

	amqp_queue_bind(conn, 1, queuename, amqp_cstring_bytes(exchange), amqp_cstring_bytes(bindingkey), amqp_empty_table);
	if (!die_on_amqp_error(amqp_get_rpc_reply(conn), "Binding queue")) {
		return false;
	}

	// no-local 0 no-ack 0 exclusive 0 consumer
	amqp_basic_consume(conn, 1, queuename, amqp_cstring_bytes(consumer_name), 1, 0, 0, amqp_empty_table);
	if (!die_on_amqp_error(amqp_get_rpc_reply(conn), "Consuming")) {
		return false;
	}
	
	return true;
}

void closeAmqp(amqp_connection_state_t &conn) {
	die_on_amqp_error(amqp_channel_close(conn, 1, AMQP_REPLY_SUCCESS), "Closing channel");
	die_on_amqp_error(amqp_connection_close(conn, AMQP_REPLY_SUCCESS), "Closing connection");
	die_on_error(amqp_destroy_connection(conn), "Ending connection");
}
