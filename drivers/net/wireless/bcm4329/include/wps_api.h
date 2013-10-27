/*
 * Entry points for wps enrollee/tester utility application for embedded platforms
 *
 * $ Copyright Broadcom Corporation 2008 $
 * $Id: wps_api.h,v 1.2 2009-03-06 18:21:48 vinoth Exp $
 */


#ifndef wps_api_h

#ifdef __cplusplus
extern "C" {
#endif

/* ---- Include Files ---------------------------------------------------- */
/* ---- Constants and Types ---------------------------------------------- */
/* ---- Variable Externs ------------------------------------------------- */
/* ---- Function Prototypes ---------------------------------------------- */


/****************************************************************************
* Function:   wps_main_args
*
* Purpose:    Entry point for wps utility application. Uses same prototype
*             as standard main() functions - argc and argv parameters.
*
* Parameters: argc (in) Number of 'argv' arguments.
*             argv (in) Array of command-line arguments.
*
* Returns:    0 on success, else error code.
*****************************************************************************
*/
int wps_main_args(int argc, char **argv);


/****************************************************************************
* Function:   wps_api_test
*
* Purpose:    Test application for verifying the wps functionality using wps api functions
*
* Parameters: none
*
* Returns:    0 on success, else error code.
*****************************************************************************
*/
int wps_api_test(void);

/****************************************************************************
* Function:   wps_main_str
*
* Purpose:    Alternate entry point for wps api tester application. This function
*             will tokenize the input command line string, before calling
*             wps_main_args() to process the command line arguments.
*
* Parameters: str (mod) Input command line string. Contents will be modified!
*
* Returns:    0 on success, else error code.
*****************************************************************************
*/
int wps_main_str(char *str);

#ifdef __cplusplus
	}
#endif

#endif  /* wlu_api_h  */
