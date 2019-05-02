/**
 *
 * \mainpage Code Documentation
 *
 * \section intro_sec Introduction
 *
 * This documentation is aimed at developers wishing
 * to study the source code or contribute to Zrythm.
 * This is a general outline of how Zrythm works, and
 * more detailed information can be found by looking
 * at each structure from the tabs above.
 *
 *
 * The main modules to look at would be Project
 * and Zrythm (inc/project.h and inc/zrythm.h)
 *
 *
 * \section project Project
 *
 * The Project data structure contains everything
 * that is unique to each project and should be
 * serialized.
 *
 * The Project contains a registry for each type
 * of object, and keeps track of all objects in use
 * by unique IDs. E.g. the regions array has all the
 * regions in the project, and each Region has an
 * id field used as an index to this array.
 *
 * To save space and due to limitations in cyaml,
 * when saving the project these arrays cannot
 * contain NULLs, so the aggregated arrays (same
 * arrays with NULLs excluded) are used.
 *
 * Transient objects such as transient Regions
 * should not be added to the project and should not
 * have unique IDs.
 *
 */
