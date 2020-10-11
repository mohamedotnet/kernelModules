#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <linux/ioctl.h>
#include <sys/types.h>

#define RAM_DISK_MAGIC 's'

#define RAM_DISK_ENCRPT _IOW(RAM_DISK_MAGIC, 1, int)
#define RAM_DISK_DECRPT _IOR(RAM_DISK_MAGIC, 2, int)

typedef struct{
  char* passwd;
  int len;
}password_t;


int int main(int argc, char const *argv[]) {
  if (argc <= 1) {
    printf("too few arguments...\n", );
    exit(0);
  }else {
    password_t password;
    char *pass;
    int fd;

    gets(pass);
    password.passwd = pass;
    password.len = strlen(pass);

    fd = open("/dev/ramdiskQ", O_SYNC);

    if (strcmp(argv[2], "-e") == 0) {
      if(ioctl(fd, RAM_DISK_ENCRPT, &password)){
        printf("Encryption problem!\n", );
      }
    }else if (strcmp(argv[2], "-d") == 0) {
      if(ioctl(fd, RAM_DISK_DECRPT, &password)){
        printf("Decryption problem!\n", );
      }
    }
  }
  return 0;
}
