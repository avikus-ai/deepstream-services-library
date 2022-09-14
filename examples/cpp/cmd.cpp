/*-------------------------------------------------------------------------*/
/**
 @file        cmd.cpp
 @path        ~/APP/Inference/src
 @date        08.29.2022
 @author      saeyong.park
 @brief       Inference command
 @history

**/
/*-------------------------------------------------------------------------*/

/*==========================================================================*/
// include header

#include "nas_ami.h"
// #include "DslApi.h"
#include "ami.h"

/*==========================================================================*/
// global variables

T_INFER_OPT T_INFER_opt;

/*-------------------------------------------------------------------------*/
// static variables

/*==========================================================================*/

char *g_p_usage__cmd_infer = "usagd: inference  - Monitor data about inference data \n" \
                              " - infer [(all|fps|infer|imu|off)]";
        
int cmd_infer(int ac, char *av[])
{
    unsigned int il =  T_INFER_opt.infer_level;

    if(ac==2)
    {
        if(!strcmp(av[1], "all"))
        {
            il = INFER_ALL;
        }
        else if(!strcmp(av[1], "fps"))
        {
            il = INFER_FPS;
        }
        else if(!strcmp(av[1], "infer"))
        {
            il = INFER_INF;
        }
        else if(!strcmp(av[1], "imu"))
        {
            il = INFER_IMU;
        }
        else if(!strcmp(av[1], "off"))
        {
            il = INFER_OFF;
        }
        else
        {
            PRC(" Wrong debug level input USE all|fps|infer|imu|off\n");
            PRC(" Current Debug level : %08x \n", T_INFER_opt.infer_level);
        }
        PRC(" Debug-Level : %08x --> %08x \n",T_INFER_opt.infer_level, il);
        T_INFER_opt.infer_level = il;
    }
    else
    {
        PRC(" INFERENCE Debug-Level: %08X \n", il);
        PRC(" ------------------------------------------ \n");
        PRC(" %c 0x0000_0001 - INFER_FPS\n",(il & INFER_FPS)?'V':' ');
        PRC(" %c 0x0000_0002 - INFER_INF\n",(il & INFER_INF)?'V':' ');
        PRC(" %c 0x0000_0004 - INFER_IMU\n",(il & INFER_IMU)?'V':' ');
    }

    return 0;
}

char *g_p_usage__cmd_pipeline = "usagd: inference  - Mondify pipeline options \n" \
                              " - pipe INI opt cmd";

int cmd_pipeline(int ac, char *av[])
{
    // AMI INI LOAD
    unsigned int ini_id;
    int rst;
    char filepath[AVKS_FILE_PATH_LEN*2];
    sprintf(filepath,"%s","/home/avikus/work/aiboat/aiboat/APP/INFER/option.ini");
    if((rst = ami_ini_load(filepath, &ini_id)) < 0)
    {
        PRC("There isn't INI file\n");
    }

    if(ac>=3)
    {
        for(int i=1; i<ac; i=i+3)
        {
            if(!strcmp(av[i],"CUDA"))
            {
                // INI CUDA option change
                PRC("CUDA option changed: %s \n",av[i+2]);
                ami_ini_chg_str(ini_id, av[i], av[i+1], av[i+2], sizeof(av[i+2])/sizeof(char));

            }
            else if(!strcmp(av[i],"URI"))
            {
                // INI URI option change
                PRC("URI option changed: %s \n",av[i+2]);
                ami_ini_chg_str(ini_id, av[i], av[i+1], av[i+2], sizeof(av[i+2])/sizeof(char));

            }
            else if(!strcmp(av[i],"INFER"))
            {
                int k = i + 6;
                if(k > ac)
                {
                    k = ac;
                }
                // INI INFER option change
                for(int j = i+1; j < k; j = j+2)
                {
                    if(!strcmp(av[j],"preprocess"))
                    {
                        PRC("INFER-preprocess option changed: %s \n",av[j+1]);
                        ami_ini_chg_str(ini_id, av[i], av[j], av[j+1], sizeof(av[j+1])/sizeof(char));
                    }
                    else if(!strcmp(av[j],"preprocess"))
                    {
                        PRC("INFER-config option changed: %s \n",av[j+1]);
                        ami_ini_chg_str(ini_id, av[i], av[j], av[j+1], sizeof(av[j+1])/sizeof(char));
                    }
                    else if(!strcmp(av[j],"model"))
                    {
                        PRC("INFER-model option changed: %s \n",av[j+1]);
                        ami_ini_chg_str(ini_id, av[i], av[j], av[j+1], sizeof(av[j+1])/sizeof(char));
                    }
                }
            }
            else if(!strcmp(av[i],"TRACKER"))
            {
                int k = i + 10;
                if(k > ac)
                {
                    k = ac;
                }
                // INI TRACKER option change
                for(int j = i+1; j < k; j = j+2)
                {
                    if(!strcmp(av[j],"tracker"))
                    {
                        PRC("TRACKER-tracker option changed: %s \n",av[j+1]);
                        ami_ini_chg_str(ini_id, av[i], av[j], av[j+1], sizeof(av[j+1])/sizeof(char));
                    }
                    else if(!strcmp(av[j],"config"))
                    {
                        PRC("TRACKER-config option changed: %s \n",av[j+1]);
                        ami_ini_chg_str(ini_id, av[i], av[j], av[j+1], sizeof(av[j+1])/sizeof(char));
                    }
                    else if(!strcmp(av[j],"width"))
                    {
                        PRC("TRACKER-width option changed: %s \n",av[j+1]);
                        ami_ini_chg_str(ini_id, av[i], av[j], av[j+1], sizeof(av[j+1])/sizeof(char));
                    }
                    else if(!strcmp(av[j],"height"))
                    {
                        PRC("TRACKER-height option changed: %s \n",av[j+1]);
                        ami_ini_chg_str(ini_id, av[i], av[j], av[j+1], sizeof(av[j+1])/sizeof(char));
                    }
                    else if(!strcmp(av[j],"vectorsize"))
                    {
                        PRC("TRACKER-vectorsize option changed: %s \n",av[j+1]);
                        ami_ini_chg_str(ini_id, av[i], av[j], av[j+1], sizeof(av[j+1])/sizeof(char));
                    }
                }
            }
            else if(!strcmp(av[i],"SETTING"))
            {
                int k = i + 16;
                if(k > ac)
                {
                    k = ac;
                }
                // INI SETTING option change
                for(int j = i+1; j < k; j = j+2)
                {
                    if(!strcmp(av[j],"agnocit"))
                    {
                        PRC("SETTING-agnocit option changed: %s \n",av[j+1]);
                        ami_ini_chg_str(ini_id, av[i], av[j], av[j+1], sizeof(av[j+1])/sizeof(char));
                    }
                    else if(!strcmp(av[j],"match_metric"))
                    {
                        PRC("SETTING-match_metric option changed: %s \n",av[j+1]);
                        ami_ini_chg_str(ini_id, av[i], av[j], av[j+1], sizeof(av[j+1])/sizeof(char));
                    }
                    else if(!strcmp(av[j],"match_threshold"))
                    {
                        PRC("SETTING-match_threshold option changed: %s \n",av[j+1]);
                        ami_ini_chg_str(ini_id, av[i], av[j], av[j+1], sizeof(av[j+1])/sizeof(char));
                    }
                    else if(!strcmp(av[j],"num_labels"))
                    {
                        PRC("SETTING-num_labels option changed: %s \n",av[j+1]);
                        ami_ini_chg_str(ini_id, av[i], av[j], av[j+1], sizeof(av[j+1])/sizeof(char));
                    }
                    else if(!strcmp(av[j],"interval"))
                    {
                        PRC("SETTING-interval option changed: %s \n",av[j+1]);
                        ami_ini_chg_str(ini_id, av[i], av[j], av[j+1], sizeof(av[j+1])/sizeof(char));
                    }
                    else if(!strcmp(av[j],"width"))
                    {
                        PRC("SETTING-width option changed: %s \n",av[j+1]);
                        ami_ini_chg_str(ini_id, av[i], av[j], av[j+1], sizeof(av[j+1])/sizeof(char));
                    }
                    else if(!strcmp(av[j],"height"))
                    {
                        PRC("SETTING-height option changed: %s \n",av[j+1]);
                        ami_ini_chg_str(ini_id, av[i], av[j], av[j+1], sizeof(av[j+1])/sizeof(char));
                    }
                    else if(!strcmp(av[j],"batch_size"))
                    {
                        PRC("SETTING-batch_size option changed: %s \n",av[j+1]);
                        ami_ini_chg_str(ini_id, av[i], av[j], av[j+1], sizeof(av[j+1])/sizeof(char));
                    }
                }
            }
        }
    }
    else
    {
        PRC("PIPELINE setting\n");
        PRC(" ------------------------------------------ \n");
        PRC("GROUP: CUDA ITEM: version\n");
        PRC("GROUP: URI ITEM: uri\n");
        PRC("GROUP: INFER ITEM: preprocess, config, model\n");
        PRC("GROUP: TRACKER ITEM: tracker, config, width, height, vectorsize\n");
        PRC("GROUP: SETTING ITEM: agnocit, match_metric, match_threshold, num_labels, interval, width, height, batch_size\n");
        PRC("USAGE: pipe GROUP ITEM value\n");
        PRC("e.g. pipe CUDA version cuda114\n");
    }
    
    // Save file
    ami_ini_save(ini_id, filepath);

    // Pipeline break
    // dsl_pipeline_stop(L"pipeline");
    // dsl_main_loop_quit();

    // Call MEDULA to operate APP
    return 0;
}