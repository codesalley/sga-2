#define _POSIX_C_SOURCE 200809L

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define DBFILE "payroll.txt"

typedef struct {
    int  id;
    char name[32];
    int  salary;
} record;

#define RECSIZE ((off_t)sizeof(record))

static void die(const char *msg)
{
    perror(msg);
    exit(EXIT_FAILURE);
}

static record make(int id, const char *name, int salary)
{
    record r;
    memset(&r, 0, sizeof r);
    r.id = id;
    r.salary = salary;
    strncpy(r.name, name, sizeof r.name - 1);
    return r;
}

static void show(int slot, const record *r)
{
    printf("  slot %d (offset %3lld):  id=%d  %-10s  salary=%d\n",
           slot, (long long)(slot * RECSIZE), r->id, r->name, r->salary);
}

int main(void)
{
    record r;

    int fd = open(DBFILE, O_RDWR | O_CREAT | O_TRUNC, 0600);
    if (fd < 0)
        die("open");
    printf("created %s (record size %lld bytes)\n\n", DBFILE, (long long)RECSIZE);

    const char *names[] = { "Alice", "Bob", "Carol" };
    int salaries[] = { 95000, 61000, 88000 };

    printf("writing 3 records:\n");
    for (int i = 0; i < 3; i++) {
        r = make(1001 + i, names[i], salaries[i]);
        if (write(fd, &r, sizeof r) != (ssize_t)sizeof r)
            die("write");
        show(i, &r);
    }

    printf("\nreading slot 2 directly:\n");
    if (lseek(fd, 2 * RECSIZE, SEEK_SET) < 0)
        die("lseek");
    if (read(fd, &r, sizeof r) != (ssize_t)sizeof r)
        die("read");
    show(2, &r);


    printf("\nupdating slot 1's salary to 70000:\n");
    if (lseek(fd, 1 * RECSIZE, SEEK_SET) < 0)
        die("lseek");
    if (read(fd, &r, sizeof r) != (ssize_t)sizeof r)
        die("read");

    r.salary = 70000;

    if (lseek(fd, 1 * RECSIZE, SEEK_SET) < 0)
        die("lseek");
    if (write(fd, &r, sizeof r) != (ssize_t)sizeof r)
        die("write");
    show(1, &r);

    printf("\nfinal contents:\n");
    if (lseek(fd, 0, SEEK_SET) < 0)
        die("lseek");
    for (int i = 0; read(fd, &r, sizeof r) == (ssize_t)sizeof r; i++)
        show(i, &r);

    off_t size = lseek(fd, 0, SEEK_END); 
    printf("\nfile size: %lld bytes (3 records, unchanged by the update)\n",
           (long long)size);

  
    if (close(fd) < 0)
        printf("\n file closed \n");
        die("close");

    return EXIT_SUCCESS;
}
