/*-------------------------------------------------------------------------*/
/**
 @file        exapp.h
 @path        ~/APP/EXAM/src
 @date        09.13.2022
 @author      saeyong.park
 @brief       EXAPP header file
 @history

**/
/*-------------------------------------------------------------------------*/

#ifndef _EXAPP_H
#define _EXAPP_H

/*==========================================================================*/
// global header

#include "ami.h"
#include "avks_did.h"
#include <iostream>
/*==========================================================================*/

// #define OPT__CFG_DIR__PATH		"path to inference" //"~/aiboat/pkg"
// #define OPT__INFER_INI__FILE	"inference_option.ini"
#define OPT__CFG_DIR__PATH		"../pkg/data"
#define OPT__EXAPP_INI__FILE	"exapp.ini"

// TODO:: you must change 'xID_PSS__EXAPP' before execute the program
#define xID__PSS__INFER			(0x80) // TODO:: not valid

/*--------------------------------------------------------------------------*/

// Command Line Options
typedef struct
{
	// Configuration File
	char		cfg_dir[AVKS_FILE_PATH_LEN];
	char		exapp_ini[AVKS_FILE_PATH_LEN];

	// Debugging action
	uint32_t	dl;
	uint32_t	tl;
	uint32_t	adl;
	uint32_t	atl;
	uint32_t	con;

} ARGS_OPT;

/*--------------------------------------------------------------------------*/

/*==========================================================================*/

typedef struct
{
	// INI parsing
	uint32_t	exapp_ini_id;
	ARGS_OPT    args_opt;

} T_EXAPP_DATA;

typedef struct 
{
	// Debug level
	unsigned int infer_level;

} T_INFER_OPT;


typedef struct
{
	float fps=0;
	int roll=0;
	int pitch=0;
	int yaw=0;
	unsigned int infer_length = 0;
	T_NAS_INF_REC infer[1024];

} T_DATA_PRC;

typedef struct
{
	uint32_t		nas_int_rec_no; // 1 ~ 1023 (max to 1024)
	T_NAS_INF_REC	nas_inf_rec[1024];
} S__INF__NAS_INF_INFO;
/*==========================================================================*/
// INFER debug level

#define INFER_ALL			    0xFFFFFFFF
#define INFER_FPS               0x00000001
#define INFER_INF               0x00000002
#define INFER_IMU               0x00000004
#define INFER_OFF			    0x00000000

/*==========================================================================*/
// EXAM debug level
#define DL_LOCK					0x00000001
#define DL_SHM					0x00000002
#define DL_UDS					0x00000004
#define DL_UDP					0x00000008

#define DL_INI					0x00000010
#define DL_DO					0x00000020
#define DL_DP					0x00000040
#define DL_CLI					0x00000080

#define DL_DBG					0x40000000
#define DL_ALM					0x80000000

#define DL_ALL					0xFFFFFFFF
#define DL_OFF					0x00000000

/*-------------------------------------------------------------------------*/

// EXAM trace level

#define TL_UDS_RX				0x00000001
#define TL_UDS_TX				0x00000002
#define TL_UDP_RX				0x00000004
#define TL_UDP_TX				0x00000008

#define TL_DP_RX				0x00000010
#define TL_DP_TX				0x00000020

#define TL_ALL					0xFFFFFFFF
#define TL_OFF					0x00000000

/*==========================================================================*/

/*--- cmd.c ---*/
extern char *g_p_usage__cmd_pipeline;
extern int cmd_pipeline(int ac,char *av[]);

extern char *g_p_usage__cmd_infer;
extern int cmd_infer(int ac,char *av[]);

/*--- main.c ---*/
extern T_EXAPP_DATA g_exapp_data;

/*--- util.c ---*/
extern int init_ami_data(void);
extern int init_exapp_data(void);

extern int create_thread(void);

extern T_INFER_OPT T_INFER_opt;

/*==========================================================================*/

#endif // _EXAPP_H
