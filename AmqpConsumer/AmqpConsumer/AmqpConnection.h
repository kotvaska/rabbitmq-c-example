#pragma once

// ---===***===--- //
// AMQP SETTINGS   //

extern char const *hostname;
extern int port;
extern char const *exchange;
extern char const *exchange_type;
extern char const *bindingkey;
extern char const *amqp_user;
extern char const *amqp_pass;
extern char const *consumer_name;

extern bool initAmqp(amqp_connection_state_t &conn, amqp_socket_t *socket, int &status, amqp_bytes_t &queuename);
extern void closeAmqp(amqp_connection_state_t &conn);