#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>

#include "esp_log.h"

#include "net_util.h"

static const char * TAG = "net";

int net_connect_timeout(uint32_t addr, uint16_t port, uint32_t connect_ms, uint32_t io_ms)
{
  int sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock < 0) return -1;

  struct sockaddr_in sa;
  memset(&sa, 0, sizeof(sa));
  sa.sin_family = AF_INET;
  sa.sin_port = htons(port);
  sa.sin_addr.s_addr = addr;

  int fl = fcntl(sock, F_GETFL, 0);
  fcntl(sock, F_SETFL, fl | O_NONBLOCK);

  int rc = connect(sock, (struct sockaddr*)&sa, sizeof(sa));
  if (rc < 0 && errno != EINPROGRESS)
  {
    ESP_LOGW(TAG, "connect: errno %d", errno);
    close(sock);
    return -1;
  }

  if (rc < 0)
  {
    fd_set wfds;
    FD_ZERO(&wfds);
    FD_SET(sock, &wfds);
    struct timeval tv;
    tv.tv_sec = connect_ms / 1000;
    tv.tv_usec = (connect_ms % 1000) * 1000;

    if (select(sock + 1, NULL, &wfds, NULL, &tv) <= 0)
    {
      ESP_LOGW(TAG, "connect: timeout");
      close(sock);
      return -1;
    }

    int err = 0;
    socklen_t len = sizeof(err);
    if (getsockopt(sock, SOL_SOCKET, SO_ERROR, &err, &len) < 0 || err)
    {
      ESP_LOGW(TAG, "connect: error %d", err);
      close(sock);
      return -1;
    }
  }

  fcntl(sock, F_SETFL, fl & ~O_NONBLOCK);

  struct timeval io;
  io.tv_sec = io_ms / 1000;
  io.tv_usec = (io_ms % 1000) * 1000;
  setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &io, sizeof(io));
  setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &io, sizeof(io));

  return sock;
}
