/**********************************************************************************
*   Created 2018 by Johny Pham
*   tuaanpv@gmail.com
*   Youtube/Tiktok/Instagram: tuaanpv
***********************************************************************************/

#ifndef __WEB_SOCKET_H__
#define __WEB_SOCKET_H__

#include "websocket.h"
#include "connection.h"

#include "event2/event.h"
#include "event2/listener.h"
#include "event2/bufferevent.h"

#include <sys/socket.h>
#include <netinet/in.h>

#include <assert.h>
#include <signal.h>
#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <map>

// using namespace std;

#define LOG(format, ...) fprintf(stdout, format "\n", ##__VA_ARGS__)

typedef struct user
{
	uint32_t id;
	ws_conn_t *wscon;
	string msg;
} user_t;

static struct event_base *base = NULL;
// static evconnlistener *listener = NULL;
std::vector<user_t *> user_vec;

static const uint32_t WS_REQ_ONCE_READ = 1;
// static const uint32_t MAX_WS_REQ_LEN = 10240;
// static const uint64_t MAX_MSG_LEN = 1024000;

// static uint32_t new_msg_count = 0;
static std::vector<std::string> _messages_in;
static std::mutex _receiv_mutex;

// static frame_buffer_t* _messages_out;
static bool _ready_to_send = false;
static std::mutex _send_mutex;

user_t *user_create()
{
	user_t *user = new (nothrow) user_t;
	if (user)
	{
		user->id = 0;
		user->wscon = ws_conn_new();
		user->msg = "";
	}
	return user;
}

void user_destroy(user_t *user)
{
	if (user)
	{
		if (user->wscon)
		{
			ws_conn_free(user->wscon);
		}
		delete user;
	}
}

void frame_recv_cb(void *arg)
{
	user_t *user = (user_t *)arg;
	if (user->wscon->frame->payload_len > 0)
	{
		user->msg += string(user->wscon->frame->payload_data, user->wscon->frame->payload_len);
	}
	if (user->wscon->frame->fin == 1)
	{
		LOG("%s", user->msg.c_str());

		frame_buffer_t *fb = frame_buffer_new(1, 1, user->wscon->frame->payload_len, user->wscon->frame->payload_data);
		_receiv_mutex.lock();
		_messages_in.push_back(fb->data);
		// new_msg_count++;
		_receiv_mutex.unlock();

		if (fb)
		{
			_send_mutex.lock();
			// send to other users
			for (int32_t i = 0; i < user_vec.size(); ++i)
			{
				if (user_vec[i] != user)
				{
#if 1
					send_a_frame(user_vec[i]->wscon, fb) == 0;
					// if (send_a_frame(user_vec[i]->wscon, fb) == 0)
					// {
					// 	LOG("i send a message");
					// }
#endif
				}
			}
			_send_mutex.unlock();

			frame_buffer_free(fb);
		}

		user->msg = "";
	}
	_ready_to_send = true;

	// _send_mutex.lock();
	// if (_messages_out)
	// {
	// 	// send to other users
	// 	for (int32_t i = 0; i < user_vec.size(); ++i)
	// 	{
	// 		if (send_a_frame(user_vec[i]->wscon, _messages_out) == 0)
	// 		{
	// 			LOG("i send a message");
	// 		}
	// 	}

	// 	frame_buffer_free(_messages_out);
	// }
	// _send_mutex.unlock();
}

// void frame_write_cb(void *arg)
// {
// 	_send_mutex.lock();
// 	if(_messages_out)
// 	{			
// 		// send to other users
// 		for (int32_t i = 0; i < user_vec.size(); ++i)
// 		{
// 			if (send_a_frame(user_vec[i]->wscon, _messages_out) == 0)
// 			{
// 				LOG("i send a message");
// 			}
// 		}
// 		frame_buffer_free(_messages_out);
// 	}
// 	_send_mutex.unlock();
// }

void user_disconnect(user_t *user)
{
	if (user)
	{
		// update user list
		for (std::vector<user_t *>::iterator iter = user_vec.begin(); iter != user_vec.end(); ++iter)
		{
			if (*iter == user)
			{
				user_vec.erase(iter);
				break;
			}
		}
		user_destroy(user);

		if(user_vec.size() == 0) _ready_to_send = false;
	}
	LOG("now, %d users connecting", user_vec.size());
}

void user_disconnect_cb(void *arg)
{
	LOG("%s", __func__);
	user_t *user = (user_t *)arg;
	user_disconnect(user);
}

void listencb(struct evconnlistener *listener, evutil_socket_t clisockfd, struct sockaddr *addr, int len, void *ptr)
{
	struct event_base *eb = evconnlistener_get_base(listener);
	struct bufferevent *bev = bufferevent_socket_new(eb, clisockfd, BEV_OPT_CLOSE_ON_FREE);
	LOG("a user logined in, socketfd = %d", bufferevent_getfd(bev));

	// create a user
	user_t *user = user_create();
	user->wscon->bev = bev;
	user_vec.push_back(user);
	// ws_conn_setcb(wscon, HANDSHAKE, testfunc, (void*)"haha");
	ws_conn_setcb(user->wscon, FRAME_RECV, frame_recv_cb, user);
	// ws_conn_setcb(user->wscon, WRITE, frame_write_cb, user);
	ws_conn_setcb(user->wscon, CLOSE, user_disconnect_cb, user);

	ws_serve_start(user->wscon);
}

static void run_websocket()
{
	// SIGPIPE ignore
	struct sigaction act;
	act.sa_handler = SIG_IGN;
	if (sigaction(SIGPIPE, &act, NULL) == 0)
	{
		LOG("SIGPIPE ignore");
	}

	// initialize
	setbuf(stdout, NULL);
	base = event_base_new();
	assert(base);

	struct sockaddr_in srvaddr;
	srvaddr.sin_family = AF_INET;
	srvaddr.sin_addr.s_addr = INADDR_ANY;
	srvaddr.sin_port = htons(10086);

	evconnlistener *listener = NULL;
	listener = evconnlistener_new_bind(base, listencb, NULL, LEV_OPT_REUSEABLE | LEV_OPT_CLOSE_ON_FREE, 500, (const struct sockaddr *)&srvaddr, sizeof(struct sockaddr));
	assert(listener);

	event_base_dispatch(base);

	LOG("loop exit");

	evconnlistener_free(listener);
	event_base_free(base);
}

class WebSocket
{
public:
	void thread_for_run()
	{
		if (!_ws_listen_thread.joinable())
			_ws_listen_thread = std::thread(&WebSocket::WSListen, this);

		if(!_ws_transfer_data_thread.joinable())
			_ws_transfer_data_thread = std::thread(&WebSocket::WSTransferData, this);
	}

	void WSListen()
	{
		run_websocket();
	}

	void WSTransferData()
	{
		while(1)
		{
			if(_ready_to_send)
			{
				_send_mutex.lock();
				if(user_vec.size() && _put_msg->getSize())
				{
					// char *cstr = new char[str.length() + 1];
					// strcpy(cstr, str.c_str());
					std::string msg = _put_msg->pop();
					frame_buffer_t* _messages_out = frame_buffer_new(1, 1, msg.length(), msg.c_str());
					// _put_msg->erase(_put_msg->begin());

					if (_messages_out)
					{
						// send to other users
						for (int32_t i = 0; i < user_vec.size(); ++i)
						{
							send_a_frame(user_vec[i]->wscon, _messages_out);
							// if (send_a_frame(user_vec[i]->wscon, _messages_out) == 0)
							// {
							// 	LOG("i send a message");
							// }
						}

						frame_buffer_free(_messages_out);
					}
				}
				_send_mutex.unlock();

				_receiv_mutex.lock();
				for(size_t i = 0; i < _messages_in.size(); i++)
				{
					_get_msg->send(std::move(std::string(_messages_in[i])));
					_messages_in.erase(_messages_in.begin() + i);
				}
				_receiv_mutex.unlock();
			}
			else
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(100));	// wait for user(s) connected
			}
		}
	}

	void setWSQueue(std::shared_ptr<sBuffer<std::string>> put_msg, std::shared_ptr<sBuffer<std::string>> get_msg)
	{
		_put_msg = put_msg;
		_get_msg = get_msg;
	}

private:
	std::thread _ws_listen_thread;
	std::thread _ws_transfer_data_thread;

	std::shared_ptr<sBuffer<std::string>> _put_msg = nullptr;
	std::shared_ptr<sBuffer<std::string>> _get_msg = nullptr;
};

#endif