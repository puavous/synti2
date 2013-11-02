/** Capacities and features for a customized, unique build of
 * the synti2 software synthesizer.
 *
 * Exported by PatchBank.cxx - don't edit manually.
 */
/* Capacities (numeric) */ 
#define NUM_CHANNELS        16  /*Number of channels (patches)*/
#define NUM_DELAY_LINES     8   /*Number of delay lines*/
#define NUM_OPERATORS       4   /*Number of operators/oscillators per channel*/
#define NUM_ENVS            6   /*Number of envelopes per channel*/
#define NUM_ENV_KNEES       5   /*Number of knees per envelope*/
#define NUM_MODULATORS      4   /*Number of controllable modulators per channel*/

/* Enabled features */ 
#define FEAT_APPLY_FM                  /*Use frequency modulation */
#define FEAT_APPLY_ADD                 /*Use addition (source mixing) */
#define FEAT_NOISE_SOURCE              /*Use noise source */
#define FEAT_PITCH_DETUNE              /*Enable detuning of operators */
#define FEAT_DELAY_LINES               /*Enable delay lines */
#define FEAT_OUTPUT_SQUASH             /*Squash function at output */
#define FEAT_RESET_PHASE               /*Enable phase reset on note-on */
#define FEAT_EXTRA_WAVETABLES          /*Generate extra wavetables with harmonics */
#define FEAT_POWER_WAVES               /*Enable squared/cubed waveforms */
#define FEAT_MODULATORS                /*Use modulators / parameter ramps */
#define FEAT_PITCH_BEND                /*Enable the 'pitch bend' modulator */
#define FEAT_NOTE_OFF                  /*Listen to note off messages */
#define FEAT_VELOCITY_SENSITIVITY      /*Respond to velocity */
#define FEAT_LOOPING_ENVELOPES         /*Enable looping envelopes */
#define FEAT_LEGATO                    /*Enable legato */
#define FEAT_PITCH_ENVELOPE            /*Enable pitch envelopes */
#define FEAT_PITCH_SCALING             /*Enable pitch scaling */
#define FEAT_FILTER                    /*Enable the lo/band/hi-pass filter */
#define FEAT_FILTER_RESO_ADJUSTABLE    /*Enable adjustable filter resonance */
#define FEAT_FILTER_RESO_ENVELOPE      /*Enable resonance envelope */
#define FEAT_FILTER_CUTOFF_ENVELOPE    /*Enable cutoff envelope */
#define FEAT_FILTER_FOLLOW_PITCH       /*Enable filter frequency to follow pitch */
#define FEAT_FILTER_OUTPUT_NOTCH       /*Enable notch filter */
#define FEAT_CHANNEL_SQUASH            /*Enable per-channel squash function */
#define FEAT_STEREO                    /*Enable stereo output */
#define FEAT_PAN_ENVELOPE              /*Enable pan envelope */

#define IPAR_ACTIVE                    0
#define IPAR_PHASE                     1
#define IPAR_HARM1                     2
#define IPAR_HARM2                     3
#define IPAR_HARM3                     4
#define IPAR_HARM4                     5
#define IPAR_POWR1                     6
#define IPAR_POWR2                     7
#define IPAR_POWR3                     8
#define IPAR_POWR4                     9
#define IPAR_EAMP1                     10
#define IPAR_EAMP2                     11
#define IPAR_EAMP3                     12
#define IPAR_EAMP4                     13
#define IPAR_EAMPN                     14
#define IPAR_EPIT1                     15
#define IPAR_EPIT2                     16
#define IPAR_EPIT3                     17
#define IPAR_EPIT4                     18
#define IPAR_EPAN                      19
#define IPAR_EFILC                     20
#define IPAR_EFILR                     21
#define IPAR_VS1                       22
#define IPAR_VS2                       23
#define IPAR_VS3                       24
#define IPAR_VS4                       25
#define IPAR_VSN                       26
#define IPAR_VSC                       27
#define IPAR_ELOOP1                    28
#define IPAR_ELOOP2                    29
#define IPAR_ELOOP3                    30
#define IPAR_ELOOP4                    31
#define IPAR_ELOOP5                    32
#define IPAR_ELOOP6                    33
#define IPAR_FMTO1                     34
#define IPAR_FMTO2                     35
#define IPAR_FMTO3                     36
#define IPAR_FMTO4                     37
#define IPAR_ADDTO1                    38
#define IPAR_ADDTO2                    39
#define IPAR_ADDTO3                    40
#define IPAR_ADDTO4                    41
#define IPAR_FILT                      42
#define IPAR_FFOLL                     43
#define IPAR_CSQUASH                   44

#define NUM_IPARS 45


#define FPAR_NULLTGT                   0
#define FPAR_MIXLEV                    1
#define FPAR_MIXPAN                    2
#define FPAR_LV1                       3
#define FPAR_LV2                       4
#define FPAR_LV3                       5
#define FPAR_LV4                       6
#define FPAR_LVN                       7
#define FPAR_LVD                       8
#define FPAR_ENV1K1T                   9
#define FPAR_ENV1K1L                   10
#define FPAR_ENV1K2T                   11
#define FPAR_ENV1K2L                   12
#define FPAR_ENV1K3T                   13
#define FPAR_ENV1K3L                   14
#define FPAR_ENV1K4T                   15
#define FPAR_ENV1K4L                   16
#define FPAR_ENV1K5T                   17
#define FPAR_ENV1K5L                   18
#define FPAR_ENV2K1T                   19
#define FPAR_ENV2K1L                   20
#define FPAR_ENV2K2T                   21
#define FPAR_ENV2K2L                   22
#define FPAR_ENV2K3T                   23
#define FPAR_ENV2K3L                   24
#define FPAR_ENV2K4T                   25
#define FPAR_ENV2K4L                   26
#define FPAR_ENV2K5T                   27
#define FPAR_ENV2K5L                   28
#define FPAR_ENV3K1T                   29
#define FPAR_ENV3K1L                   30
#define FPAR_ENV3K2T                   31
#define FPAR_ENV3K2L                   32
#define FPAR_ENV3K3T                   33
#define FPAR_ENV3K3L                   34
#define FPAR_ENV3K4T                   35
#define FPAR_ENV3K4L                   36
#define FPAR_ENV3K5T                   37
#define FPAR_ENV3K5L                   38
#define FPAR_ENV4K1T                   39
#define FPAR_ENV4K1L                   40
#define FPAR_ENV4K2T                   41
#define FPAR_ENV4K2L                   42
#define FPAR_ENV4K3T                   43
#define FPAR_ENV4K3L                   44
#define FPAR_ENV4K4T                   45
#define FPAR_ENV4K4L                   46
#define FPAR_ENV4K5T                   47
#define FPAR_ENV4K5L                   48
#define FPAR_ENV5K1T                   49
#define FPAR_ENV5K1L                   50
#define FPAR_ENV5K2T                   51
#define FPAR_ENV5K2L                   52
#define FPAR_ENV5K3T                   53
#define FPAR_ENV5K3L                   54
#define FPAR_ENV5K4T                   55
#define FPAR_ENV5K4L                   56
#define FPAR_ENV5K5T                   57
#define FPAR_ENV5K5L                   58
#define FPAR_ENV6K1T                   59
#define FPAR_ENV6K1L                   60
#define FPAR_ENV6K2T                   61
#define FPAR_ENV6K2L                   62
#define FPAR_ENV6K3T                   63
#define FPAR_ENV6K3L                   64
#define FPAR_ENV6K4T                   65
#define FPAR_ENV6K4L                   66
#define FPAR_ENV6K5T                   67
#define FPAR_ENV6K5L                   68
#define FPAR_DT1                       69
#define FPAR_DT2                       70
#define FPAR_DT3                       71
#define FPAR_DT4                       72
#define FPAR_PBAM1                     73
#define FPAR_PBAM2                     74
#define FPAR_PBAM3                     75
#define FPAR_PBAM4                     76
#define FPAR_PBVAL                     77
#define FPAR_FFREQ                     78
#define FPAR_FRESO                     79
#define FPAR_DINLV1                    80
#define FPAR_DINLV2                    81
#define FPAR_DINLV3                    82
#define FPAR_DINLV4                    83
#define FPAR_DINLV5                    84
#define FPAR_DINLV6                    85
#define FPAR_DINLV7                    86
#define FPAR_DINLV8                    87
#define FPAR_DLEN1                     88
#define FPAR_DLEN2                     89
#define FPAR_DLEN3                     90
#define FPAR_DLEN4                     91
#define FPAR_DLEN5                     92
#define FPAR_DLEN6                     93
#define FPAR_DLEN7                     94
#define FPAR_DLEN8                     95
#define FPAR_DLEV1                     96
#define FPAR_DLEV2                     97
#define FPAR_DLEV3                     98
#define FPAR_DLEV4                     99
#define FPAR_DLEV5                     100
#define FPAR_DLEV6                     101
#define FPAR_DLEV7                     102
#define FPAR_DLEV8                     103
#define FPAR_CDST1                     104
#define FPAR_CDST2                     105
#define FPAR_CDST3                     106
#define FPAR_CDST4                     107
#define FPAR_LEGLEN                    108
#define FPAR_PSCALE                    109

#define NUM_FPARS 110

/* FIXME: These need to be written somewhere: */
#define ENV_TRIGGER_STAGE (NUM_ENV_KNEES + 1)
#define ENV_RELEASE_STAGE 2
#define ENV_LOOP_STAGE 1
#define NUM_ENV_DATA (NUM_ENV_KNEES*2)
#define NUM_MAX_OPERATORS 6
#define NUM_MAX_MODULATORS 4
#define NUM_MAX_CHANNELS 256

#define SYNTI2_DELAYSAMPLES 0x10000
#define SYNTI2_MAX_SONGBYTES 30000
#define SYNTI2_MAX_SONGEVENTS 15000
