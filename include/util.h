#if !defined(_UTIL_H)
#define _UTIL_H


#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#if !defined(BUFSIZE)
#define BUFSIZE 256
#endif

#if !defined(EXTRA_LEN_PRINT_ERROR)
#define EXTRA_LEN_PRINT_ERROR   512
#endif

#define EXIT_ON_SIGNAL_HANDLING(fun, msg)  \
	if((fun) == -1){	\
		fprintf(stderr, msg); exit(EXIT_FAILURE);	\
	}	
	

#define SYSCALL_EXIT(name, r, sc, str, ...)	\
    if ((r=sc) == -1) {				\
	perror(#name);				\
	int errno_copy = errno;			\
	print_error(str, __VA_ARGS__);		\
	exit(errno_copy);			\
    }

#define SYSCALL_PRINT(name, r, sc, str, ...)	\
    if ((r=sc) == -1) {				\
	perror(#name);				\
	int errno_copy = errno;			\
	print_error(str, __VA_ARGS__);		\
	errno = errno_copy;			\
    }

#define CHECK_EQ_EXIT(name, X, val, str, ...)	\
    if ((X)==val) {				\
        perror(#name);				\
	int errno_copy = errno;			\
	print_error(str, __VA_ARGS__);		\
	exit(errno_copy);			\
    }

#define CHECK_NEQ_EXIT(name, X, val, str, ...)	\
    if ((X)!=val) {				\
        perror(#name);				\
	int errno_copy = errno;			\
	print_error(str, __VA_ARGS__);		\
	exit(errno_copy);			\
    }

/**
 * \brief Procedura di utilita' per la stampa degli errori
 *
 */
static inline void print_error(const char * str, ...) {
    const char err[]="ERROR: ";
    va_list argp;
    char * p=(char *)malloc(strlen(str)+strlen(err)+EXTRA_LEN_PRINT_ERROR);
    if (!p) {
	perror("malloc");
        fprintf(stderr,"FATAL ERROR nella funzione 'print_error'\n");
        return;
    }
    strcpy(p,err);
    strcpy(p+strlen(err), str);
    va_start(argp, str);
    vfprintf(stderr, p, argp);
    va_end(argp);
    free(p);
}


/** 
 * \brief check if the name provided as first argument is a regular file
 * \return  1 -> success, 0 -> not a regular file, -1 -> error
 * the argument size is set only if name is a regular file and if size is not NULL
 */
static inline int isRegular(const char name[], size_t *size) {
  struct stat statbuf;
  if(stat(name, &statbuf) == -1)
    return -1;
  if(!S_ISREG(statbuf.st_mode)) 
    return 0;
  if (size != NULL)
    *size = statbuf.st_size;  
  return 1;
}



/** 
 * \brief Controlla se la stringa passata come primo argomento e' un numero.
 * \return  0 ok  1 non e' un numbero   2 overflow/underflow
 */
static inline int isNumber(const char* s, long* n) {
  if (s==NULL) return 1;
  if (strlen(s)==0) return 1;
  char* e = NULL;
  errno=0;
  long val = strtol(s, &e, 10);
  if (errno == ERANGE) return 2;    // overflow/underflow
  if (e != NULL && *e == (char)0) {
    *n = val;
    return 0;   // successo 
  }
  return 1;   // non e' un numero
}

/**
 * @brief Reads up to given bytes from given descriptor, saves data to given pre-allocated buffer.
 * @returns read size on success, -1 on failure.
 * @exception The function may fail and set "errno" for any of the errors specified for the routine "read".
*/
static inline int
readn(long fd, void* buf, size_t size)
{
	size_t left = size;
	int r;
	char* bufptr = (char*) buf;
	while (left > 0)
	{
		if ((r = read((int) fd, bufptr, left)) == -1)
		{
			if (errno == EINTR) continue;
			return -1;
		}
		if (r == 0) return 0; // EOF
		left -= r;
		bufptr += r;
	}
	return size;
}

/**
 * @brief Writes buffer up to given size to given descriptor.
 * @returns 1 on success, -1 on failure.
 * @exception The function may fail and set "errno" for any of the errors specified for routine "write".
*/
static inline int
writen(long fd, void* buf, size_t size)
{
	size_t left = size;
	int r;
	char* bufptr = (char*) buf;
	while (left > 0)
	{
		if ((r = write((int) fd, bufptr, left)) == -1)
		{
			if (errno == EINTR) continue;
			return -1;
		}
		if (r == 0) return 0;
		left -= r;
		bufptr += r;
	}
	return 1;
}



#endif /* _UTIL_H */
