#include <iostream>
#include <zookeeper.h>
#include <unistd.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

const char *srv_path = "/srv";

zhandle_t *handle = NULL;
int connected = 0;
int expired = 0;

void watcher(zhandle_t *zkh, int type, int state, const char *path, void *context);
void create_eph_seq_path(const char *path, const char *value);
void create_eph_seq_path_callback(int rc, const char *value, const void *data);

int init()
{
	handle = zookeeper_init("127.0.0.1:2181", watcher, 15000, NULL, 0, 0);
	if (!handle)
		return -1;

	while (connected != 1)
	{
		std::cout << __func__ << ": waiting connected... " << std::endl;
		::sleep(1);
	}
}

int main(int argc, char *argv[])
{
	zoo_set_debug_level(ZOO_LOG_LEVEL_DEBUG);
	init();

	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	struct sockaddr_in serv_addr;
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = 0; /* bind() will choose a random port*/

	bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
	//listen(sockfd,5);

	socklen_t len = sizeof(serv_addr);
	getsockname(sockfd, (struct sockaddr *)&serv_addr, &len);
	char buffer[100];
	inet_ntop(AF_INET, &serv_addr.sin_addr, buffer, sizeof(buffer));

	char msg[1024] = {
		0,
	};
	sprintf(msg, "%s:%d", buffer, serv_addr.sin_port);
	std::cout << msg << std::endl;

	create_eph_seq_path((std::string(srv_path) + "/srv_provider_").c_str(), msg);

	while (true)
	{
		::sleep(3);
	}

	return 0;
}

void watcher(zhandle_t *zkh, int type, int state, const char *path, void *context)
{

	if (type == ZOO_SESSION_EVENT)
	{
		if (state == ZOO_CONNECTED_STATE)
		{
			connected = 1;
		}
		else if (state == ZOO_CONNECTING_STATE)
		{
			if (connected == 1)
				std::cout << __func__ << ": disconnected..." << std::endl;
			connected = 0;
		}
		else if (state == ZOO_EXPIRED_SESSION_STATE)
		{
			expired = 1;
			connected = 0;
			zookeeper_close(zkh);
		}
		else
		{
			connected = 0;
			std::cout << __func__ << ": unknown state:" << state << std::endl;
		}
	}
}

void create_eph_seq_path(const char *path, const char *value)
{
	zoo_acreate(handle, path, value, strlen(value),
				&ZOO_OPEN_ACL_UNSAFE, ZOO_EPHEMERAL | ZOO_SEQUENCE, create_eph_seq_path_callback, NULL);
}

void create_eph_seq_path_callback(int rc, const char *value, const void *data)
{
	switch (rc)
	{
	case ZCONNECTIONLOSS:
		create_eph_seq_path(value, (const char *)data);
		break;

	case ZOK:
		std::cout << __func__ << ": created eph_sql node:" << value << std::endl;
		break;

	case ZNODEEXISTS:
		std::cout << __func__ << ": node already exists" << std::endl;
		break;

	default:
		std::cout << __func__ << ": something went wrong..." << rc << std::endl;
		break;
	}
}