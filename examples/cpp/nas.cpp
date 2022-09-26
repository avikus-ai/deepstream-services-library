/*
The MIT License

Copyright (c) 2022, Prominence AI, Inc.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in-
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#include <iostream>
#include <cstdio>
#include <glib.h>
#include <gst/gst.h>
#include <gstnvdsmeta.h>
#include <nvdspreprocess_meta.h>
#include <iomanip>
// https://github.com/dpilger26/NumCpp/blob/master/docs/markdown/Installation.md
#include <cstdint>
#include <chrono>

#include "DslApi.h"
#include "nas_ami.h"

static T_DATA_PRC data;

class ReportData 
{
public:
    int m_report_count;
    int m_header_interval;

    ReportData(int count, int interval) : m_report_count(count), m_header_interval(interval) {}
};

static void register_cli_command(void);
uint send_data(void* buffer, void* client_data);
void ode_occurrence_monitor(dsl_ode_occurrence_info* pInfo, void* client_data);
void eos_event_listener(void* client_data);
boolean dsl_pph_meter_cb(double* session_fps_averages, double* interval_fps_averages, uint source_count, void* client_data);
void xwindow_key_event_handler(const wchar_t* in_key, void* client_data);
void xwindow_delete_event_handler(void* client_data);

T_EXAPP_DATA g_exapp_data;

/*-------------------------------------------------------------------------*/
// static variables

static const char *s_usage =  // TODO:: add your options
///// DO NOT REMOVE below options (i,D,T,X,Y,C,h)
" \
-i --ini <file-name>       // EXAPP CONFIG file (exapp.ini)\n \
\n \
-D --dl <level:hexa, all>  // EXAPP debug level. 32bit\n \
-T --tl <level:hexa, all>  // EXAPP trace level. 32bit\n \
-X --adl <level:hexa, all> // AMI debug level. 32bit\n \
-Y --atl <level:hexa, all> // AMI trace level. 32bit\n \
-C --con                   // set LOG out to CONSOLE\n \
\n \
-h --help                  // print usage...\n \
";

static const char *short_options = "i:D:T:X:Y:Ch"; // TODO:: add your options
static const struct option long_options[] = {
    { "ini",        1, NULL, 'i' },

	{ "dl",         1, NULL, 'D' },
    { "tl",         1, NULL, 'T' },
	{ "adl",        1, NULL, 'X' },
    { "atl",        1, NULL, 'Y' },
	{ "con",        0, NULL, 'C' },

	// TODO:: add your arguments options
    { "help",       0, NULL, 'h' }
};

/*==========================================================================*/

static int proc_get_options(int ac,char *av[])
{
    int next_option;
	int quit_flag=FALSE;
    uint32_t u32;
    //uint16_t u16;
	//char string[AVKS_FILE_PATH_LEN];

	// init 'g_exapp_data.args_opt' entry
	memset((char *)&g_exapp_data.args_opt, 0x00, sizeof(g_exapp_data.args_opt));

	sprintf(g_exapp_data.args_opt.exapp_ini, "%s/%s", OPT__CFG_DIR__PATH, OPT__EXAPP_INI__FILE);
	// TODO:: add your code (init)

    do {
        next_option = getopt_long(ac,av,short_options,long_options,NULL);

        switch (next_option)
        {
        case 'i': // EXAPP.INI
            strcpy(g_exapp_data.args_opt.exapp_ini, optarg);
            break;

        case 'D':
            u32 = (uint32_t)strtoul(optarg, NULL, 16);
            g_exapp_data.args_opt.dl = u32;
			g_AMI_STS.debug_level    = u32;
            break;
        case 'T':
            u32 = (uint32_t)strtoul(optarg, NULL, 16);
            g_exapp_data.args_opt.tl = u32;
			g_AMI_STS.trace_level    = u32;
            break;
        case 'X':
            u32 = (uint32_t)strtoul(optarg, NULL, 16);
            g_exapp_data.args_opt.adl  = u32;
			g_ami_data.ami_debug_level = u32;
            break;
        case 'Y':
            u32 = (uint32_t)strtoul(optarg, NULL, 16);
            g_exapp_data.args_opt.atl  = u32;
			g_ami_data.ami_trace_level = u32;
            break;
		case 'C':
			g_exapp_data.args_opt.con = AMI_LOG_CH__CON;
			g_ami_data.log_channel    = AMI_LOG_CH__CON;
			break;

        case 'h': // help. print usage
			PR("Usage:\n%s\n",s_usage);
			quit_flag = TRUE;
            break;

        default:
            if (next_option != -1)
                PR("[OPT] Illegal option (%d,%x) \n", next_option,next_option);
            break;
        }
    } while (next_option != -1);

	// disp args
	PR("\n=== Running Options ===\n");
	PR(" ini  = %s\n",  g_exapp_data.args_opt.exapp_ini);
	PR(" dl   = %08x\n",g_exapp_data.args_opt.dl);
	PR(" tl   = %08x\n",g_exapp_data.args_opt.tl);
	PR(" adl  = %08x\n",g_exapp_data.args_opt.adl);
	PR(" atl  = %08x\n",g_exapp_data.args_opt.atl);
	PR(" con  = %08x\n",g_exapp_data.args_opt.con);
	PR("=========================\n");

	// set global variables to args config
	if (g_exapp_data.args_opt.con == AMI_LOG_CH__CON)
		g_ami_data.console_cli = TRUE;

	return (quit_flag == TRUE) ? -1 : 0;
}

int main(int argc, char** argv)
{
    // 0.display logo
    PR("=== INFERENCE Start... ===\n");

    int rst_init = 0;

    // 2. get options
	if ((rst_init = proc_get_options(argc, argv)) < 0)
		{ PR("proc_get_options() failed. err=%d\n",rst_init); return 11; }

    // 1.ami_init
    if ((rst_init = ami_init(xID__PSS__APP_NAS, argc, argv)) < 0)
		{ PR("ami_init(%02x) failed. err=%d\n", xID__PSS__APP_NAS, rst_init); return 21; }

    // 2.Register command
    register_cli_command();
    
    // 3.read INI for setting
    unsigned int ini_id;
    char filepath[AVKS_FILE_PATH_LEN*2];
    sprintf(filepath,"%s","option.ini");
    int rst_ini = 0;

    rst_ini = ami_ini_load(filepath, &ini_id);
    if(rst_ini < 0)
    {
        PR("There isn't INI file\n");
    }
    else{
        PR("Success load INI file\n");
    }

    char char_buffer[255];
    ////////////// SETTING
    std::string cudaversion;
    ReportData report_data(0, 12);
    int log;
    std::wstring log_path;

    ////////////// INPUT
    std::wstring input_type;
    std::wstring uri;
    int repeat;
    int uri_cnt;
    int drop_frame_interval;

    ////////////// PREPROCESS
    int preproc_enable;
    int batch_size;
    std::wstring preproc_config;
    
    ////////////// INFER
    std::wstring infer_config;
    std::wstring infer_model;
    int interval;

    ////////////// TRACKER
    int trk_enable;
    std::wstring trk_method;
    std::wstring trk_config;
    int trk_width;
    int trk_height;
    
    ////////////// POSTPROCESS 
    int postproc_enable;
    int class_agnostic;
    std::wstring postproc_method;
    std::wstring postproc_match_metric;
    float postproc_match_threshold;
    std::wstring label_file;

    ////////////// OSD
    int osd_enable;
    int num_labels;
    int monitor;
    int font_size;
    int bbox_border_size;
    
    ////////////// SINK
    std::wstring sink_method;
    int window_width;
    int window_height;
    
    int rst_ini_parse = 0;
    if ((rst_ini_parse = ami_ini_str(ini_id,"CUDA","version",char_buffer,255))< 0) {
        cudaversion = "cuda102";
    }
    else {
        cudaversion = std::string(char_buffer);
    }
    
    ////////////// SETTING
    log = ami_ini_s32(ini_id,"SETTING","log",0,10);
    if((rst_ini_parse = ami_ini_str(ini_id,"SETTING","log_path",char_buffer, 255)) < 0)
    {
        std::string str("log.csv");
        log_path = std::wstring(str.begin(), str.end());
    }
    else
    {
        std::string str(char_buffer);
        log_path = std::wstring(str.begin(), str.end());
    }

    ////////////// INPUT
    if((rst_ini_parse = ami_ini_str(ini_id,"INPUT","type",char_buffer, 255)) < 0)
    {
        std::string str("video");
        input_type = std::wstring(str.begin(), str.end());
    }
    else
    {
        std::string str(char_buffer);
        input_type = std::wstring(str.begin(), str.end());
    }

    if((rst_ini_parse = ami_ini_str(ini_id,"INPUT","uri",char_buffer, 255)) < 0)
    {
        std::string str("");
        uri = std::wstring(str.begin(), str.end());
    }
    else
    {
        std::string str(char_buffer);
        uri = std::wstring(str.begin(), str.end());
    }

    repeat = ami_ini_s32(ini_id,"INPUT","repeat",0,10);
    uri_cnt = ami_ini_s32(ini_id,"INPUT","uri_cnt",1,10);
    drop_frame_interval = ami_ini_s32(ini_id,"INPUT","drop_frame_interval",0,10);
    ////////////// PREPOCESS
    preproc_enable = ami_ini_s32(ini_id,"PREPROCESS","enable",1,10);
    batch_size = ami_ini_s32(ini_id,"PREPROCESS","batch_size",1,10);

    if((rst_ini_parse = ami_ini_str(ini_id,"PREPROCESS","config",char_buffer, 255)) < 0)
    {
        std::string str("");
        preproc_config = std::wstring(str.begin(), str.end());
    }
    else
    {
        std::string str(char_buffer);
        preproc_config = std::wstring(str.begin(), str.end());
    }

    ////////////// INFER
    if((rst_ini_parse = ami_ini_str(ini_id,"INFER","config",char_buffer, 255)) < 0)
    {
        std::string str("");
        infer_config = std::wstring(str.begin(), str.end());
    }
    else
    {
        std::string str(char_buffer);
        infer_config = std::wstring(str.begin(), str.end());
    }

    if((rst_ini_parse = ami_ini_str(ini_id,"INFER","model",char_buffer, 255)) < 0)
    {
        std::string str("");
        infer_model = std::wstring(str.begin(), str.end());
    }
    else
    {
        std::string str(char_buffer);
        infer_model = std::wstring(str.begin(), str.end());
    }

    interval = ami_ini_s32(ini_id,"INFER","interval",0,10);

    ////////////// TRACKER
    trk_enable = ami_ini_s32(ini_id,"TRACKER","enable",1, 10);

    if((rst_ini_parse = ami_ini_str(ini_id,"TRACKER","method",char_buffer, 255)) < 0)
    {
        std::string str("");
        trk_method = std::wstring(str.begin(), str.end());
    }
    else
    {
        std::string str(char_buffer);
        trk_method = std::wstring(str.begin(), str.end());
    }

    if((rst_ini_parse = ami_ini_str(ini_id,"TRACKER","config",char_buffer, 255)) < 0)
    {
        std::string str("");
        trk_config = std::wstring(str.begin(), str.end());
    }
    else
    {
        std::string str(char_buffer);
        trk_config = std::wstring(str.begin(), str.end());
    }

    trk_width = ami_ini_s32(ini_id,"TRACKER","width",1280, 10);
    trk_height = ami_ini_s32(ini_id,"TRACKER","height",1280, 10);

    ////////////// POSTPROCESS
    postproc_enable = ami_ini_s32(ini_id,"POSTPROCESS","enable",1, 10);
    class_agnostic = ami_ini_s32(ini_id,"POSTPROCESS","agnostic",1, 10);

    if((rst_ini_parse = ami_ini_str(ini_id,"POSTPROCESS","method",char_buffer, 255)) < 0)
     {
        std::string str("");
        postproc_method = std::wstring(str.begin(), str.end());
    }
    else
    {
        std::string str(char_buffer);
        postproc_method = std::wstring(str.begin(), str.end());
    }

    if((rst_ini_parse = ami_ini_str(ini_id,"POSTPROCESS","match_metric",char_buffer, 255)) < 0)
    {
        std::string str("");
        postproc_match_metric = std::wstring(str.begin(), str.end());
    }
    else
    {
        std::string str(char_buffer);
        postproc_match_metric = std::wstring(str.begin(), str.end());
    }

    postproc_match_threshold = ami_ini_float(ini_id,"POSTPROCESS","match_threshold",0.6f); 
    
    if((rst_ini_parse = ami_ini_str(ini_id,"POSTPROCESS","label_file",char_buffer, 255)) < 0)
    {
        std::string str("");
        label_file = std::wstring(str.begin(), str.end());
    }
    else
    {
        std::string str(char_buffer);
        label_file = std::wstring(str.begin(), str.end());
    }

    ////////////// SINK
    osd_enable = ami_ini_s32(ini_id,"OSD","enable",1, 10);
    num_labels = ami_ini_s32(ini_id,"OSD","num_labels",7, 10);
    monitor = ami_ini_s32(ini_id,"OSD","monitor",0, 10);
    bbox_border_size = ami_ini_s32(ini_id,"OSD","bbox_border_size",3, 10);
    font_size = ami_ini_s32(ini_id,"OSD","font_size",16, 10);

    ////////////// SINK
    if((rst_ini_parse = ami_ini_str(ini_id,"SINK","method",char_buffer, 255)) < 0)
    {
        std::string str("");
        sink_method = std::wstring(str.begin(), str.end());
    }
    else
    {
        std::string str(char_buffer);
        sink_method = std::wstring(str.begin(), str.end());
    }

    window_width = ami_ini_s32(ini_id,"SINK","width",1920, 10); 
    window_height = ami_ini_s32(ini_id,"SINK","height",1080, 10);

    DslReturnType retval;
    // std::string cudaversion;
    
    ////////////// INPUT
    std::cout << "log: " << log << "\n";
    std::wcout << "log_path: " << log_path << "\n";

    ////////////// INPUT
    std::wcout << "input_type: " << input_type << "\n";
    std::wcout << "uri: " << uri << "\n";
    std::cout << "repeat: " << repeat << "\n";
    std::cout << "uri_cnt: " << uri_cnt << "\n";
    std::cout << "drop_frame_interval: " << drop_frame_interval << "\n";
    
    //////////////// PREPROCESS
    std::cout << "preproc_enable: " << preproc_enable << "\n";
    std::cout << "batch_size: " << batch_size << "\n";
    std::wcout << "preproc_config: " << preproc_config << "\n";

    //////////////// INFER
    std::wcout << "infer_config: " << infer_config << "\n";
    std::wcout << "infer_model: " << infer_model << "\n";
    std::cout << "interval: " << interval << "\n";

    // ////////////// TRACKER
    std::cout << "trk_enable: " << trk_enable << "\n";
    std::wcout << "trk_method: " << trk_method << "\n";
    std::wcout << "trk_config: " << trk_config << "\n";
    std::cout << "trk_width: " << trk_width << "\n";
    std::cout << "trk_height: " << trk_height << "\n";
    
    //////////////// POSTPROCESS 
    std::cout << "postproc_enable: " << postproc_enable << "\n";
    std::cout << "class_agnostic: " << class_agnostic << "\n";
    std::wcout << "postproc_method: " << postproc_method << "\n";
    std::wcout << "postproc_match_metric: " << postproc_match_metric << "\n";
    std::cout << "postproc_match_threshold: " << postproc_match_threshold << "\n";
    std::wcout << "label_file: " << label_file << "\n";

    //////////////// OSD
    std::cout << "osd_enable: " << osd_enable << "\n";
    std::cout << "num_labels: " << num_labels << "\n";
    std::cout << "monitor: " << monitor << "\n";
    std::cout << "font_size: " << font_size << "\n";
    std::cout << "bbox_border_size: " << bbox_border_size << "\n";

    //////////////// SINK
    std::wcout << "sink_method: " << sink_method << "\n";
    std::cout << "window_width: " << window_width << "\n";
    std::cout << "window_height: " << window_height << "\n";

    // Since we're not using args, we can Let DSL initialize GST on first call    
    while(true) 
    {    
        if (osd_enable) {
            // Create an Any-Class Occurrence Trigger for our remove Actions
            retval = dsl_ode_trigger_occurrence_new(L"every-occurrence-trigger", DSL_ODE_ANY_SOURCE, DSL_ODE_ANY_CLASS, DSL_ODE_TRIGGER_LIMIT_NONE);
            if (retval != DSL_RESULT_SUCCESS) break;

            retval = dsl_display_type_rgba_color_custom_new(L"full-white", 1.0, 1.0, 1.0, 1.0);
            if (retval != DSL_RESULT_SUCCESS) break;
            
            retval = dsl_display_type_rgba_color_custom_new(L"opaque-black", 0.0, 0.0, 0.0, 0.8);
            if (retval != DSL_RESULT_SUCCESS) break;

            retval = dsl_display_type_rgba_font_new(L"verdana-bold-16-white", L"verdana bold", font_size, L"full-white");
            if (retval != DSL_RESULT_SUCCESS) break;

            retval = dsl_ode_action_label_format_new(L"format-label", 
                L"verdana-bold-16-white", 
                true, 
                L"opaque-black");
            if (retval != DSL_RESULT_SUCCESS) break;

            retval = dsl_display_type_rgba_color_palette_random_new(L"random-color", num_labels, DSL_COLOR_HUE_RANDOM, DSL_COLOR_LUMINOSITY_RANDOM, 1.0, 1000);
            if (retval != DSL_RESULT_SUCCESS) break;

            retval = dsl_ode_action_bbox_format_new(L"format-bbox", bbox_border_size, L"random-color", false, nullptr);
            if (retval != DSL_RESULT_SUCCESS) break;
            
            uint content_types[] = {DSL_METRIC_OBJECT_TRACKING_ID, DSL_METRIC_OBJECT_CLASS};
            retval = dsl_ode_action_label_customize_new(L"customize-label-action", content_types, sizeof(content_types)/sizeof(uint));
            if (retval != DSL_RESULT_SUCCESS) break;

            retval = dsl_ode_action_monitor_new(L"every-occurrence-monitor", ode_occurrence_monitor, nullptr);
            if (retval != DSL_RESULT_SUCCESS) break;

            retval = dsl_ode_action_label_offset_new(L"offset-label-action", 0, -15);
            if (retval != DSL_RESULT_SUCCESS) break;
            
            // DSL_EVENT_FILE_FORMAT_CSV, DSL_EVENT_FILE_FORMAT_MOTC
            // retval = dsl_ode_action_file_new(L"write-data-log", 
            //     log_path.c_str(), DSL_WRITE_MODE_TRUNCATE, 
            //     DSL_EVENT_FILE_FORMAT_MOTC, false);
            // if (retval != DSL_RESULT_SUCCESS) break;

            if (monitor) {
                const wchar_t* actions[] = {L"format-bbox", L"format-label", L"every-occurrence-monitor", L"offset-label-action", L"customize-label-action", nullptr};
                retval = dsl_ode_trigger_action_add_many(L"every-occurrence-trigger", actions);
            }
            // else if(log) {
            //     const wchar_t* actions[] = {L"format-bbox", L"format-label", L"customize-label-action", L"offset-label-action", L"write-data-log", nullptr};
            //     retval = dsl_ode_trigger_action_add_many(L"every-occurrence-trigger", actions);
            // }
            else {
                const wchar_t* actions[] = {L"format-bbox", L"format-label", L"customize-label-action", L"offset-label-action", nullptr};
                retval = dsl_ode_trigger_action_add_many(L"every-occurrence-trigger", actions);
            }
            if (retval != DSL_RESULT_SUCCESS) break;

            // `````````````````````````````````````````````````````````````````````````````
            // New ODE Handler to handle all ODE Triggers with their Areas and Actions
            retval = dsl_pph_ode_new(L"ode-handler");
            if (retval != DSL_RESULT_SUCCESS) break;

            // Add the two Triggers to the ODE PPH to be invoked on every frame. 
            const wchar_t* triggers[] = {L"every-occurrence-trigger", nullptr};
            retval = dsl_pph_ode_trigger_add_many(L"ode-handler", triggers);
            if (retval != DSL_RESULT_SUCCESS) break;
        }
        
        if (input_type == L"file") {
            // New File Source
            retval = dsl_source_file_new(L"file-source", uri.c_str(), repeat);
            if (retval != DSL_RESULT_SUCCESS) break;
            
            const wchar_t* component_names[] = 
            {
                L"file-source",
                NULL
            };

            retval = dsl_pipeline_new_component_add_many(L"pipeline",component_names);
            if (retval != DSL_RESULT_SUCCESS) break;

            PR("file-source component add\n");
        }
        else {
            // # For each camera, create a new RTSP Source for the specific RTSP URI    
            retval = dsl_source_rtsp_new(L"rtsp-source", uri.c_str(), DSL_RTP_ALL,     
                false, drop_frame_interval, 100, 2);
            if (retval != DSL_RESULT_SUCCESS)    
                return retval;

            const wchar_t* component_names[] = 
            {
                L"rtsp-source",
                NULL
            };
            
            retval = dsl_pipeline_new_component_add_many(L"pipeline", component_names);
            if (retval != DSL_RESULT_SUCCESS) break;

            PR("rtsp-source component add\n");
        }
        
        if (preproc_enable) {
            // New Preprocessor component using the config filespec defined above.
            retval = dsl_preproc_new(L"preprocessor", preproc_config.c_str());
            if (retval != DSL_RESULT_SUCCESS) break;

            retval = dsl_pipeline_component_add(L"pipeline", L"preprocessor");
            if (retval != DSL_RESULT_SUCCESS) break;

            PR("preprocessor component add\n");
        }
        
        // New Primary GIE using the filespecs defined above, with interval and Id
        retval = dsl_infer_gie_primary_new(L"primary-gie", 
            infer_config.c_str(), infer_model.c_str(), interval);
        if (retval != DSL_RESULT_SUCCESS) break;
        
        if (preproc_enable) {
            // **** IMPORTANT! for best performace we explicity set the GIE's batch-size 
            // to the number of ROI's defined in the Preprocessor configuraton file.
            retval = dsl_infer_batch_size_set(L"primary-gie", batch_size);
            if (retval != DSL_RESULT_SUCCESS) break;
            
            // **** IMPORTANT! we must set the input-meta-tensor setting to true when
            // using the preprocessor, otherwise the GIE will use its own preprocessor.
            retval = dsl_infer_gie_tensor_meta_settings_set(L"primary-gie",
                true, false);
            if (retval != DSL_RESULT_SUCCESS) break;
        }

        retval = dsl_pipeline_component_add(L"pipeline", L"primary-gie");
        if (retval != DSL_RESULT_SUCCESS) break;

        PR("primary-gie component add\n");

        if (trk_enable) {
            if (trk_method == L"IOU") {
                retval = dsl_tracker_iou_new(L"tracker", trk_config.c_str(), trk_width, trk_height);
                if (retval != DSL_RESULT_SUCCESS) break;
            }
            else if (trk_method == L"KLT") {
                retval = dsl_tracker_ktl_new(L"tracker", trk_width, trk_height);
                if (retval != DSL_RESULT_SUCCESS) break;
            }
            else if (trk_method == L"DCF") {
                retval = dsl_tracker_dcf_new(L"tracker", trk_config.c_str(), trk_width, trk_height, true, false);
                if (retval != DSL_RESULT_SUCCESS) break;
            }

            retval = dsl_pipeline_component_add(L"pipeline", L"tracker");
            if (retval != DSL_RESULT_SUCCESS) break;

            PR("tracker component add\n");
        }
        
        // New OSD with text, clock and bbox display all enabled.
        if (osd_enable) {
            retval = dsl_osd_new(L"on-screen-display", true, true, true, false);
            if (retval != DSL_RESULT_SUCCESS) break;
            
            if (postproc_enable && trk_enable) {
                auto DSL_NMP_PROCESS_METHOD = (postproc_method == L"NMM") ? 
                                                DSL_NMP_PROCESS_METHOD_MERGE : DSL_NMP_PROCESS_METHOD_SUPRESS;
                auto DSL_NMP_MATCH_METHOD = (postproc_match_metric == L"IOS") ? 
                                                DSL_NMP_MATCH_METHOD_IOS : DSL_NMP_MATCH_METHOD_IOU;

                std::cout << "DSL_NMP_PROCESS_METHOD: " <<DSL_NMP_PROCESS_METHOD << "\n";
                std::cout << "DSL_NMP_MATCH_METHOD: " <<DSL_NMP_MATCH_METHOD << "\n";
                
                if (class_agnostic) {
                    retval = dsl_pph_nmp_new(L"nmp-pph", nullptr,
                        DSL_NMP_PROCESS_METHOD, DSL_NMP_MATCH_METHOD, postproc_match_threshold);
                    if (retval != DSL_RESULT_SUCCESS) break;
                }
                else {
                    retval = dsl_pph_nmp_new(L"nmp-pph", label_file.c_str(),
                        DSL_NMP_PROCESS_METHOD, DSL_NMP_MATCH_METHOD, postproc_match_threshold);
                    if (retval != DSL_RESULT_SUCCESS) break;
                }

                retval = dsl_tracker_pph_add(L"tracker", L"nmp-pph", DSL_PAD_SINK);
                if (retval != DSL_RESULT_SUCCESS) break;
            }

            retval = dsl_pph_meter_new(L"meter-pph", 1, dsl_pph_meter_cb, &report_data);
            if (retval != DSL_RESULT_SUCCESS) break;

            retval = dsl_osd_pph_add(L"on-screen-display", L"meter-pph", DSL_PAD_SINK);
            if (retval != DSL_RESULT_SUCCESS) break;
            
            retval = dsl_osd_pph_add(L"on-screen-display", L"ode-handler", DSL_PAD_SINK);
            if (retval != DSL_RESULT_SUCCESS) break;
            
            retval = dsl_pph_custom_new(L"send-to-medula", send_data, nullptr);
            if (retval != DSL_RESULT_SUCCESS) break;

            retval = dsl_osd_pph_add(L"on-screen-display", L"send-to-medula", DSL_PAD_SINK);
            if (retval != DSL_RESULT_SUCCESS) break;
        
            retval = dsl_pipeline_component_add(L"pipeline", L"on-screen-display");
            if (retval != DSL_RESULT_SUCCESS) break;

            PR("on-screen-display component add\n");
        }
        else if (trk_enable) {
            if (postproc_enable) {
                auto DSL_NMP_PROCESS_METHOD = (postproc_method == L"NMM") ? 
                                                DSL_NMP_PROCESS_METHOD_MERGE : DSL_NMP_PROCESS_METHOD_SUPRESS;
                auto DSL_NMP_MATCH_METHOD = (postproc_match_metric == L"IOS") ? 
                                                DSL_NMP_MATCH_METHOD_IOS : DSL_NMP_MATCH_METHOD_IOU;

                std::cout << "DSL_NMP_PROCESS_METHOD: " << DSL_NMP_PROCESS_METHOD << "\n";
                std::cout << "DSL_NMP_MATCH_METHOD: " << DSL_NMP_MATCH_METHOD << "\n";

                if (class_agnostic) {
                    retval = dsl_pph_nmp_new(L"nmp-pph", nullptr,
                        DSL_NMP_PROCESS_METHOD, DSL_NMP_MATCH_METHOD, postproc_match_threshold);
                    if (retval != DSL_RESULT_SUCCESS) break;
                }
                else {
                    retval = dsl_pph_nmp_new(L"nmp-pph", label_file.c_str(),
                        DSL_NMP_PROCESS_METHOD, DSL_NMP_MATCH_METHOD, postproc_match_threshold);
                    if (retval != DSL_RESULT_SUCCESS) break;
                }

                retval = dsl_tracker_pph_add(L"tracker", L"nmp-pph", DSL_PAD_SINK);
                if (retval != DSL_RESULT_SUCCESS) break;
            }

            retval = dsl_pph_meter_new(L"meter-pph", 1, dsl_pph_meter_cb, &report_data);
            if (retval != DSL_RESULT_SUCCESS) break;

            retval = dsl_tracker_pph_add(L"tracker", L"meter-pph", DSL_PAD_SRC);
            if (retval != DSL_RESULT_SUCCESS) break;
            
            retval = dsl_pph_custom_new(L"send-to-medula", send_data, nullptr);
            if (retval != DSL_RESULT_SUCCESS) break;
            
            retval = dsl_tracker_pph_add(L"tracker", L"send-to-medula", DSL_PAD_SRC);
            if (retval != DSL_RESULT_SUCCESS) break;
        }

        if (sink_method == L"window") {
            // New Overlay Sink, 0 x/y offsets and same dimensions as Tiled Display
            retval = dsl_sink_window_new(L"window-sink", 0, 0, window_width, window_height);
            if (retval != DSL_RESULT_SUCCESS) break;
            
            retval = dsl_pipeline_component_add(L"pipeline", L"window-sink");
            if (retval != DSL_RESULT_SUCCESS) break;

            // Add the EOS listener and XWindow event handler functions defined above
            retval = dsl_pipeline_eos_listener_add(L"pipeline", eos_event_listener, nullptr);
            if (retval != DSL_RESULT_SUCCESS) break;

            retval = dsl_pipeline_xwindow_key_event_handler_add(L"pipeline", 
                xwindow_key_event_handler, nullptr);
            if (retval != DSL_RESULT_SUCCESS) break;

            retval = dsl_pipeline_xwindow_delete_event_handler_add(L"pipeline", 
                xwindow_delete_event_handler, nullptr);
            if (retval != DSL_RESULT_SUCCESS) break;

            PR("window-sink component add\n");
        }
        else if (sink_method == L"overlay") {
            retval = dsl_sink_overlay_new(L"overlay-sink", 0, 0, 0, 0, window_width, window_height);
            if (retval != DSL_RESULT_SUCCESS) break;

            retval = dsl_pipeline_component_add(L"pipeline", L"overlay-sink");
            if (retval != DSL_RESULT_SUCCESS) break;
        }
        else if (sink_method == L"fake") {
            retval = dsl_sink_fake_new(L"fake-sink");
            if (retval != DSL_RESULT_SUCCESS) break;

            retval = dsl_pipeline_component_add(L"pipeline", L"fake-sink");
            if (retval != DSL_RESULT_SUCCESS) break;

            PR("fake-sink component add\n");
        }
 
        // Play the pipeline
        retval = dsl_pipeline_play(L"pipeline");
        if (retval != DSL_RESULT_SUCCESS) break;

        // Start and join the main-loop
        dsl_main_loop_run();
        break;
    }
    
    // Print out the final result
    std::wcout << dsl_return_value_to_string(retval) << std::endl;

    dsl_delete_all();

    PRC("[INFERENCE] terminated... call am_term()\n");
	(void)ami_term();
    return 0;
}
            
boolean dsl_pph_meter_cb(double* session_fps_averages, double* interval_fps_averages, 
    uint source_count, void* client_data)
{
    // cast the C void* client_data back to a py_object pointer and deref
    // report_data = cast(client_data, POINTER(py_object)).contents.value
    ReportData *report_data = static_cast<ReportData*>(client_data);

    // Print header on interval
    if (report_data->m_report_count % report_data->m_header_interval == 0) {
        std::wstring header = L"";

        for(int i=0; i<source_count; i++) {  
            header += L"FPS ";
            header += std::to_wstring(i);
            header += L" (AVG)";
        }
        // std::wcout << header << "\n";
    }
    
    // Print FPS counters
    std::wstring counters = L"";

    for(int i=0; i<source_count; i++) {
        counters += std::to_wstring(interval_fps_averages[i]);
        if (i==0) {
            data.fps = interval_fps_averages[i];
        }
        counters += L" ";
        counters += std::to_wstring(session_fps_averages[i]);
    }

    // std::wcout << counters << "\n";
    // Increment reporting count
    report_data->m_report_count += 1;
    
    return true;
}

// 
// Function to be called on XWindow KeyRelease event
// 
void xwindow_key_event_handler(const wchar_t* in_key, void* client_data)
{   
    std::wstring wkey(in_key); 
    std::string key(wkey.begin(), wkey.end());
    std::cout << "key released = " << key << std::endl;
    key = std::toupper(key[0]);
    if(key == "P"){
        dsl_pipeline_pause(L"pipeline");
    } else if (key == "R"){
        dsl_pipeline_play(L"pipeline");
    } else if (key == "Q" or key == "" or key == ""){
        std::cout << "Main Loop Quit" << std::endl;
        dsl_pipeline_stop(L"pipeline");
        dsl_main_loop_quit();
    }
}

// 
// Function to be called on XWindow Delete event
//
void xwindow_delete_event_handler(void* client_data)
{
    std::cout<<"delete window event"<<std::endl;
    dsl_pipeline_stop(L"pipeline");
    dsl_main_loop_quit();
}
    
// 
// Function to be called on End-of-Stream (EOS) event
// 
void eos_event_listener(void* client_data)
{
    std::cout<<"Pipeline EOS event"<<std::endl;
    dsl_pipeline_stop(L"pipeline");
    dsl_main_loop_quit();
}    

// 
// Function to be called on every change of Pipeline state
// 
void state_change_listener(uint old_state, uint new_state, void* client_data)
{
    std::cout<<"previous state = " << dsl_state_value_to_string(old_state) 
        << ", new state = " << dsl_state_value_to_string(new_state) << std::endl;
}

// 
// Callback function for the ODE Monitor Action - illustrates how to
// dereference the ODE "info_ptr" and access the data fields.
// Note: you would normally use the ODE Print Action to print the info
// to the console window if that is the only purpose of the Action.
// 
void ode_occurrence_monitor(dsl_ode_occurrence_info* pInfo, void* client_data)
{
    std::wcout << "Trigger Name        : " << pInfo->trigger_name << "\n";
    std::cout << "  Unique Id         : " << pInfo->unique_ode_id << "\n";
    std::cout << "  NTP Timestamp     : " << pInfo->ntp_timestamp << "\n";
    std::cout << "  Source Data       : ------------------------" << "\n";
    std::cout << "    Id              : " << pInfo->source_info.source_id << "\n";
    std::cout << "    Batch Id        : " << pInfo->source_info.batch_id << "\n";
    std::cout << "    Pad Index       : " << pInfo->source_info.pad_index << "\n";
    std::cout << "    Frame           : " << pInfo->source_info.frame_num << "\n";
    std::cout << "    Width           : " << pInfo->source_info.frame_width << "\n";
    std::cout << "    Height          : " << pInfo->source_info.frame_height << "\n";
    std::cout << "    Infer Done      : " << pInfo->source_info.inference_done << "\n";

    if (pInfo->is_object_occurrence)
    {
        std::cout << "  Object Data       : ------------------------" << "\n";
        std::cout << "    Class Id        : " << pInfo->object_info.class_id << "\n";
        std::cout << "    Infer Comp Id   : " << pInfo->object_info.inference_component_id << "\n";
        std::cout << "    Tracking Id     : " << pInfo->object_info.tracking_id << "\n";
        std::cout << "    Label           : " << pInfo->object_info.label << "\n";
        std::cout << "    Persistence     : " << pInfo->object_info.persistence << "\n";
        std::cout << "    Direction       : " << pInfo->object_info.direction << "\n";
        std::cout << "    Infer Conf      : " << pInfo->object_info.inference_confidence << "\n";
        std::cout << "    Track Conf      : " << pInfo->object_info.tracker_confidence << "\n";
        std::cout << "    Left            : " << pInfo->object_info.left << "\n";
        std::cout << "    Top             : " << pInfo->object_info.top << "\n";
        std::cout << "    Width           : " << pInfo->object_info.width << "\n";
        std::cout << "    Height          : " << pInfo->object_info.height << "\n";
    }
    else
    {
        std::cout << "  Accumulative Data : ------------------------" << "\n";
        std::cout << "    Occurrences     : " << pInfo->accumulative_info.occurrences_total << "\n";
        std::cout << "    Occurrences In  : " << pInfo->accumulative_info.occurrences_total << "\n";
        std::cout << "    Occurrences Out : " << pInfo->accumulative_info.occurrences_total << "\n";
    }
    std::cout << "  Trigger Criteria  : ------------------------" << "\n";
    std::cout << "    Class Id        : " << pInfo->criteria_info.class_id << "\n";
    std::cout << "    Infer Comp Id   : " << pInfo->criteria_info.inference_component_id << "\n";
    std::cout << "    Min Infer Conf  : " << pInfo->criteria_info.min_inference_confidence << "\n";
    std::cout << "    Min Track Conf  : " << pInfo->criteria_info.min_tracker_confidence << "\n";
    std::cout << "    Infer Done Only : " << pInfo->criteria_info.inference_done_only << "\n";
    std::cout << "    Min Width       : " << pInfo->criteria_info.min_width << "\n";
    std::cout << "    Min Height      : " << pInfo->criteria_info.min_height << "\n";
    std::cout << "    Max Width       : " << pInfo->criteria_info.max_width << "\n";
    std::cout << "    Max Height      : " << pInfo->criteria_info.max_height << "\n";
    std::cout << "    Interval        : " << pInfo->criteria_info.interval << "\n";
}

uint send_data(void* buffer, void* client_data)
{
    GstBuffer* pGstBuffer = (GstBuffer*)buffer;

    NvDsBatchMeta* pBatchMeta = gst_buffer_get_nvds_batch_meta(pGstBuffer);
    
    S__INF__NAS_INF_INFO data_list;
    D__INF__NAS_INF_INFO *outbuff;
    T_NAS_INF_REC data_src;
    D__INF__NAS_INF_INFO data_get;
    T_NAS_INF_REC data_print;

    int len_check;
    if(T_INFER_opt.infer_level == INFER_ALL)
    {
        PRC(" FPS: %f \n", data.fps);
        for(int i = 0; i< data.infer_length; i++)
        {
            PRC(" Tracking ID: %d, Class: %d, x: %d, y: %d, width: %d, heigth: %d, bearing: %d \n",\ 
            data.infer[i].trkID, data.infer[i].type, static_cast<int>(data.infer[i].x), static_cast<int>(data.infer[i].y), \
            static_cast<int>(data.infer[i].width),static_cast<int>(data.infer[i].height), static_cast<int>(data.infer[i].bearing));
        }
        PRC("Roll: %d, Pitch: %d, Yaw: %d", static_cast<int>(data.roll), static_cast<int>(data.pitch), static_cast<int>(data.yaw));
    }
    else if(T_INFER_opt.infer_level == INFER_FPS)
    {
        PRC(" FPS: %f \n", data.fps);
    }
    else if(T_INFER_opt.infer_level == INFER_INF)
    {
        for(int i = 0; i< data.infer_length; i++)
        {
            PRC(" Tracking ID: %d, Class: %d, x: %d, y: %d, width: %d, heigth: %d, bearing: %d \n",\ 
            data.infer[i].trkID, data.infer[i].type, static_cast<int>(data.infer[i].x), static_cast<int>(data.infer[i].y), \
            static_cast<int>(data.infer[i].width),static_cast<int>(data.infer[i].height), static_cast<int>(data.infer[i].bearing));
        }
    }
    else if(T_INFER_opt.infer_level == INFER_IMU)
    {
        PRC("Roll: %d, Pitch: %d, Yaw: %d", static_cast<int>(data.roll), static_cast<int>(data.pitch), static_cast<int>(data.yaw));
    }

    // For each frame in the batched meta data
    for (NvDsMetaList* pFrameMetaList = pBatchMeta->frame_meta_list; 
        pFrameMetaList; pFrameMetaList = pFrameMetaList->next)
    {
        // Check for valid frame data
        NvDsFrameMeta* pFrameMeta = (NvDsFrameMeta*)(pFrameMetaList->data);
        if (pFrameMeta != nullptr)
        {
            NvDsMetaList* pObjectMetaList = pFrameMeta->obj_meta_list;

            // For each detected object in the frame.
            len_check = 0;
            while (pObjectMetaList)
            {
                // Check for valid object data
                NvDsObjectMeta* pObjectMeta = (NvDsObjectMeta*)(pObjectMetaList->data);
                
                data_src.type = static_cast<int>(pObjectMeta->class_id); // gint 	
                data_src.trkID = static_cast<int>(pObjectMeta->object_id); // guint64

                data_src.x = pObjectMeta->rect_params.left; // float
                data_src.y = pObjectMeta->rect_params.top; // float
                data_src.width = pObjectMeta->rect_params.width; // float
                data_src.height = pObjectMeta->rect_params.height; // float
                
                data_list.nas_inf_rec[len_check] = data_src;
                data.infer[len_check] = data_src;

                len_check += 1;   
                pObjectMetaList = pObjectMetaList->next;
            }
       }
    }

    data_list.nas_int_rec_no = len_check;

    int rst_put = 0;
    int len = sizeof(len_check) + len_check * sizeof(T_NAS_INF_REC);
    outbuff = (D__INF__NAS_INF_INFO*)malloc(len);
    memcpy(outbuff,(D__INF__NAS_INF_INFO *)  &data_list, len);
    rst_put = ami_put_obj(DID__INF__NAS_INF_INFO, outbuff, len);
    if(rst_put < 0)
    {
        PRC("DATA PUT ERROR\n");
    }

    return DSL_PAD_PROBE_OK;
}

static void register_cli_command(void)
{
    ami_cli_reg_cmd("infer",cmd_infer, g_p_usage__cmd_infer);
    ami_cli_reg_cmd("pipe",cmd_pipeline, g_p_usage__cmd_pipeline);
}
