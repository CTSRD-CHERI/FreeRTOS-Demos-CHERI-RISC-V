/* 
 * Copyright (c) 2022
 *   Jonathan Woodruff
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include "shmem.h"

// A couple of globals; assuming one thread per instance of this code.
int pe_id;
#define PE_COUNT 2
int pe_count = PE_COUNT;
int pe_barrier = 0;
//typedef enum {WORKING, WAITING, RELEASED} pe_barrier_state_t;
//pe_barrier_state_t pe_barrier[PE_COUNT] = {WORKING};

void *shmem_addr (void * addr, int pe)
{
        return (void *)(((unsigned long long)addr) | (((unsigned long long)pe)<<47) | (((unsigned long long)1)<<63));
}

void shmem_init (void)
{
        // Read (x,y) ID from router configuration register.
        pe_id = *(volatile uint16_t *)(0x20000000000);
        //pe_count = 2;
}

void shmem_finalize (void)
{
        // XXX do nothing for now.
}

// My Processing Element number
int shmem_my_pe (void)
{
        // XXX Establish how to identify my node number.
        return pe_id;
}

// Total number of processing elements
int shmem_n_pes (void)
{
        // XXX Establish how to identify number of nodes.
        return pe_count;
}

void shmem_barrier_all (void)
{
        __atomic_fetch_add((int *)shmem_addr(&pe_barrier,0), 1, __ATOMIC_RELAXED);
        while (shmem_int_g(&pe_barrier, 0)>0 && shmem_int_g(&pe_barrier, 0)<PE_COUNT);
        // ToDo XXX surely there's something wrong with this. Need professional help.
        if (pe_id == 0) shmem_int_p(&pe_barrier, 0, 0);
        else while(shmem_int_g(&pe_barrier, 0)==PE_COUNT);
        /*
        if (pe_id == 0) {
                int again = 1;
                while (again) {
                        again = 0;
                        for (int i = 1; i < PE_COUNT; i++) {
                                if (shmem_char_g(pe_barrier + i, 0) != WAITING) {
                                        again = 1;
                                        break;
                                }
                        }
                }
                for (int i = 1; i < PE_COUNT; i++) shmem_char_p(pe_barrier + i, RELEASED, 0);
        } else {
                shmem_char_p(pe_barrier + pe_id, WAITING, 0);
                while (shmem_char_g(pe_barrier + pe_id, 0) == WAITING);
                shmem_char_p(pe_barrier + pe_id, WORKING, 0);
        }
        */
}

long long shmem_longlong_fadd (long long *target, long long value,
                                   int pe)
{
        // XXX Make atomic.  LL old, (target); ADD new, old, value; SC new, (target); BEQZ new, LL_Label;
        long long old = *target;
        *target = old + value;
        return old;
}

void shmem_char_p (char *addr, char value, int pe)
{
        *((volatile char *)(shmem_addr(addr,pe))) = value;
}

void shmem_int_p (int *addr, int value, int pe)
{
        *((volatile int *)(shmem_addr(addr,pe))) = value;
}

void shmem_longlong_p (long long *addr, long long value, int pe)
{
        *((long long *)(shmem_addr(addr,pe))) = value;
}

char shmem_char_g (char *addr, int pe)
{
        return *((volatile char *)(shmem_addr(addr,pe)));
}

int shmem_int_g (int *addr, int pe)
{
        return *((volatile int *)(shmem_addr(addr,pe)));
}

void shmem_longlong_put (long long *dest, const long long *src,
                             size_t nelems, int pe)
{
        for (int i = 0; i < nelems; i += 1)
                ((long long *)shmem_addr(dest,pe))[i] = src[i];
}

void shmem_int_sum_to_all (int *target, int *source, int nreduce,
                               int PE_start, int logPE_stride,
                               int PE_size, int *pWrk, long *pSync)
{
        int pe, pe_count;
        for (int i = 0; i < nreduce; i += 1)
                for (pe = PE_start, pe_count = 0; pe_count < PE_size; pe += (1<<logPE_stride), pe_count++)
                        target[i] += ((int *)shmem_addr(source,pe))[i];
}

void shmem_longlong_sum_to_all (long long *target, long long *source,
                                    int nreduce, int PE_start,
                                    int logPE_stride, int PE_size,
                                    long long *pWrk, long *pSync)
{
        int pe, pe_count;
        for (int i = 0; i < nreduce; i += 1)
                for (pe = PE_start, pe_count = 0; pe_count < PE_size; pe += (1<<logPE_stride), pe_count++)
                        target[i] += ((long long *)shmem_addr(source,pe))[i];
}

void shmem_broadcast64 (void *target, const void *source,
                        size_t nelems, int PE_root, int PE_start,
                        int logPE_stride, int PE_size, long *pSync)
{
        for (int i = 0; i < nelems; i += 1)
                ((int *)target)[i] = ((int *)source)[i]; // If multiple PEs, copy source[i] to target[i] on each PE.
}

void shfree (void *ptr)
{
        return vPortFree(ptr);
}

void *shmalloc (size_t size)
{
        //printf("shmalloc called: size %zu bytes\r\n", size);
        void * ptr = pvPortMalloc(size);
        //printf("shmalloc returning %p \r\n", ptr);
        return ptr;
}

void *shrealloc (void *ptr, size_t size)
{
        return realloc(ptr, size);
}
