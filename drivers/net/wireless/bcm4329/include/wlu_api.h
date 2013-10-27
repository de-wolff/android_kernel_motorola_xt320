/*
 * Entry points for wl utility application for platforms where the wl utility
 * is not a standalone application.
 *
 * $ Copyright Broadcom Corporation 2008 $
 * $Id: wlu_api.h,v 1.3 2008-12-02 01:01:35 nvalji Exp $
 */


#ifndef wlu_api_h
#define wlu_api_h

#ifdef __cplusplus
extern "C" {
#endif

/* ---- Include Files ---------------------------------------------------- */
/* ---- Constants and Types ---------------------------------------------- */
/* ---- Variable Externs ------------------------------------------------- */
/* ---- Function Prototypes ---------------------------------------------- */


/****************************************************************************
* Function:   wlu_main_args
*
* Purpose:    Entry point for wl utility application. Uses same prototype
*             as standard main() functions - argc and argv parameters.
*
* Parameters: argc (in) Number of 'argv' arguments.
*             argv (in) Array of command-line arguments.
*
* Returns:    0 on success, else error code.
*****************************************************************************
*/
int wlu_main_args(int argc, char **argv);

/****************************************************************************
* Function:   wlu_remote_server_main_args
*
* Purpose:    Entry point for remote wl server application. Uses same prototype
*             as standard main() functions - argc and argv parameters.
*
* Parameters: argc (in) Number of 'argv' arguments.
*             argv (in) Array of command-line arguments.
*
* Returns:    0 on success, else error code.
*****************************************************************************
*/
int wlu_remote_server_main_args(int argc, char **argv);


/****************************************************************************
* Function:   wlu_main_str
*
* Purpose:    Alternate entry point for wl utility application. This function
*             will tokenize the input command line string, before calling
*             wlu_main_args() to process the command line arguments.
*
* Parameters: str (mod) Input command line string. Contents will be modified!
*
* Returns:    0 on success, else error code.
*****************************************************************************
*/
int wlu_main_str(char *str);


#ifdef __cplusplus
	}
#endif

#endif  /* wlu_api_h  */
