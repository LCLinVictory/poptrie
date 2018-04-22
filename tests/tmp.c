#include "../poptrie.h"
/* Add by lcl */
///*
#include "../buddy.h"
#include "../buddy.c"
#include "../poptrie_private.h"
#include "../poptrie.c"
#include "../poptrie4.c"
//*/
/* Add by lcl */
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>    // for gettimeofday()

#define NUM_THREADS 2

#define TEST_PROGRESS()                              \
    do {                                             \
        printf(".");                                 \
        fflush(stdout);                              \
    } while ( 0 )

struct thread_data{
   int  thread_id;
   //struct poptrie *poptrie;
};

//struct timeval{
//   long int tv_sec; // 秒数
//   long int tv_usec; // 微秒数
//}

struct poptrie *poptrie;

// 线程的运行函数
void* say_hello(void* args)
{
    printf("%s\n", "Hello Runoob！");
    return 0;
}

static int
test_init(struct poptrie *poptrie)
{
   FILE *fp;
   char buf[4096];
   int prefix[4];
   int prefixlen;
   int nexthop[4];
   int ret;
   u32 addr1;
   u32 addr2;
   u64 i;

    /* Load from the linx file */
    //fp = fopen("tests/linx-rib.20141217.0000-p52.txt", "r");
    fp = fopen("linx-rib.20141217.0000-p52.txt", "r");
    if ( NULL == fp ) {
        return -1;
    }

    /* Load the full route */
    i = 0;
    while ( !feof(fp) ) {
        if ( !fgets(buf, sizeof(buf), fp) ) {
            continue;
        }
        ret = sscanf(buf, "%d.%d.%d.%d/%d %d.%d.%d.%d", &prefix[0], &prefix[1],
                     &prefix[2], &prefix[3], &prefixlen, &nexthop[0],
                     &nexthop[1], &nexthop[2], &nexthop[3]);
        if ( ret < 0 ) {
            return -1;
        }

        /* Convert to u32 */
        addr1 = ((u32)prefix[0] << 24) + ((u32)prefix[1] << 16)
            + ((u32)prefix[2] << 8) + (u32)prefix[3];
        addr2 = ((u32)nexthop[0] << 24) + ((u32)nexthop[1] << 16)
            + ((u32)nexthop[2] << 8) + (u32)nexthop[3];

        /* Add an entry */
        ret = poptrie_route_add(poptrie, addr1, prefixlen, (void *)(u64)addr2);
        if ( ret < 0 ) {
            return -1;
        }
        if ( 0 == i % 10000 ) {
            TEST_PROGRESS();
        }
        i++;
    }

    /* Close */
    fclose(fp);

    printf("%s\n", "\n--Load Finish--");
    return 0;
}

static int
test_lookup(void *threadarg)
{
   struct thread_data *my_data;
   my_data = (struct thread_data *) threadarg;
   //struct poptrie *poptrie = my_data->poptrie;
   printf("%s%d\n", "\ntid = ", my_data->thread_id);

   FILE *fp;
   char buf[4096];
   int prefix[4];
   int prefixlen;
   int nexthop[4];
   int ret;
   u32 addr1;
   u32 addr2;
   u64 i;
   int tm;
   char type;
   int tmp1, tmp2;

   struct timeval start, end;
   gettimeofday( &start, NULL );
   printf("start : %d.%d\n", start.tv_sec, start.tv_usec);

   fp = fopen("linx-update.20141217.0000-p52.txt", "r");
    //fp = fopen("rib.20141217.0000", "r");
    if ( NULL == fp ) {
        return -1;
    }

   i = 0;
   while ( !feof(fp) ) {
     if ( !fgets(buf, sizeof(buf), fp) ) {
         continue;
     }
     ret = sscanf(buf, "%d %c %d.%d.%d.%d/%d %d.%d.%d.%d", &tm, &type,
                     &prefix[0], &prefix[1], &prefix[2], &prefix[3], &prefixlen,
                     &nexthop[0], &nexthop[1], &nexthop[2], &nexthop[3]);
      if ( ret < 0 ) {
         return -1;
      }

      /* Convert to u32 */
      addr1 = ((u32)prefix[0] << 24) + ((u32)prefix[1] << 16)
         + ((u32)prefix[2] << 8) + (u32)prefix[3];
      addr2 = ((u32)nexthop[0] << 24) + ((u32)nexthop[1] << 16)
         + ((u32)nexthop[2] << 8) + (u32)nexthop[3];

     /* Search */
      tmp1 = poptrie_lookup(poptrie, addr1);
      tmp2 = poptrie_rib_lookup(poptrie, addr1);
     if (tmp1  !=  tmp2) {
         printf("%s%d%s%d\n", "\nLookUp Error->", tmp1, " != ", tmp2);
         //return -1;
     }
     if ( 0 == i % 1000 ) {
         //sleep(1);
         TEST_PROGRESS();
         printf("%s%d\n", "lp-", poptrie_lookup(poptrie, addr1));
     }
     i++;
   }

   /*
   printf("%s%lld%s%lld%s\n", "\n--", 0x100000000ULL, "--", 0x10000000ULL, "\n");

   for ( i = 0; i < 0x100000000ULL; i++ ) {
     if ( 0 == i % 0x10000000ULL ) {
         if ( NULL == poptrie ) {
           printf("%s\n", "poptrie init error");
           return -1;
       }
         TEST_PROGRESS();
         printf("%s%d\n", "lp-", poptrie_lookup(poptrie, i));
     }
     if ( poptrie_lookup(poptrie, i) != poptrie_rib_lookup(poptrie, i) ) {
         return -1;
     }
   }
   */
   gettimeofday( &end, NULL );
   printf("end   : %d.%d\n", end.tv_sec, end.tv_usec);

   printf("%s%d\n", "\n--LookUp Finish-->", i);
   my_data->thread_id = -1;
   return 0;
}
 
static int
test_linx_update(void *threadarg)
{
   struct thread_data *my_data;
   my_data = (struct thread_data *) threadarg;
   //struct poptrie *poptrie = my_data->poptrie;

   FILE *fp;
   char buf[4096];
   int prefix[4];
   int prefixlen;
   int nexthop[4];
   int ret;
   u32 addr1;
   u32 addr2;
   u64 i;
   int tm;
   char type;


    /* Load from the update file */
    //fp = fopen("tests/linx-update.20141217.0000-p52.txt", "r");
    fp = fopen("linx-update.20141217.0000-p52.txt", "r");
    if ( NULL == fp ) {
        return -1;
    }

    /* Load the full route */
    i = 0;
    while ( !feof(fp) ) {
        if ( !fgets(buf, sizeof(buf), fp) ) {
            continue;
        }
        ret = sscanf(buf, "%d %c %d.%d.%d.%d/%d %d.%d.%d.%d", &tm, &type,
                     &prefix[0], &prefix[1], &prefix[2], &prefix[3], &prefixlen,
                     &nexthop[0], &nexthop[1], &nexthop[2], &nexthop[3]);
        if ( ret < 0 ) {
            return -1;
        }

        /* Convert to u32 */
        addr1 = ((u32)prefix[0] << 24) + ((u32)prefix[1] << 16)
            + ((u32)prefix[2] << 8) + (u32)prefix[3];
        addr2 = ((u32)nexthop[0] << 24) + ((u32)nexthop[1] << 16)
            + ((u32)nexthop[2] << 8) + (u32)nexthop[3];

        if ( 'a' == type ) {
            /* Add an entry (use update) */
            ret = poptrie_route_update(poptrie, addr1, prefixlen,
                                       (void *)(u64)addr2);
            if ( ret < 0 ) {
                return -1;
            }
        } else if ( 'w' == type ) {
            /* Delete an entry */
            ret = poptrie_route_del(poptrie, addr1, prefixlen);
            if ( ret < 0 ) {
                /* Ignore any errors */
            }
        }
        if ( 0 == i % 1000 ) {
            printf("^|-");
            fflush(stdout);
        }
        i++;
    }

    /* Close */
    fclose(fp);
    printf("%s\n", "\n--Update Finish--");
    my_data->thread_id = -1;
    return 0;
}

int main()
{
    //struct poptrie *poptrie = NULL;
    int i;
    int ret;
    int endflag = 0;
    // 定义线程的 id 变量，多个变量使用数组
    pthread_t tids[NUM_THREADS];
    struct thread_data td[NUM_THREADS];

    /* Initialize */
    poptrie = poptrie_init(NULL, 19, 22);
    if ( NULL == poptrie ) {
        printf("%s\n", "poptrie init error");
        return -1;
    }
    test_init(poptrie);
    for (i = 0; i < NUM_THREADS; i++) {
       td[i].thread_id = i;
       //td[i].poptrie = poptrie;
    }

    //参数依次是：创建的线程id，线程参数，调用的函数，传入的函数参数    
    ret = pthread_create(&tids[0], NULL, test_linx_update, (void *)&td[1]);
    if (ret != 0) {
       printf("%s%d\n", "pthread_create error: error_code=", ret);
    }

    ret = pthread_create(&tids[1], NULL, test_lookup, (void *)&td[0]);
    if (ret != 0) {
       printf("%s%d\n", "pthread_create error: error_code=", ret);
    }

    //等各个线程退出后，进程才结束，否则进程强制结束了，线程可能还没反应过来；
    pthread_join(tids[0], NULL);
    pthread_join(tids[1], NULL);

    // /* Release 
    poptrie_release(poptrie);
    printf("%s%d\n", "\n---\tEnd\t---", popcnt(0x100100100100));
    //pthread_exit(NULL);

    return 0;
}