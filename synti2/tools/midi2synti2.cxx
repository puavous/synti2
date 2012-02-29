/** @file midi2synti2.cxx
 *
 * A converter program that filters and transforms standard midi
 * messages (both on-line, and from SMF0 and SMF1 sequence files) to
 * messages that synti2 can receive (also both on-line, and in
 * stand-alone sequence playback mode). A graphical user interface is
 * provided for the on-line mode, and a non-graphical command-line
 * mode is available for the SMF conversion stage. (FIXME: Not all is
 * implemented as of yet)
 *
 * This is made with some more rigor than earlier proof-of-concept
 * hacks, but there is a hard calendar time limitation before
 * Instanssi 2012, and thus many kludges and awkwardness are likely to
 * persist.
 *
 */

