#ifndef PROFILOMETRE_H
#define PROFILOMETRE_H


/**
 * @mainpage Profilometre profiling library
 * @author Sebastien Roy
 * @version 1.0
 * @date 2010-03
 *
 * Profilometre is a simple library for profiling code.
 * It is meant to be extremely easy to use.
 *
 * <h2>First use : profiling code</h2>
 * 
 * This first usage if to measure the time spent in a piece of code.
 *
 * @code
 * #define USE_PROFILOMETRE
 * #include <profilometre.h>
 *
 * ...
 *
 * // Must call init before any other from the library
 * profilometre_init();
 *
 * ...
 *
 * profilometre_start("example");
 * do_something();
 * profilometre_stop("exemple");
 *
 * ...
 * 
 * profilometre_dump();
 * @endcode
 *
 * <h2>Second use : monitoring an integer over time</h2>
 *
 * The second usage is to gather statistics of a varying integer over time.
 * This is ideal to monitor the size of a queue, the number of connected users, etc.
 * The integer number is tracked over time, and statistics are made about
 * its minimum, maximum, average value and standard deviation.
 *
 * @code
 * #define USE_PROFILOMETRE
 * #include <profilometre.h>
 *
 * // you need one structured for each integer tracked
 * profilometre_countinfo C;
 *
 * // no need to profilometre_init() for this use of the library
 * ...
 *
 * profilometre_count_reset(&C);
 *
 * ...
 *
 * // anytime the value N changes, call the function:
 * profilometre_count_update(&C,N);
 *
 * ...
 *
 * profilometre_count_stats(&C); // compute the statistics
 *
 * // print yourself the statistics.
 * printf("min=%d max=%d mean=%d stddev=%d percent0=%d\n",
 *      C.min,C.max,C.mean,C.std,C.percent0);
 *
 * @endcode
 *
*/




//
// To actually profile, you must define USE_PROFILOMETRE before including this
//

#ifdef USE_PROFILOMETRE

#ifdef __cplusplus
        extern "C" {
#endif

    void profilometre_init(void);
    void profilometre_reset(void);
    void profilometre_start(const char *name);
    void profilometre_stop(const char *name);
    void profilometre_dump(void);
    void profilometre_setup_histogram(const char *name,double hmin,double hmax,double hstep);

/**
 * Gather statistics about an integer
 *
 * this structure is for statistics inside data structure.
 * It keeps stats of a number over time.
 * min, max, mean, std, % 0
*/
typedef struct profilometre_countinfo {
    double count,sum,sum2;	///< accumulation of statistics
    double count0;			///< total time spent with n=0
    int lastVal;			///< the last value given to update()
    double lastUpdate;		///< the time of the last call to update() (<0 mean 'never called')
    double lastUpdate0;		///< the time of the last call to update with n=0 (<0 means no call active)
    // stats
    int min,max;	///< stats, computed profilometre_count_update()
    // stat computed on demand
    double mean,std; ///< mean and deviation, computed on demande in profilometre_count_stats()
    double percent0; ///< percentage (as a fraction) of time the number is 0
} profilometre_countinfo;


    void profilometre_count_reset(profilometre_countinfo *ci);
    void profilometre_count_update(profilometre_countinfo *ci,int n);
    void profilometre_count_stats(profilometre_countinfo *ci);

#ifdef __cplusplus
        }
#endif
#else
    #define profilometre_init(void)     {}
    #define profilometre_reset(void)	{}
    #define profilometre_start(name)        {}
    #define profilometre_stop(name)     {}
    #define profilometre_dump(void)     {}
    #define profilometre_setup_histogram(name,hmin,hmax,bins) {}


	typedef struct profilometre_countinfo { int x; } profilometre_countinfo;
    #define profilometre_count_reset(ci)			{}
    #define profilometre_count_update(ci,n)	{}
    #define profilometre_count_stats(ci)			{}
#endif



#endif
