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
 * ģ����
 * ��ȡ 
 */
template <int const N>
class Readbuf {
private:
    int sock; // ���
	char lastdet;
    char buf[N + 1];	// ������
    char *t; // ��Ч����ָ��
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
 * ��buffer�е�len�ֽڶ�ȡ��ָ��b��
 *  ���buffer���㹻�����ݣ�ֱ�ӿ�����Ȼ��buffer����move����
 * 	���buffer���������������ȿ����꣬Ȼ��ֱ�Ӵ�sock�ж�ȡ��b��
 */ 
template <int N>
int Readbuf<N>::readn(char *b, int len) {	
	int n = t - buf;
	printf("%d bytes in Readbuf, %d bytes to read", n, len);
	// buffer�Ѿ����㹻�������ֽڣ�ֱ��cpy��Ȼ��move����
	if (n >= len) {
		memcpy(b, buf, len);
		memmove(buf, buf + len, n - len);
		t = buf + n - len;
	}
	else {
	// buffer�е����ݲ������ȿ���һ���֣��ٴ�sock��ֱ�Ӷ�ȡ�������ݵ�b��
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
 * Ҳ�ǽ�buffer�����ݿ�����b��
 */ 
template <int N>
int Readbuf<N>::read_until(char *b, char det) {
	
	// buffer��������
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

	// buffer���кܶ�ռ���Զ�����
	while (t-buf < N / 2) {

		int flags;
        bool ready = sock_ready_to_read(sock);
        if(!ready)
            return -1;

        // t����
		// ����ֱ�ӴӶ�ȡ��buffer
		int n = recv(sock, t, N - (t - buf), 0);

		if (n <= 0) {
			return n;
		}
		// Ѱ���Ƿ��Ѿ���ȡ����del
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