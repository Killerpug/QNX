/*
 * Demonstrate recovery of a shared-memory mutex when a process dies while holding the mutex.
 */

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <sys/neutrino.h>

int mutex_lock_with_recovery(pthread_mutex_t *mtx)
{
    int ret;
    ret = pthread_mutex_lock(mtx);

    if( ret == EINVAL ) {
        ret = SyncMutexRevive_r(mtx);
        if( ret == EOK ) {
            // we have revived the mutex, make sure data is consistent here
            // if we can't recover the data state, destroy the mutex
            return ret;
        } else {
            // maybe someone else revived it first, try to lock again.
            ret = pthread_mutex_lock(mtx);
            return ret;
        }
    }
    return ret;
}

int main(int argc, char *argv[])
{
    int ret;
    char name[256];
    struct sigevent ev;
    int rcvid;
    struct _pulse pulse;

    // setup a channel to get revive events on
    int chid = ChannelCreate(_NTO_CHF_PRIVATE);
    int coid = ConnectAttach(0, 0, chid, _NTO_SIDE_CHANNEL, 0 );

    // create a shared memory object, using pid to make it unique
    sprintf(name, "object%d", getpid());
    int fd = shm_open(name, O_RDWR | O_CREAT | O_EXCL, 0600);
    if (fd == -1) {
        perror("shm_open");
        exit(EXIT_FAILURE);
    }
    ret = ftruncate(fd, 4096);
    if (-1 == ret) {
        perror("ftruncate");
        exit(EXIT_FAILURE);
    }
    pthread_mutex_t *ptr = mmap(0, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (ptr == MAP_FAILED)
    {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    // don't need fd anymore, so close it
    close(fd);

    pthread_mutexattr_t mattr;
    pthread_mutexattr_init( &mattr );
    pthread_mutexattr_setpshared( &mattr, PTHREAD_PROCESS_SHARED );
    pthread_mutex_init( ptr, &mattr );

    // Request an event when/if the mutex goes "dead", that is a process owning it dies.
    SIGEV_PULSE_PTR_INIT(&ev, coid, 10, 0, ptr);
    SyncMutexEvent(ptr, &ev );

    pid_t pid = fork();
    if( pid == 0 ) {
        ret = mutex_lock_with_recovery( ptr );
        printf("lock returned %d in child, exit with mutex held\n", ret );
        exit(0);
    }

    while(1) {
        rcvid = MsgReceive_r(chid, &pulse, sizeof pulse, NULL);
        if( rcvid != 0 ) {
            printf("unexpected error or message from receive, returned %d\n", rcvid);
           continue;
        }
        // attempt to recover mutex
        ret = SyncMutexRevive_r(pulse.value.sival_ptr);
        if( ret == EOK ) {
            // we have recovered the mutex, make sure the data protected by it is sane.
            printf("recovered a mutex\n");
            // if we can't recover the data, destroy the mutex
        } else {
            // test to make sure the mutex is in a sane state
            ret = pthread_mutex_trylock(pulse.value.sival_ptr);
            if(ret == EOK ) {
                // sane, we have the lock
                pthread_mutex_unlock(pulse.value.sival_ptr);
                continue;
            } else if (ret == EBUSY) {
                // sane, someone else has the lock
                continue;
            }
            // the mutex is in an inconsistent state, couldn't be recovered, or the data could not be made consistent
            // system abort or multi-level recovery, life is now more complicated
        }
    }

    shm_unlink(name);
    return 0;
}
