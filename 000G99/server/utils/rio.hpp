#include <string>
#include <unistd.h>
#include <sys/select.h>

int readn(int fd, char *buf, int len) {
	int n = 0;
	int m = 0;
	while (n < len)
	{
		bool ready = sock_ready_to_read(fd);
		if(!ready)
			return 1;
		//printf("readn n = %d, len = %d\n", n, len);
		if ((m = recv(fd, buf + n, len - n, 0)) < 0) {
			// auto err = terrno();
			return -1;
		}
		n += m;
	}
	return 0;
}

/**
 * 模板类
 * 读取 
 */
template <int const N>
class Readbuf {
private:
    int sock; // 句柄
	char lastdet;
    char buf[N + 1];	// 缓冲区
    char *t; // 有效数据指针
public:
    Readbuf(int sock_);
	/* return len */
    int read_until(char *b, char det = '\0');
	/* return 0 for success */
	int readn(char *b, int len);
};

template <int N>
Readbuf<N>::Readbuf(int sock_) {
	lastdet = -1;
	t = buf;
	sock = sock_;
}

/**
 * 将buffer中的len字节读取到指定b中
 *  如果buffer有足够的数据，直接拷贝，然后buffer进行move即可
 * 	如果buffer的数据量不够，先拷贝完，然后直接从sock中读取到b中
 */ 
template <int N>
int Readbuf<N>::readn(char *b, int len) {	
	int n = t - buf;
	printf("%d bytes in Readbuf, %d bytes to read", n, len);
	// buffer已经有足够数量的字节，直接cpy，然后move即可
	if (n >= len) {
		memcpy(b, buf, len);
		memmove(buf, buf + len, n - len);
		t = buf + n - len;
	}
	else {
	// buffer中的数据不够，先拷贝一部分，再从sock中直接读取部分数据到b中
		int m = len - n;
		/* __fix this__ */
		memcpy(b, buf, n);
		if (readn(sock, b + n, m) < 0)
			return -1;
		t = buf;
	}
	return 0;
}

/**
 * 也是将buffer的数据拷贝到b中
 */ 
template <int N>
int Readbuf<N>::read_until(char *b, char det) {
	
	// buffer还有数据
	if (t-buf > 0) {
		int n = t - buf;
		char *p = buf;
		bool ok = false;
		while (n > 0) {
			if (*p == det) {
				++p; --n;
				ok = true;
				break;
			}
			++p; --n;
		}
		if (ok) {
			int len = p - buf;
			memcpy(b, buf, len);
			memmove(buf, p, n);
			t = buf + n;
			return len;
		}
	}
	lastdet = det;

	// buffer还有很多空间可以读数据
	while (t-buf < N / 2) {

		int flags;
        bool ready = sock_ready_to_read(sock);
        if(!ready)
            return -1;

        // t代表
		// 尝试直接从读取满buffer
		int n = recv(sock, t, N - (t - buf), 0);

		if (n <= 0) {
			return n;
		}
		// 寻找是否已经读取到了del
		bool ok = false;
		while (n > 0) {
			if (*t == det) {
				++t; --n;
				ok = true;
				break;
			}
			++t; --n;
		}
		if (ok) {
			int len = t - buf;
			memcpy(b, buf, len);
			memmove(buf, t, n);
			t = buf + n;
			return len;
		}
	}

	int len = t - buf;
	memcpy(b, buf, len);
	t = buf;

	return len;
}

#define READBUFN (1024)
typedef Readbuf<READBUFN> Readbuf_;