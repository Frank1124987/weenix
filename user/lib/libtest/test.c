#include <test/test.h>

#include <sys/mman.h>
#include <fcntl.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <stdarg.h>

typedef struct test_data {
        int td_passed;
        int td_failed;
} test_data_t;

static void _default_test_fail(const char *file, int line, const char *name, const char *fmt, va_list args);
static void _default_test_pass(int val, const char *file, int line, const char *name, const char *fmt, va_list args);

static test_data_t *_test_data = NULL;
static test_pass_func_t _pass_func = _default_test_pass;
static test_fail_func_t _fail_func = _default_test_fail;

void
test_init(void)
{
        int zfd = -1;

        if (0 > (zfd = open("/dev/zero", O_RDWR, 0))) {
                printf("test_init: open(\"/dev/zero\"): %s\n", strerror(errno));
                goto failed;
        }

        if (MAP_FAILED == (_test_data = mmap(NULL, sizeof(*_test_data), PROT_READ | PROT_WRITE, MAP_SHARED, zfd, 0))) {
                printf("test_init: mmap(\"/dev/zero\"): %s\n", strerror(errno));
                goto failed;
        }

        if (0 > close(zfd)) {
                printf("test_init: close(\"/dev/zero\"): %s\n", strerror(errno));
                goto failed;
        }

        _test_data->td_passed = 0;
        _test_data->td_failed = 0;
        return;

failed:
        printf("could not initialize testing library, exiting\n");
        exit(-1);
}

void
test_fini(const char *name)
{
        if (NULL == name) {
                printf("tests completed:\n");
                printf("%d passed\n", _test_data->td_passed);
                printf("%d failed\n", _test_data->td_failed);
        } else {
                printf("%s subtests completed, ", name);
                printf("%d passed and ", _test_data->td_passed);
                printf("%d failed so far\n", _test_data->td_failed);
        }
}

const char *
test_errstr(int err)
{
        switch (err) {
                case 1:
                        return "EPERM";
                case 2:
                        return "ENOENT";
                case 3:
                        return "ESRCH";
                case 4:
                        return "EINTR";
                case 5:
                        return "EIO";
                case 6:
                        return "ENXIO";
                case 7:
                        return "E2BIG";
                case 8:
                        return "ENOEXEC";
                case 9:
                        return "EBADF";
                case 10:
                        return "ECHILD";
                case 11:
                        return "EAGAIN";
                case 12:
                        return "ENOMEM";
                case 13:
                        return "EACCES";
                case 14:
                        return "EFAULT";
                case 15:
                        return "ENOTBLK";
                case 16:
                        return "EBUSY";
                case 17:
                        return "EEXIST";
                case 18:
                        return "EXDEV";
                case 19:
                        return "ENODEV";
                case 20:
                        return "ENOTDIR";
                case 21:
                        return "EISDIR";
                case 22:
                        return "EINVAL";
                case 23:
                        return "ENFILE";
                case 24:
                        return "EMFILE";
                case 25:
                        return "ENOTTY";
                case 26:
                        return "ETXTBSY";
                case 27:
                        return "EFBIG";
                case 28:
                        return "ENOSPC";
                case 29:
                        return "ESPIPE";
                case 30:
                        return "EROFS";
                case 31:
                        return "EMLINK";
                case 32:
                        return "EPIPE";
                case 33:
                        return "EDOM";
                case 34:
                        return "ERANGE";
                case 35:
                        return "EDEADLK";
                case 36:
                        return "ENAMETOOLONG";
                case 37:
                        return "ENOLCK";
                case 38:
                        return "ENOSYS";
                case 39:
                        return "ENOTEMPTY";
                case 40:
                        return "ELOOP";
                default:
                        return "UNKNOWN";
        }
}

static void
_default_test_fail(const char *file, int line, const char *name, const char *fmt, va_list args)
{
        _test_data->td_failed++;
        if (NULL == fmt) {
                printf("FAILED: %s(%d): %s\n", file, line, name);
        } else {
                printf("FAILED: %s(%d): %s: ", file, line, name);
                vprintf(fmt, args);
                printf("\n");
        }
        printf("td_passed = %d, td_failed = %d\n", _test_data->td_passed, _test_data->td_failed);
}

static void
_default_test_pass(int val, const char *file, int line, const char *name, const char *fmt, va_list args)
{
        _test_data->td_passed++;
}

int
_test_assert(int val, const char *file, int line, const char *name, const char *fmt, ...)
{
        va_list args;
        va_start(args, fmt);

        if (0 == val) {
                if (NULL != _fail_func) {
                        _fail_func(file, line, name, fmt, args);
                }
        } else {
                if (NULL != _pass_func) {
                        _pass_func(val, file, line, name, fmt, args);
                }
        }

        va_end(args);
        return val;
}
