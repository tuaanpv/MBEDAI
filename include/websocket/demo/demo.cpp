#include "demo.h"


user_t *user_create() {
	user_t *user = new (nothrow) user_t;
	if (user) {
		user->id = 0;
		user->wscon = ws_conn_new();
		user->msg = "";
	}
	return user;
}

static struct event_base *base = NULL;
static evconnlistener *listener = NULL;
std::vector<user_t*> user_vec;


static const uint32_t WS_REQ_ONCE_READ = 1;
//static const uint32_t MAX_WS_REQ_LEN = 10240;
//static const uint64_t MAX_MSG_LEN = 1024000;


#define LOG(format, ...) fprintf(stdout, format"\n", ## __VA_ARGS__)


void user_disconnect(user_t *user) {
	if (user) {
		//update user list
		for (std::vector<user_t*>::iterator iter = user_vec.begin(); iter != user_vec.end(); ++iter) {
			if (*iter == user) {
				user_vec.erase(iter);
				break;
			}
		}
		user_destroy(user);
	}
	LOG("now, %d users connecting", user_vec.size());
}


void user_disconnect_cb(void *arg) {
	LOG("%s", __func__);
	user_t *user = (user_t*)arg;
	user_disconnect(user);
}


void listencb(struct evconnlistener *listener, evutil_socket_t clisockfd, struct sockaddr *addr, int len, void *ptr) {
	struct event_base *eb = evconnlistener_get_base(listener);
	struct bufferevent *bev = bufferevent_socket_new(eb, clisockfd, BEV_OPT_CLOSE_ON_FREE);
	LOG("a user logined in, socketfd = %d", bufferevent_getfd(bev));

	//create a user
	user_t *user = user_create();
	user->wscon->bev = bev;
	user_vec.push_back(user);
	//ws_conn_setcb(wscon, HANDSHAKE, testfunc, (void*)"haha");
	ws_conn_setcb(user->wscon, FRAME_RECV, frame_recv_cb, user);
	ws_conn_setcb(user->wscon, CLOSE, user_disconnect_cb, user);

	ws_serve_start(user->wscon);
}


void user_destroy(user_t *user) {
	if (user) {
		if (user->wscon) {
			ws_conn_free(user->wscon);
		}
		delete user;
	}
}


void frame_recv_cb(void *arg) {
	user_t *user = (user_t*)arg;
	if (user->wscon->frame->payload_len > 0) {
		user->msg += string(user->wscon->frame->payload_data, user->wscon->frame->payload_len);
	}
	if (user->wscon->frame->fin == 1) {
		LOG("%s", user->msg.c_str());

		frame_buffer_t *fb = frame_buffer_new(1, 1, user->wscon->frame->payload_len, user->wscon->frame->payload_data);

		if (fb) {
			//send to other users
			for (int32_t i = 0; i < user_vec.size(); ++i) {
				if (user_vec[i] != user) {
#if 1
					if (send_a_frame(user_vec[i]->wscon, fb) == 0) {
						LOG("i send a message");
					}
#endif
				}
			}

			frame_buffer_free(fb);
		}

		user->msg = "";
	}
}



int main() {
#ifdef WIN32
	WORD wVersionRequested;
	WSADATA wsaData;
	int err;

	/* Use the MAKEWORD(lowbyte, highbyte) macro declared in Windef.h */
	wVersionRequested = MAKEWORD(2, 2);

	err = WSAStartup(wVersionRequested, &wsaData);
	if (err != 0) {
		/* Tell the user that we could not find a usable */
		/* Winsock DLL.                                  */
		printf("WSAStartup failed with error: %d\n", err);
	}

#else
	//SIGPIPE ignore
	struct sigaction act;
	act.sa_handler = SIG_IGN;
	if (sigaction(SIGPIPE, &act, NULL) == 0) {
		LOG("SIGPIPE ignore");
	}
#endif

	//initialize
	setbuf(stdout, NULL);
	base = event_base_new();
	assert(base);

	struct sockaddr_in srvaddr;
	srvaddr.sin_family = AF_INET;
	srvaddr.sin_addr.s_addr = INADDR_ANY;
	srvaddr.sin_port = htons(10086);

	listener = evconnlistener_new_bind(base, listencb, NULL, LEV_OPT_REUSEABLE | LEV_OPT_CLOSE_ON_FREE, 500, (const struct sockaddr*)&srvaddr, sizeof(struct sockaddr));
	assert(listener);

	event_base_dispatch(base);

	LOG("loop exit");

	evconnlistener_free(listener);
	event_base_free(base);

	return 0;
}
