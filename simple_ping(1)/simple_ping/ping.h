#include 	<netinet/in_systm.h>
#include	<netinet/ip.h>
#include	<netinet/ip_icmp.h>
#include	<netinet/icmp6.h>
#include	<netinet/ip6.h>
#include	<sys/types.h>	/* basic system data types */
#include	<sys/socket.h>	/* basic socket definitions */
#include	<sys/time.h>	/* timeval{} for select() */
#include	<time.h>		/* timespec{} for pselect() */
#include	<netinet/in.h>	/* sockaddr_in{} and other Internet defns */
#include	<arpa/inet.h>	/* inet(3) functions */
#include	<netdb.h>
#include	<signal.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<errno.h>
#include 	<pwd.h>
#include	<unistd.h>
#include	<sys/un.h>		/* for Unix domain sockets */
#include	<sys/ioctl.h>
#include	<net/if.h>
#include <stdarg.h>
#include <syslog.h>
#include <pthread.h>
#include <assert.h>
#ifdef  HAVE_SOCKADDR_DL_STRUCT
# include       <net/if_dl.h>
#endif

#define IPV6

#define BUFSIZE		1500
#define MAXLINE         4096

/* globals */
char	 recvbuf[BUFSIZE];
char	 sendbuf[BUFSIZE];

int    datalen;	/* #bytes of data, following ICMP header */
char	*host;
int	 nsent;			/* add 1 for each sendto() */
pid_t pid;			/* our PID */
int	 sockfd;    //本地套接字
int	 verbose;
int    daemon_proc;            /* set nonzero by daemon_init() */

/* function prototypes */
void	 proc_v4(char *, ssize_t, struct timeval *);
void	 proc_v6(char *, ssize_t, struct timeval *);
void	 send_v4(void);
void	 send_v6(void);
void	 readloop(void);
void	 sig_alrm(int);
void	 tv_sub(struct timeval *, struct timeval *);

char * Sock_ntop_host(const struct sockaddr *sa, socklen_t salen);
struct addrinfo* host_serv(const char *host, const char *serv, int family, int socktype);
static void err_doit(int errnoflag, int level, const char *fmt, va_list ap);
void err_quit(const char *fmt, ...);
void err_sys(const char *fmt, ...);

struct proto {
  void	 (*fproc)(char *, ssize_t, struct timeval *);
  void	 (*fsend)(void);
  struct sockaddr  *sasend;	/* sockaddr{} for send, from getaddrinfo */
  struct sockaddr  *sarecv;	/* sockaddr{} for receiving */
  socklen_t	    salen;		/* length of sockaddr{}s */
  int	   	    icmpproto;	/* IPPROTO_xxx value for ICMP */
} *pr;
/*消息格式，可以看到里面有
1、2、根据不同ip类型（ipv4/ipv6）采用的不同首发函数
3、4、socket套接字
5、套接字长度，ipv4与ipv6长度不同
6、icmp协议号，表现ipv与ipv6
*/

/*****************new add************/
enum mismark{
    FREQINVALID,
    FREQNOTNUMBER
};
struct sdata{
    int send;// label is 0
    int recv;//label is 1
    int size;
    int mxsize;
    double min;
    double max;
    double *data;//label is 2;
};
typedef struct sdata sdata;
void sigint_handler(int sig);
int getnum(const char *str);
void errorhandle(int m);
void init_sd(void);
void pushsd(int label,double num);
void* pthread_fun(void *arg);
int freq=0; /*use for c*/
int willfreq=0;
int flowing =0;/*use for flow*/
int quiet=0;/*use for q*/
int havethread=0;/*use for flow*/
sdata sd;
/*****************new add************/

