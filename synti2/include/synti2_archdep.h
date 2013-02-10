#ifndef SYNTI2_ARCHDEP_DEFINED
#endif

/* TODO: This should basically be generated by a configurator
 * program. So far the only assumption seems to be that UINT_MAX is a
 * 32 bit value, so that the full length can be used for counters and
 * the division to wavetable length can be done by a bit shift of 16
 * bytes.
 */

/* Maximum value of the counter type depends on C implementation, so
 * use limits.h -- Actually should probably use C99 and stdint.h but
 * that's going to be in some later project.
 */
#define MAX_COUNTER UINT_MAX

/* The wavetable sizes and divisor-bitshift must be consistent. */
#define WAVETABLE_SIZE 0x10000
#define WAVETABLE_BITMASK 0xffff
/* FIXME: This is implementation dependent! Hmm... is there some way
 * to get the implementation-dependent bit-count here? Sure.. but it
 * would require some configuration script.. Note that some
 * configuration scrpit will appear at some point... There could be
 * just a c code that tries out these things and then writes some
 * "platform.h" with possibly varying stuff. And if 0.0f is not
 * encoded as all-zero-bits, then that's it for the show on such a
 * platform...
 */
#define COUNTER_TO_TABLE_SHIFT 16
