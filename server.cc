#include <iostream>
#include <zookeeper.h>
#include <string.h>
#include <unistd.h>

const char *srv_path = "/srv";

zhandle_t *handle = NULL;
int connected = 0;
int expired = 0;

void watcher(zhandle_t *zkh, int type, int state, const char *path, void *context);
void create_path(const char *path, const char *value);
void create_path_callback(int rc, const char *value, const void *data);
void show_info();
void info_get_callback(int rc, const struct String_vector *strings, const void *data);

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

	// zoo_set_debug_level(ZOO_LOG_LEVEL_DEBUG);
	zoo_set_debug_level(ZOO_LOG_LEVEL_ERROR);
	init();

	create_path(srv_path, "");
	show_info();

	while (true)
	{
		::sleep(20);
	}

	return 0;
}

void show_info()
{
	zoo_awget_children(handle, srv_path, watcher, NULL, info_get_callback, NULL);
}

void create_path(const char *path, const char *value)
{
	zoo_acreate(handle, path, value, strlen(value),
				&ZOO_OPEN_ACL_UNSAFE, 0, create_path_callback, NULL);
}

void info_get_callback(int rc, const struct String_vector *strings, const void *data)
{
	switch (rc)
	{
	case ZCONNECTIONLOSS:
	case ZOPERATIONTIMEOUT:
		show_info();
		break;

	case ZOK:
		for (int i = 0; i < strings->count; i++)
		{
			std::cout << "=>" << strings->data[i] << std::endl;
			char buff[128] = {
				0,
			};
			int len = sizeof(buff);
			zoo_get(handle, (std::string(srv_path) + "/" + strings->data[i]).c_str(),
					0, buff, &len, NULL);
			std::cout << buff << std::endl;
		}
		break;

	default:
		std::cout << __func__ << ": something went wrong..." << rc << std::endl;
		break;
	}
}

void create_path_callback(int rc, const char *value, const void *data)
{
	switch (rc)
	{
	case ZCONNECTIONLOSS:
		create_path(value, (const char *)data);
		break;

	case ZOK:
		std::cout << __func__ << ": created node:" << value << std::endl;
		break;

	case ZNODEEXISTS:
		std::cout << __func__ << ": node already exists" << std::endl;
		break;

	default:
		std::cout << __func__ << ": something went wrong..." << rc << std::endl;
		break;
	}
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
	else if (type == ZOO_CHILD_EVENT)
	{
		if (strcmp(path, srv_path) != 0)
			std::cout << __func__ << ": error path info:" << path << std::endl;

		std::cout << __func__ << ": children event detected!" << std::endl;
		show_info(); // 注意，需要再次安插watch event
	}
}