#ifndef _UTILITITES_H_
#define _UTILITIES_H_

#define MBYTE 0.000001f

#include <errno.h>
#include <linux/limits.h> // PATH_MAX
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

// handling error in fopen
#define FOPEN(file_descriptor, pathname, mode) \
{ \
	do \
	{ \
			if ((file_descriptor = fopen(pathname, mode)) == NULL) \
			{ \
					fprintf(stderr, "File %s could not be opened in given mode <%s>.\n", pathname, mode); \
					exit(1); \
			} \
	} while(0); \
}

#define EXIT_IF_NEQ(variable, expected_value, function_call, error_message) \
	do \
	{ \
 		if ((variable = function_call) != expected_value) \
		{ \
			errno = variable; \
			perror(#error_message); \
			exit(EXIT_FAILURE); \
		} \
	} while(0);

#define EXIT_IF_EQ(variable, expected_value, function_call, error_message) \
	do \
	{ \
                                                                           \
                                                                 \
 		if ((variable = function_call) == expected_value) \
		{ \
			perror(#error_message); \
			exit(EXIT_FAILURE); \
		} \
	} while(0);

#define RETURN_IF_EQ(variable, expected_value, function_call, error_message) \
	if ((variable = function_call) == expected_value) \
		{ \
			perror(#error_message); \
			return variable; \
		}

#define EXIT_IF_NULL(variable, function_call, error_message) \
	do \
	{ \
 		if ((variable = function_call) == NULL) \
		{ \
			perror(#error_message); \
			exit(EXIT_FAILURE); \
		} \
	} while(0);

#define SIGNALSAFE_EXIT_IF_EQ(variable, expected_value, function_call, error_message) \
	do \
	{ \
 		if ((variable = function_call) == expected_value) \
		{ \
			perror(#error_message); \
			_exit(EXIT_FAILURE); \
		} \
	} while(0);



/**
 * @brief Implementation of mkdir -p in C.
 * @returns 0 on success, -1 on failure.
 * @param path cannot be NULL or longer than system-defined PATH_MAX.
 * @exception It sets "errno" to "EINVAL" if any param is not valid. The function may also fail and set "errno"
 * for any of the errors specified in routine "mkdir".
 * @note source: https://gist.github.com/JonathonReinhart/8c0d90191c38af2dcadb102c4e202950
*/
static inline int
mkdir_p(const char *path)
{
	if (!path)
	{
		errno = EINVAL;
		return -1;
	}
	const size_t len = strlen(path);
	char _path[PATH_MAX];
	char *p;
	errno = 0;
	/* Copy string so its mutable */
	if (len > sizeof(_path)-1)
	{
		errno = ENAMETOOLONG;
		return -1;
	}
	strcpy(_path, path);

	/* Iterate the string */
	for (p = _path + 1; *p; p++)
	{
		if (*p == '/')
		{
			/* Temporarily truncate */
			*p = '\0';

			if (mkdir(_path, S_IRWXU) != 0)
			{
				if (errno != EEXIST)
					return -1;
			}

			*p = '/';
		}
	}

	if (mkdir(_path, S_IRWXU) != 0)
	{
		if (errno != EEXIST)
			return -1;
	}

	return 0;
}

/**
 * @brief Saves given content in given filename.
 * @returns 0 on success, -1 on failure.
 * @param path cannot be NULL.
 * @param contents cannot be NULL.
 * @exception It sets "errno" to "EINVAL" if any param is not valid. The function may also fail and set "errno"
 * for any of the errors specified for routines "malloc", "mkdir_p", "fopen", "fputs".
*/
static inline int
savefile(const char* path, const char* contents)
{
	if (!path || !contents)
	{
		errno = EINVAL;
		return -1;
	}
	int len = strlen(path);
	char* tmp_path = (char*) malloc(sizeof(char) * (len + 1));
	if (!tmp_path) return -1;
	memset(tmp_path, 0, len + 1);
	strcpy(tmp_path, path);
	char* tmp = strrchr(tmp_path, '/');
	if (tmp) *tmp = '\0';
	if (mkdir_p(tmp_path) != 0)
	{
		free(tmp_path);
		return -1;
	}
	mode_t mask = umask(033);
	FILE* file = fopen(path, "w+");
	if (!file)
	{
		free(tmp_path);
		return -1;
	}
	free(tmp_path);
	umask(mask);
	if (contents)
	{
		if (fputs(contents, file) == EOF)
		{
			fclose(file);
			return -1;
		}
	}
	fclose(file);
	return 0;
}

#endif /*_UTILITITES_H_*/