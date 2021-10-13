#pragma once

#define __attribute_const__
#include <unistd.h>

#include <stdbool.h>

#define SG_CHUNKS_SIZE 128
#define SG_ALL 128

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long u64;
typedef unsigned int uint;
typedef short s16;
typedef int s32;
typedef long s64;

typedef u64 __be64;
typedef u64 __le64;
typedef u64 __u64;
typedef u32 __be32;
typedef u32 __le32;
typedef u32 __u32;
typedef u16 __be16;
typedef u16 __le16;
typedef u16 __u16;
typedef u8 __u8;


#define __user
#define __force
#define __iomem
#define __must_check
#define __bitwise

typedef u64 dma_addr_t;
typedef u64 phys_addr_t;
#include <time.h>
typedef time_t ktime_t;
typedef u64 sector_t;


struct seq_file;

struct blk_mq_tag_set { };


#define HZ 100

typedef struct spinlock { } spinlock_t;

typedef struct mutex { } mutex_t;

struct rw_semaphore { };

typedef struct atomic {
	int v;
} atomic_t;

static inline void atomic_set(atomic_t *v, int i)
{
	v->v = i;
}

struct device {
	struct device *parent;
};

struct completion { };

typedef struct wait_queue_head { } wait_queue_head_t;

struct list_head { };

struct delayed_work { };

struct work_struct { };

struct workqueue_struct { };

struct device_attribute { };



#define ALTERNATIVE(a,b,cond) a

#include <endian.h>


#define be64_to_cpu(x) be64toh(x)
#define be32_to_cpu(x) be32toh(x)
#define be16_to_cpu(x) be16toh(x)
#define le64_to_cpu(x) le64toh(x)
#define le32_to_cpu(x) le32toh(x)
#define le16_to_cpu(x) le16toh(x)

#define cpu_to_le64(x) htole64(x)
#define cpu_to_le32(x) htole32(x)
#define cpu_to_le16(x) htole16(x)
#define cpu_to_be64(x) htobe64(x)
#define cpu_to_be32(x) htobe32(x)
#define cpu_to_be16(x) htobe16(x)




#include <errno.h>

#include <assert.h>

#define STATIC_ASSERT(e, MSG) \
	((void)sizeof(char[1 - 2*!(e)]))
	// (sizeof(struct { int:-!(e); }))
	// typedef char static_assertion_##MSG[(COND)?1:-1]


#define BUILD_BUG_ON(e) STATIC_ASSERT(!(e), default_msg)
#define BUILD_BUG_ON_MSG(e, msg) STATIC_ASSERT(!(e), msg)
// (sizeof(struct { int:-!(e); }))
#define BUG_ON(e) assert(!(e))


#define EXPORT_SYMBOL_GPL(x)
#define EXPORT_SYMBOL(x)

#include <stdlib.h>
#define kfree free
#define kmalloc(x,f) malloc(x)
#define kzalloc(x,f) calloc(1,x)


enum {
	DUMP_PREFIX_NONE,
	DUMP_PREFIX_ADDRESS,
	DUMP_PREFIX_OFFSET
};
// #define print_hex_dump(...) (void)(__VA_ARGS__)
static inline void print_hex_dump(const char *prefix_str,
				  int prefix_type, int rowsize, int groupsize,
				  const void *buf, size_t len, bool ascii)
{
}


/**
 * enum irqreturn
 * @IRQ_NONE		interrupt was not from this device or was not handled
 * @IRQ_HANDLED		interrupt was handled by this device
 * @IRQ_WAKE_THREAD	handler requests to wake the handler thread
 */
enum irqreturn {
	IRQ_NONE		= (0 << 0),
	IRQ_HANDLED		= (1 << 0),
	IRQ_WAKE_THREAD		= (1 << 1),
};

typedef enum irqreturn irqreturn_t;
#define IRQ_RETVAL(x)	((x) ? IRQ_HANDLED : IRQ_NONE)


typedef int async_cookie_t;

typedef irqreturn_t (*irq_handler_t)(int irq, void *__hba);


#define IRQF_SHARED 0xFFFF /* Faked */
static inline int
request_irq(unsigned int irq, irq_handler_t handler, unsigned long flags,
	    const char *name, void *dev)
{
	return 0;
}
static inline void *free_irq(unsigned int irq, void *p)
{
	return NULL;
}

#include <stdio.h>
#define dev_err(dev, fmt, ...) \
	fprintf(stderr, fmt, ##__VA_ARGS__)
#define dev_dbg dev_err
#define dev_warn dev_err

static inline int atomic_dec_and_test(struct atomic *a)
{
	a->v--;
	return a->v;
}

static inline int atomic_inc_return(struct atomic *a)
{
	a->v++;
	return a->v;
}


#define __same_type(a, b) \
	__builtin_types_compatible_p(typeof(a), typeof(b))


#ifndef offsetof
#define offsetof(TYPE, MEMBER)	((size_t)&((TYPE *)0)->MEMBER)
#endif

#define container_of(ptr, type, member) ({				\
	void *__mptr = (void *)(ptr);					\
	BUILD_BUG_ON_MSG(!__same_type(*(ptr), ((type *)0)->member) &&	\
			 !__same_type(*(ptr), void),			\
			 "pointer type mismatch in container_of()");	\
	((type *)(__mptr - offsetof(type, member))); })

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

struct rcu_head { };

#define __rcu

#define DIV_ROUND_UP(n, d) (((n) + (d) - 1) / (d))

#define BITS_PER_LONG 64
#define BITS_PER_LONG_LONG 64

#define BIT(nr)			((1UL) << (nr))
#define BIT_ULL(nr)		((1ULL) << (nr))
#define BIT_MASK(nr)		((1UL) << ((nr) % BITS_PER_LONG))
#define BIT_WORD(nr)		((nr) / BITS_PER_LONG)
#define BIT_ULL_MASK(nr)	((1ULL) << ((nr) % BITS_PER_LONG_LONG))
#define BIT_ULL_WORD(nr)	((nr) / BITS_PER_LONG_LONG)
#define BITS_PER_BYTE		8
#define BITS_PER_TYPE(type) (sizeof(type) * BITS_PER_BYTE)
#define BITS_TO_LONGS(nr) DIV_ROUND_UP(nr, BITS_PER_TYPE(long))

#define DECLARE_BITMAP(name,bits) \
	unsigned long name[BITS_TO_LONGS(bits)]

static inline int
_find_next_zero_bit_le(const unsigned long *addr, unsigned int maxbit,
		       int offset)
{
	int b;
	int bits_per_long = BITS_PER_TYPE(long);
	int counter = bits_per_long;
	for (b = 0; b < maxbit; ++b, ++counter) {
		if (counter == bits_per_long) {
			addr++;
			counter = 0;
		}
		if (counter < offset) continue;
		if ((*addr & (1UL << counter)) == 0)
			return b;
	}
	return b;
}

static inline int
_find_next_bit_le(const unsigned long *addr, unsigned int maxbit,
		  int offset)
{
	int b;
	int bits_per_long = BITS_PER_TYPE(long);
	int counter = bits_per_long;
	for (b = 0; b < maxbit; ++b, ++counter) {
		if (counter == bits_per_long) {
			addr++;
			counter = 0;
		}
		if (counter < offset) continue;
		if ((*addr & (1UL << counter)) != 0)
			return b;
	}
	return b;
}

static inline int
_find_first_zero_bit_le(const unsigned long *addr, unsigned int maxbit)
{
	return _find_next_zero_bit_le(addr, maxbit, 0);
}

static inline int
_find_first_bit_le(const unsigned long *addr, unsigned int maxbit)
{
	return _find_next_bit_le(addr, maxbit, 0);
}

int _find_last_zero_bit_le(const unsigned long *addr, unsigned int maxbit);

static inline int
_find_last_bit_le(const unsigned long *addr, unsigned int maxbit)
{
	int ret = maxbit;
	int b;
	int bits_per_long = BITS_PER_TYPE(long);
	int counter = bits_per_long;
	for (b = 0; b < maxbit; ++b, ++counter) {
		if (counter == bits_per_long) {
			addr++;
			counter = 0;
		}
		if ((*addr & (1UL << counter)) == 0) {
			ret = b;
		}
	}
	return ret;
}

/*
 * These are the little endian, atomic definitions.
 */
#define find_first_zero_bit(p,sz)	_find_first_zero_bit_le(p,sz)
#define find_next_zero_bit(p,sz,off)	_find_next_zero_bit_le(p,sz,off)
#define find_first_bit(p,sz)		_find_first_bit_le(p,sz)
#define find_next_bit(p,sz,off)		_find_next_bit_le(p,sz,off)
#define find_last_zero_bit(p,sz)	_find_last_zero_bit_le(p,sz)
#define find_last_bit(p,sz)		_find_last_bit_le(p,sz)



#define for_each_set_bit(bit, addr, size) \
	for ((bit) = find_first_bit((addr), (size));		\
	     (bit) < (size);					\
	     (bit) = find_next_bit((addr), (size), (bit) + 1))



struct execute_work { };

#define __printf(x,y)

struct kref { };

typedef u32 gfp_t;
typedef u32 req_flags_t;
typedef u32 blk_status_t;


struct request;
static inline void *blk_mq_rq_to_pdu(struct request *rq)
{
	BUG_ON(1);
	//return rq + 1;
	return NULL;
}

enum dma_data_direction {
	DMA_BIDIRECTIONAL = 0,
	DMA_TO_DEVICE = 1,
	DMA_FROM_DEVICE = 2,
	DMA_NONE = 3,
};

struct page;

#define PAGE_SHIFT 12
#ifndef PAGE_SIZE
#define PAGE_SIZE (1<<12)
#endif

static inline sector_t blk_rq_pos(const struct request *rq)
{
	BUG_ON(1);
	return 0;
//	return rq->__sector;
}

static inline void *dmam_alloc_coherent(struct device *dev, size_t size,
		dma_addr_t *dma_handle, gfp_t gfp)

{
	return malloc(size);
}

static inline void *devm_kcalloc(struct device *dev,
				 size_t n, size_t size, gfp_t flags)
{
	return calloc(n, size);
}




#define GFP_KERNEL 0 /* faked */


#define __branch_check__(x, expect, is_constant) ({			\
			long ______r;					\
			______r = __builtin_expect(!!(x), expect);	\
			______r;					\
		})



#if 0
# ifndef likely
#  define likely(x)	(__branch_check__(x, 1, __builtin_constant_p(x)))
# endif
# ifndef unlikely
#  define unlikely(x)	(__branch_check__(x, 0, __builtin_constant_p(x)))
# endif
#endif



#define __WARN_printf(arg...)	do { fprintf(stderr, arg); } while (0)

#define WARN(condition, format...) ({		\
	int __ret_warn_on = !!(condition);	\
	if (unlikely(__ret_warn_on))		\
		__WARN_printf(format);		\
	unlikely(__ret_warn_on);		\
})

#define WARN_ON(condition) ({					\
	int __ret_warn_on = !!(condition);			\
	if (unlikely(__ret_warn_on))				\
		__WARN_printf("assertion failed at %s:%d\n",	\
				__FILE__, __LINE__);		\
	unlikely(__ret_warn_on);				\
})


#define upper_32_bits(n) ((u32)(((n) >> 16) >> 16))

#define lower_32_bits(n) ((u32)(n))


static inline void mutex_init(struct mutex *m) { }
static inline void mutex_lock(struct mutex *m) { }
static inline void mutex_unlock(struct mutex *m) { }
static inline void init_rwsem(struct rw_semaphore *m) { }
static inline void down_read(struct rw_semaphore *m) { }
static inline void down_write(struct rw_semaphore *m) { }
static inline void up_read(struct rw_semaphore *m) { }
static inline void up_write(struct rw_semaphore *m) { }
static inline void spin_lock(spinlock_t *m) { }
static inline void spin_unlock(spinlock_t *m) { }
#define spin_lock_irqsave(l,f) __spin_lock_irqsave(l,&f)
#define spin_unlock_irqrestore(l,f) __spin_unlock_irqrestore(l,&f)
static inline void __spin_lock_irqsave(spinlock_t *m, unsigned long *flags) { }
static inline void __spin_unlock_irqrestore(spinlock_t *m, unsigned long *flags) { }


static inline int ktime_to_us(time_t t)
{
	return t;
}

static inline void schedule_work()
{
	BUG_ON(1);
}

static inline void wake_up()
{
	BUG_ON(1);
}
static inline void complete()
{
	BUG_ON(1);
}

#define WRITE_ONCE(var, val) \
		(*((volatile typeof(val) *)(&(var))) = (val))

#define READ_ONCE(var) (*((volatile typeof(var) *)(&(var))))

typedef long atomic_long_t;

//////// FIXME
static inline long
atomic_long_fetch_add_acquire(long i, atomic_long_t *v)
{
	long _v = *v;
	*v += i;
	return _v;
}
static inline long
atomic_long_fetch_or_acquire(long i, atomic_long_t *v)
{
	long _v = *v;
	*v |= i;
	return _v;
}

static inline long
atomic_long_fetch_andnot_release(long i, atomic_long_t *v)
{
	long _v = *v;
	*v &= ~i;
	return _v;
}
static inline void
atomic_long_set_release(atomic_long_t *v, long i)
{
	*v = i;
}


/**
 * test_and_set_bit_lock - Set a bit and return its old value, for lock
 * @nr: Bit to set
 * @addr: Address to count from
 *
 * This operation is atomic and provides acquire barrier semantics if
 * the returned value is 0.
 * It can be used to implement bit locks.
 */
static inline int test_and_set_bit_lock(unsigned int nr,
					volatile unsigned long *p)
{
	long old;
	unsigned long mask = BIT_MASK(nr);

	p += BIT_WORD(nr);
	if (READ_ONCE(*p) & mask)
		return 1;

	old = atomic_long_fetch_or_acquire(mask, (atomic_long_t *)p);
	return !!(old & mask);
}


/**
 * clear_bit_unlock - Clear a bit in memory, for unlock
 * @nr: the bit to set
 * @addr: the address to start counting from
 *
 * This operation is atomic and provides release barrier semantics.
 */
static inline void clear_bit_unlock(unsigned int nr, volatile unsigned long *p)
{
	p += BIT_WORD(nr);
	atomic_long_fetch_andnot_release(BIT_MASK(nr), (atomic_long_t *)p);
}

/**
 * __clear_bit_unlock - Clear a bit in memory, for unlock
 * @nr: the bit to set
 * @addr: the address to start counting from
 *
 * A weaker form of clear_bit_unlock() as used by __bit_lock_unlock(). If all
 * the bits in the word are protected by this lock some archs can use weaker
 * ops to safely unlock.
 *
 * See for example x86's implementation.
 */
static inline void __clear_bit_unlock(unsigned int nr,
				      volatile unsigned long *p)
{
	unsigned long old;

	p += BIT_WORD(nr);
	old = READ_ONCE(*p);
	old &= ~BIT_MASK(nr);
	atomic_long_set_release((atomic_long_t *)p, old);
}


#include <string.h>

static inline void usleep_range(u32 a, u32 b)
{
	int i;
	volatile double f = 22;
	for (i = 0; i < 1000000; ++i)
		f = f * 1.230 / 3.01 * 3.11;

}

typedef void (*async_func_t) (void *data, async_cookie_t cookie);

static inline void
async_schedule(async_func_t func, void *data)
{
	func(data, 0);
}


#define wait_event(wq_head, condition)			\
	do {						\
		if (condition)				\
			break;				\
	} while (1)


static inline void __set_bit(int nr, volatile unsigned long *addr)
{
	unsigned long mask = BIT_MASK(nr);
	unsigned long *p = ((unsigned long *)addr) + BIT_WORD(nr);

	*p  |= mask;
}

static inline void __clear_bit(int nr, volatile unsigned long *addr)
{
	unsigned long mask = BIT_MASK(nr);
	unsigned long *p = ((unsigned long *)addr) + BIT_WORD(nr);

	*p &= ~mask;
}

#define MIN(a,b) ((a)<(b)?(a):(b))

#define ETIMEDOUT       110
#define EOPNOTSUPP      95
#define ENOTSUP         EOPNOTSUPP
#define EIO              5
#define ENXIO            6
#define ENODEV          19

#define __unused__ __attribute__((unused))
