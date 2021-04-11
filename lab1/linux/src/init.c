#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/sysmacros.h>

int main() {
     char* const argv_execv1[]={"1",NULL};
     char* const argv_execv2[]={"2",NULL};
     char* const argv_execv3[]={"3",NULL};
//     if (mknod("./dev/ttyS0", S_IFCHR | S_IRUSR | S_IWUSR, makedev(4, 64)) == -1) {
//        perror("mknod() failed");
//    }
    if (mknod("./dev/ttyAMA0", S_IFCHR | S_IRUSR | S_IWUSR, makedev(204, 64)) == -1) {
        perror("mknod() failed");
    }
    if (mknod("./dev/fb0", S_IFCHR | S_IRUSR | S_IWUSR, makedev(29, 0)) == -1) {
        perror("mknod() failed");
   }
   if(vfork() == 0)
        if(execv("1",argv_execv1)<0)
            {
            	printf("error1\n");
            	perror("Error on execv");
                return -1;
            }
   wait(NULL);
   if(vfork() == 0)
        if(execv("2",argv_execv2)<0)
            {
            	 printf("error2\n");
            	 perror("Error on execv");
                return -1;
            }
   wait(NULL);
   if(vfork() == 0)
        if(execv("3",argv_execv3)<0)
            {	
            	printf("error3\n");
            	perror("Error on execv");
                return -1;
            }
     wait(NULL);
    int input;
    scanf("%d",&input);
}
