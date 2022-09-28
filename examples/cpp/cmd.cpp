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
#include "DslApi.h"
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
             T_INFER_opt.infer_level = INFER_ALL;
        }
        else if(!strcmp(av[1], "fps"))
        {
             T_INFER_opt.infer_level = INFER_FPS;
        }
        else if(!strcmp(av[1], "infer"))
        {
             T_INFER_opt.infer_level = INFER_INF;
        }
        else if(!strcmp(av[1], "imu"))
        {
             T_INFER_opt.infer_level = INFER_IMU;
        }
        else if(!strcmp(av[1], "off"))
        {
             T_INFER_opt.infer_level = INFER_OFF;
        }
        else
        {
            PRC(" Wrong debug level input USE all|fps|infer|imu|off\n");
            PRC(" Current Debug level : %08x \n", T_INFER_opt.infer_level);
        }
        PRC(" Debug-Level : %08x --> %08x \n",il,T_INFER_opt.infer_level);
        // T_INFER_opt.infer_level = il;
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
    sprintf(filepath,"%s","../cfg/option.ini"); // route modify
    if((rst = ami_ini_load(filepath, &ini_id)) < 0)
    {
        PRC("There isn't INI file\n");
    }

    if(ac>=4)
    {
        for(int i=1; i<ac; i=i+3)
        {
            if(!strcmp(av[i],"CUDA"))
            {
                // INI CUDA option change
                PRC("CUDA option changed: %s \n",av[i+2]);
                ami_ini_chg_str(ini_id, av[i], av[i+1], av[i+2], strlen(av[i+2]));

            }
            else if(!strcmp(av[i],"SETTING"))
            {
                // INI URI option change
                PRC("SETTING option changed: %s \n",av[i+2]);
                ami_ini_chg_str(ini_id, av[i], av[i+1], av[i+2], strlen(av[i+2]));

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
                    if(!strcmp(av[j],"config"))
                    {
                        PRC("INFER-config option changed: %s \n",av[j+1]);
                        ami_ini_chg_str(ini_id, av[i], av[j], av[j+1], strlen(av[j+1]));
                    }
                    else if(!strcmp(av[j],"model"))
                    {
                        PRC("INFER-model option changed: %s \n",av[j+1]);
                        ami_ini_chg_str(ini_id, av[i], av[j], av[j+1], strlen(av[j+1]));
                    }
                    else if(!strcmp(av[j],"interval"))
                    {
                        PRC("INFER-interval option changed: %s \n",av[j+1]);
                        ami_ini_chg_str(ini_id, av[i], av[j], av[j+1], strlen(av[j+1]));
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
                    if(!strcmp(av[j],"enable"))
                    {
                        PRC("TRACKER-enable option changed: %s \n",av[j+1]);
                        ami_ini_chg_str(ini_id, av[i], av[j], av[j+1], strlen(av[j+1]));
                    }
                    else if(!strcmp(av[j],"method"))
                    {
                        PRC("TRACKER-method option changed: %s \n",av[j+1]);
                        ami_ini_chg_str(ini_id, av[i], av[j], av[j+1], strlen(av[j+1]));
                    }
                    else if(!strcmp(av[j],"width"))
                    {
                        PRC("TRACKER-width option changed: %s \n",av[j+1]);
                        ami_ini_chg_str(ini_id, av[i], av[j], av[j+1], strlen(av[j+1]));
                    }
                    else if(!strcmp(av[j],"height"))
                    {
                        PRC("TRACKER-height option changed: %s \n",av[j+1]);
                        ami_ini_chg_str(ini_id, av[i], av[j], av[j+1], strlen(av[j+1]));
                    }
                    else if(!strcmp(av[j],"config"))
                    {
                        PRC("TRACKER-config option changed: %s \n",av[j+1]);
                        ami_ini_chg_str(ini_id, av[i], av[j], av[j+1], strlen(av[j+1]));
                    }
                }
            }
            else if(!strcmp(av[i],"INPUT"))
            {
                int k = i + 8;
                if(k > ac)
                {
                    k = ac;
                }
                // INI SETTING option change
                for(int j = i+1; j < k; j = j+2)
                {
                    if(!strcmp(av[j],"type"))
                    {
                        PRC("INPUT-type option changed: %s \n",av[j+1]);
                        ami_ini_chg_str(ini_id, av[i], av[j], av[j+1], strlen(av[j+1]));
                    }
                    else if(!strcmp(av[j],"uri"))
                    {
                        PRC("INPUT-uri option changed: %s \n",av[j+1]);
                        ami_ini_chg_str(ini_id, av[i], av[j], av[j+1], strlen(av[j+1]));
                    }
                    else if(!strcmp(av[j],"repeat"))
                    {
                        PRC("INPUT-repeat option changed: %s \n",av[j+1]);
                        ami_ini_chg_str(ini_id, av[i], av[j], av[j+1], strlen(av[j+1]));
                    }
                    else if(!strcmp(av[j],"uri_cnt"))
                    {
                        PRC("INPUT-uri_cnt option changed: %s \n",av[j+1]);
                        ami_ini_chg_str(ini_id, av[i], av[j], av[j+1], strlen(av[j+1]));
                    }
                    else if(!strcmp(av[j],"drop_frame_interval"))
                    {
                        PRC("INPUT-drop_frame_interval option changed: %s \n",av[j+1]);
                        ami_ini_chg_str(ini_id, av[i], av[j], av[j+1], strlen(av[j+1]));
                    }
                }
            }
            else if(!strcmp(av[i],"PREPROCESS"))
            {
                int k = i + 6;
                if(k > ac)
                {
                    k = ac;
                }
                // INI SETTING option change
                for(int j = i+1; j < k; j = j+2)
                {
                    if(!strcmp(av[j],"enable"))
                    {
                        PRC("PREPROCESS-enable option changed: %s \n",av[j+1]);
                        ami_ini_chg_str(ini_id, av[i], av[j], av[j+1], strlen(av[j+1]));
                    }
                    else if(!strcmp(av[j],"batch_size"))
                    {
                        PRC("PREPROCESS-batch_size option changed: %s \n",av[j+1]);
                        ami_ini_chg_str(ini_id, av[i], av[j], av[j+1], strlen(av[j+1]));
                    }
                    else if(!strcmp(av[j],"config"))
                    {
                        PRC("PREPROCESS-config option changed: %s \n",av[j+1]);
                        ami_ini_chg_str(ini_id, av[i], av[j], av[j+1], strlen(av[j+1]));
                    }
                }
            }
            else if(!strcmp(av[i],"POSTPROCESS"))
            {
                int k = i + 10;
                if(k > ac)
                {
                    k = ac;
                }
                // INI SETTING option change
                for(int j = i+1; j < k; j = j+2)
                {
                    if(!strcmp(av[j],"enable"))
                    {
                        PRC("POSTPROCESS-enable option changed: %s \n",av[j+1]);
                        ami_ini_chg_str(ini_id, av[i], av[j], av[j+1], strlen(av[j+1]));
                    }
                    else if(!strcmp(av[j],"agnostic"))
                    {
                        PRC("POSTPROCESS-agnostic option changed: %s \n",av[j+1]);
                        ami_ini_chg_str(ini_id, av[i], av[j], av[j+1], strlen(av[j+1]));
                    }
                    else if(!strcmp(av[j],"method"))
                    {
                        PRC("POSTPROCESS-method option changed: %s \n",av[j+1]);
                        ami_ini_chg_str(ini_id, av[i], av[j], av[j+1], strlen(av[j+1]));
                    }
                    else if(!strcmp(av[j],"match_metric"))
                    {
                        PRC("POSTPROCESS-match_metric option changed: %s \n",av[j+1]);
                        ami_ini_chg_str(ini_id, av[i], av[j], av[j+1], strlen(av[j+1]));
                    }
                    else if(!strcmp(av[j],"match_threshold"))
                    {
                        PRC("POSTPROCESS-match_threshold option changed: %s \n",av[j+1]);
                        ami_ini_chg_str(ini_id, av[i], av[j], av[j+1], strlen(av[j+1]));
                    }
                    else if(!strcmp(av[j],"label_file"))
                    {
                        PRC("POSTPROCESS-label_file option changed: %s \n",av[j+1]);
                        ami_ini_chg_str(ini_id, av[i], av[j], av[j+1], strlen(av[j+1]));
                    }
                }
            }
            else if(!strcmp(av[i],"OSD"))
            {
                int k = i + 10;
                if(k > ac)
                {
                    k = ac;
                }
                // INI SETTING option change
                for(int j = i+1; j < k; j = j+2)
                {
                    if(!strcmp(av[j],"enable"))
                    {
                        PRC("OSD-enable option changed: %s \n",av[j+1]);
                        ami_ini_chg_str(ini_id, av[i], av[j], av[j+1], strlen(av[j+1]));
                    }
                    else if(!strcmp(av[j],"num_labels"))
                    {
                        PRC("OSD-num_labels option changed: %s \n",av[j+1]);
                        ami_ini_chg_str(ini_id, av[i], av[j], av[j+1], strlen(av[j+1]));
                    }
                    else if(!strcmp(av[j],"monitor"))
                    {
                        PRC("OSD-monitor option changed: %s \n",av[j+1]);
                        ami_ini_chg_str(ini_id, av[i], av[j], av[j+1], strlen(av[j+1]));
                    }
                    else if(!strcmp(av[j],"bbox_border_size"))
                    {
                        PRC("OSD-bbox_border_size option changed: %s \n",av[j+1]);
                        ami_ini_chg_str(ini_id, av[i], av[j], av[j+1], strlen(av[j+1]));
                    }
                    else if(!strcmp(av[j],"font_size"))
                    {
                        PRC("OSD-font_size option changed: %s \n",av[j+1]);
                        ami_ini_chg_str(ini_id, av[i], av[j], av[j+1], strlen(av[j+1]));
                    }
                }
            }
            else if(!strcmp(av[i],"SINK"))
            {
                int k = i + 6;
                if(k > ac)
                {
                    k = ac;
                }
                // INI SETTING option change
                for(int j = i+1; j < k; j = j+2)
                {
                    if(!strcmp(av[j],"method"))
                    {
                        PRC("SINK-method option changed: %s \n",av[j+1]);
                        ami_ini_chg_str(ini_id, av[i], av[j], av[j+1], strlen(av[j+1]));
                    }
                    else if(!strcmp(av[j],"width"))
                    {
                        PRC("SINK-width option changed: %s \n",av[j+1]);
                        ami_ini_chg_str(ini_id, av[i], av[j], av[j+1], strlen(av[j+1]));
                    }
                    else if(!strcmp(av[j],"height"))
                    {
                        PRC("SINK-height option changed: %s \n",av[j+1]);
                        ami_ini_chg_str(ini_id, av[i], av[j], av[j+1], strlen(av[j+1]));
                    }
                }
            }
        }
        ami_ini_save(ini_id, filepath);

        // Pipeline break
        dsl_pipeline_stop(L"pipeline");
        dsl_main_loop_quit();

        // Call MEDULA to operate APP
        PRC("[NAS] INI modified... restart the process()\n");
        (void)ami_term();
    }
    else
    {
        PRC("PIPELINE setting\n");
        PRC(" ------------------------------------------ \n");
        PRC("GROUP: CUDA ITEM: version\n");
        PRC("GROUP: SETTING ITEM: perf\n");
        PRC("GROUP: INPUT ITEM: type, uri, repeat, uri_cnt, drop_frame_interval\n");
        PRC("GROUP: PREPROCESS ITEM: enable, batch_size, config\n");
        PRC("GROUP: INFER ITEM: config, model, interval\n");
        PRC("GROUP: TRACKER ITEM: enable, method, config, width, height\n");
        PRC("GROUP: POSTPROCESS ITEM: enable, agnostic, method, match_metric, match_threshold, label_file\n");
        PRC("GROUP: OSD ITEM: enable, num_labels, monitor, bbox_border_size\n");
        PRC("GROUP: SINK ITEM: method, width, height\n");
        PRC("USAGE: pipe GROUP ITEM value\n");
        PRC("e.g. pipe CUDA version cuda114\n");
    }
    
    // Save file
    
    return 0;
}