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
#define PE_COUNT 1
int pe_count = PE_COUNT;
const uint16_t pe_addrs[] = {0x0000, 0x0001, 0x0100, 0x0101};
void *pe_ptrs[4];
int pe_counter = 0;
int pe_release_barrier_global = 0;
//typedef enum {WORKING, WAITING, RELEASED} pe_counter_state_t;
//pe_counter_state_t pe_counter[PE_COUNT] = {WORKING};

void *shmem_addr (void * addr, int pe)
{
        //return (void *)(((unsigned long long)addr) | (((unsigned long long)pe_addrs[pe])<<47) | (((unsigned long long)1)<<63));
        return pe_ptrs[pe] + (u_int64_t)addr;
}

extern void * pvAlmightyDataCap;

void shmem_init (void)
{
        // Read (x,y) ID from router configuration register.
        uint16_t pe_addr = *(volatile uint16_t *)(pvAlmightyDataCap + 0x20000000000);
        //uint16_t pe_addr = *(cheri_build_data_cap((volatile uint16_t *)0x20000000000, 2, 0xfff));
        for (int i=0; i<PE_COUNT; i++) {
                if (pe_addr==(pe_addrs[i])) pe_id = i;
                pe_ptrs[i] = __builtin_cheri_bounds_set((pvAlmightyDataCap + ((((unsigned long long)pe_addrs[i])<<47) | (((unsigned long long)1)<<63))), 1ull << 47);
        }
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
        int pe_release_barrier_local = shmem_int_g(&pe_release_barrier_global, 0);
        int old_pe_counter = __atomic_fetch_add((int *)shmem_addr(&pe_counter,0), 1, __ATOMIC_RELAXED);
        if (old_pe_counter + 1 == PE_COUNT) {
                shmem_int_p(&pe_counter, 0, 0);
                shmem_int_p(&pe_release_barrier_global, pe_release_barrier_local+1, 0);
        } else while(shmem_int_g(&pe_release_barrier_global, 0)==pe_release_barrier_local);
        /*
        if (pe_id == 0) {
                int again = 1;
                while (again) {
                        again = 0;
                        for (int i = 1; i < PE_COUNT; i++) {
                                if (shmem_char_g(pe_counter + i, 0) != WAITING) {
                                        again = 1;
                                        break;
                                }
                        }
                }
                for (int i = 1; i < PE_COUNT; i++) shmem_char_p(pe_counter + i, RELEASED, 0);
        } else {
                shmem_char_p(pe_counter + pe_id, WAITING, 0);
                while (shmem_char_g(pe_counter + pe_id, 0) == WAITING);
                shmem_char_p(pe_counter + pe_id, WORKING, 0);
        }
        */
}

long long shmem_longlong_fadd (long long *target, long long value,
                                   int pe)
{
        // XXX Make atomic.  LL old, (target); ADD new, old, value; SC new, (target); BEQZ new, LL_Label;
        return __atomic_fetch_add((long long *)shmem_addr(target,pe), value, __ATOMIC_RELAXED);
        //long long old = *target;
        //*target = old + value;
        //return old;
}

void shmem_char_p (char *addr, char value, int pe)
{
        *((volatile char *)(shmem_addr(addr,pe))) = value;
}

void shmem_int_p (int *addr, int value, int pe)
{
        *((volatile int *)(shmem_addr(addr,pe))) = value;
}

void shmem_long_p (long *addr, long value, int pe)
{
        *((long *)(shmem_addr(addr,pe))) = value;
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

long long shmem_longlong_g (long long *addr, int pe)
{
        return *((volatile long long *)(shmem_addr(addr,pe)));
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
        int pe, pe_count, sum;
        for (int i = 0; i < nreduce; i += 1){
                sum = 0;
                for (pe = PE_start, pe_count = 0; pe_count < PE_size; pe += (1<<logPE_stride), pe_count++)
                        sum += ((int *)shmem_addr(source,pe))[i];
                target[i] = sum;
        }
        //shmem_barrier_all();
}

void shmem_longlong_sum_to_all (long long *target, long long *source,
                                    int nreduce, int PE_start,
                                    int logPE_stride, int PE_size,
                                    long long *pWrk, long *pSync)
{
        int pe, pe_count, sum;
        for (int i = 0; i < nreduce; i += 1) {
                sum = 0;
                for (pe = PE_start, pe_count = 0; pe_count < PE_size; pe += (1<<logPE_stride), pe_count++)
                        sum += ((long long *)shmem_addr(source,pe))[i];
                target[i] = sum;
        }
        //shmem_barrier_all();
}

void shmem_broadcast64 (void *target, const void *source,
                        size_t nelems, int PE_root, int PE_start,
                        int logPE_stride, int PE_size, long *pSync)
{
        int pe, pe_count;
        for (int i = 0; i < nelems; i += 1)
        {
                long long bcast_val = ((long long *)shmem_addr(source,PE_root))[i];
                for (pe = PE_start, pe_count = 0; pe_count < PE_size; pe += (1<<logPE_stride), pe_count++)
                        ((long long *)shmem_addr(target,pe))[i] = bcast_val;
        }
        //shmem_barrier_all();
        //for (int i = 0; i < nelems; i += 1)
        //        ((int *)target)[i] = ((int *)source)[i]; // If multiple PEs, copy source[i] to target[i] on each PE.
}

void shmem_quiet (void)
{
        return;
}

void shmem_set_lock (long *lock)
{
        while (__atomic_exchange_n((long long *)shmem_addr(lock,0), 1, __ATOMIC_RELAXED));
}

void shmem_clear_lock (long *lock)
{
        shmem_long_p(lock, 0, 0);
}

void shmem_free (void *ptr)
{
        return vPortFree(ptr);
}

long shmem_malloc_print_lock = 0;
void *shmem_malloc (size_t size)
{
        //shmem_set_lock(&shmalloc_print_lock);
        //printf("shmalloc called: size %zu bytes\r\n", size);
        //shmem_clear_lock(&shmalloc_print_lock);
        void * ptr = pvPortMalloc(size);
        //shmem_set_lock(&shmalloc_print_lock);
        //printf("shmalloc returning %p \r\n", ptr);
        //shmem_clear_lock(&shmalloc_print_lock);
        //shmem_barrier_all();
        return ptr;
}

void *shmem_realloc (void *ptr, size_t size)
{
        return realloc(ptr, size);
}
